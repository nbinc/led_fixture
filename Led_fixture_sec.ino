// versija 
// v. 1.0 - pievienoju visus LED kanalus regulesanai
// 2014.03.30 v. 1.1 - pievienoju individulos kanalu taimerus
// 2014.06.20 v. 1.2 - partaisiju DIMMeasanu uz 1 sekundes intervalu
// 2014.07.19 v. 2.0 - izlaboju lielu kludu DIMMesanas algoritma, pieliku DIMM rezima indikaciju
// 2014.07.21 v. 2.1 - koda optimizacija, iznemtas liekas strukturas un mainigie
// 2014.07.24 v. 2.11 - pieliku LED atslegsanu ja parkarst radiators
// 2014.12.20 v. 2.12 - pielaboju DIMM atzimes +/- uz ekrana, pamainiju on/off/Auto secibu
// 2014.12.27 v. 3.0 - now 12 bit !!!
// 2014.12.28 v. 3.1 - add logarithmic values table (0-4096 to 256 steps)
// 2015.02.08 v. 3.2 - add Neutral White (NWhite), merge Royal Blue chanel (RB.., RB.), change chanel PIN's 
// 2015.02.21 v. 3.3 - fix screen on/off in day/night mode
// 2016.07.15 v. 3.4 - fix for new controller PCB - change some pins, remove unwanted fn
// 2016.07.24 v. 3.5 - remove pld code for 8 bit PWM
// 2016.08.08 v. 3.6 - remap LED color, replace moon to individual chanel, add T5 bulb 230V relays, som fixes

// ~/Documents/Arduino/! Mani projekti/Led_fixture_12_sec/

#include "Arduino.h"

/* aquacontroller PIN

Digital PIN 
 2 - PWM LCD Screen background
 3 - T5 DIMM 1-10V 1
 4 - T5 DIMM 1-10V 2
 8 - PWM Encoder A
 9 - Tlc5940 GSCLK
10 - PWM Encoder B
11 - Tlc5940 XLAT
12 - Tlc5940 BLANC
20 - I2C SDA
21 - I2C SCL
23 - Keyboard cols 1
25 - Keyboard cols 2
26 - Keyboard cols 3
27 - Keyboard rows 1
31 - LCD SI (28)
33 - LCD CLK (29)
35 - LCD CSB (38)
37 - LCD RS (39)
39 - 1-Wire Datas Thermometer
47 - Keyboard rows 2
48 - relay 1
49 - FAN on/off
50 - relay 2
51 - Tlc5940 SIN
52 - Tlc5940 SCLK

Analog PIN
*/


// *********************************************************************************************
//            EPROM Memory
// *********************************************************************************************
#include <EEPROM.h>

// *********************************************************************************************
//            TLC 5940 chip
// *********************************************************************************************

// I started to add 12-bit PWM functionality
  #include "Tlc5940.h"

// *********************************************************************************************
//            Temperature
// *********************************************************************************************
#include "OneWire.h"
OneWire ds(39); //for timer

float temp[]={0,0,0}; // max 3 temperature peobes: Heatsink, Power Supply, reserved
int temp_counter=0;
unsigned long temp_time=0; 
int temperature_mode=0; // temp screen modes 0-hide, 1-show [1] 10,2

// *********************************************************************************************
//            Rotary encoder
// *********************************************************************************************
#include <QuadEncoder.h>
QuadEncoder qe(8,10); //10,8
int qe1Move=0; // rotary encoder last values


// *********************************************************************************************
//            Keypad
// *********************************************************************************************
#include <Keypad.h>
const byte ROWS = 2; // 2 rows
const byte COLS = 3; // 3 columns
// Define the Keymap
char keys[ROWS][COLS] = {
  {'1','2','a'  },
  {'3','4','b'  }
};

byte rowPins[ROWS] = {27, 29 };         // Connect keypad rows.
byte colPins[COLS] = {23, 25, 47 };     // Connect keypad columns. //28

// Create the Keypad
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// pogu kodi -> 2, 4, 1, 3, [a], b, RESET
//                        encoder  

// *********************************************************************************************
//            Timer RTC 1307
// *********************************************************************************************


#include <Wire.h>  // Incluye la librería Wire
#include <DS1307.h> // written by  mattt on the Arduino forum and modified by D. Sjunnesson

// global date parameter
int hour = 0; 
int second = 0;
int minute = 0;
int monthDay = 0;
int month = 0;
int year = 0;

int flash_timer=0;

// koordinates uz ekrama mainigiem iestadisanai
int date_line[]={ 1, 1, 1, 2, 2, 2};
int date_pos[]= { 4, 7,10, 3, 8,11};

int timer_line[]={ 1, 1, 1, 1, 2};
int timer_pos[]= { 5, 8,11,14, 8};

int temp_line[]={ 1, 1};
int temp_pos[]= { 4, 13};

int overheat_line[]={ 1, 1};
int overheat_pos[]= { 5, 13};

int cursor_pos=0; // kura pozicija atrodas kursots

int pinFAN = 49; // ventilatora izvads
int fan_on=0; // ReadEEPROM(9);
int fan_off=0; // ReadEEPROM(9);
int FAN_status=0; // 0-off, 1-on
int fan_warning=0; // heatsink temperature warning, -50% light power
int fan_alert=0; // heatsink temperature alert, power off all LEDs

int pinRelay1 = 48; // relay for T5 electronic balast
int pinRelay2 = 50; // relay for T5 electronic balast

// koordinates screen saver rezima LED knalu statusiem,
// kad visi uzreiz uz ekrana
int LED_line[]={ 1, 1, 1, 1, 2, 2, 2, 2, 3, 3};
int LED_pos[]= { 1, 5, 9,13, 1, 5, 9,13, 1, 9};
// koordinates uz ekrana bar attelosahani
int dimm_pos[]= {0,0,8,8};
int dimm_line[]={1,2,1,2};
int i_counter; // chanel modification counter

// gaismas taimeri
int on_h[]=  {15,18,15,18, 0, 1, 2, 3, 0, 0,0};
int on_m[]=  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0};
int off_h[]= {23,20,23,20, 1, 2, 3, 4, 0, 0,0};
int off_m[]= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0};
// sunset taimeri
int sunset[]={10,10,10,10,10,10,10,10,10,10,0};


// *********************************************************************************************
//            DOG-M LCD Display
// *********************************************************************************************
#include <DogLcd.h>

// initialize the library with the numbers of the interface pins
DogLcd lcd(31, 33, 37, 35); //SI, CLK, RS, CSB
int pinLCD=2; // LCD background LED pin //13

