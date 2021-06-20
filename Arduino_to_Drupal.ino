
#include <WiFiEsp.h>
#include <ArduinoJson.h>
#include <DS3231.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "arduino_secrets.h"

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);

// Data wire is conntec to the Arduino digital pin 2
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);


// your network SSID (name)
char ssid[] = SECRET_SSID;
// your network password 
char pass[] = SECRET_PASS; 


// the Wifi radio's status
int status = WL_IDLE_STATUS;     

char server[] = "thesis.stefan.engineer";

// Initialize the Ethernet client object
WiFiEspClient client;

bool flag = 1;

void setup() {
  // initialize serial for debugging
  Serial.begin(115200);
  // initialize serial for ESP module
  Serial1.begin(9600);

  // Initialize the rtc object
  rtc.begin();

  // Start up the library DallasTemperature
  sensors.begin();
  
  wifiConnect();
  
  printWifiStatus();

  Serial.println();
  Serial.println("Starting connection to server...");
  
  // if you get a connection, report back via serial
  if (client.connect(server, 80)) {
    Serial.println("Connected to server");
    // Make a HTTP request
    client.println(F("GET /rest/time_service HTTP/1.0"));
    client.println(F("Host: thesis.stefan.engineer"));
    client.println("Authorization: Basic YWRtaW46U3VuY2VDdmV0YTIwMTgu");
    client.println(F("Cache-Control: no-cache"));
    client.println(F("Connection: close"));
    client.println();
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print("Unexpected response: ");
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println("Invalid response");
    return;
  }

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity_date = JSON_OBJECT_SIZE(6)+60;
  DynamicJsonBuffer jsonBuffer(capacity_date);

  // Parse JSON object
  //const char* json = "{"year":"2019","month":"04","day":"26","hour":"07","minute":"43","second":"33"}";
  JsonObject& root_date = jsonBuffer.parseObject(client);

  if (!root_date.success()) {
    Serial.println("Parsing failed!");
    return;
  }


  // Extract values
  Serial.println("HTTP GET Server time");
  Serial.print(root_date["year"].as<char*>());
  Serial.print(".");
  Serial.print(root_date["month"].as<char*>());
  Serial.print(".");
  Serial.print(root_date["day"].as<char*>());
  Serial.print("  ");
  Serial.print(root_date["hour"].as<char*>());
  Serial.print(":");
  Serial.print(root_date["minute"].as<char*>());
  Serial.print(":");
  Serial.println(root_date["second"].as<char*>());

  //int y = root_date["year"];

  // The following lines can be uncommented to set the date and time
  //rtc.setDOW(WEDNESDAY);     // Set Day-of-Week to SUNDAY
  rtc.setTime(root_date["hour"], root_date["minute"], root_date["second"]);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(root_date["day"], root_date["month"], root_date["year"]);   // Set the date to January 1st, 2014

  // Disconnect
  Serial.println("Disconnecting from server...");
  client.flush();
  client.stop();
  Serial.println();

}

void loop() {
  
  // if there are incoming bytes available
  // from the server, read them and print them
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if the server's disconnected, stop the client
  if (!client.connected()) {  
    // Wait one second before repeating :)
    //delay (1000);

    // do nothing forevermore
    while (true){

      if (rtc.getTime().min%2==0 && flag){

        //wifiConnect();
                
        // Send date
        Serial.print(rtc.getDateStr());
        Serial.print(" -- ");
        // Send time
        Serial.println(rtc.getTimeStr());
        
        // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
        sensors.requestTemperatures(); 
        
        Serial.print("Celsius temperature: ");
        // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
        Serial.print(sensors.getTempCByIndex(0)); 
        Serial.print(" C");
        Serial.print(" - Fahrenheit temperature: ");
        Serial.println(sensors.getTempFByIndex(0));


        // Allocate JsonBuffer
        const size_t capacity2 = 3*JSON_ARRAY_SIZE(1) + 3*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3);
        DynamicJsonBuffer jsonBuffer(capacity2);
      
        // Create JsonObject
        JsonObject& root = jsonBuffer.createObject();
      
        JsonArray& type = root.createNestedArray("type");
        JsonObject& type_0 = type.createNestedObject();
        type_0["target_id"] = "iot_data";
        
        JsonArray& title = root.createNestedArray("title");
        JsonObject& title_0 = title.createNestedObject();
        title_0["value"] = "title";
      
        JsonArray& field_iot_float = root.createNestedArray("field_iot_float");
        JsonObject& field_iot_float_0 = field_iot_float.createNestedObject();
        //field_iot_float_0["value"] = "0";
        field_iot_float_0["value"] = sensors.getTempCByIndex(0);
        
        Serial.println("Starting connection to server...");
        
        // if you get a connection, report back via serial
        if (client.connect(server, 80)) {
          Serial.println("Connected to server");
      
          // Make a HTTP request
          client.println(F("POST /node?_format=json HTTP/1.1"));
          client.println(F("Host: thesis.stefan.engineer"));
          client.println(F("Accept: application/json"));
          //client.println(F( "Connection: close" ));
      
          client.print("Content-Length: ");
          client.println(root.measureLength());
          client.println(F("Content-Type: application/json")); 
          client.println(F("Cache-Control: no-cache"));   
          client.println(F("Authorization: Basic YWRtaW46U3VuY2VDdmV0YTIwMTgu"));
          client.println(F("cache-control: no-cache")); 
          
          // Terminate headers with a blank line
          client.println();
      
          String output;
          root.printTo(output);
         
          client.println(output); 

          Serial.println("HTTP POST");
          Serial.println("Disconnecting from server...");
          client.flush();
          client.stop();
          Serial.println();
      
          // Send JSON document in body
          //root.prettyPrintTo(client);
        }
     
        flag = 0;
      }
      if (rtc.getTime().min%2!=0) {
        flag = 1;  
      }
      
      }
  }

}

void wifiConnect() {

  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
  Serial.println();
  
  }

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
