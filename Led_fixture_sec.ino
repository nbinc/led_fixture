// versija 
// v. 1.0 - pievienoju visus LED kanalus regulesanai
// 2014.03.30 v. 1.1 - pievienoju individulos kanalu taimerus
// 2014.06.20 v. 1.2 - partaisiju DIMMeasanu uz 1 sekundes intervalu
// 2014.07.19 v. 2.0 - izlaboju lielu kludu DIMMesanas algoritma, pieliku DIMM rezima indikaciju
// 2014.07.21 v. 2.1 - koda optimizacija, iznemtas liekas strukturas un mainigie
// 2014.07.24 v. 2.11 - pieliku LED atslegsanu ja parkarst radiators

// ~/Documents/Arduino/350W LED Fixture LDD/led_fixture

#define TLC       0    // set to 1 if using 12bit LED DIMMing
#define SERIAL    0    // set to 1 if need debug data serial output

#include "Arduino.h"

/* aquacontroller PIN

Digital PIN
1
2 - Night LED PWM Light
3 - LED
4 - LED
5 - LED
6 - LED
7 - LED Royal Blue
8 - LED Blue
9 - LED Royal Blue ..
10 - LED White

11 - PWM Encoder A
12 - PWM Encoder B

13 - PWM LCD Screen background

20 - I2C SDA
21 - I2C SCL

23 - Keyboard cols 1
25 - Keyboard cols 2
26 - Keyboard cols 3
27 - Keyboard rows 1
28 - 1-Wire Datas Thermometer
29 - Keyboard rows 2

30 - Fan on/off

31 - LCD SI (28)
33 - LCD CLK (29)
35 - LCD CSB (38)
37 - LCD RS (39)

Analog PIN
3 - Current Sensor 1
4 - Current Sensor 2
14 - Voltage sensor 1
15 - voltage sensor 2
*/


// *********************************************************************************************
//            EPROM Memory
// *********************************************************************************************
#include <EEPROM.h>

// *********************************************************************************************
//            TLC 5940 chip
// *********************************************************************************************

// I started to add 12-bit PWM functionality
// nothing worked properly yet
#if TLC
  #include "Tlc5940.h"

#endif 

// *********************************************************************************************
//            Temperature
// *********************************************************************************************
#include <OneWire.h>
OneWire ds(28);

float temp[]={0,0,0}; // max 3 temperature peobes: Heatsink, Power Supply, reserved
int temp_counter=0;
unsigned long temp_time=0; 
int temperature_mode=0; // temp screen modes 0-hide, 1-show [1] 10,2

// *********************************************************************************************
//            Voltage values
// ********************************************************************************************* 
// variables for input pin and control LED


  float vout = 0.0;
  float vin = 0.0;
  float R1 = 1000000.0;    // !! resistance of R1 !!
  float R2 = 100000.0;     // !! resistance of R2 !!
  
  float volt[]={24,48}; // get nominal default

  int volt_sensot_PIN[] ={15,14};
  int volt_sensor_value = 0;

// For Current sensor

int A1_pin=3;
int A2_pin=4;

int currentSens1=0;
int currentSens2=0;

int CurrentValue1=0;
int CurrentValue2=0;
float amps1=0;
float amps2=0;


// *********************************************************************************************
//            Rotary encoder
// *********************************************************************************************
// A -> pin 11 (need PWM)
// B -> pin 12 (need PWM)
#include <QuadEncoder.h>
QuadEncoder qe(12,11);
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
byte colPins[COLS] = {23, 25, 26 };     // Connect keypad columns.

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

// koordinates laika un datuma iestadisanai
int date_line[]={ 1, 1, 1, 2, 2, 2};
int date_pos[]= { 4, 7,10, 3, 8,11};

int timer_line[]={ 1, 1, 1, 1, 2};
int timer_pos[]= { 5, 8,11,14, 8};


int temp_line[]={ 1, 1};
int temp_pos[]= { 4, 13};


int cursor_pos=0; // kura pozicija atrodas kursots

// ventilatora izvads
int pinFAN = 30;
int fan_on=0; //ReadEEPROM(9);
int fan_off=0; //ReadEEPROM(9);
int FAN_status=0; // 0-off, 1-on
int fan_warning=40; // heatsink temperature warning, -50% light power
int fan_alert=50; // heatsink temperature alert, power off all LEDs

// koordinates screen saver rezima LED knalu statusiem,
// kad visi uzreiz uz ekr'na
int LED_line[]={ 1, 1, 1, 1, 2, 2, 2, 2};
int LED_pos[]= { 1, 5, 9,13, 1, 5, 9,13};
// koordinates uz ekrana bar attelosahani
int dimm_pos[]= {0,0,8,8};
int dimm_line[]={1,2,1,2};