// custom Character for DOGM 16x3 display
byte progress[8] = {  B00000,  B11011,  B11011,  B11011,  B11011,  B11011,  B00000}; // ||
byte     pro1[8] = {  B00000,  B11000,  B11000,  B11000,  B11000,  B11000,  B00000}; // |.
byte     pro2[8] = {  B00011,  B00011,  B00011,  B00011,  B00011,  B00011,  B00011}; // .|
byte    proto[8] = {  B11111,  B11111,  B11111,  B11111,  B11111,  B11111,  B11111}; //|||
// small box with Char
byte       l1[8] = {  B11111,  B11011,  B10101,  B10101,  B10001,  B10101,  B11111}; //A
byte       l2[8] = {  B11111,  B10011,  B10101,  B10011,  B10101,  B10011,  B11111}; //B
byte       l3[8] = {  B11111,  B11011,  B10101,  B10111,  B10101,  B11011,  B11111}; //C
byte       l4[8] = {  B11111,  B10011,  B10101,  B10101,  B10101,  B10011,  B11111}; //D
// small box with Char
byte       l5[8] = {  B11111,  B10001,  B10111,  B10011,  B10111,  B10001,  B11111}; //E
byte       l6[8] = {  B11111,  B10001,  B10111,  B10011,  B10111,  B10111,  B11111}; //F
byte       l7[8] = {  B11111,  B11001,  B10111,  B10101,  B10101,  B11001,  B11111}; //G
byte       l8[8] = {  B11111,  B10101,  B10101,  B10001,  B10101,  B10101,  B11111}; //H

// menu bar
byte       br[8] = {  B00000,  B00000,  B11011,  B11011,  B00000,  B00000,  B00000}; // ||
byte      pr1[8] = {  B11000,  B11000,  B11011,  B11011,  B11000,  B11000,  B00000}; // |.
byte      pr2[8] = {  B00011,  B00011,  B11011,  B11011,  B00011,  B00011,  B00000}; // .|


// *********************************************************************************************
//            Mani mainigie
// *********************************************************************************************

int interval = 100;// interval at which to blink (milliseconds)
long previousMillis = 0;        // will store last time Data was updated

long interval2 = 900; // intervals taimeru darbibai
long previousMillis2 = 0;
unsigned long currentMillis=0; //timers
int null=0;

// mainigie prieks menu
char* menu_text[] ={
  "LED A",         //1
  "LED B",         //2
  "T5 Lamp",      //3
  "Fan Control",   //4
  "LED Control",   //5
  "Date/Time",     //6
  "Total Power",   //7

  "White",         //8
  "Violet", //9 
  "Royal Blue", // 10
  "Blue", //11
  "Cyan", //12
  "Red", //13
  "Amber", //14
  "Moon", //15

  "T5 Actinic", // 16
  "T5 Azure" // 17
};

int menu_id[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};

int menu_pos=0;
int menu_old=-1;
int menu_size =(sizeof(menu_text)/sizeof(char *))-1;
int menu_mode=0;


String txt;
int flash=0;
int flash_stat=0;
int flash_pos;
int flash_line;
String flash_txt;
int timeout=0;

char Button; // last pressed button;
char key;

int lcd_max=255; // max LCD bright
int lcd_min=5;  // min (darknes) :CD bright -> go to screen slip mode
int moon_active=0; // 1 active/0 unactive
int moon_active_temp=-1;


// DIMM kanalu mainigie
// white, Royal Blue.., Blue, Royal Blue.,
// Red, Green, UV, Royal Blue

//





//                    W   V  RB   B   C   R   A   M               %
  int DIMM_pin[]=   {10, 14, 11,  7,  4,  5,  8, 13,  4,  3,  0,  0};
                                                    // 8, 9 PWM 8bit, 10 % of light
  const int PWMTable[] = {
   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
  64,  65,  66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86,  88,  90,  92,
  94,  96,  98, 100, 102, 104, 106, 108, 111, 114, 117, 120, 123, 126, 129, 132,
 135, 138, 141, 144, 147, 150, 153, 157, 161, 165, 169, 173, 177, 181, 185, 189,
 193, 197, 201, 206, 211, 216, 221, 226, 231, 237, 243, 249, 255, 261, 267, 273,
 279, 285, 292, 299, 306, 313, 320, 327, 335, 343, 351, 359, 367, 375, 384, 393,
 402, 412, 422, 432, 442, 452, 462, 473, 484, 495, 507, 519, 531, 543, 555, 567,
 580, 593, 606, 620, 634, 648, 663, 678, 693, 709, 725, 741, 758, 775, 792, 810,
 829, 848, 868, 888, 908, 929, 950, 971, 993,1015,1038,1061,1085,1109,1134,1160,
1186,1213,1240,1268,1297,1326,1356,1387,1418,1450,1483,1517,1551,1586,1622,1659,
1697,1736,1775,1815,1856,1898,1941,1985,2030,2076,2123,2171,2220,2270,2321,2374,
2427,2480,2533,2586,2639,2692,2745,2798,2851,2904,2957,3010,3063,3116,3169,3223,
3277,3331,3385,3440,3495,3550,3605,3660,3716,3772,3828,3884,3937,3990,4040,4095};

int DIMM_value[]=   {  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  0, 0 };
int DIMM_actual[]=  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 };
int DIMM_old[]=     { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0 };
int DIMM_temp[]=    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0 };
int sunset_actual[]={  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 };
int DIMM_status[]=  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 };




// kanalu krasas nosaukums
char* DIMM_text[] ={
  "White",        //1
  "Violet", //2
  "Royal Blue",        //3
  "Blue",     //4
  "Cyan",    //5
  "Red",
  "Amber",
  "Moon",
  "RB Moon",
  "T5 Actinic",
  "T5 Azure"
};



int DIMM_active=0;
int DIMM_increment_B=8.5;     //solis liela grafika dimmesanai
int DIMM_increment_S=18.2;    //solis mazaa grafika dimmesanai
int DIMM_increment_P=3.3;    //solis procentu (100) grafika dimmesanai
int DIMM_increment=0;         // tekoshais solis
int cha=0; // tekoshais kanaals reguleeshana

int char_small_step=char(165);
int char_big_step=char(161);
int cha_incr=0; // otram ranalu komplektam jabut 4

int hour2 = 0;
int minute2 = 0;
int second2 = 0;
int manual_on=2; // LED light modes: on-1, off-0, auto-2

// *********************************************************************************************
//            S E T U P 
// *********************************************************************************************

void setup() {
  Wire.begin(); // Establece la velocidad de datos del bus I2C

    // Serial.begin(9600);
    // Serial.println('Serial begin'); 
  Tlc.init();

// pin modes
  pinMode(pinLCD, OUTPUT);      // LCD screen background
  pinMode(pinFAN, OUTPUT);      // FAN on/off
  pinMode(pinRelay1, OUTPUT);      // Relay 1 on/off
  pinMode(pinRelay2, OUTPUT);      // Relay 2 on/off
  

  // set up the LCD
  lcd.begin(DOG_LCD_M163, 0x1F); // contrast from 0x1A to 0x28
  analogWrite(pinLCD, lcd_max);  // set LCD display background light to maximum 

  // create custom char
  lcd.createChar(0, progress);
  lcd.createChar(1, pro1); // <
  lcd.createChar(2, pro2); // >
  lcd.createChar(3, proto); // | lcd.write(3);
  lcd.createChar(4, l1);
  lcd.createChar(5, l2);
  lcd.createChar(6, l3);
  lcd.createChar(7, l4);

  // additional display settings
  lcd.home(); // need before use custom char
  lcd.noCursor();
  lcd.noAutoscroll();

  ShowMenu();  

  RTC.stop();
  RTC.start(); 
  
  // load chanel data
  for (int i=0; i<=10; i++){ //0-7 LED, 8,9 T5, 10 - percent, 
    ReadEEPROM(i);
    pinMode(DIMM_pin[i], OUTPUT);
  }
  ReadEEPROM(11); // load fan temperature data
  ReadEEPROM(12); // load overheat temperature data
  SetMenu();
  i_counter=0;

}
// *********************************************************************************************
//            L O O P  
// *********************************************************************************************


