/*
 Program to test the main project:
  A variable resistor will control the speed of a continuos servomotor to provide controlled rotation
*/

#include <Servo.h>

Servo servo;

int potentiometerPin = 0;
int value;
int servoPin = 9;
int servoSpeed = 0;

int ledR = 12;
int ledY = 11;
int ledG = 10;

void setup() {
  pinMode(ledR, HIGH);
  pinMode(ledY, HIGH);
  pinMode(ledG, HIGH);

  Serial.begin(9600);
  servo.attach(servoPin);

}

void loop() {
  value = analogRead(potentiometerPin);
  servoSpeed = 90 + map(value, 0, 1023, 0, 140);
  showState(servoSpeed);
  Serial.println(servoSpeed);
  servo.write(servoSpeed);
  delay(15);
}

void showState(int servoSpeed){
/*
Receives the servomotor speed and set the leds to show the state, Green-Yellow-Red leds minimum speed to fastest.
The errorMinBound is to set the minor limit in the speed, this is because the servo minimum speed value and we have 4 posible states, the 0 is for the servo in 0 speed. 
*/
  int errorMinBound = -35;
  
  int state = map(servoSpeed - 90, errorMinBound, 90, 0, 3);
  Serial.println(state);
  switch (state){
    case 0:
      digitalWrite(ledG, LOW);
      digitalWrite(ledY, LOW);
      digitalWrite(ledR, LOW);
      break;
    case 1:
      digitalWrite(ledG, HIGH);
      digitalWrite(ledY, LOW);
      digitalWrite(ledR, LOW);
      break;
    case 2:
      digitalWrite(ledG, LOW);
      digitalWrite(ledY, HIGH);
      digitalWrite(ledR, LOW);
      break;
    case 3:
      digitalWrite(ledG, LOW);
      digitalWrite(ledY, LOW);
      digitalWrite(ledR, HIGH);
      break;
    default:
      digitalWrite(ledG, LOW);
      digitalWrite(ledY, LOW);
      digitalWrite(ledR, LOW);
      break;
  }
}