// gaismas taimeri
int on_h[]=  {15,18,15,18, 0, 1, 2, 3};
int on_m[]=  { 0, 0, 0, 0, 0, 0, 0, 0};
int off_h[]= {23,20,23,20, 1, 2, 3, 4};
int off_m[]= { 0, 0, 0, 0, 0, 0, 0, 0};
// sunset taimeri
int sunset[]={10,10,10,10,10,10,10,10};


// *********************************************************************************************
//            DOG-M LCD Display
// *********************************************************************************************
#include <DogLcd.h>

// initialize the library with the numbers of the interface pins
DogLcd lcd(31, 33, 37, 35); //SI, CLK, RS, CSB
int pinLCD=13; // LCD background LED pin

// custom Character for DOGM 16x3 display
byte progress[8] = {  B00000,  B11011,  B11011,  B11011,  B11011,  B11011,  B00000}; // ||
byte     pro1[8] = {  B00000,  B11000,  B11000,  B11000,  B11000,  B11000,  B00000}; // |.
byte     pro2[8] = {  B00011,  B00011,  B00011,  B00011,  B00011,  B00011,  B00011}; // .|
byte    proto[8] = {  B11111,  B11111,  B11111,  B11111,  B11111,  B11111,  B11111}; //|||
byte       l1[8] = {  B11111,  B11011,  B10101,  B10101,  B10001,  B10101,  B11111}; //A
byte       l2[8] = {  B11111,  B10011,  B10101,  B10011,  B10101,  B10011,  B11111}; //B
byte       l3[8] = {  B11111,  B11011,  B10101,  B10111,  B10101,  B11011,  B11111}; //C
byte       l4[8] = {  B11111,  B10011,  B10101,  B10101,  B10101,  B10011,  B11111}; //D

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

long interval2 = 1000; // intervals taimeru darbibai
long previousMillis2 = 0;
unsigned long currentMillis=0; //timers
int null=0;

// mainigie prieks menu
char* menu_text[] ={
  "LED A",        //1
  "LED B", //2
  "LED Moon",        //3
  "Temperature",         //4
  "Setup",    //5
  "Date/Time", //6
  "Manual mode", //7

  "White",  //8
  "Royal Blue..", //9 

  "Blue", // 10
  "Royal Blue.", //11
  "Green", //12
  "UV", //13
  "Red", //14
  "Royal Blue", //15

  "Reserved" // 16
};