void loop() {
  // input devices scan
  // encoder
  qe1Move=qe.tick();

 // keypad
  key = kpd.getKey();
  Button = ' ';
  if(key) { // same as if (key != NO_KEY)
    Button=key;
    timeout=0;
  }  
  // rotary encoder
  if (qe1Move=='>') timeout=0;
  if (qe1Move=='<') timeout=0;


  if (Button=='b') { // go to main menu
    lcd.clear();
    temperature_mode=0;    
    flash=1;
    menu_old=-1;
    menu_mode=0;
    SetMenu();
    ShowMenu();

    analogWrite(pinLCD, lcd_max);
  } 

  if (Button=='1') { // set screensaver active
    timeout=timeout+51;
    menu_mode=0;
    flash=2;
    lcd.clear();
    analogWrite(pinLCD, lcd_min);
  } 

  if (Button=='2') { // on/off/auto light
      manual_on--;
      if (manual_on<0) manual_on=2;
      manual_modes_show();
  } 




// mode switch section
    if (menu_mode==0) {
        Menu();    
        if (Button=='a') { // encoder button
          flash=0;
          menu_mode=menu_pos+1;
          lcd.clear();
          if (menu_mode==1) {// color A
            Set1();
            cha_incr=0;
            temperature_mode=3;
            DIMM_Init();
          }
          if (menu_mode==2) {// color B
            Set2();
            cha_incr=4;
            temperature_mode=3;
            DIMM_Init();
          }
          if (menu_mode==3) {// T5
            Set2();
            cha_incr=8;
            temperature_mode=3;
            DIMM_Init();
          }
                   
          
          if (menu_mode==4) { // temperature settup
            lcd.setCursor(0, 0);
            lcd.print(flash_txt);
            temperature_mode=1;
            cursor_pos=0;
              // print mode screen
              lcd.setCursor(0, 1);
              lcd.print("On: ");
              lcd.print(fan_on);
              lcd.print((char)223);
              lcd.setCursor(8, 1);
              lcd.print("Off: ");
              lcd.print(fan_off);
              lcd.print((char)223);
              lcd.setCursor(1, 2);
              lcd.print("Current: ");
          }

          if (menu_mode==5) { // LED overheating  settup
            lcd.setCursor(0, 0);
            lcd.print(flash_txt);
            temperature_mode=1;
            cursor_pos=0;
              // print mode screen
              lcd.setCursor(0, 1);
              lcd.print("50%: ");
              lcd.print(fan_warning);
              lcd.print((char)223);
              lcd.setCursor(9, 1);
              lcd.print("0%: ");
              lcd.print(fan_alert);
              lcd.print((char)223);
              lcd.setCursor(1, 2);
              lcd.print("Current: ");
          }
          if (menu_mode==6) { // date/time
            lcd.setCursor(0, 0);
            lcd.print(flash_txt);
            temperature_mode=3;        
            cursor_pos=0;
            printTimeOnly();
          }
          if (menu_mode==7) {// manual mode
            SetBar();
            DimmingBIGStartP();
            temperature_mode=3;
            cursor_pos=0;
          }
          if (menu_mode>=8 && menu_mode<=17) {
            SetBar();
            temperature_mode=3;
            DimmingBIGStart();
            cursor_pos=0;
          }
        }
    } else if (menu_mode==-2) {                               // screen saver
      // nope
    } else if (menu_mode==1 || menu_mode==2 || menu_mode==3) {// dimming main color menu
      Dimming();
    } else if (menu_mode==4) {                                // set Fan setting
      SetTemp();
    } else if (menu_mode==5) {                                // set overheat temperatures
      SetOverheat();       
    } else if (menu_mode==6) {                                // set date/time mode
      SetDateTime();
    } else if (menu_mode==7) {                                // set manual mode
        cha=10;
        Dimming_big_percent();
    } else if (menu_mode>=8 || menu_mode<=17) {               // individual chanel setup
        cha=menu_mode-8;
        Dimming_big();
    } else {
      // nope
}  


  if (timeout>50 && menu_mode==0) {   // screensaver on
    flash=2;
    menu_mode=-2;
    lcd.clear();
    SetBar();
    if (moon_active==0) {
      analogWrite(pinLCD, lcd_min);
    } else {
      analogWrite(pinLCD, 0);
    }
    
  }

  if (moon_active_temp!=moon_active) {
    if (flash==2) {
      moon_active_temp=moon_active;
      if (moon_active==0) {
        analogWrite(pinLCD, lcd_min);
      } else {
        analogWrite(pinLCD, 0);
      }
    }
  }


  // run only one time in interval
  currentMillis = millis();
  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    Flash(); // cursor imitation - blinking active element
  } 


// the procedures to each cycle 
  if(currentMillis - previousMillis2 > interval2) {
    previousMillis2 = currentMillis;

      if (manual_on==2) { Watch(); }   // Auto
      if (manual_on==0) { WatchOff(); }                // off
      if (manual_on==1) { WatchOn(); }    // on

    
    
    Termo(); // temperature
    PrintStatus(); // show status un screen
  }
 /* 
  i_counter++;
  if (i_counter>9) {
    i_counter=0;
  }
    Watch(i_counter); // light timers
*/  

}

// *********************************************************************************************
//            Manas apaksh proceduras
// *********************************************************************************************
String toDouble(int a){
  if (a<10) return "0"+String(a);
  return String(a);
}

String toTriple(int a){
  if (a<10) return "00"+String(a);
  if (a<100) return "0"+String(a);
  return String(a);
}


// *********************************************************************************************
//            Manas proceduras
// *********************************************************************************************

// *********************************************************************************************
//            Ekrana 2 pedeju liniju attirisana
// *********************************************************************************************

void CLS() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 2);
  lcd.print("                ");  
}


// *********************************************************************************************
//            Custom Character setup
// *********************************************************************************************


void Set1(){ // set A B C D
  lcd.createChar(4, l1);
  lcd.createChar(5, l2);
  lcd.createChar(6, l3);
  lcd.createChar(7, l4);
  lcd.home();
  SetBar(); 
  
}

void Set2(){ // set E F G H
  lcd.createChar(4, l5);
  lcd.createChar(5, l6);
  lcd.createChar(6, l7);
  lcd.createChar(7, l8);
  lcd.home();
  SetBar();
}

void SetMenu(){ // progress bar set to Main menu style
  lcd.createChar(0, br);
  lcd.createChar(1, pr1);
  lcd.createChar(2, pr2);
  lcd.home(); 
}

void SetBar() { // progress bar sey to LED DIMMING style
  lcd.createChar(0, progress);
  lcd.createChar(1, pro1); // <
  lcd.createChar(2, pro2); // > 
  lcd.home(); 
}



// *********************************************************************************************
//            MENU
// *********************************************************************************************

void Menu(){ // ja menu ir 0 tad sheit parsledzam ar enkoderu menu poziciju
  if (qe1Move=='>') {
    menu_pos++;
    if (menu_pos>menu_size) menu_pos=0;
    ShowMenu();
  } 
  else if (qe1Move=='<') {
    menu_pos--;
    if (menu_pos<0) menu_pos=menu_size;
    ShowMenu();
  }

}


// *********************************************************************************************
//            Show Menu
// *********************************************************************************************

