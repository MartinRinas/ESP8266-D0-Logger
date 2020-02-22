#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
//#include <string.h>

// Global constants for WiFi connections
// ***********************************
// Need to replace with WiFiManager
// ***********************************

const char* ssid = "mySSID";             // your network SSID (name)
const char* pass = "myPass";       // your network password
const char* hostname = "ESP8266-D0";

const bool SendUDP = 0;                  // Broadcast recieved datagrams via UDP? Useful for debugging and further processing outside of HTTP requests. 
IPAddress udpip(192,168,178,255);        // specify target IP for UDP broadcast
const int udpport = 12345;               // Specify Source and Target Port

bool isDebug = 0;                        // Send debug messages to serial port? needs to be turned off once connected to the meter to avoid interference

// define datagram
const int MaxDatagramLength = 4000;      // Whats the maximum datagram size to expect
char datagram[MaxDatagramLength] = "No Data received";    // array that will hold the different bytes 
const long interval = 15*1000;           // polling interval for new values in ms
const int SerialTimeout = 3*1000;        // serial transmission timeout for values in ms
int  datagramLenght = 0;                 // lenght of recieved datagram
const String InitSeq= "/?!";             // Init sequence to request data via D0

WiFiUDP Udp;                               // Create a UDP instance to send and receive packets over UDP

ESP8266WebServer server(80);              // HTTP server port
ESP8266HTTPUpdateServer httpUpdater;      // HTTP update server, allows OTA flash by navigating to http://<ESP8266IP>/update

unsigned long previousMillis = 0;         // last time data was fetched

// search strings to get measures, these are OBIS codes. i.e. see https://www.promotic.eu/en/pmdoc/Subsystems/Comm/PmDrivers/IEC62056_OBIS.htm for a list of all available codes
const String AbsolutePower = "15.7.0";
const String CurrentPhase1Indicator = "31.7.0";
const String CurrentPhase2Indicator = "51.7.0";
const String CurrentPhase3Indicator = "71.7.0";

void WriteLog(String msg,bool NewLine=1)  // helper function for logging, only write to serial if isDebug is true
{
  if(NewLine)
  {
    if(isDebug){Serial.println(msg);}
  }
  else
  {
    if(isDebug){Serial.print(msg);}
  } 
}

void HandleRoot()                                                 // Handle Webserver request on root
{
  String Serverantwort = datagram;
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/plain",datagram);
}

void HandleCurrentPhase1()
{   
    // this code is redundant, should be moved into a function 
    String temp = String(datagram);                               // entire datagram
    int start = temp.indexOf(CurrentPhase1Indicator);             // find search string
    start = temp.indexOf("(",start)+1;                            // find next delimintator, exclude the delimintor
    int end = temp.indexOf("*",start);                            // find closing deliminator
    float CurrentPhase1 = temp.substring(start,end).toFloat();    // extract value and convert to float

    String res = String(CurrentPhase1);
    res.trim();                                                   //remove whitespace
    WebserverResponse(res);                                       // send webserver response
}

void HandleCurrentPhase2()
{
    String temp = String(datagram);                               // entire datagram
    int start = temp.indexOf(CurrentPhase2Indicator);             // find search string
    start = temp.indexOf("(",start)+1;                            // find next delimintator, exclude the delimintor
    int end = temp.indexOf("*",start);                            // find closing deliminator
    float CurrentPhase2 = temp.substring(start,end).toFloat();    // extract value and convert to float

    String res = String(CurrentPhase2);
    res.trim();                                                   //remove whitespace
    WebserverResponse(res);                                       // send webserver response
}

void HandleCurrentPhase3()
{
    String temp = String(datagram);                               // entire datagram
    int start = temp.indexOf(CurrentPhase3Indicator);             // find search string
    start = temp.indexOf("(",start)+1;                            // find next delimintator, exclude the delimintor
    int end = temp.indexOf("*",start);                            // find closing deliminator
    float CurrentPhase3 = temp.substring(start,end).toFloat();    // extract value and convert to float

    String res = String(CurrentPhase3);
    res.trim();                                                   //remove whitespace
    WebserverResponse(res);                                       // send webserver response
}

