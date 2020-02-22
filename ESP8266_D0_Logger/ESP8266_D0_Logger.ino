#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>

//#include <string.h>

// Global constants for WiFi connections
// ***********************************
// Need to replace with WiFiManager
// ***********************************

// Network setup
const char* ssid = "ssid";              // your network SSID (name)
const char* pass = "pass";        // your network password
const char* hostname = "ESP8266-D0";      

// MQTT Setup
IPAddress MQTT_Broker(192,168,178,51); // MQTT Broker IP
const int MQTT_Broker_Port = 1883;

const char* MQTT_Power = "openWB/set/evu/W"; //int
const char* MQTT_CurrentPhase1 = "openWB/set/evu/APhase1"; //float
const char* MQTT_CurrentPhase2 = "openWB/set/evu/APhase2"; //float
const char* MQTT_CurrentPhase3 = "openWB/set/evu/APhase3"; //float
const char* MQTT_VoltagePhase1 = "openWB/set/evu/VPhase1"; //float
const char* MQTT_VoltagePhase2 = "openWB/set/evu/VPhase2"; //float
const char* MQTT_VoltagePhase3 = "openWB/set/evu/VPhase3"; //float
const char* MQTT_GridFrequency = "openWB/set/evu/HzFrequenz"; //float
const char* MQTT_WhImportedEnergy = "openWB/set/evu/WhImported"; // Wh, not kW. float
const char* MQTT_WhExportedEnergy = "openWB/set/evu/WhExported"; // Wh, not kW. float

float CurrentPhase1 = 0;
float CurrentPhase2 = 0;
float CurrentPhase3 = 0;
float VoltagePhase1 = 0;
float VoltagePhase2 = 0;
float VoltagePhase3 = 0;
float GridFrequency = 0;
float AbsPowerkW = 0;
int AbsPower = 0;
float kWhImportedEnergy = 0;
float kWhExportedEnergy = 0;

WiFiClient espClient;
PubSubClient MQTTClient(espClient);
long lastReconnectAttempt = 0;

// Config flags do enable features
const bool SendUDP = 1;                  // Broadcast recieved datagrams via UDP? Useful for debugging and further processing outside of HTTP requests. 
const bool SendMQTT = 1;
const bool isDebug = 0;                        // Send debug messages to serial port? needs to be turned off once connected to the meter to avoid interference


// Configure UDP Broadcast settings
IPAddress udpip(192,168,178,255);        // specify target IP for UDP broadcast
const int udpport = 12345;               // Specify Source and Target Port
WiFiUDP Udp;                               // Create a UDP instance to send and receive packets over UDP

// define datagram
const int MaxDatagramLength = 4000;      // Whats the maximum datagram size to expect
char datagram[MaxDatagramLength] = "No Data received";    // array that will hold the different bytes 
const long interval = 10*1000;           // polling interval for new values in ms
const int SerialTimeout = 3*1000;        // serial transmission timeout for values in ms
int  datagramLenght = 0;                 // lenght of recieved datagram
const String InitSeq= "/?!";             // Init sequence to request data via D0

// ESP8266 Webserver and update server
ESP8266WebServer server(80);              // HTTP server port
ESP8266HTTPUpdateServer httpUpdater;      // HTTP update server, allows OTA flash by navigating to http://<ESP8266IP>/update

unsigned long previousMillis = 0;         // last time data was fetched

// search strings to get measures, these are OBIS codes. i.e. see https://www.promotic.eu/en/pmdoc/Subsystems/Comm/PmDrivers/IEC62056_OBIS.htm for a list of all available codes
const String AbsolutePowerIndicator = "15.7.0";
const String CurrentPhase1Indicator = "31.7.0";
const String CurrentPhase2Indicator = "51.7.0";
const String CurrentPhase3Indicator = "71.7.0";
const String VoltagePhase1Indicator = "32.7.0";
const String VoltagePhase2Indicator = "52.7.0";
const String VoltagePhase3Indicator = "72.7.0";
const String GridFrequencyIndicator = "14.7.0";
const String kWhImportedIndicator = "1.8.0";
const String kWhExportedIndicator = "2.8.0";

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

boolean MQTTReconnect() 
{
  if (MQTTClient.connect(hostname)) 
  {
    WriteLog("MQTT Reconnected");
  }
  return MQTTClient.connected();
}

void HandleRoot()                                                 // Handle Webserver request on root
{
  String res = datagram;
  WebserverResponse(res);
}

void HandleMQTTStatus()
{
  String res = String(MQTTClient.state());
  WebserverResponse(res);
}

float GetFloatValue(String Indicator)
{
    String temp = String(datagram);                               // entire datagram
    int start = temp.indexOf(Indicator);             // find search string
    start = temp.indexOf("(",start)+1;                            // find next delimintator, exclude the delimintor
    int end = temp.indexOf("*",start);                            // find closing deliminator
    float ReturnValue = temp.substring(start,end).toFloat();    // extract value and convert to float
    return ReturnValue;
}

void HandleCurrentPhase1()
{   
    String res = String(CurrentPhase1);
    WebserverResponse(res);                                       // send webserver response
}

void HandleCurrentPhase2()
{
    String res = String(CurrentPhase2);
    WebserverResponse(res);                                       // send webserver response
}

void HandleCurrentPhase3()
{
    String res = String(CurrentPhase3);
    WebserverResponse(res);                                       // send webserver response
}

void HandleVoltagePhase1()
{
    String res = String(VoltagePhase1);
    WebserverResponse(res);                                       // send webserver response
}

