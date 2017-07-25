/*  Another stupid clock project.
    (c) A.G.Doswell 2017
    License: The MIT License (See full license at the bottom of this file)

    PWM referenced clock, driving 3 analogue moving coil meters, set up to read hours, minutes and seconds.
    Has remote 433 MHz receiver for reception of remote time sync signal from master clock project

    See http://www.andydoz.blogspot.com/
    for more information and the circuit diagram.

*/
int clockInt = 0;            // Digital pin 2 is now interrupt 0
int masterClock = 0;         // Counts rising edge clock signals
int seconds = 0;             // Seconds
int minutes = 00;            // Minutes
int hours = 00;              // Hours
int secondsAnaloguePin = 3;  // Output pin for the Seconds driver circuit and meter
int minutesAnaloguePin = 5;  // As above for minutes...
int hoursAnaloguePin = 6;    // ... and hours
int calPin = 7;              // Calibration push button connects here and ground
int minAdjustPin = 8;        // Minutes adjust button to pin 8....
int hoursAdjustPin = A2;     // ... and hours to A2
int twentyFourHourPin = 14;  // 24 hour mode selct switch connects to A0
int smoothSelectPin = 15;    // Smooth mode select switch connects to A1
int displayHours;            // Actual hours displayed, this is to take care of the 24h mode.
unsigned long totalSecondsHour; // Used in smooth mode, humber of current seconds in the hour
unsigned long totalSecondsMinute; // Used in smooth mode, current seconds in a minute
unsigned long smoothSeconds; // Used in Ssmooth mode. Number of clock "ticks" in a minutes worth of seconds.

// set up for remote master clock
#include <VirtualWire.h> // available from https://www.pjrc.com/teensy/td_libs_VirtualWire.html
#include <VirtualWire_Config.h>
int rxTX_ID = 1; //rx indicates remotely received data
int rxHours;
int rxMins;
int rxSecs;
int rxDay;
int rxMonth;
int rxYear;
char buffer[5];

void setup() {

  pinMode (secondsAnaloguePin, OUTPUT);
  pinMode (minutesAnaloguePin, OUTPUT);
  pinMode (hoursAnaloguePin, OUTPUT);
  pinMode (calPin, INPUT_PULLUP);  // cal switch connected to pin 7. This allows FSD on meters to be set
  pinMode (minAdjustPin, INPUT_PULLUP);  // Minutes adjust switch
  pinMode (hoursAdjustPin, INPUT_PULLUP);  // Hours adjust switch
  pinMode (twentyFourHourPin, INPUT_PULLUP); // 24 hour clock mode switch
  pinMode (smoothSelectPin, INPUT_PULLUP); // Smooth select switch
  attachInterrupt(clockInt, clockCounter, RISING); //  clockInt is the interrupt, clockCounter function is called when invoked on a RISING clock edge
  analogWrite(11, 127);     // This starts our PWM 'clock' ticking with a 490 Hz 50% duty cycle
  vw_set_tx_pin(12); // Even though it's not used, we'll define it.
  vw_set_rx_pin(13); // RX pin set to pin 13
  vw_setup(1200); // Sets virtualwire for a tx rate of 1200 bits per second
  vw_rx_start(); // Start the receiver
}


void clockCounter()        // Called by interrupt, driven by the PWM
{
  masterClock ++;          // With each clock rise add 1 to masterClock count
  smoothSeconds ++;   // Used to develop smooth seconds
  if (masterClock >= 490)  // 490Hz reached
  {
    seconds ++;            // Add 1 second
    masterClock = 0;       // Reset
  }
  return;
}

void updateClockDisplaySmooth() {

  totalSecondsMinute = (seconds + (minutes * 60)); //Number of seconds in the current minute
  totalSecondsHour = ((seconds / 10) + (minutes * 6) + (displayHours * 360)); // Hours as seconds divided by 10, so as not to exceed a 16 bit unsigned integer
  int x = map(smoothSeconds, 0, 29400, 0, 255); // Map smoothSeconds to 8-bit value
  analogWrite(3 , x);                           // Update PWM
  x = map (totalSecondsMinute, 0, 3600, 0, 255);    // Map and update PWM for minutes
  analogWrite(5, x);
  if (digitalRead (twentyFourHourPin) == true) {
    x = map (totalSecondsHour, 0, 8640, 0, 255);     // Map and update the PWM for hours, conditionally on 24h mode.
  }
  else {
    x = map (totalSecondsHour, 0, 4320, 0, 255);
  }
  analogWrite(6, x);
}

void updateClockDisplayCoarse() {
  int x = map(seconds, 0, 59, 0, 255); // Map seconds to 8-bit value and update PWM
  analogWrite(3 , x);
  x = map (minutes, 0, 60, 0, 255); // As above for minutes....
  analogWrite(5, x);
  if (digitalRead (twentyFourHourPin) == true) { // ... and hours, conditionally on 24h mode.
    x = map(displayHours, 0, 24, 0, 255);
  }
  else {
    x = map(displayHours, 0, 12, 0, 255);
  }
  analogWrite(6, x);
}

