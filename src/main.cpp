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
const float RatedOutput = TorqueRatedOutput * 5; //The total value for the lecture indicated for the excitation 
                                                    // applied to the sensor

//Required consts
const int IntervalS = 5; //The interval in seconds to save the logfile
const int SampleRate = 100; //CHANGE THIS VARIABLE TO SAMPLE RATE

const unsigned long IntervalUs = IntervalS * 1000000UL; //IntervalUs in microSeconds to save the logfile
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
  Serial.println(IntervalUs);
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
    
    String fileName = getFileName(); //Gets the timeStamp name for the file
    fileName.toCharArray(logfileName, 24);
    //Create the file and print in it the headers
    logfile.open(logfileName, O_RDWR | O_CREAT);

    logfile.print("SPS,");
    logfile.println(SampleRate);

    logfile.print("microSeconds");
    logfile.print(",");
    logfile.print("mV");
    logfile.print(",");
    logfile.println("NM");
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
  // log("Writing on file ");

  float mV = result * Multiplier - ratedZero;

  logfile.print((micros() - microsAdded) / 1000);
  logfile.print(",");
  logfile.print(mV, 4); //Printing the mV to the corresponding column
  logfile.print(",");
  logfile.println(map(mV, -RatedOutput, RatedOutput, -3, 3), 4); //Printing the torque value to the corresponding column
  // log("End writing on file ");

  // int passedTime = millis() - lastSample;
  // log("Before delay ");
  int passedTime = SampleInterval + lastSample - micros();
  if(passedTime > 0)
    delayMicroseconds(passedTime);
  lastSample = micros();

  if (micros() - lastFlush >= IntervalUs)
  {
    logfile.close(); //Close the file every <IntervalUs> (to save the data)
    //This takes around 30ms every close and opens the file so, we will lose that range of time in samples every <IntervalUs> 
    logfile.open(logfileName, FILE_WRITE);
    lastFlush = micros();
    }
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
  Serial.print("Rated zero");
  Serial.println(total/lectures);
  return (total/lectures);
}

String getFileName(){
  DateTime now = rtc.now();
  String timeStamp = String(now.timestamp());
  timeStamp.replace(':', '_');

  String fileName = timeStamp + ".csv";


  Serial.println("Logging to: " + fileName);
  Serial.println(fileName);

  return fileName;
}