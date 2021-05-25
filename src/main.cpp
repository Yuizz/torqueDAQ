#include <Arduino.h> //Cause we are using platformIO
#include <SPI.h>
#include <SDfat.h> //Longer file names, allow us to put timestamp on log files
#include <RTClib.h>
#include <Adafruit_ADS1X15.h> //docs: https://github.com/RobTillaart/ADS1X15

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
#define FILE_WRITE (O_RDWR | O_CREAT | O_AT_END)

//Objects
SdFat SD;
SdFile logfile;
Adafruit_ADS1115 ads;
RTC_PCF8523 rtc;

//Required values
const float TorqueRatedOutput = 1.3161; //The rated output in mV/V indicated by certificate of calibration or data sheet
const float Multiplier = 0.0078125F; //The value for each bit in the results

//Required consts
const int IntervalS = 5; //The interval in seconds to flush he logfile
const unsigned long IntervalMs = IntervalS * 1000000UL; //IntervalMs in us to flush the logfile
const int SampleRate = 100; //CHANGE THIS VARIABLE TO SAMPLE RATE
const unsigned long SampleInterval = 1000000UL / SampleRate;

//Requiered variables
float ratedZero = 0;
char logfileName[24];

//Time variables
unsigned long microsAdded = 0;
unsigned long lastFlush = 0;
unsigned long lastSample = 0;

//Pinouts
const int ChipSelect = 10;
const int BuzzerPin = 9;

//Function prototypes
float getZero(Adafruit_ADS1115 adsModule, float multiplier);
String getFileName();

/*-----------------------------------SETUP-----------------------------------*/
void setup() {
  Serial.begin(115200);
  Serial.println(IntervalMs);
  Serial.println(SampleInterval);
  ads.setGain(GAIN_SIXTEEN); //Set the gain to the 16x / 1bit = 0.0078125mV
  ads.begin();

  #ifdef SDcard
  
    while(!SD.begin(ChipSelect) || !rtc.begin()){
      //Alarm indicating that the SD or rtc is not present or the wiring is incorrect
      tone(BuzzerPin, 10000, 50);
      delay(100);
      tone(BuzzerPin, 10000, 100);
      delay(200);
    }
    
    String fileName = getFileName();
    fileName.toCharArray(logfileName, 24);
    //Create the file and print in it the headers
    logfile.open(logfileName, O_RDWR | O_CREAT);
    logfile.print("millis");
    logfile.print(",");
    logfile.println("mV");
    logfile.close();
#endif

  //To set the Zero of the torque sensor
  ratedZero = getZero(ads, Multiplier);
  microsAdded = micros();
  lastFlush = microsAdded;
  lastSample = microsAdded;
  //TODO make and alarm with buzzer or something indicating that the system is ready SDCARD
  logfile.open(logfileName, FILE_WRITE);
}
void log(String message) {
  Serial.print(message);
  Serial.print(" - ");
  Serial.println(millis());
}
/*-----------------------------------LOOP-----------------------------------*/
void loop() {
  int16_t result;
    //Read the results from the Adafruit differential from 0 and 1 inputs
  // log("Reading from ADS");
  result = ads.readADC_Differential_0_1();
  // log("Ended read from ADS");
  // Serial.print("Differential: "); 
  //   Serial.print(result); //Raw data
  //   Serial.print("(");
  //   Serial.print(result*multiplier-ratedZero, 4); //Real mV difference
  //   Serial.print("mV");
  //   Serial.println(")");
  // log("Writing on file ");
  logfile.print((micros() - microsAdded) / 1000);
  logfile.print(",");
  logfile.println(result * Multiplier - ratedZero, 4);
  // log("End writing on file ");

  // int passedTime = millis() - lastSample;
  // log("Before delay ");
  int passedTime = SampleInterval + lastSample - micros();
  if(passedTime > 0)
    delayMicroseconds(passedTime);
  // delay(SampleInterval);
  // log("After delay ");
  lastSample = micros();

  if (micros() - lastFlush >= IntervalMs)
  {
    //Close the file every <IntervalMs> (to save the data)
    // log("---------------------Closing file---------------------- ");
    logfile.close();
    logfile.open(logfileName, FILE_WRITE);
    // log("FIle reopened");
    lastFlush = micros();
    }
    // log("#########Cycle ended#######");
}
/*-------------------------------FUNCTIONS-------------------------------*/

float getZero(Adafruit_ADS1115 adsModule, float multiplier){
  //Receives an adsModule object and the multiplier and 
  //  returns a value to set the zero on the lectures
  int lectures = 32; //TODO make this easy controllable
  float total = 0.0f;

  Serial.println("Reading...");
  for (int i = 0; i < lectures; i++){
    int16_t result = adsModule.readADC_Differential_0_1();
    total += result*multiplier;
    delay(4000/lectures);
  }
  logfile.close();
  Serial.println(total/lectures);
  return (total/lectures);
}

String getFileName(){
  DateTime now = rtc.now();
  String timeStamp = String(now.timestamp());
  timeStamp.replace(':', '_');

  String fileName = timeStamp + ".CSV";

  Serial.println("Logging to: " + fileName);
  Serial.println(timeStamp);

  // fileName = "12345.CSV";
  return fileName;
}