#include <Servo.h>
/*
 *This program modifies the Frequency Detection program so that a servo motor
 *can turn the tuning pegs on a guitar to tune it.
 *
 */

// clipping indicator variables
boolean clipping = 0;

// data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time =
    0;         // keeps time and sends vales to store in timer[] occasionally
int timer[10]; // sstorage for timing of events
int slope[10]; // storage fro slope of events
unsigned int totalTimer; // used to calculate period
unsigned int period;     // storage for period of wave
byte index = 0;          // current storage index
float frequency;         // storage for frequency calculations
int maxSlope = 0;        // used to calculate max slope as trigger point
int newSlope;            // storage for incoming slope data

// variables for decided whether you have a match
byte noMatch = 0;  // counts how many non-matches you've received to reset
                   // variables if it's been too long
byte slopeTol = 3; // slope tolerance- adjust this if you need
int timerTol = 10; // timer tolerance- adjust this if you need
Servo myservo;     // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 90; // variable to store the servo position and start it in middle
const int buttonPin = 2; // the number of the pushbutton pin
int buttonState = 0;     // variable for reading the pushbutton status
// Constant string frequencies
float E = 82.4; // low e string
float A = 110.0;
float D = 146.8;
float G = 196.0;
float B = 246.9;
float e = 329.6; // high e string

void setup() {

  Serial.begin(9600);

  pinMode(13, OUTPUT); // led indicator pin
  pinMode(12, OUTPUT); // output pin
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);
  cli(); // diable interrupts

  // set up continuous sampling of analog pin 0 at 38.5kHz

  // clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;

  ADMUX |= (1 << REFS0); // set reference voltage
  ADMUX |= (1 << ADLAR); // left align the ADC value- so we can read highest 8
                         // bits from ADCH register only

  ADCSRA |= (1 << ADPS2) |
            (1 << ADPS0); // set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); // enabble auto trigger
  ADCSRA |= (1 << ADIE);  // enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN);  // enable ADC
  ADCSRA |= (1 << ADSC);  // start ADC measurements

  sei();             // enable interrupts
  myservo.attach(9); // attaches the servo on pin 9 to the servo object
}

ISR(ADC_vect) { // when new ADC value ready

  PORTB &= B11101111; // set pin 12 low
  prevData = newData; // store previous value
  newData = ADCH;     // get value from A0
  if (prevData < 200 &&
      newData >=
          200) { // if increasing and crossing midpoint
                 // ///**************************************************************************
    newSlope = newData - prevData;             // calculate slope
    if (abs(newSlope - maxSlope) < slopeTol) { // if slopes are ==
      // record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0) {     // new max slope just reset
        PORTB |= B00010000; // set pin 12 high
        noMatch = 0;
        index++; // increment index
      } else if (abs(timer[0] - timer[index]) < timerTol &&
                 abs(slope[0] - newSlope) <
                     slopeTol) { // if timer duration and slopes match
        // sum timer values
        totalTimer = 0;
        for (byte i = 0; i < index; i++) {
          totalTimer += timer[i];
        }
        period = totalTimer; // set period
        // reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;          // set index to 1
        PORTB |= B00010000; // set pin 12 high
        noMatch = 0;
      } else {   // crossing midpoint but not match
        index++; // increment index
        if (index > 9) {
          reset();
        }
      }
    } else if (newSlope >
               maxSlope) { // if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0; // reset clock
      noMatch = 0;
      index = 0; // reset index
    } else {     // slope not steep enough
      noMatch++; // increment no match counter
      if (noMatch > 9) {
        reset();
      }
    }
  }

  if (newData == 0 || newData == 1023) { // if clipping
    PORTB |= B00100000; // set pin 13 high- turn on clipping indicator led
    clipping = 1;       // currently clipping
  }

  time++; // increment timer at rate of 38.5kHz
}

void reset() {  // clea out some variables
  index = 0;    // reset index
  noMatch = 0;  // reset match couner
  maxSlope = 0; // reset slope
}

void checkClipping() {  // manage clipping indicator LED
  if (clipping) {       // if currently clipping
    PORTB &= B11011111; // turn off clipping indicator led
    clipping = 0;
  }
}

void waitButton() {
  while (buttonState == HIGH) {
    delay(10);
    buttonState = digitalRead(buttonPin);
  }
  delay(30);

  while (buttonState == LOW) {
    delay(10);
    buttonState = digitalRead(buttonPin);
  }
}
void loop() {

  float avg = 0;
  float sum = 0;
  for (int i = 0; i < 50;) {
    checkClipping();
    frequency = 38462 / float(period); // calculate frequency timer rate/period
    buttonState = digitalRead(buttonPin); // check if button is pressed

    if (buttonState == LOW || pos < 11 || pos > 169) {
      while (buttonState == LOW) {
        delay(10);
        buttonState = digitalRead(buttonPin);
      }
      Serial.println(
          "Please remove the device from the tuning peg so that the servo can "
          "be reset. Then press and release the button."); // buttonstate is
                                                           // high when pressed
      waitButton();

      pos = 90;
      myservo.write(pos);
      delay(500);
      Serial.println("Please return the device to the tuning peg. Then press "
                     "and release the button.");
      waitButton();
    }

    if (frequency > 50 && frequency < 380) {
      // print results
      // Serial.print(frequency);
      // Serial.println(" hz");
      sum += frequency;
      i++;
    }
    delay(5); // feel free to remove this if you want
  }
  {
    avg = sum / 50.0; // average value of frequencies over 50 times calculated
    Serial.print("Average: ");
    Serial.println(avg);
    float string; // string is which string it is tuning
    if (avg < 95)
      string = E;
    else if (avg < 125)
      string = A;
    else if (avg < 170)
      string = D;
    else if (avg < 220)
      string = G;
    else if (avg < 290)
      string = B;
    else if (avg < 380)
      string = e;
    else
      Serial.println("SOMETHING IS SCREWED UP!");
    float diff = string - avg;
    if (diff > 1 && diff < 3) {
      pos += 5;
      myservo.write(pos);
      Serial.println("up");
    } else if (diff < -1 && diff > -3) {
      pos += -5;
      myservo.write(pos);
      Serial.println("down");
    } else if ((string - avg) > 3.0) // must be within 1 hz
    {
      pos += 10;
      myservo.write(pos);
      Serial.println("up");

    } else if ((avg - string) > 3.0) // must be within 1 hz
    {
      pos += -10;
      myservo.write(pos);
      Serial.println("down");
    }

    if (diff < 1 && diff > -1) {
      Serial.println("IN TUNE!!!");
      Serial.println(
          "Please remove the device from the tuning peg so that the servo can "
          "be reset. Then press and release the button."); // buttonstate is
                                                           // high when pressed
      waitButton();

      pos = 90;
      myservo.write(pos);
      delay(500);
      Serial.println("Please return the device to the tuning peg. Then press "
                     "and release the button.");
      waitButton();
    }
    sum = 0.0;

    delay(500);
  }

  // do other stuff here
}
