/*  Another stupid clock project.
    (c) A.G.Doswell 2017

    PWM referenced clock, driving 3 analogue moving coil meters, set up to read hours, minutes and seconds.

    See http://www.andydoz.blogspot.com/
    for more information and the circuit diagram.

*/
int clockInt = 0;            // Digital pin 2 is now interrupt 0
int masterClock = 0;         // Counts rising edge clock signals
int seconds = 0;             // variable
int minutes = 00;            // variable
int hours = 00;
int secondsAnaloguePin = 3;
int minutesAnaloguePin = 5;
int hoursAnaloguePin = 6;
int calPin = 7;
int minAdjustPin = 8;
int hoursAdjustPin = 10;
int twentyFourHourPin = 14; // Pin A0
int smoothSelectPin = 15;   // Pin A1
int displayHours;
unsigned long totalSecondsHour;
unsigned long totalSecondsMinute;
unsigned long smoothSeconds;

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
  analogWrite(9, 127);     // this starts our PWM 'clock' with a 50% duty cycle
  Serial.begin (115200);
}


void clockCounter()        // Called by interrupt, driven by the PWM
{
  masterClock ++;          // With each clock rise add 1 to masterClock count
  smoothSeconds ++;   // Used to develop smooth seconds
  if (masterClock >= 490)  // 490Hz reached
  {
    seconds ++;            // After one 490Hz cycle add 1 second ;)
    masterClock = 0;       // Reset after 1 second is reached
  }
  return;
}

void updateClockDisplaySmooth() {

  totalSecondsMinute = (seconds + (minutes * 60));
  totalSecondsHour = ((seconds / 10) + (minutes * 6) + (displayHours * 360)); // Hours as seconds divided by 10, so as not to exceed a 16 bit unsigned integer
  int x = map(smoothSeconds, 0, 29400, 0, 255); // Map smoothSeconds to 8-bit value
  analogWrite(3 , x);                           // Update PWM
  x = map (totalSecondsMinute, 0, 3600, 0, 255);    // Map and update for minutes
  analogWrite(5, x);
  if (digitalRead (twentyFourHourPin) == true) {
    x = map (totalSecondsHour, 0, 8640, 0, 255);     // Map and update the PWM for hours
  }
  else {
    x = map (totalSecondsHour, 0, 4320, 0, 255);
  }
  analogWrite(6, x);

}

void updateClockDisplayCoarse() {
  int x = map(seconds, 0, 59, 0, 255); // map seconds to 8-bit value
  analogWrite(3 , x);
  x = map (minutes, 0, 60, 0, 255);
  analogWrite(5, x);
  if (digitalRead (twentyFourHourPin) == true) {
    x = map(displayHours, 0, 24, 0, 255);
  }
  else {
    x = map(displayHours, 0, 12, 0, 255);
  }
  analogWrite(6, x);
}

void loop() {
  if (seconds == 60) {   // Real timekeeping starts here
    minutes ++;          // Increment minutes by 1
    seconds = 0;         // Reset the seconds
    smoothSeconds=0;
  }

  if (minutes == 60) {
    hours ++;             // Increment hours
    minutes = 0;          // Reset minutes
  }

  if (hours == 24) {
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

  if (digitalRead (7) == LOW) { // Cal button pressed
    analogWrite (3, 0);   // Sets all meters to 0% to set zero for 10 seconds
    analogWrite (5, 0);
    analogWrite (6, 0);
    delay (10000);
    analogWrite (3, 255); // Sets all meters to 100% to set FSD for 10 seconds
    analogWrite (5, 255);
    analogWrite (6, 255);
    delay (10000);
  }

  if (digitalRead (8) == LOW) {   // Minutes adjust switch
    detachInterrupt (clockInt);   // Temporaily stop the clock running
    seconds = 0;                  // Zero the seconds, so if the display is set to smooth, it's setting is not confused by the seconds count
    smoothSeconds = 0;
    minutes ++;                   // Increment minutes

    updateClockDisplayCoarse();   // Update the display as coarse, regardless of switch setting
    delay (750);
    attachInterrupt(clockInt, clockCounter, RISING);    // Restart clock
  }

  if (digitalRead (10) == LOW) {   // Hours adjust switch
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
  //debugging
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.print(seconds);
  Serial.print("  ");
  Serial.print(displayHours);
  Serial.print("   ");
  Serial.println(smoothSeconds);
}


