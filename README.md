# ESP8266-D0-Logger
ESP8266 based data logger for power meters with infrared D0 interface

This sketch reads values every 15s. Once data is recieved it can be read via http://<yourESP> and it will also be send via UDP broadcast. 

# Setup
Besides a ESP8266 you'll also need a IR read/write adapter with TTL level connected to the RX/TX ports of your ESP8266. You can build this your own with a IR LED and IR phototransistor or get one read built.

You'll need to disconnect the RX port while flashing via USB to avoid interferences, I had lots of failed attempts to flash. Once the sketch is up and running you can flash OTA by navigating to http://<yourESP>/update to avoid this problem.

This sketch is built for meters requiring a initialization string, my meters didn't send the data withouth that. 

# Data format
Currently there is not parsing, so this works best for D0. If your meter sends data in SML you'll need add extra parsing. Either in this sketch or perhaps after you've retrieved the raw data from the ESP. 

I found this list of OBIS codes quite useful to find the right dataset:
https://www.promotic.eu/en/pmdoc/Subsystems/Comm/PmDrivers/IEC62056_OBIS.htm


# Known Issues
Currently there's no post-processing of recieved data. Any special or non-printable charcters will prevent you from seeing the results in a browser, instead the results will be downloaded as an unnamed file. Bit of an annoyance while testing but doesn't impact your ability to process the results of this logger. Any programmatic access to the webinterface will work just fine.

