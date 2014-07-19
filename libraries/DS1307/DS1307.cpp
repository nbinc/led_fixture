/*
 *
 ********************************************************************************************************
 * 			DS1307.cpp 	 library for DS1307 I2C rtc clock				*
 ********************************************************************************************************
 *
 * Created by D. Sjunnesson 1scale1.com d.sjunnesson (at) 1scale1.com
 * Modified by bricofoy - bricofoy (at) free.fr
 *
 * Created with combined information from
 *  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1180908809
 *  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1191209057
 *
 * Big credit to  mattt (please contact me for a more correct name...) from the Arduino forum
 * which has written the main part of the library which I have modified
 *
 * Rev history :
 *
 * ??-??-??	- mattt & dsjunnesson 	- creation
 * 19-feb-2012	- bricofoy 		- added arduino 1.0 compatibility
 * 20-feb-2012	- bricofoy 		- bugfix : time was not preserved when setting or stopping clock
 * 21-feb-2012	- bricofoy 		- bugfix : preserve existing seconds when starting/stopping clock
 *
 *TODO: enable AM/PM
 *	enable square wave output
 *
 */
#include "DS1307.h"

DS1307::DS1307()
{
  Wire.begin();
}

DS1307 RTC=DS1307();

// PRIVATE FUNCTIONS

// Aquire data from the RTC chip in BCD format
// refresh the buffer
void DS1307::read(void)
{
  // use the Wire lib to connect to the rtc
  // reset the register pointer to zero
  Wire.beginTransmission(DS1307_CTRL_ID);

  #if defined(ARDUINO) && ARDUINO >= 100
  Wire.write((byte)0x00);//workaround for issue #527
  #else
  Wire.send(0x00);
  #endif

  Wire.endTransmission();

  // request the 7 bytes of data    (secs, min, hr, dow, date. mth, yr)
  Wire.requestFrom(DS1307_CTRL_ID, 7);
  for(int i=0; i<7; i++)
  {
    // store data in raw bcd format
    #if defined(ARDUINO) && ARDUINO >= 100
    rtc_bcd[i]=Wire.read();
    #else
    rtc_bcd[i]=Wire.receive();
    #endif
  }
}

// update the data on the IC from the bcd formatted data in the buffer
void DS1307::save(void)
{
  Wire.beginTransmission(DS1307_CTRL_ID);

  #if defined(ARDUINO) && ARDUINO >= 100
  Wire.write((byte)0x00);// reset register pointer
  #else
  Wire.send(0x00);
  #endif

  for(int i=0; i<7; i++)
  {
    #if defined(ARDUINO) && ARDUINO >= 100
    Wire.write(rtc_bcd[i]);
    #else
    Wire.send(rtc_bcd[i]);
    #endif
  }
  Wire.endTransmission();
}


// PUBLIC FUNCTIONS
void DS1307::get(int *rtc, boolean refresh)   // Aquire data from buffer and convert to int, refresh buffer if required
{
  if(refresh) read();
  for(int i=0;i<7;i++)  // cycle through each component, create array of data
  {
    rtc[i]=get(i, 0);
  }
}

int DS1307::get(int c, boolean refresh)  // aquire individual RTC item from buffer, return as int, refresh buffer if required
{
  if(refresh) read();
  int v=-1;
  switch(c)
  {
  case DS1307_SEC:
    v=(10*((rtc_bcd[DS1307_SEC] & DS1307_HI_SEC)>>4))+(rtc_bcd[DS1307_SEC] & DS1307_LO_BCD);
    break;
  case DS1307_MIN:
    v=(10*((rtc_bcd[DS1307_MIN] & DS1307_HI_MIN)>>4))+(rtc_bcd[DS1307_MIN] & DS1307_LO_BCD);
    break;
  case DS1307_HR:
    v=(10*((rtc_bcd[DS1307_HR] & DS1307_HI_HR)>>4))+(rtc_bcd[DS1307_HR] & DS1307_LO_BCD);
    break;
  case DS1307_DOW:
    v=rtc_bcd[DS1307_DOW] & DS1307_LO_DOW;
    break;
  case DS1307_DATE:
    v=rtc_bcd[DS1307_DATE]/16 * 10 +  rtc_bcd[DS1307_DATE] % 16;
    break;
  case DS1307_MTH:
    v=(10*((rtc_bcd[DS1307_MTH] & DS1307_HI_MTH)>>4))+(rtc_bcd[DS1307_MTH] & DS1307_LO_BCD);
    break;
  case DS1307_YR:
    v=2000 + rtc_bcd[DS1307_YR]/16 * 10 + rtc_bcd[DS1307_YR] % 16;
    break;
  } // end switch
  return v;
}

void DS1307::set(int c, int v)  // Update buffer, then update the chip
{
  read(); //first read the buffer to preserve current time when writing modified values, as the function writes all values at the same time
  switch(c)
  {
  case DS1307_SEC:
    if(v<60 && v>-1)
    {
      //preserve existing clock state (running/stopped)
      int state=rtc_bcd[DS1307_SEC] & DS1307_CLOCKHALT;
      rtc_bcd[DS1307_SEC]=state | ((v / 10)<<4) + (v % 10);
    }
    break;
  case DS1307_MIN:
    if(v<60 && v>-1)
    {
      rtc_bcd[DS1307_MIN]=((v / 10)<<4) + (v % 10);
    }
    break;
  case DS1307_HR:
    // TODO : AM/PM  12HR/24HR
    if(v<24 && v>-1)
    {
      rtc_bcd[DS1307_HR]=((v / 10)<<4) + (v % 10);
    }
    break;
  case DS1307_DOW:
    if(v<8 && v>-1)
    {
      rtc_bcd[DS1307_DOW]=v;
    }
    break;
  case DS1307_DATE:
    if(v<32 && v>-1)
    {
      rtc_bcd[DS1307_DATE]=((v / 10)<<4) + (v % 10);
    }
    break;
  case DS1307_MTH:
    if(v<13 && v>-1)
    {
      rtc_bcd[DS1307_MTH]=((v / 10)<<4) + (v % 10);
    }
    break;
  case DS1307_YR:
    if(v<50 && v>-1)
    {
      rtc_bcd[DS1307_YR]=((v / 10)<<4) + (v % 10);
    }
    break;
  } // end switch
  save();
}

void DS1307::stop(void)
{
  // set the ClockHalt bit high to stop the rtc
  // this bit is part of the seconds byte
  read(); 	//refresh buffer first to preserve existing time
  rtc_bcd[DS1307_SEC]=rtc_bcd[DS1307_SEC] | DS1307_CLOCKHALT; //set the halt bit in the seconds value
  save(); //write register to the chip
}

void DS1307::start(void)
{
  // unset the ClockHalt bit to start the rtc
  read();				 //refresh buffer to get existing time
  rtc_bcd[DS1307_SEC]-=DS1307_CLOCKHALT; //unset the halt bit in the seconds value
  save(); //write register to the chip
}