int menu_id[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

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
int lcd_min=50;  // min (darknes) :CD bright -> go to screen slip mode



// DIMM kanalu mainigie
// white, Royal Blue.., Blue, Royal Blue.,
// Red, Green, UV, Royal Blue

int DIMM_pin[]=     { 10,  9,  8,  7,  6,  5,  4,  3,  2 };
int DIMM_value[]=   { 76,128,220,150, 20,100,200, 36,128 };
int DIMM_actual[]=  {  0,  0,  0,  0,  0,  0,  0,  0,128 };
int DIMM_temp[]=    { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
int sunset_actual[]={  0,  0,  0,  0,  0,  0,  0,  0,  0 };
int DIMM_status[]=  {  0,  0,  0,  0,  0,  0,  0,  0,  0 };




// kanalu krasas nosaukums
char* DIMM_text[] ={
  "White",        //1
  "RB ..", //2
  "Blue",        //3
  "RB .",         //4
  "Green",    //5
  "UV",
  "Red",
  "RB",
  "LED Moon"
};



int DIMM_active=0;
int DIMM_increment_B=8.5;     //solis liela grafika dimmesanai
int DIMM_increment_S=18.2;    //solis mazaa grafika dimmesanai
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
  #if SERIAL
    Serial.begin(9600);
    Serial.println('Serial begin'); 
  #endif   
  


// pin modes
  pinMode(pinLCD, OUTPUT);      // LCD screen background
  pinMode(pinFAN, OUTPUT);      // FAN on/off

  // set up the LCD
  lcd.begin(DOG_LCD_M163, 0x27); // contrast from 0x1A to 0x28
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
  for (int i=0; i<=8; i++){ //0-7 LED, 8 - moon led
    ReadEEPROM(i);
    pinMode(DIMM_pin[i], OUTPUT);
  }
  ReadEEPROM(9); // load fan temperature data

  SetMenu();

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
    //Serial.println(Button);
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
   // lcd.setCursor(4, 0);
   // lcd.print("Reef LED");
    analogWrite(pinLCD, lcd_min);
  } 

  if (Button=='2') { // manual modes
      manual_on++;
      if (manual_on>2) manual_on=0;
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
          if (menu_mode==3) {// moon light
            SetBar();
            DimmingBIGStart();
            temperature_mode=3;
            cursor_pos=0;
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


          if (menu_mode==6) { // date/time
            lcd.setCursor(0, 0);
            lcd.print(flash_txt);
            temperature_mode=3;        
            cursor_pos=0;
            printTimeOnly();
          }

          if (menu_mode>=8 && menu_mode<=15) {
            SetBar();
            temperature_mode=3;
            DimmingBIGStart();
            cursor_pos=0;
          }
        }
    } else if (menu_mode==-2) { // screen saver
      // nope
    } else if (menu_mode==1 || menu_mode==2) {                 // dimming main color menu
      Dimming();
    } else if (menu_mode==3) {                // LED Moon settup
        cha=8;
        Dimming_big();
    } else if (menu_mode==4) {                                 // set temperature mode
      SetTemp();     
    } else if (menu_mode==6) {                                 // set date/time mode
      SetDateTime();
    } else if (menu_mode>=8 || menu_mode<=15) {                // individual chanel setup
        cha=menu_mode-8;
        Dimming_big();
    } else {
      // nope
}  


  if (timeout>50 && menu_mode==0) {   // screensaver on
    flash=2;
    menu_mode=-2;
    lcd.clear();
    analogWrite(pinLCD, lcd_min);
    SetBar();
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
    Watch(); // light timers
    Termo(); // temperature
    Voltage(); // voltage + current from power suply
    PrintStatus(); // show status un screen
  }
 
  


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
    } //2
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
      } else if (menu_mode>=8 && menu_mode<=15) {
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
  if (flash==2){ // iesl'dzas screensacer
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
  if (a<=7){ // save LED chanel data
    EEPROM.write((a*10), on_h[a]);
    EEPROM.write((1+a*10), on_m[a]);
    EEPROM.write((2+a*10), off_h[a]);
    EEPROM.write((3+a*10), off_m[a]);
    EEPROM.write((4+a*10), sunset[a]);
    EEPROM.write((5+a*10), DIMM_value[a]);
  }
  if (a==8){// save moon light data
    EEPROM.write(80, DIMM_value[a]);
  }
  if (a==9){// save fan data
    EEPROM.write(81,fan_on);
    EEPROM.write(82,fan_off);
  }  
}

void ReadEEPROM(int a){
  if (a<=7){ // load LED chanel data
    on_h[a] = EEPROM.read(a*10);
    on_m[a] = EEPROM.read(1+a*10);
    off_h[a] = EEPROM.read(2+a*10);
    off_m[a] = EEPROM.read(3+a*10);
    sunset[a] = EEPROM.read(4+a*10);
    DIMM_value[a] = EEPROM.read(5+a*10);
  }
  if (a==8){// load moon light data
    DIMM_value[a] = EEPROM.read(80);
  }
  if (a==9){// load fan settings
    fan_on = EEPROM.read(81);
    fan_off = EEPROM.read(82);
  }
  
  
}

// *********************************************************************************************
//            SetTemperature -  iestadisana
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
      aWrite(DIMM_pin[cha_incr+i], DIMM_value[cha_incr+i]);
    }  
  }  
}