void ShowMenu(){
  if (menu_pos!=menu_old){
    flash_line=1;
    lcd.setCursor(0, flash_line); // poz, rinda
    lcd.print("<              >");

    MenuPos(menu_size, menu_pos);

    flash_txt=menu_text[menu_pos];
    flash_pos=(16-flash_txt.length())/2;
    menu_old=menu_pos;
    flash=1;
    // add 2015-02-13
    temperature_mode=0;
  }
}



void MenuPos(int a, int b) {// a cik kopa, b= kura vieta
  int d;
  int d2=a/2;
  d=1+(16-(a/2))/2;
  d=d-1;
  for (int i=0; i<= d2; i++){
    lcd.setCursor(d+i, 2);
    lcd.write(null);
  } 
  if (b+d!=b-1) {
    lcd.setCursor(d+(b/2), 2);
    if ( (b % 2) == 0) { 
      lcd.write(1); 
    } 
    else { 
      lcd.write(2); 
    } 
  }


}



// *********************************************************************************************
//            Flash text
// *********************************************************************************************

void Flash(){
  if(flash==1){
    int ln=flash_txt.length();
    if (flash_stat==0) {
      flash_stat++;
      lcd.setCursor(flash_pos, flash_line);
      for (int i=0; i<=ln-1; i++){
        lcd.print(" ");
      }
      // palielinam intervalu
      timeout++;
    } 
    else {
      lcd.setCursor(flash_pos, flash_line);


      if (menu_mode==1 || menu_mode==2) {
        lcd.write(cha+4);
      } else if (menu_mode>=7 && menu_mode<=15) {
        if (cursor_pos==0){
          lcd.write(3);
        } else {
          lcd.print(flash_txt);
        }
      } else if (menu_mode==3) {
          lcd.write(3);
      } else {
        lcd.print(flash_txt);
      }
      flash_stat++;
      if (flash_stat>4) flash_stat=0;
    }   
  } 
  if (flash==2){ // iesledzas screensacer
    //printTime();
  }

}



// *********************************************************************************************
//            Print time
// *********************************************************************************************


void printTimeOnly(){
  hour = RTC.get(DS1307_HR,true);
  minute = RTC.get(DS1307_MIN,false);
  second = RTC.get(DS1307_SEC,false);
  monthDay =RTC.get(DS1307_DATE,false);
  month = RTC.get(DS1307_MTH,false);

  year = RTC.get(DS1307_YR,false);
  lcd.setCursor(4, 1);  
  lcd.print(toDouble(hour));
  lcd.print(":");
  lcd.print(toDouble(minute));
  lcd.print(":");
  lcd.print(toDouble(second));

  lcd.setCursor(3, 2);  
  lcd.print(year);
  lcd.print("-");
  lcd.print(toDouble(month));
  lcd.print("-");
  lcd.print(toDouble(monthDay));   
}

// *********************************************************************************************
//            EPROM data save and read
// *********************************************************************************************


void SaveEEPROM(int a){
  if (a<=9){ // save LED chanel data + T5
    EEPROM.write((a*10), on_h[a]);
    EEPROM.write((1+a*10), on_m[a]);
    EEPROM.write((2+a*10), off_h[a]);
    EEPROM.write((3+a*10), off_m[a]);
    EEPROM.write((4+a*10), sunset[a]);
    EEPROM.write((5+a*10), DIMM_value[a]);
  }
  if (a==10){// save moon light data
    EEPROM.write(180, DIMM_value[a]);
  }
  if (a==11){// save fan data
    EEPROM.write(181,fan_on);
    EEPROM.write(182,fan_off);
  }
  if (a==12){// save overheat data
    EEPROM.write(183,fan_warning);
    EEPROM.write(184,fan_alert);
  }   
}

void ReadEEPROM(int a){
  if (a<=9){ // load LED chanel data
    on_h[a] = EEPROM.read(a*10);
    on_m[a] = EEPROM.read(1+a*10);
    off_h[a] = EEPROM.read(2+a*10);
    off_m[a] = EEPROM.read(3+a*10);
    sunset[a] = EEPROM.read(4+a*10);
    DIMM_value[a] = EEPROM.read(5+a*10);
  }
  if (a==10){// load moon light data
    DIMM_value[a] = EEPROM.read(180);
  }
  if (a==11){// load fan settings
    fan_on = EEPROM.read(181);
    fan_off = EEPROM.read(182);
  }
  if (a==12){// load overheat settings
    fan_warning = EEPROM.read(183);
    fan_alert = EEPROM.read(184);
  } 
  
}

// *********************************************************************************************
//            Set Temperature / Fan settings
// *********************************************************************************************

void SetTemp(){
  flash=1;
  switch (cursor_pos) {
  case 0:
    flash_txt=toDouble(fan_on);
    break;
  case 1:
    flash_txt=toDouble(fan_off);
    break;
  case 2:
    // save Temperature
    SaveEEPROM(9);
    menu_old=-1;
    menu_mode=0;
    lcd.clear();
    temperature_mode=0;
    SetMenu();
    ShowMenu();
    break;      
  }
  if (cursor_pos<2) {
    flash_line=temp_line[cursor_pos];
    flash_pos=temp_pos[cursor_pos];
    if (Button=='a') {
      cursor_pos++;
      lcd.setCursor(flash_pos, flash_line);
      lcd.print(flash_txt);
    }

  }

  if (qe1Move=='>') {
    switch (cursor_pos) {
    case 0:
      fan_on++;
      if (fan_on>40) fan_on=22;
      break;
    case 1:
      fan_off++;
      if (fan_off>40) fan_off=22;
      break;
    }
  }  
  else if (qe1Move=='<') {
    switch (cursor_pos) {
    case 0:
      fan_on--;
      if (fan_on<22) fan_on=40;
      break;
    case 1:
      fan_off--;
      if (fan_off<22) fan_off=40;
      break;
    }
  }

}


// *********************************************************************************************
//            Set Overhear Temperature -  iestadisana
// *********************************************************************************************

void SetOverheat(){
  flash=1;
  switch (cursor_pos) {
  case 0:
    flash_txt=toDouble(fan_warning);
    break;
  case 1:
    flash_txt=toDouble(fan_alert);
    break;
  case 2:
    // save "Overheat"
    SaveEEPROM(12);
    menu_old=-1;
    menu_mode=0;
    lcd.clear();
    temperature_mode=0;
    SetMenu();
    ShowMenu();
    break;      
  }
  if (cursor_pos<2) {
    flash_line=overheat_line[cursor_pos];
    flash_pos=overheat_pos[cursor_pos];
    if (Button=='a') {
      cursor_pos++;
      lcd.setCursor(flash_pos, flash_line);
      lcd.print(flash_txt);
    }

  }

  if (qe1Move=='>') {
    switch (cursor_pos) {
    case 0:
      fan_warning++;
      if (fan_warning>70) fan_warning=22;
      break;
    case 1:
      fan_alert++;
      if (fan_alert>70) fan_alert=22;
      break;
    }
  }  
  else if (qe1Move=='<') {
    switch (cursor_pos) {
    case 0:
      fan_warning--;
      if (fan_warning<22) fan_warning=70;
      break;
    case 1:
      fan_alert--;
      if (fan_alert<22) fan_alert=70;
      break;
    }
  }

}



