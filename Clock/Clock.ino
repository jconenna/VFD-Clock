// Arduino controlled Vacuum Flourescent Clock with RTC module
// Last Modified: January 8th 2016
// Author: Joey Conenna

// Libraries for use with DS3232 RTC 
#include <DS3232RTC.h>  
#include <Time.h>    
#include <Wire.h>  

#define SQW_PIN 2 // Pin to listen to the 1 HZ Square Wave from the RTC
#define dt 1      // delay time

//Pin connected to ST_CP of both 74HC595
#define LATCH  8
//Pin connected to SH_CP of both 74HC595
#define CLOCK 12
////Pin connected to DS of Left 74HC595
#define DATA  11

// Bytes to control VFD, 1 SEG OFF, 0 SEG ON. 
byte L = B11111111;      // x1, x2, s1, x3, s2, s3, AM, PM  // Left shift register pinout to VFD 
byte R = B11111111;      // x2a/b, a, b, c, d, e, f, g      // Right shift register pinout to VFD 

// Flag for RTC 1HZ SQ Wave ISR
boolean interruptSet = false;

// Vars to hold time data after fetching from RTC upon setup
int seconds = 0;
int minutes = 0;
int hours = 0;
boolean AM = true; 

void setup() 
{
  // Set latch, clock, and data to OUTPUT to control Shift Register
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(DATA , OUTPUT);

  // Fetch time from RTC 
  tmElements_t tm;
  RTC.read(tm);
  hours = tm.Hour;
  minutes = tm.Minute;
  seconds = tm.Second;

  // Switch 24 HR to 12 HR format
  if(hours / 12 == 1)
    {
      AM = false;
      setPM();
      if(hours > 12)
        hours = hours % 12;
      else hours = 12;
    }
  else
      setAM();
  if(hours > 9)          // If hour is now 10-12 Set '1' segments
      setTen();
  
    
  // Set a 1 Hz Square Wave from the RTC
  RTC.squareWave(SQWAVE_NONE);
  RTC.squareWave(SQWAVE_1_HZ); 
  
  // Set ISR on Pin 2 to interrupt each second with the Square Wave
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), tick, FALLING);
}


void loop() 
{
  
  // When ISR changes interruptSet flag every second
  
  if(interruptSet)
  {
    if(seconds < 59)            // Update seconds
    {
      seconds++;
    }
    
    else                        // Unless time for a new minute
      {
      seconds = 0;
      if(minutes < 59)          // Update minutes
      {
        minutes++;
      }
      
      else                      // Unless time for a new hour
      {
        minutes = 0;
        if(hours < 12)          // Update hours
        {
         hours++;  
         if(hours > 9)          // If hour is now 10-12 Set '1' segments
           setTen();
         if(hours == 12)        // If hour is now 12 flip AM/PM
         {
           if(AM == true)
             {
              AM = false;
              setPM();
             }
           else
           {
            AM = true;
            setAM();
           }
         }
        }
        
        else                    // If hour was 12, turn off '1' sements and set hours to 1
        {
         clearTen();
         hours = 1;
        }
      }
    }
    interruptSet = false;       // Reset interrupt flag 
  }


  // Each loop iterration turns on and off each segment in a sequence to multiplex the VFD 
  
  // Turn on AM/PM, '1', and colon breifly
  setSegment(0);
  setSegment(1);
  setSegment(3);
  setDisplay();
  delay(dt);
  clearSegment(0);
  clearSegment(1);
  clearSegment(3);
  
  // Turn on hours 7 Segment briefly
  setNumber(hours%10);
  setSegment(2);
  setDisplay();
  delay(dt);
  clearSegment(2);
  
  // Turn on first minutes 7 Segment briefly
  setNumber(minutes/10);
  setSegment(4);
  setDisplay();
  delay(dt);
  clearSegment(4);
  
  // Turn on second minutes 7 Segment briefly
  setNumber(minutes%10);
  setSegment(5);
  setDisplay();
  delay(dt);
  clearSegment(5);
 
} // loop

// Note for the following bitwise operations for the L and R bytes, that logical 1 = VFD segment OFF, and logical 0 = VFD segment ON
// See schematic and note that a transistor is switched OFF when the output from the shift register is HIGH, which turns off a segment,
// while a transistor is OFF when the shift register outputs LOW, which leaves a segment connected to 12 V rather than GND. 

