#include <Arduino.h>
#include <TM1637Display.h>
#include <RunningMedian.h>

/*
   display_workout(mode)
   Convert mode to TM1637 segments
   return segments to display

*/


// pin definitions
const int freezerTempPin = A2; // analog read freezer temperature
const int fridgeTempPin = A3;  // analog read fridge temperature
const int displayCLKPin = 2;  // TM11637 CLK - Digital pin 2, physical pin 7
const int displayDIOPin = 1;  // TM11637 DIO - Digital pin 1, physical pin 6
const int doorAjarPin = 0;    // digital read if door is opened, PWM output for alarm

// Thermistor constants
// See also https://www.instructables.com/ATTiny85-Mono-Temperature-Monitor/
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and find median
// but is more 'smooth'
#define NUMSAMPLES 11
// The beta coefficient of the thermistor (usually 3000-4000)
// Model#: NTCLE350E4103JHB0
#define BCOEFFICIENT 3984
// the value of the 'other' resistor
#define SERIESRESISTOR 10000

RunningMedian freezerSamples = RunningMedian(NUMSAMPLES);
RunningMedian fridgeSamples = RunningMedian(NUMSAMPLES);

TM1637Display display(displayCLKPin, displayDIOPin);

boolean blinkFlag = false;
boolean doorAjarFlag = false;

/*
   Read freezer and fridge voltage, convert to temp F

   If door ajar goes high, begin timer and start flashing the temp display
   If timer exceeds low threshold, begin flashing temps
   If timer exceed high threshold, change doorAjarPin to output
*/

// default parameters
int freezerTemp = 0;
int fridgeTemp = 37;
int displayValue = fridgeTemp * 100 + freezerTemp;
unsigned long openedMillis;
unsigned long audioThreshold = 20000; // milliseconds of delay before audible alarm

void setup() {
  // test audio
  pinMode(doorAjarPin, OUTPUT);
  delay(100);
  tone(doorAjarPin, 262, 500);
  delay(100);
  tone(doorAjarPin, 220, 250);
  delay(100);
  tone(doorAjarPin, 330, 500);
  delay(1000);
  pinMode(doorAjarPin, INPUT_PULLUP);
  delay(1000);
  
  pinMode(freezerTempPin, INPUT);
  pinMode(fridgeTempPin, INPUT);
  pinMode(doorAjarPin, INPUT_PULLUP);
  pinMode(displayCLKPin, OUTPUT);
  pinMode(displayDIOPin, OUTPUT);

  display.clear();
  display.setBrightness(4); //0 (lowes brightness) to 7 (highest brightness)
}

int convertTemp(float measured) {
  // convert the value to resistance
  float average = 1023.0 / measured - 1.0;
  float R = SERIESRESISTOR / average;

  float calcnum;
  calcnum = R / THERMISTORNOMINAL;                // (R/Ro)
  calcnum = log(calcnum);                         // ln(R/Ro)
  calcnum /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
  calcnum += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  calcnum = 1.0 / calcnum;                        // Invert
  calcnum -= 273.15;                              // convert to C

  float calcnumF;
  calcnumF = (9.0 / 5.0) * calcnum + 32;
  return calcnumF;
}

void loop() {
  // put your main code here, to run repeatedly:

  // Read door pin, if door is open, blank the display during calculations
  if (digitalRead(doorAjarPin)) {
    // Door is ajar. Check if this is new
    display.clear();
    delay(50);

    if (doorAjarFlag) {
      if ((unsigned long)(millis() - openedMillis) >= audioThreshold) {
        // Door opened too long
        // Change doorAjarPin to output, play notes

        pinMode(doorAjarPin, OUTPUT);
        delay(100);
        tone(doorAjarPin, 262, 500);
        delay(100);
        tone(doorAjarPin, 220, 250);
        delay(100);
        tone(doorAjarPin, 330, 500);
        delay(100);
        display.setBrightness(0); //0 (lowes brightness) to 7 (highest brightness)
        delay(1000);
        pinMode(doorAjarPin, INPUT_PULLUP);
        delay(1000);
        doorAjarFlag = false;

      }
    } else {
      // Door is newly ajar. Begin timer.
      openedMillis = millis();
      doorAjarFlag = true;
    }
  } else {
    // pin is pulled to ground, door is closed
    doorAjarFlag = false;
  }

    freezerSamples.add(analogRead(freezerTempPin));
    delay(5);
    fridgeSamples.add(analogRead(fridgeTempPin));
    delay(5);
    fridgeTemp = convertTemp(fridgeSamples.getMedian());
    freezerTemp = convertTemp(freezerSamples.getMedian());


  // Update display
  displayValue = fridgeTemp * 100 + freezerTemp;
  display.setBrightness(4); //0 (lowes brightness) to 7 (highest brightness)
  display.showNumberDec(displayValue, true);
  delay(100);

}