void HandleVoltagePhase2()
{
    String res = String(VoltagePhase2);
    WebserverResponse(res);                                       // send webserver response
}

void HandleVoltagePhase3()
{
    String res = String(VoltagePhase3);
    WebserverResponse(res);                                       // send webserver response
}

void HandleGridFrequency()
{
    String res = String(GridFrequency);
    WebserverResponse(res);                                       // send webserver response
}

void HandleAbsPower()
{
    String res = String(AbsPower);
    WebserverResponse(res);             // send webserver response
}

void HandlekWhImportedEnergy()
{
  String res = String(kWhImportedEnergy*1000);
  WebserverResponse(res);
}

void HandlekWhExportedEnergy()
{
  String res = String(kWhExportedEnergy*1000);
  WebserverResponse(res);
}

void WebserverResponse(String str)
{ 
    str.trim();
    WriteLog("Sending WebServer response, requested URI: " + server.uri());
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "text/plain",String(str));
    WriteLog("Sending HTTP response: " + str);
}

// ------------------------------------------------
//   SETUP running once at the beginning
// ------------------------------------------------
//   Initialize  Serial, WIFi and UDP

void setup() 
{
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
  server.on("/VoltagePhase1",HandleVoltagePhase1);
  server.on("/VoltagePhase2",HandleVoltagePhase2);
  server.on("/VoltagePhase3",HandleVoltagePhase3);
  server.on("/GridFrequency",HandleGridFrequency);
  server.on("/kWhExportedEnergy",HandlekWhExportedEnergy);
  server.on("/kWhImportedEnergy",HandlekWhImportedEnergy);
  server.on("/MQTTStatus",HandleMQTTStatus);

  httpUpdater.setup(&server);         // Updater
  server.begin();                     // start HTTP server
  WriteLog("HTTP server started");
  
  if (SendUDP)
  {
    WriteLog("Starting UDP service on port: ",0);
    WriteLog(String(udpport));
    Udp.begin(udpport);
    WriteLog("INIT Done");
  }
  
  if(SendMQTT)
  {
    MQTTClient.setServer(MQTT_Broker,MQTT_Broker_Port);
    lastReconnectAttempt = 0;
    MQTTReconnect;   
  }

  Serial.println(InitSeq);           // Request the first datagram
}

void PublishFloat(const char* topic, float value)
{
  char result[20];
  dtostrf(value,10,3,result);
  MQTTClient.publish(topic, result);
}

// ------------------------------------------------
//   MAIN LOOP RUNNING all the time
// ------------------------------------------------
void loop() 
{
  unsigned long currentMillis = millis(); // Get current time to check if we need to read new data

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.reconnect();                   // WiFi not connected, reconnecting.
  }

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
      WriteLog("Extracting values from datagram");
      
      CurrentPhase1 = GetFloatValue(CurrentPhase1Indicator);
      CurrentPhase2 = GetFloatValue(CurrentPhase2Indicator);
      CurrentPhase3 = GetFloatValue(CurrentPhase3Indicator);

      VoltagePhase1 = GetFloatValue(VoltagePhase1Indicator);
      VoltagePhase2 = GetFloatValue(VoltagePhase2Indicator);
      VoltagePhase3 = GetFloatValue(VoltagePhase3Indicator);

      kWhImportedEnergy = GetFloatValue(kWhImportedIndicator);
      kWhExportedEnergy = GetFloatValue(kWhExportedIndicator);
      
      AbsPowerkW = GetFloatValue(AbsolutePowerIndicator);
      AbsPower = int(AbsPowerkW*1000);                             // Value is in kW, we need W

      GridFrequency = GetFloatValue(GridFrequencyIndicator);

      if (SendUDP)
      {
        WriteLog("Sending UDP-Paket");
        Udp.beginPacket(udpip, udpport);  // Start new paket
        Udp.write(datagram);
        Udp.endPacket();                  // Send paket
      }

      if(SendMQTT)
      {
        if (!MQTTClient.connected())      // non blocking MQTT reconnect sequence
        {
          long now = millis();
          if (now - lastReconnectAttempt > 5000) 
          {
            lastReconnectAttempt = now;
            WriteLog("Attempting to reconnect MQTT");
            if (MQTTReconnect()) 
            {
              lastReconnectAttempt = 0;
            }
          }
        }
        else                            // MQTT is connected, lets send some data
        {
          WriteLog("Submitting MQTT Data");
          MQTTClient.loop();            // good practice to call loop, even if we don't subscribe to any topics right now.

          // AbsPower is Int, needs to be char
          char c[10];
          String s;
          s=String(AbsPower);
          s.toCharArray(c,10);
          MQTTClient.publish(MQTT_Power,c);

          PublishFloat(MQTT_CurrentPhase1,CurrentPhase1);
          PublishFloat(MQTT_CurrentPhase2,CurrentPhase2);
          PublishFloat(MQTT_CurrentPhase3,CurrentPhase3);
          PublishFloat(MQTT_VoltagePhase1,VoltagePhase1);
          PublishFloat(MQTT_VoltagePhase2,VoltagePhase2);
          PublishFloat(MQTT_VoltagePhase3,VoltagePhase3);
          PublishFloat(MQTT_GridFrequency,GridFrequency);
          PublishFloat(MQTT_WhExportedEnergy,kWhExportedEnergy*1000);
          PublishFloat(MQTT_WhImportedEnergy,kWhImportedEnergy*1000);
        }
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