void WebserverResponse(String str)
{ 
    WriteLog("Sending WebServer response, requested URI: " + server.uri());
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "text/plain",String(str));
    WriteLog("Sending HTTP response: " + str);
}

void HandleAbsPower()
{
    String temp = String(datagram);                       // entire datagram
    int start = temp.indexOf(AbsolutePower);              // find search string
    start = temp.indexOf("(",start)+1;                            // find next delimintator, exclude the delimintor
    int end = temp.indexOf("*",start);                    // find closing deliminator
    float AbsPowerkW = temp.substring(start,end).toFloat();          // extract value and convert to float
    int AbsPower = int(AbsPowerkW*1000);                             // Value is in kW, we need W

    String res = String(AbsPower);
    WebserverResponse(res);             // send webserver response
}


// ------------------------------------------------
//   SETUP running once at the beginning
// ------------------------------------------------
//   Initialize  Serial, WIFi and UDP
void setup() {
  //Serial.begin(9600);                       // Open serial communications and wait for port to open:
  Serial.begin(9600,SERIAL_7E1);

  while (!Serial) { // wait for serial port to connect. 
    ; 
  }
  WriteLog("ESP8266-DO-Logger init");
  
  WriteLog("Waiting for WiFi connection");
  WiFi.mode(WIFI_STA);                             // connect to AP
  WiFi.begin(ssid, pass);                          // set WiFi connections params
  WiFi.hostname(hostname);
 
  // Connecting
  int timout = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    WriteLog("O",0);
    timout++;
    if  (timout > 20)                 // couldn'T connect to WiFi within timeout. No WiFi. Need to add better handling
    {
      WriteLog("");
      WriteLog("Not Connected to WiFi");
      break;
    }
  }
 
  if (WiFi.status() == WL_CONNECTED)
  {
    WriteLog("");
    WriteLog("Connected to WiFi");
  }

  MDNS.begin(hostname);               // Start mDNS 
  server.on("/", HandleRoot);         // Call function if root is called
  server.on("/AbsPower",HandleAbsPower);
  server.on("/CurrentPhase1",HandleCurrentPhase1);
  server.on("/CurrentPhase2",HandleCurrentPhase2);
  server.on("/CurrentPhase3",HandleCurrentPhase3);
  httpUpdater.setup(&server);         // Updater
  server.begin();                     // start HTTP server
  WriteLog("HTTP server started");
  
  if (SendUDP)
  {
    // Start UDP Object
    WriteLog("Starting UDP service on port: ",0);
    WriteLog(String(udpport));
    Udp.begin(udpport);
    WriteLog("INIT Done");
  }
  
  Serial.println(InitSeq);           // Request the first datagram
}

// ------------------------------------------------
//   MAIN LOOP RUNNING all the time
// ------------------------------------------------

void loop() 
{
  unsigned long currentMillis = millis(); // Get current time to check if we need to read new data

  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;     // Yes, time is up, we need to get new data.
    Serial.println(InitSeq);            // Send initialization string to read meter
    Serial.setTimeout(SerialTimeout);   // Set serial timeout to allow long datagrams without waiting forever

    datagramLenght = Serial.readBytes(datagram,MaxDatagramLength); // Read all serial data until we hit timeout or buffer is full. Need to add handling for buffer full scenario

    if (datagramLenght >0) // did we really receive data?
    { 
      WriteLog("Datagram received. Total Bytes: ",0);
      WriteLog(String(datagramLenght));

      if (SendUDP)
      {
        WriteLog("Sending UDP-Paket");
        Udp.beginPacket(udpip, udpport);  // Start new paket
        Udp.write(datagram);
        Udp.endPacket();                  // Send paket
      }

      int RemainingSerialBytes = Serial.available();        // Is data left in the serial buffer? Would be unexpected, but you'll never know. Just read all of it as we work under the assumption that we have to request new datagrams.
      if (RemainingSerialBytes>0)
      {
        WriteLog("Found additional serial bytes: ",0);
        WriteLog(String(RemainingSerialBytes));
        while (Serial.available()>0)    // lets read the remaining bytes from the serial buffer, in case there are some.
        {     
          char t = Serial.read();
        }
      }
    }
  }
  server.handleClient();                // handle webserver requests
  MDNS.update();                        // handle mDNS requests
}