// *********************************************************************************************
//            SetDateTime - datuma un laika iestadisana
// *********************************************************************************************

void SetDateTime(){
  switch (cursor_pos) {
    case 0:
    flash=1;
      flash_txt=toDouble(hour);
      break;
    case 1:
      flash_txt=toDouble(minute);
      break;
    case 2:
      flash_txt=toDouble(second);
      break;
    case 3:
      flash_txt=String(year);
      break;
    case 4:
      flash_txt=toDouble(month);
      break;
    case 5:
      flash_txt=toDouble(monthDay);
      break;
    case 6:
      // set date/time
      flash=0;
      RTC.stop();
      RTC.set(DS1307_SEC,second);        //set the seconds
      RTC.set(DS1307_MIN,minute);        //set the minutes
      RTC.set(DS1307_HR,hour);           //set the hours
      RTC.set(DS1307_DATE,monthDay);     //set the date
      RTC.set(DS1307_MTH,month);         //set the month
      RTC.set(DS1307_YR,(year-2000));    //set the year
      RTC.start();    
      // end
      menu_old=-1;
      menu_mode=0;
      lcd.clear();
      SetMenu();
      ShowMenu();
      break;      
  }
  if (cursor_pos<6) {
    flash_line=date_line[cursor_pos];
    flash_pos=date_pos[cursor_pos];
    if (Button=='a') {
      cursor_pos++;
      lcd.setCursor(flash_pos, flash_line);
      lcd.print(flash_txt);
    }

  }

  if (qe1Move=='>') {
    switch (cursor_pos) {
    case 0:
      hour++;
      if (hour>23) hour=0;
      break;
    case 1:
      minute++;
      if (minute>59) minute=0;
      break;
    case 2:
      second++;
      if (second>59) second=0;
      break;
    case 3:
      year++;
      break;
    case 4:
      month++;
      if (month>12) month=1;
      break;
    case 5:
      monthDay++;
      if (monthDay>31) monthDay=1;
      break;
    }
  }  
  else if (qe1Move=='<') {
    switch (cursor_pos) {
    case 0:
      hour--;
      if (hour<0) hour=23;
      break;
    case 1:
      minute--;
      if (minute<0) minute=59;
      break;
    case 2:
      second--;
      if (second<0) second=59;
      break;
    case 3:
      year--;
      if (year<0) year=2012;
      break;
    case 4:
      month--;
      if (month<1) month=12;
      break;
    case 5:
      monthDay--;
      if (monthDay<1) monthDay=31;
      break;
    }
  }

}
// *********************************************************************************************
//            Darbs ar BAR zimesanu un dimmesanu
// *********************************************************************************************

void printBARs(){
  for (int i=0; i<=3; i++){  
    if (DIMM_value[cha_incr+i]!=DIMM_temp[cha_incr+i]){
      lcd.setCursor(dimm_pos[i],dimm_line[i]);
      lcd.write(4+i);
      BarS(DIMM_value[cha_incr+i],dimm_pos[i]+1,dimm_line[i]); 
      DIMM_temp[cha_incr+i]=DIMM_value[cha_incr+i];
      +(DIMM_pin[cha_incr+i], DIMM_value[cha_incr+i],cha_incr+i);
    }  
  }  
}

// *********************************************************************************************
//            Zimejam LIELO BAR
// *********************************************************************************************
void BarL(int a) {// a cik/vertiba
  int a_interpolating=a/8.5;
  int b=1; // position on line
  int c=1; // line on screen
  a_interpolating=a_interpolating+1;
  int d;
  d=a_interpolating/2;
  d=d-1;
  float p;

  for (int i=0; i<= d-1; i++){
    lcd.setCursor(b+i, c);
    lcd.write(null);
  } 
  if (b+d!=b-1) {
    lcd.setCursor(b+d, c);
    if ( (a_interpolating % 2) == 0) { 
      lcd.write(1); 
    } 
    else { 
      lcd.write(null); 
    } //2
  }

  if (b+d+1!=b+15) { // 7 cik zimigas pozicijas aiznjem BAR
    lcd.setCursor(b+d+1, c);
    lcd.write(" ");
  } 


  
  if (menu_mode!=3) { 
    lcd.setCursor(8, 2);
    lcd.print("        ");
    lcd.setCursor(8, 2);
    //lcd.print("TLS ");
    //lcd.print(PWMTable[a]);
    p=(float(PWMTable[a])/4095)*100;
    lcd.print("% ");
    lcd.print(p);    
  }
  
}

// *********************************************************************************************
//            Zimejam LIELO BAR 100%
// *********************************************************************************************
void BarP(int a) {// a cik/vertiba
  int a_interpolating=a/3.3;
  int b=1; // position on line
  int c=1; // line on screen
  a_interpolating=a_interpolating+1;
  int d;
  d=a_interpolating/2;
  d=d-1;
  float p;

  for (int i=0; i<= d-1; i++){
    lcd.setCursor(b+i, c);
    lcd.write(null);
  } 
  if (b+d!=b-1) {
    lcd.setCursor(b+d, c);
    if ( (a_interpolating % 2) == 0) { 
      lcd.write(1); 
    } 
    else { 
      lcd.write(null); 
    } //2
  }

  if (b+d+1!=b+15) { // 7 cik zimigas pozicijas aiznjem BAR
    lcd.setCursor(b+d+1, c);
    lcd.write(" ");
  } 


  
  if (menu_mode!=3) { 
    lcd.setCursor(8, 2);
    lcd.print("        ");
    lcd.setCursor(10, 2);
    lcd.print("% ");
    lcd.print(a);    
  }
  
}


// *********************************************************************************************

void BarS(int a, int b, int c) {// a cik
  int a_interpolating=a/18.2;
  a_interpolating=a_interpolating+1;
  int d;
  d=a_interpolating/2;
  d=d-1;
  for (int i=0; i<= d-1; i++){
    lcd.setCursor(b+i, c);
    lcd.write(null);
  } 
  if (b+d!=b-1) {
    lcd.setCursor(b+d, c);
    if ( (a_interpolating % 2) == 0) { 
      lcd.write(1); 
    } 
    else { 
      lcd.write(null); 
    } //2
  }

  if (b+d+1!=b+7) { // 7 cik zimigas pozicijas aiznjem BAR
    lcd.setCursor(b+d+1, c);
    lcd.write(" ");
  } 
}


// *********************************************************************************************
//            Darbs ar BAR pirmatneja ekrana iestadisana
// *********************************************************************************************
void DIMM_Init(){
  cha=0;
  lcd.setCursor(0, 0);
//  lcd.print(menu_text[menu_mode-1]);
//  lcd.setCursor(5, 0);
//  lcd.write(":");
  lcd.write(DIMM_text[cha_incr+cha]); 

  lcd.setCursor(12, 0);
  lcd.write(char_big_step);

  DIMM_increment=DIMM_increment_S;

  DIMM_temp[cha_incr+0]=-1;
  DIMM_temp[cha_incr+1]=-1;
  DIMM_temp[cha_incr+2]=-1;
  DIMM_temp[cha_incr+3]=-1;
  printBARs();
}