void remoteClockSet () { // Remotely sets the clock
  typedef struct rxRemoteData //Defines the received data, this is the same as the TX struct
  {
    int rxTX_ID;
    int rxHours;
    int rxMins;
    int rxSecs;
    int rxDay;
    int rxMonth;
    int rxYear;

  };

  struct rxRemoteData receivedData;
  uint8_t rcvdSize = sizeof(receivedData);

  if (vw_get_message((uint8_t *)&receivedData, &rcvdSize)) // Process message if received
  {
    if (receivedData.rxTX_ID == 1)     { //Only if the TX_ID=1 do we process the data.
      int TX_ID = receivedData.rxTX_ID;
      int rxHours = receivedData.rxHours;
      int rxMins = receivedData.rxMins;
      int rxSecs = receivedData.rxSecs;
      int rxDay =  receivedData.rxDay;
      int rxMonth =  receivedData.rxMonth;
      int rxYear =  receivedData.rxYear;

      if (isBST() == true) { // Is it British Summer Time?
        rxHours = rxHours + 1;
      }

      // Set the clock
      hours = rxHours;
      minutes = rxMins;
      seconds = rxSecs;
    }
  }
}

boolean isBST() // this bit of code blatantly plagarised from http://my-small-projects.blogspot.com/2015/05/arduino-checking-for-british-summer-time.html
{
  int imonth = rxMonth;
  int iday = rxDay;
  int hr = rxHours;

  //January, february, and november are out.
  if (imonth < 3 || imonth > 10) {
    return false;
  }
  //April to September are in
  if (imonth > 3 && imonth < 10) {
    return true;
  }

  // find last sun in mar and oct - quickest way I've found to do it
  // last sunday of march
  int lastMarSunday =  (31 - (5 * rxYear / 4 + 4) % 7);
  //last sunday of october
  int lastOctSunday = (31 - (5 * rxYear / 4 + 1) % 7);

  //In march, we are BST if is the last sunday in the month
  if (imonth == 3) {

    if ( iday > lastMarSunday)
      return true;
    if ( iday < lastMarSunday)
      return false;

    if (hr < 1)
      return false;

    return true;

  }
  //In October we must be before the last sunday to be bst.
  //That means the previous sunday must be before the 1st.
  if (imonth == 10) {

    if ( iday < lastOctSunday)
      return true;
    if ( iday > lastOctSunday)
      return false;

    if (hr >= 1)
      return false;

    return true;
  }

}

void loop() {
  remoteClockSet ();
  if (seconds >= 60) {   // Put down your knitting grandma, the real timekeeping starts here
    minutes ++;          // Increment minutes by 1
    seconds = 0;         // Reset the seconds
    smoothSeconds = 0;
  }

  if (minutes >= 60) {
    hours ++;             // Increment hours
    minutes = 0;          // Reset minutes
  }

  if (hours >= 24) {
    hours = 0;
  }

  if (digitalRead (twentyFourHourPin) == true) { // If the 24 hour select is true, then set the display hours
    displayHours = hours;
  }
  else {                  // If 24 hours is not selected, set the hours conditionally set the display hours - 12
    if (hours >= 12) {
      displayHours = hours - 12;
    }
    else {
      displayHours = hours;
    }
  }

  if (digitalRead (smoothSelectPin) == true) {
    updateClockDisplaySmooth ();
  }
  else {
    updateClockDisplayCoarse ();
  }

  if (digitalRead (calPin) == LOW) { // Cal button pressed
    analogWrite (3, 0);   // Sets all meters to 0% to set zero for 10 seconds
    analogWrite (5, 0);
    analogWrite (6, 0);
    delay (10000);
    analogWrite (3, 255); // Sets all meters to 100% to set FSD for 10 seconds
    analogWrite (5, 255);
    analogWrite (6, 255);
    delay (10000);
  }

  if (digitalRead (minAdjustPin) == LOW) {   // Minutes adjust switch
    detachInterrupt (clockInt);   // Temporaily stop the clock running
    seconds = 0;                  // Zero the seconds, so if the display is set to smooth, it's setting is not confused by the seconds count
    smoothSeconds = 0;
    minutes ++;                   // Increment minutes

    updateClockDisplayCoarse();   // Update the display as coarse, regardless of switch setting
    delay (750);
    attachInterrupt(clockInt, clockCounter, RISING);    // Restart clock
  }

  if (digitalRead (hoursAdjustPin) == LOW) {   // Hours adjust switch
    detachInterrupt (clockInt);   // Temporaily stop the clock running
    seconds = 0;
    smoothSeconds = 0;
    hours ++;                     // Increment hours
    int minutesTemp = minutes;    // Temporarily store the minutes
    minutes = 0;                  // Zero the minutes, just so the hours display is not confused during setting by a high number of minutes, if the display is set to smooth
    updateClockDisplayCoarse ();  // Update the display as coarse, regardless of switch setting
    delay (750);
    minutes = minutesTemp;        // Restore the correct minutes.
    attachInterrupt(clockInt, clockCounter, RISING);   // Restart clock
  }
}
/*
   Copyright (c) 2017 Andrew Doswell

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute and sublicense 
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   Any commercial use is prohibited without prior arrangement.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESerial FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHOR(S) OR COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

