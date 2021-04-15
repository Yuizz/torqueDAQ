#include <Arduino.h>
#include <SD.h>
#include <Adafruit_ADS1X15.h>

int sensorPin = 0;
int yellowPin = 1;
int lecture = 0;
double torqueValue = 0;

void setup(){
  Serial.begin(9600);
}

void loop(){
  lecture = analogRead(sensorPin);
  torqueValue = map(lecture, 0, 1023, 0, 5000);
  Serial.print(torqueValue/10.00);

  lecture = analogRead(yellowPin);
  torqueValue = map(lecture, 0, 1023, 0, 5000);
  Serial.print("--");
  Serial.println(torqueValue/1.000);
  delay(150);
}