// *********************************************************************************************
//            Darbs ar BAR pirmatneja BIG bar ekrana iestadisana
// *********************************************************************************************
void DimmingBIGStart(){
  lcd.setCursor(0, 0);
  lcd.print(menu_text[menu_mode-1]);

  lcd.setCursor(0, 2);
  lcd.write(char_big_step);

  DIMM_increment=DIMM_increment_B;

  for (int i=0; i<= 10; i++){ // clear tmporary data, 8,9 - T5, 10 Moon chanel
    DIMM_temp[i]=-1;
  } 
  // print local menu position
  if (menu_mode!=3) {
    lcd.setCursor(14, 0);
    lcd.write(3);
    lcd.print(".");
  }

}

void DimmingBIGStartP(){
  lcd.setCursor(0, 0);
  lcd.print(menu_text[menu_mode-1]);

  lcd.setCursor(0, 2);
  lcd.write(char_big_step);

  DIMM_increment=DIMM_increment_P;
  DIMM_temp[10]=-1;
  // print local menu position
  if (menu_mode!=3) {
    lcd.setCursor(14, 0);
    lcd.write(3);
    lcd.print(".");
  }

}


// *********************************************************************************************
//            Darbs ar mazo BAR
// *********************************************************************************************
void Dimming(){

  if (Button=='a') {   // ENCODER BUTTON
    lcd.setCursor(dimm_pos[cha], dimm_line[cha]);
    lcd.write(cha+4);
    SaveEEPROM(cha_incr+cha);
    cha++;
    if (cha>=4) {
      menu_old=-1;
      menu_mode=0;
      Set1();
      SetBar();
      SetMenu();
      lcd.clear();
      ShowMenu();
    } else {
      // rakstam krasas nosaukumu
      lcd.setCursor(0, 0); // 6, 0
      lcd.write("            ");
      lcd.setCursor(0, 0);
      lcd.write(DIMM_text[cha_incr+cha]);     
    }  
  }

  if (Button=='3') { // fine tuning on/off
    lcd.setCursor(12, 0);
    if (DIMM_increment==1) {
      DIMM_increment=DIMM_increment_S;
      lcd.write(char_big_step);
    } 
    else {
      DIMM_increment=1;
      lcd.write(char_small_step);
    }
  }

  if (qe1Move=='>') {
    DIMM_value[cha_incr+cha]=DIMM_value[cha_incr+cha]+DIMM_increment;
    if (DIMM_value[cha_incr+cha]>255) {
      DIMM_value[cha_incr+cha]=255;
    }
    aWrite(DIMM_pin[cha_incr+cha], DIMM_value[cha_incr+cha], cha_incr+cha);
  }  
  else if (qe1Move=='<') {
    DIMM_value[cha_incr+cha]=DIMM_value[cha_incr+cha]-DIMM_increment;
    if (DIMM_value[cha_incr+cha]<0) {
      DIMM_value[cha_incr+cha]=0;
    }
    aWrite(DIMM_pin[cha_incr+cha], DIMM_value[cha_incr+cha], cha_incr+cha);   
  }  

  if (cha<4) {
    printBARs();
    lcd.setCursor(13, 0);
    lcd.print(toTriple(DIMM_value[cha_incr+cha]));
    flash_pos=dimm_pos[cha];
    flash_line=dimm_line[cha];
    flash_txt=String(cha+4);
    flash=1;
  }


  // end void  
}


// *********************************************************************************************
//            Darbs ar LIEL0 BAR
// *********************************************************************************************

void Dimming_big(){
 
  if (Button=='a') {   // ENCODER BUTTON
    cursor_pos++;
    if (menu_mode==7) { // it kaa prieksh moon light ieprieks atslegsana
     cursor_pos=6;
    } else {
      if (cursor_pos>0) {
        flash_pos=timer_pos[cursor_pos-1];
        flash_line=timer_line[cursor_pos-1];
     // atkartoti simejam laikus lai atjaunotu flash rezimaa neuzssiimeetus numurus
            lcd.setCursor(0, 1);
            lcd.print("Day: ");
            lcd.print(toDouble(on_h[cha]));
            lcd.print(":");
            lcd.print(toDouble(on_m[cha]));
            lcd.print("-");      
            lcd.print(toDouble(off_h[cha]));
            lcd.print(":");
            lcd.print(toDouble(off_m[cha]));   
      }
    }
  
    switch (cursor_pos) {
        case 1:
          CLS();
      
          // zimejam tekosha ekrana poziciju
          lcd.setCursor(14, 0);
          lcd.print(".");
          lcd.write(3);

          lcd.setCursor(0, 1);
          lcd.print("Day: ");
          lcd.print(toDouble(on_h[cha]));
          lcd.print(":");
          lcd.print(toDouble(on_m[cha]));
          lcd.print("-");      
          lcd.print(toDouble(off_h[cha]));
          lcd.print(":");
          lcd.print(toDouble(off_m[cha]));
          lcd.setCursor(0, 2);
          lcd.print("Sunset: ");
          lcd.print(toDouble(sunset[cha]));
          lcd.print(" min.");

          flash_txt=toDouble(on_h[cha]);
          flash=1;
          break;
       case 2:
          flash_txt=toDouble(on_m[cha]);
          flash=1; 
          break;
       case 3:
          flash_txt=toDouble(off_h[cha]);
          flash=1;
          break;
       case 4:
          flash_txt=toDouble(off_m[cha]);
          flash=1;
          break;
       case 5:
          flash_txt=toDouble(sunset[cha]);
          flash=1;
          break;            
    }

    if (cursor_pos>=6) {
      SaveEEPROM(cha);
      menu_old=-1;
      menu_mode=0;
      lcd.clear();
      SetMenu();
      ShowMenu();
    }     
  }

    if (cursor_pos==0) { // set LED level with bar
      if (Button=='3') { // fine tunning on/off
        lcd.setCursor(0, 2);
        if (DIMM_increment==1) {
          DIMM_increment=DIMM_increment_B;
          lcd.write(char_big_step);
        } 
        else {
          DIMM_increment=1;
          lcd.write(char_small_step);
        }
      }
    }  
    if (qe1Move=='>') {
      switch (cursor_pos) {
        case 0:
          DIMM_value[cha]=DIMM_value[cha]+DIMM_increment;
          if (DIMM_value[cha]>255) {
            DIMM_value[cha]=255;
          }
          break;
        case 1:
          on_h[cha]++;
          if (on_h[cha]>23) on_h[cha]=0;
          flash_txt=toDouble(on_h[cha]);
          break;
        case 2:
          on_m[cha]++;
          if (on_m[cha]>59) on_m[cha]=0;
          flash_txt=toDouble(on_m[cha]);
          break;
        case 3:
          off_h[cha]++;
          if (off_h[cha]>23) off_h[cha]=0;
          flash_txt=toDouble(off_h[cha]);
          break;
        case 4:
          off_m[cha]++;
          if (off_m[cha]>59) off_m[cha]=0;
          flash_txt=toDouble(off_m[cha]);
          break;
        case 5:
          sunset[cha]=sunset[cha]+1;
          if (sunset[cha]>60) sunset[cha]=1;
          flash_txt=toDouble(sunset[cha]);
          break;          
      }   
    }  
    else if (qe1Move=='<') {
      switch (cursor_pos) {
        case 0:
          DIMM_value[cha]=DIMM_value[cha]-DIMM_increment;
          if (DIMM_value[cha]<0) {
            DIMM_value[cha]=0;
          }    
          break;
        case 1:
          on_h[cha]--;
          if (on_h[cha]<0) on_h[cha]=23;
          flash_txt=toDouble(on_h[cha]);
          break;
        case 2:
          on_m[cha]--;
          if (on_m[cha]<0) on_m[cha]=59;
           flash_txt=toDouble(on_m[cha]);
          break;
        case 3:
          off_h[cha]--;
          if (off_h[cha]<0) off_h[cha]=23;
          flash_txt=toDouble(off_h[cha]);
          break;
        case 4:
          off_m[cha]--;
          if (off_m[cha]<0) off_m[cha]=59;
          flash_txt=toDouble(off_m[cha]);
          break;
        case 5:
          sunset[cha]=sunset[cha]-1;
          if (sunset[cha]<0) sunset[cha]=60;
           flash_txt=toDouble(sunset[cha]);
          break;           
      }
    }
    
    if (cursor_pos==0) {
      if (DIMM_value[cha]!=DIMM_temp[cha]){
        BarL(DIMM_value[cha]);
        DIMM_temp[cha]=DIMM_value[cha];
        aWrite(DIMM_pin[cha], DIMM_value[cha],cha);
      }
      lcd.setCursor(1, 2);
      lcd.print(toTriple(DIMM_value[cha]));
      
      flash_pos=0;
      flash_line=1;
      flash_txt=" "; //String(cha+4);
      flash=1;
    }

  // end void  
}


