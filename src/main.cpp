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
  
  
  ADS and the RTC module (mounted on the DataLogger Shield) use the I2C communication protocol
    that allows multiple devices to be connected in the same 2 wires

  I2C pins on the arduino UNO
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

//Required settings (Getting from client or calculated by the settings)
int SaveInterval = 5; //The interval in seconds to save the logfile
int SampleRate = 100; //This may change on the client of the control program

unsigned long SaveIntervalUs = SaveInterval * 1000000UL; //SaveIntervalUs in microSeconds to save the logfile
unsigned long SampleInterval = 1000000UL / SampleRate;   // The interval in microSeconds to take a sample
float RatedOutput = TorqueRatedOutput * 5;  //The total value for the lecture indicated for the excitation 
                                            //  applied to the sensor

//Requiered variables
volatile bool isLogging = false;
float ratedZero = 0;
char logfileName[24];

//Time variables
unsigned long microsAdded = 0;
unsigned long lastSave = 0;
unsigned long lastSample = 0;

//Pinouts
const int ChipSelect = 10;
const int BuzzerPin = 9;
const int isLoggingPin = 7;
const int stopLoggingPin = 2;

//Function prototypes
float getZero(Adafruit_ADS1115 adsModule, float multiplier);
String getFileName();
void calculations();
void stopLogging();
void printHeaders();

/*-----------------------------------SETUP-----------------------------------*/
void setup() {
  Serial.begin(115200);
  Serial.println(SaveIntervalUs);
  Serial.println(SampleInterval);

  pinMode(isLoggingPin, OUTPUT);
  pinMode(stopLoggingPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(stopLoggingPin), stopLogging, RISING);

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
  ratedZero = getZero(ads, Multiplier);
  #endif

  //To set the Zero of the torque sensor
  microsAdded = micros();
  lastSave = microsAdded;
  lastSample = microsAdded;
  //TODO make and alarm with buzzer or something indicating that the system is ready SDCARD
}
/*-----------------------------------LOOP-----------------------------------*/
void loop() {
  if(Serial.available() > 0){
    String message = Serial.readString();

    if(message.startsWith("init")){
      int receivedSampleRate = message.substring(message.lastIndexOf("=") + 1).toInt();
      if(receivedSampleRate >= 10 && receivedSampleRate <= 260){
        //Here would be the code if the data received is validated

        isLogging = true;

        SampleRate = receivedSampleRate;
        calculations();

        #ifdef SDcard
          String fileName = getFileName();
          fileName.toCharArray(logfileName, 24);
          printHeaders();
          logfile.open(logfileName, FILE_WRITE);
        #endif

        microsAdded = micros();
        lastSave = microsAdded;
        lastSample = microsAdded;

        digitalWrite(isLoggingPin, HIGH);
      }
    }
  }

  #ifdef SDcard
    while(isLogging){
      int16_t result;
      //Read the results from the Adafruit differential from 0 and 1 inputs
      result = ads.readADC_Differential_0_1();

      float mV = result * Multiplier - ratedZero;

      logfile.print((micros() - microsAdded) / 1000);
      logfile.print(",");
      logfile.print(mV, 4); //Printing the mV to the corresponding column
      logfile.print(",");
      logfile.println(map(mV, -RatedOutput, RatedOutput, -3, 3), 4); //Printing the torque value to the corresponding column

      // int passedTime = millis() - lastSample;
      int passedTime = SampleInterval + lastSample - micros();
      if(passedTime > 0)
        delayMicroseconds(passedTime);
      lastSample = micros();

      if (micros() - lastSave >= SaveIntervalUs){
        logfile.close(); //Close the file every <SaveIntervalUs> (to save the data)
        //This takes around 30ms every close and opens the file so, we will lose that range of time in samples every <SaveIntervalUs> 
        logfile.open(logfileName, FILE_WRITE);
        lastSave = micros();
      }
    }

  //Close the file in case it stills open
    if(logfile.isOpen()){
      logfile.close();
    }
  #endif
  delay(5);
}


/*-------------------------------FUNCTIONS-------------------------------*/
void stopLogging(){
  isLogging = false;
  digitalWrite(isLoggingPin, LOW);
}

void calculations(){
  //Recalculates this variables in case the config file
  //  had diffrent ones
  RatedOutput = TorqueRatedOutput * 5;
  SaveIntervalUs = SaveInterval * 1000000UL;
  SampleInterval = 1000000UL / SampleRate;
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
void printHeaders(){
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
}