void setSegment(int seg) // seg 0-5
{
  L &= ~(1 << (7 - seg));
}

void clearSegment(int seg)
{
  L |= (1 << (7 - seg)); 
}


void setAM() 
{
  L |=  (1 << 0); // Turn OFF PM
  L &= ~(1 << 1); // Turn ON  AM
}

void setPM() 
{
  L &= ~(1 << 0); // Turn ON  PM
  L |=  (1 << 1); // Turn OFF AM
}

void setTen() // Turn ON '1' VFD segment
{
  R &= ~(1 << 7);
}

void clearTen() // Turn OFF '1' VFD segment
{
  R |= (1 << 7);
}

void setNumber(int num) // Sets bits in R byte for each VFD pin controlling each segment of the 0-9 segment group
{

  switch(num)

  { 
    case 0:
    R &= ~(1 << 6); // a
    R &= ~(1 << 5); // b
    R &= ~(1 << 4); // c
    R &= ~(1 << 3); // d
    R &= ~(1 << 2); // e
    R &= ~(1 << 1); // f
    R |=  (1 << 0); // g
    break;
    
    case 1:
    R |=  (1 << 6);
    R &= ~(1 << 5);
    R &= ~(1 << 4);
    R |=  (1 << 3);
    R |=  (1 << 2);
    R |=  (1 << 1);
    R |=  (1 << 0);
    break;

    case 2:
    R &= ~(1 << 6);
    R &= ~(1 << 5);
    R |=  (1 << 4);
    R &= ~(1 << 3);
    R &= ~(1 << 2);
    R |=  (1 << 1);
    R &= ~(1 << 0);
    break;

    case 3:
    R &= ~(1 << 6);
    R &= ~(1 << 5);
    R &= ~(1 << 4);
    R &= ~(1 << 3);
    R |=  (1 << 2);
    R |=  (1 << 1);
    R &= ~(1 << 0);
    break;

    case 4:
    R |=  (1 << 6);
    R &= ~(1 << 5);
    R &= ~(1 << 4);
    R |=  (1 << 3);
    R |=  (1 << 2);
    R &= ~(1 << 1);
    R &= ~(1 << 0);
    break;

    case 5:
    R &= ~(1 << 6);
    R |=  (1 << 5);
    R &= ~(1 << 4);
    R &= ~(1 << 3);
    R |=  (1 << 2);
    R &= ~(1 << 1);
    R &= ~(1 << 0);
    break;

    case 6:
    R &= ~(1 << 6);
    R |=  (1 << 5);
    R &= ~(1 << 4);
    R &= ~(1 << 3);
    R &= ~(1 << 2);
    R &= ~(1 << 1);
    R &= ~(1 << 0);
    break;

    case 7:
    R &= ~(1 << 6);
    R &= ~(1 << 5);
    R &= ~(1 << 4);
    R |=  (1 << 3);
    R |=  (1 << 2);
    R |=  (1 << 1);
    R |=  (1 << 0);
    break;

    case 8:
    R &= ~(1 << 6);
    R &= ~(1 << 5);
    R &= ~(1 << 4);
    R &= ~(1 << 3);
    R &= ~(1 << 2);
    R &= ~(1 << 1);
    R &= ~(1 << 0);
    break;
    
    case 9:
    R &= ~(1 << 6);
    R &= ~(1 << 5);
    R &= ~(1 << 4);
    R |=  (1 << 3);
    R |=  (1 << 2);
    R &= ~(1 << 1);
    R &= ~(1 << 0);
    break;
  }

}

void tick() // Interrupt Service Routine (ISR) is executed when the 1 HZ square-wave from the RTC on pin 2 goes from HIGH to LOW.
{
   interruptSet = true; // Set ISR flag ON
}

void setDisplay()  // Shifts out L and R bytes to shift registers and sets the states on for the VFD 
{
  
  // Set LATCH LOW to begin shifting out bytes to the shift register
  digitalWrite(LATCH, LOW);
  
  // Shift out the bytes
  shiftOut(DATA, CLOCK, LSBFIRST, R);  
  shiftOut(DATA, CLOCK, LSBFIRST, L);
    
  // Set LATCH HIGH to write the memory register to the output pins, turning on the segments
  digitalWrite(LATCH, HIGH);
}
