#include <Arduino.h> //Cause we are using platformIO
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ADS1X15.h> //To see the documentation: https://github.com/RobTillaart/ADS1X15

/*
  SD card pins configuration (For the adafruit shield board)
    ** MOSI - pin 11
    ** MISO - pin 12
    ** CLK - pin 13
    ** CS - pin 10 (for MKRZero SD: SDCARD_SS_PIN) 

  ADS1115 pins configuration
    ** SCL A5
    ** SDA A4
    
*/

#define SDcard //Comment this if the SD card Shield is not present

const float torqueRatedOutput = 1.3161; //The rated output in mV/V indicated by certificate of calibration or data sheet
const float multiplier = 0.0078125F; //The value for each bit in the results
float ratedZero = 0;

Adafruit_ADS1115 ads;

const int chipSelect = 10;
const int buzzerPin = 2;

void setup() {
  Serial.begin(9600);

  ads.setGain(GAIN_SIXTEEN); //Set the gain to the 16x / 1bit = 0.0078125mV
  ads.setDataRate(2); //32 SPS
  ads.begin();

  //To set the Zero of the torque sensor
  ratedZero = getZero(ads, multiplier);

  #ifdef SDcard

    while(!SD.begin(chipSelect)){
      //Alarm indicating that the SD is not present or the wiring is incorrect
      tone(buzzerPin, 10000, 50);
      delay(100);
      tone(buzzerPin, 10000, 100);
      delay(200);
    }
  #endif

  //TODO make and alarm with buzzer or something indicating that the system is ready SDCARD
}

void loop() {
  int16_t results;

    //Read the results from the Adafruit differential from 0 and 1 inputs
  results = ads.readADC_Differential_0_1(); 

  Serial.print("Differential: "); 
    Serial.print(results); //Raw data
    Serial.print("(");
    Serial.print(results*multiplier-ratedZero); //Real mV 
    Serial.print("mV");
    Serial.println(")");

  delay(32);
}


float getZero(Adafruit_ADS1115 adsModule, float multiplier){
  //Receives an adsModule object and the multiplier and returns a value to set the zero on the lectures
  float total = 0.0f;

  for (int i = 0; i < 128; i++){
    int16_t result = adsModule.readADC_Differential_0_1();
    total += result*multiplier;
    delay(32);
  }

  return (total/128);
}