// *********************************************************************************************
//            Darbs ar LIEL0 BAR 100%
// *********************************************************************************************

void Dimming_big_percent(){
 
  if (Button=='a') {   // ENCODER BUTTON
    cursor_pos++;
    if (cursor_pos>=1) {
      SaveEEPROM(cha);
      menu_old=-1;
      menu_mode=0;
      lcd.clear();
      SetMenu();
      ShowMenu();
    }     
  }

    if (cursor_pos==0) { // set LED level with bar
      if (Button=='3') { // fine tunning on/off
        lcd.setCursor(0, 2);
        if (DIMM_increment==1) {
          DIMM_increment=DIMM_increment_P;
          lcd.write(char_big_step);
        } 
        else {
          DIMM_increment=1;
          lcd.write(char_small_step);
        }
      }
    }  
    if (qe1Move=='>') {
          DIMM_value[cha]=DIMM_value[cha]+DIMM_increment;
          if (DIMM_value[cha]>100) {
            DIMM_value[cha]=100;
          }
    }  
    else if (qe1Move=='<') {
          DIMM_value[cha]=DIMM_value[cha]-DIMM_increment;
          if (DIMM_value[cha]<0) {
            DIMM_value[cha]=0;
          }      
    }
    
    if (cursor_pos==0) {
      if (DIMM_value[cha]!=DIMM_temp[cha]){
        BarP(DIMM_value[cha]);
        DIMM_temp[cha]=DIMM_value[cha];
        aWrite(DIMM_pin[cha], DIMM_value[cha],cha);
      }
      lcd.setCursor(1, 2);
      lcd.print(toTriple(DIMM_value[cha]));

// reset TLC
DIMM_old[0]=-1;
DIMM_old[1]=-1;
DIMM_old[2]=-1;
DIMM_old[3]=-1;
DIMM_old[4]=-1;
DIMM_old[5]=-1;
DIMM_old[6]=-1;
DIMM_old[7]=-1;
      
      flash_pos=0;
      flash_line=1;
      flash_txt=" "; //String(cha+4);
      flash=1;
    }

  // end void  
}




// *********************************************************************************************
//            LED kanalu Taimeru darbibas
// *********************************************************************************************

void Watch(){
  long on_sec;
  long off_sec;
  long now_sec;
  long tmp_sunset;

// for internal
  long DIMM_long;
  float tmp;
  
//int out;
  int actual_led;
  int now_mode=0; // 0 - normal, 1 sun Up, 2 sun Down for indicate sunset/sundown

  hour2 = RTC.get(DS1307_HR,true);
  minute2 = RTC.get(DS1307_MIN,false);
  second2 = RTC.get(DS1307_SEC,false);
  now_sec=((long(hour2)*3600)+(long(minute2)*60))+long(second2); // count seconds

// Serial.print("> ");

  // for all defined timer
  for (int i=0; i<=9; i++){
    
    // init for calculation
    on_sec=(long(on_h[i])*3600)+(long(on_m[i])*60);
    off_sec=(long(off_h[i])*3600)+(long(off_m[i])*60); 
    tmp_sunset=long(sunset[i])*60;
    DIMM_long=long(DIMM_value[i]);
//**********************************************************************************************
// normal situation - time_on < time_off
      if (on_sec<off_sec) {
          // light on
          if (now_sec>=on_sec && now_sec<=off_sec){
            DIMM_actual[i]=(int) DIMM_long;
            now_mode=0;
          // normals sunset +             
            if (now_sec<=(on_sec+tmp_sunset)){
                tmp=(((float)DIMM_long/(float)tmp_sunset)*(float)((on_sec+tmp_sunset)-now_sec));
                DIMM_long = DIMM_long -tmp;
                DIMM_actual[i]=(int) DIMM_long;
                now_mode=1;
            }
         // normals sundown -            
            if (now_sec>=off_sec-tmp_sunset){
                tmp=((float)DIMM_long/(float)tmp_sunset)*(float)((off_sec-tmp_sunset)-now_sec);
                tmp=abs(tmp);
      	        DIMM_long = DIMM_long -(long)tmp;
                DIMM_actual[i]=(int) DIMM_long;
                now_mode=2;
            }
        // light off             
          } else {                                                                     
            DIMM_actual[i]=0;
            now_mode=0;
          }
      }
//**********************************************************************************************
// extra situation - time_on > to time_off
      if (on_sec>off_sec) {
          // light off        
         if (now_sec>=off_sec && now_sec<on_sec){   	                              
	          DIMM_actual[i]=0;
            now_mode=0;
          // light on            
         } else {  
  	        DIMM_actual[i]=(int) DIMM_long;
          // normals sunset  
            if (now_sec<on_sec+tmp_sunset && now_sec>off_sec){                 
                 tmp=(((float)DIMM_long/(float)tmp_sunset)*(float)((on_sec+tmp_sunset)-now_sec));
      	         DIMM_long = DIMM_long -(long) tmp;
                 DIMM_actual[i]=(int) DIMM_long;
                 now_mode=1;
              }
          // sunsets, kas turpinas pec pusnakts (iesledz 23.55, izsledz 02.00, sunrise 20)
	      if ((on_sec+tmp_sunset)>86400 && (now_sec-on_sec)<0 && now_sec<(tmp_sunset-(86400-on_sec))){
                  tmp=(((float)DIMM_long/(float)tmp_sunset)*(float)(((on_sec+tmp_sunset)-86400)-now_sec));		  
      	          DIMM_long = DIMM_long -(long) tmp;
                  DIMM_actual[i]=(int) DIMM_long;
                  now_mode=1;
              }
        // normals sundown              
	      if (now_sec>=off_sec-tmp_sunset && now_sec<off_sec){                
                  tmp=((float)DIMM_long/(float)tmp_sunset)*(float)((off_sec-tmp_sunset)-now_sec);
                  tmp=abs(tmp);
      	          DIMM_long = DIMM_long -(long) tmp;
                  DIMM_actual[i]=(int) DIMM_long;
                  now_mode=2;
              }
        // sundowns, kas jasak rekinat pirms pusnakts (iesledz 20.00 , izsledz 00.10 un sunrise 15)
  	      if ((tmp_sunset-off_sec)>0 && (86400-now_sec)<=(tmp_sunset-off_sec)){
                  tmp=((float)DIMM_long/(float)tmp_sunset)*(float)(((off_sec-tmp_sunset)-now_sec)+86400);
                  tmp=abs(tmp);
      	          DIMM_long = DIMM_long -(long)tmp;
                  DIMM_actual[i]=(int) DIMM_long;
                  now_mode=2;
              }	
          }
      }

      if (manual_on==2) { aWrite(DIMM_pin[i], DIMM_actual[i], i); }   // Auto
      if (manual_on==0) { aWrite(DIMM_pin[i], 0, i); }                // off
      if (manual_on==1) { aWrite(DIMM_pin[i], (int) DIMM_value[i], i); }    // on
    
      if (menu_mode==-2) {
        if (i<8){
          lcd.setCursor(LED_pos[i], LED_line[i]);
          lcd.print(toTriple(DIMM_actual[i]));
        
          lcd.setCursor(LED_pos[i]-1, LED_line[i]);
          
//          if (now_mode==0) {
//            if (i==7 && DIMM_actual[i]==0) { // set moon  on chanel 7
//              lcd.print("m");
//            } else {  
//              lcd.print(" ");
//            }
//          }
          if (now_mode==1) {
            lcd.print("+");
          }
          if (now_mode==2) {
            lcd.print("-");
          }
        }          
      }
  } // for 
  // Tlc.update(); 
}