// *********************************************************************************************
//            Zimejam LIELO BAR
// *********************************************************************************************
void BarL(int a) {// a cik
  int a_interpolating=a/8.5;
  int b=1;
  int c=1;
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

  if (b+d+1!=b+15) { // 7 cik zimigas pozicijas aiznjem BAR
    lcd.setCursor(b+d+1, c);
    lcd.write(" ");
  } 


  
  if (menu_mode!=3) { 
    lcd.setCursor(10, 2);
    lcd.print("      ");
    lcd.setCursor(10, 2);
    lcd.print(a/5.6);
    lcd.print("W");
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
  lcd.print(menu_text[menu_mode-1]);

  lcd.setCursor(5, 0);
  lcd.write(":");
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

  for (int i=0; i<= 8; i++){ // clear tmporary data, 8 Moon chanel
    DIMM_temp[i]=-1;
  } 
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
    // rakstam krasas nosaukumu
    lcd.setCursor(6, 0);
    lcd.write("     ");
    lcd.setCursor(6, 0);
    lcd.write(DIMM_text[cha_incr+cha+1]);    

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
  }  
  else if (qe1Move=='<') {
    DIMM_value[cha_incr+cha]=DIMM_value[cha_incr+cha]-DIMM_increment;
    if (DIMM_value[cha_incr+cha]<0) {
      DIMM_value[cha_incr+cha]=0;
    }    
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
    if (menu_mode==3) { // it kaa prueksh moon light ieprieks atslegsana
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
          sunset[cha]=sunset[cha]-5;
          if (sunset[cha]<0) sunset[cha]=60;
           flash_txt=toDouble(sunset[cha]);
          break;           
      }
    }
    
    if (cursor_pos==0) {
      if (DIMM_value[cha]!=DIMM_temp[cha]){
        BarL(DIMM_value[cha]);
        DIMM_temp[cha]=DIMM_value[cha];
        aWrite(DIMM_pin[cha], DIMM_value[cha]);
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
//            LDE kanalu Taimeru darbibas
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
  int now_mode=0; // 0 - normal, 1 sun Up, 2 sun Down fpr indicate sunset/sundown


  hour2 = RTC.get(DS1307_HR,true);
  minute2 = RTC.get(DS1307_MIN,false);
  second2 = RTC.get(DS1307_SEC,false);
  now_sec=((long(hour2)*3600)+(long(minute2)*60))+long(second2); // cout seconds
  
  // for all defined timer
  for (int i=0; i<=7; i++){
    // init for calcultion
    on_sec=(long(on_h[i])*3600)+(long(on_m[i])*60);
    off_sec=(long(off_h[i])*3600)+(long(off_m[i])*60); 
    tmp_sunset=long(sunset[i])*60;
    DIMM_long=long(DIMM_value[i]);


// normal situation - time_on < to time_off
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

// normals sunrise  
            if (now_sec<on_sec+tmp_sunset && now_sec>off_sec){                 
                 tmp=(((float)DIMM_long/(float)tmp_sunset)*(float)((on_sec+tmp_sunset)-now_sec));
      	         DIMM_long = DIMM_long -(long) tmp;
                 DIMM_actual[i]=(int) DIMM_long;
                 now_mode=2;
              }

// sunsets, kas turpinas pec pusnakts (iesledz 23.55, izsledz 02.00, sunrise 20)
	      if ((on_sec+tmp_sunset)>86400 && (now_sec-on_sec)<0 && now_sec<(tmp_sunset-(86400-on_sec))){
                  tmp=(((float)DIMM_long/(float)tmp_sunset)*(float)(((on_sec+tmp_sunset)-86400)-now_sec));		  
      	          DIMM_long = DIMM_long -(long) tmp;
                  DIMM_actual[i]=(int) DIMM_long;
                  now_mode=1;
              }
              
// normals sunset              
	      if (now_sec>=off_sec-tmp_sunset && now_sec<off_sec){                
                  tmp=((float)DIMM_long/(float)tmp_sunset)*(float)((off_sec-tmp_sunset)-now_sec);
                  tmp=abs(tmp);
      	          DIMM_long = DIMM_long -(long) tmp;
                  DIMM_actual[i]=(int) DIMM_long;
                  now_mode=1;
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

      if (manual_on==2) { aWrite(DIMM_pin[i], DIMM_actual[i]); }
      if (manual_on==0) { aWrite(DIMM_pin[i], 0); }
      if (manual_on==1) { aWrite(DIMM_pin[i], DIMM_value[i]); }
    
      if (menu_mode==-2) {
        lcd.setCursor(LED_pos[i], LED_line[i]);
        lcd.print(toTriple(DIMM_actual[i]));
        
        lcd.setCursor(LED_pos[i]-1, LED_line[i]);
        if (now_mode==0) {
          lcd.print(" ");
        }
        if (now_mode==1) {
          lcd.print("+");
        }
        if (now_mode==2) {
          lcd.print("-");
        }        
      }
      

  } // for 

}



// *********************************************************************************************
//            LED light analogWrite emulate
// *********************************************************************************************
void aWrite (int pin, int value){
 if (temp[0]>=fan_warning){ // 50% of LED's power
 value=value/2;
 }

 if (temp[0]>=fan_alert){ // turn off all LED's
 value=0;
 }
 analogWrite(pin, value);
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
//            Voltage
// *********************************************************************************************

void Voltage(){
  // read the value on analog input
  //for ( int i = 0; i < 2; i++) {
    /*
    volt_sensor_value = analogRead(volt_sensot_PIN[0]);
    vout = (volt_sensor_value * 5.0) / 1024.0;
    vin = vout / (R2/(R1+R2)); 
    volt[0]=(volt[0]+vin)/2; 
    */
    //volt[0]=volt_sensor_value;
 // }
 /*
  Serial.print("Vin=");
  Serial.print(vin);
  Serial.print("V");

  Serial.print("  V=");
  Serial.print(volt[0]);
  Serial.print("V");
  
  
  Serial.print("  value=");
  Serial.print(volt_sensor_value);

  
  Serial.println();
 */
 
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

void ScreenSaver(){ //zerro

  currentSens1 = analogRead(A1_pin);           
  // convert to milli amps
  CurrentValue1 = (((long)currentSens1 * 5000 / 1024) - 500 ) * 1000 / 100;
  amps1 = (float) CurrentValue1 / 1000;
 // lcd.setCursor(8, 1);
  
 // lcd.print(amps1); 
 // lcd.print("mA");
  //lcd.print(currentSens1);
  float watts = (amps1 * volt[0])/100;
  //lcd.setCursor(8, 2);
  
 // lcd.print(watts);
 // lcd.print("W");
  
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
