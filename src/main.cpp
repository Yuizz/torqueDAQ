#include <Arduino.h> //Cause we are using platformIO
#include <SPI.h>
#include <SDfat.h> //Longer file names, allow us to put timestamp on log files
#include <RTClib.h>
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
#define FILE_WRITE (O_RDWR | O_CREAT | O_AT_END)

//Objects
SdFat SD;
SdFile logfile;
Adafruit_ADS1115 ads;
RTC_PCF8523 rtc;

//Required values
const int Interval = 5000; //Interval in ms to flushg the logfile
const float torqueRatedOutput = 1.3161; //The rated output in mV/V indicated by certificate of calibration or data sheet
const float multiplier = 0.0078125F; //The value for each bit in the results
// const int dataRate[] = {8, 16, 32, 64, 128, 250, 475, 860}; //TODO make a way to easy select the data rate on ejecution time
float ratedZero = 0;
char logfileName[24];
// float results[60][2]={};
int millisAdded = 0;
int lastFlush = 0;

//Pinouts
const int chipSelect = 10;
const int buzzerPin = 9;


//Function prototypes
float getZero(Adafruit_ADS1115 adsModule, float multiplier);
String getFileName();

void setup() {
  Serial.begin(115200);

  ads.setGain(GAIN_SIXTEEN); //Set the gain to the 16x / 1bit = 0.0078125mV
  ads.begin();

  #ifdef SDcard
  
    while(!SD.begin(chipSelect) || !rtc.begin()){
      //Alarm indicating that the SD or rtc is not present or the wiring is incorrect
      tone(buzzerPin, 10000, 50);
      delay(100);
      tone(buzzerPin, 10000, 100);
      delay(200);
    }
    
    String fileName = getFileName();
    fileName.toCharArray(logfileName, 24);
    logfile.open(logfileName, O_RDWR | O_CREAT);
    logfile.print("millis");
    logfile.print(",");
    logfile.println("mV");
    logfile.close();
#endif

  //To set the Zero of the torque sensor
  ratedZero = getZero(ads, multiplier);
  millisAdded = millis();
  lastFlush = millisAdded;
  //TODO make and alarm with buzzer or something indicating that the system is ready SDCARD
  // log("Open file");
  logfile.open(logfileName, FILE_WRITE);
  // log("File Opened");
}

void log(String time){
  Serial.print(time);
  Serial.print(" - ");
  Serial.println(millis());
}

void loop() {
  int16_t result;
    //Read the results from the Adafruit differential from 0 and 1 inputs
  // log("Read from ADS");
  result = ads.readADC_Differential_0_1();
  // log("End read from ADS");


  // Serial.print("Differential: "); 
  //   Serial.print(result); //Raw data
  //   Serial.print("(");
  //   Serial.print(result*multiplier-ratedZero, 4); //Real mV difference
  //   Serial.print("mV");
  //   Serial.println(")");


    logfile.print(millis()-millisAdded);
    logfile.print(",");
    logfile.println(result * multiplier - ratedZero, 4);
    log("Writed on file");
    // logfile.close();
    // log("Closed File");
    log("Begin delay");
    delay(10);
    log("End cycle");
    if(millis() - lastFlush >= Interval){
      logfile.close();
      logfile.open(logfileName, FILE_WRITE);
      lastFlush = millis();
      log("flushed");
    }
}

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