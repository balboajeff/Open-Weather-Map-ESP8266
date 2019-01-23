/*
  * Adapted from code written by Emmanuel Odunlade 
  * 
  */
  
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Replace with your SSID and password details
char ssid[] = "Your WiFi name";        
char pass[] = "Your WiFi password";   

WiFiClient client;

// Open Weather Map API server name
const char server[] = "api.openweathermap.org";

// Replace the next line to match your city and 2 letter country code
String nameOfCity = "Estes Park,US"; 


// Replace the next line with your API Key
String apiKey = "Your Key Here"; 

String text;

int jsonend = 0;
boolean startJson = false;
int status = WL_IDLE_STATUS;
/*
int rainLed = 2;  // Indicates rain
int clearLed = 3; // Indicates clear sky or sunny
int snowLed = 4;  // Indicates snow
int hailLed = 5;  // Indicates hail
*/
#define JSON_BUFF_DIMENSION 2500

unsigned long lastConnectionTime = 10 * 1000;     // last time you connected to the server, in milliseconds
const unsigned long postInterval = 10* 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)

void setup() {
  
  Serial.begin(9600);
  
  text.reserve(JSON_BUFF_DIMENSION);
  
  WiFi.begin(ssid,pass);
  Serial.println("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");
  printWiFiStatus();
}

void loop() { 
  //OWM requires 10mins between request intervals
  //check if 10mins has passed then conect again and pull
  if (millis() - lastConnectionTime > postInterval) {
    // note the time that the connection was made:
    lastConnectionTime = millis();
    makehttpRequest();
  }
}

// print Wifi status
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// to request data from OWM
void makehttpRequest() {
  // close any connection before send a new request to allow client make connection to server
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, 80)) {
     Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("GET /data/2.5/forecast?q=" + nameOfCity + "&APPID=" + apiKey + "&mode=json&units=metric&cnt=2 HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
    
    char c = 0;
    while (client.available()) {
      c = client.read();
      // since json contains equal number of open and close curly brackets, this means we can determine when a json is completely received  by counting
      // the open and close occurences,
      //Serial.print(c);
      if (c == '{') {
        startJson = true;         // set startJson true to indicate json message has started
        jsonend++;
      }
      if (c == '}') {
        jsonend--;
      }
      if (startJson == true) {
        text += c;
      }
      // if jsonend = 0 then we have have received equal number of curly braces 
      if (jsonend == 0 && startJson == true) {
        parseJson(text.c_str());  // parse c string text in parseJson function
        text = "";                // clear text string for the next time
        startJson = false;        // set startJson to false to indicate that a new message has not yet started
      }
    }
  }
  else {
    // if no connction was made:
    Serial.println("connection failed");
    return;
  }
}

//to parse json data recieved from OWM
void parseJson(const char * jsonString) {
  //StaticJsonBuffer<4000> jsonBuffer;
  const size_t bufferSize = 2*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 4*JSON_OBJECT_SIZE(1) + 3*JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 2*JSON_OBJECT_SIZE(7) + 2*JSON_OBJECT_SIZE(8) + 720;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  // FIND FIELDS IN JSON TREE
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  JsonArray& list = root["list"];
  JsonObject& nowT = list[0];
  JsonObject& later = list[1];

  // including temperature and humidity for those who may wish to hack it in
  
  String city = root["city"]["name"];
  
  float tempNow = nowT["main"]["temp"];
  float humidityNow = nowT["main"]["humidity"];
  String weatherNow = nowT["weather"][0]["description"];
Serial.print("temp= ");
Serial.println(tempNow);
Serial.print("Humidity= ");
Serial.println(humidityNow);
Serial.print("Weather= ");
Serial.println(weatherNow);

  float tempLater = later["main"]["temp"];
  float humidityLater = later["main"]["humidity"];
  String weatherLater = later["weather"][0]["description"];

  Serial.println();
}

