
# Torque DAQ

The purpose of the project is to realize a stand alone
mini DAQ (Data Acquisition) from a torque Sensor (Forsentek FY02).

The torque sensor with the mini DAQ will be used in some
projects to adquire data to analize it later in order
to improve the development of those projects.

The project was developed using an Arduino Uno as the logger with 
a Data Logger Shield and a ESP-8266-01 as a Async Web Server
with its own WiFi Network to control the Sample Rate of the device
and to start/stop the logging.

## Related

Here are some related projects. This are the projects related
that works together, the Async Web Server is hosted by 
ESP8266-01, but could be hosted by any board of the family
ESP, then the connections to the Arduino are in the related github.

[Torque DAQ](https://github.com/Yuizz/torqueDAQ)

[Async Web Server](https://github.com/Yuizz/NodeMCUWebServer)

  
