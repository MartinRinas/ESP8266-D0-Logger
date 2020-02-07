#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

// Global constants for WiFi connections
// ***********************************
// Need to replace with WiFiManager
// ***********************************

const char* ssid = "MyWiFi";             // your network SSID (name)
const char* pass = "MyPass";       // your network password
const char* hostname = "ESP8266-D0";

IPAddress udpip(192,168,178,255);        // specify target IP for UDP broadcast
const int udpport = 12345;               // Specify Source and Target Port

bool isDebug = 0;                        // Send debug messages to serial port? needs to be turned off once connected to the meter to avoid interference

// define datagram
const int MaxDatagramLength = 4000;      // Whats the maximum datagram size to expect
const char LastChar = '!';               // last character of the datagram so that we don't wait too long
char datagram[MaxDatagramLength] = "No Data received";    // array that will hold the different bytes 
const long interval = 15*1000;           // polling interval for new values in ms
const int SerialTimeout = 6*1000;        // serial transmission timeout for values in ms

int  datagramLenght = 0;                 // lenght of recieved datagram
const String InitSeq= "/?!";             // Init sequence to request data via D0

WiFiUDP Udp;                              // Create a UDP instance to send and receive packets over UDP

ESP8266WebServer server(80);              // HTTP server port
ESP8266HTTPUpdateServer httpUpdater;      // HTTP update server, allows OTA flash by navigating to http://<ESP8266IP>/update

unsigned long previousMillis = 0;         // last time data was fetched

void WriteLog(String msg,bool NewLine=1)  // helper function for logging, only write to serial if isDebug is true
{
  if(isDebug)
  {
    if(NewLine)
    {
      Serial.println(msg);
    }
    else
    {
      Serial.print(msg);
    } 
  }
  
}

void HandleRoot()                          // Handle Webserver request on root
{
  String Serverantwort = datagram;
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/plain",datagram);
}

// ------------------------------------------------
//   SETUP running once at the beginning
// ------------------------------------------------
//   Initialize  Serial, WIFi and UDP
void setup() {
  //Serial.begin(9600);   // Open serial communications and wait for port to open:
  Serial.begin(9600,SERIAL_7E1);

  while (!Serial) { // wait for serial port to connect. 
    ; 
  }
  WriteLog("ESP8266-DO-Logger init");
  
  // Wait for connect to AP
  
  WriteLog("Waiting for WiFi connection");
 
  WiFi.mode(WIFI_STA);                             // station  modus verbinde mit dem Router
  WiFi.begin(ssid, pass); // WLAN Login daten
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

  MDNS.begin(hostname);                   // Start mDNS 
  server.on("/", HandleRoot);         //  Call function if root is called
  httpUpdater.setup(&server);         //  Updater
  server.begin();                     // start HTTP server
  WriteLog("HTTP server started");
  
  // Start UDP Object
  WriteLog("Starting UDP service on port: ",0);
  WriteLog(String(udpport));
  Udp.begin(udpport);
  WriteLog("INIT Done");

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
    datagramLenght = Serial.readBytesUntil(LastChar,datagram,MaxDatagramLength); // Read serial bytes until we hit (1) timeout or (2) the LastChar character as stop char.
    
    if (datagramLenght >0) // did we really receive data?
    { 
      WriteLog("Datagram received. Total Bytes: ",0);
      WriteLog(String(datagramLenght));
      WriteLog("Sending UDP-Paket");
    
      Udp.beginPacket(udpip, udpport);  // Start new paket
      Udp.write(datagram);
      Udp.endPacket();   // Send paket
    }
  }
  // handle HTTP requests
  server.handleClient();
  MDNS.update();
}