// *********************************************************************************************
//            LED kanalu Izlegsana
// *********************************************************************************************

void WatchOff(){
      for (int i=0; i<=7; i++){
          Tlc.set(DIMM_pin[i], 0);
          DIMM_old[i]=0;
      }
      Tlc.update();
      analogWrite(DIMM_pin[8],0);
      digitalWrite(pinRelay1, LOW);
      analogWrite(DIMM_pin[9],0);
      digitalWrite(pinRelay2, LOW);
      DIMM_old[8]=0;
      DIMM_old[9]=0;
}

// *********************************************************************************************
//            LED kanalu Ieslēgsana
// *********************************************************************************************

void WatchOn(){
      for (int i=0; i<=7; i++){
          Tlc.set(DIMM_pin[i], DIMM_value[i]);
          DIMM_old[i]=DIMM_value[i];
      }
      Tlc.update();
      analogWrite(DIMM_pin[8],DIMM_value[8]);
      if (DIMM_value[8]==0){
        digitalWrite(pinRelay1, LOW);
      } else {
        digitalWrite(pinRelay1, HIGH);
      }
      analogWrite(DIMM_pin[9],DIMM_value[9]);
      if (DIMM_value[9]==0){
        digitalWrite(pinRelay2, LOW);
      } else {
        digitalWrite(pinRelay2, HIGH);
      }      
      DIMM_old[8]=DIMM_value[8];
      DIMM_old[9]=DIMM_value[9];
}


// *********************************************************************************************
//            LED light analogWrite emulate
// *********************************************************************************************
void aWrite (int pin, int value, int chanel){
  float tmp_value=0;
  int tmp_percent=DIMM_value[10];

// clear alert status
lcd.setCursor(11, 0);
lcd.print(' ');
 
  if (temp[0]>=fan_warning){ // 50% of LED's power
    value=value/2;
lcd.setCursor(11, 0);
lcd.print('!');
  }

  if (temp[0]>=fan_alert){ // turn off all LED's
    value=0;
lcd.setCursor(11, 0);
lcd.print('?');    
  }
  
tmp_value=((float)PWMTable[value]/(float)100)*(float)tmp_percent;
/*
if (chanel==0) {
  lcd.setCursor(0, 0);
//lcd.print(tmp_value);
  lcd.print(DIMM_value[chanel]);
  lcd.setCursor(8, 0);
  lcd.print(DIMM_actual[chanel]);
  //tmp_value=(PWMTable[value]/100)*tmp_percent;
  // tmp_value=PWMTable[value];
}
  */
   //if (DIMM_old[chanel]!=value) {
      if (chanel<8) {
          Tlc.set(pin, (int)tmp_value); 
          Tlc.update();
      }
   //}
   DIMM_old[chanel]=value;

    // for T5
    if (chanel>7) {
        analogWrite(pin,value);

        if (chanel==8) {
          analogWrite(pin,value);
          if (value==0){
            digitalWrite(pinRelay1, LOW);
          } else {
            digitalWrite(pinRelay1, HIGH);
          }
        }
        if (chanel==9) {
          analogWrite(pin,value);
          if (value==0){
            digitalWrite(pinRelay2, LOW);
          } else {
            digitalWrite(pinRelay2, HIGH);
          }
        }
        
    }
}





// *********************************************************************************************
//            Temperatura
// *********************************************************************************************
void Termo() {
  
  if (millis() > (temp_time+1000)) {
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius, fahrenheit;
    
    if ( !ds.search(addr)) {
      temp_counter=0;
      ds.reset_search();
      return;
    }
  
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    
   
    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
    }
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
    temp[temp_counter]=celsius;
    temp_counter++;
    temp_time=millis();
  
  // contriol Temp
   if (temp[0]>=fan_on) {
     digitalWrite(pinFAN, HIGH);
     FAN_status=1;
   }
   if (temp[0]<=fan_off) {
     digitalWrite(pinFAN, LOW);
     FAN_status=0;
   }
}

  
}



// *********************************************************************************************
//            Print status on screenSaver mode
// *********************************************************************************************

void PrintStatus(){
// te vajag displeja Modi skat'ties un tad druk't

  if (menu_mode==-2) { // tipa screensavers
      int hr = hour2;
      int mn = minute2;
      int sc = second2;
  
 //   flash_timer++;
 //   if (flash_timer<=1) {
      lcd.setCursor(0, 0);  
      lcd.print(toDouble(hr));
      lcd.print(":");
      lcd.print(toDouble(mn));
      lcd.print(":");
      lcd.print(toDouble(sc));
 //   }
 //  if (flash_timer==2) {
 //     flash_timer=0;
 //    lcd.setCursor(0, 0);  
 //     lcd.print(toDouble(hr));
 //     lcd.print(" ");
 //     lcd.print(toDouble(mn )); 
 //   }
    
  }
 
   if (temperature_mode==0) {
     int tmpa=int(temp[0]);
     lcd.setCursor(12, 0);
     lcd.print(tmpa);
     lcd.write(223);
     lcd.print("C");  
     lcd.setCursor(10, 0);
     if (FAN_status==1) {
        lcd.write(42);
     } else {
        lcd.write(238);
     } 
     
    manual_modes_show();
   }
   
   if (temperature_mode==1) {
     int tmpa=int(temp[0]);
     lcd.setCursor(10, 2);
     lcd.print(temp[0]);
     lcd.write(223);       
     
   }
}

void manual_modes_show(){
       if (manual_on==2) {
        lcd.setCursor(9, 0);
        lcd.print("A");
      }
      if (manual_on==0) {
        lcd.setCursor(9, 0);
        //lcd.write(245);
       lcd.print("_");

      }
      if (manual_on==1) {
        lcd.setCursor(9, 0);
        lcd.write(255);

      } 
}


