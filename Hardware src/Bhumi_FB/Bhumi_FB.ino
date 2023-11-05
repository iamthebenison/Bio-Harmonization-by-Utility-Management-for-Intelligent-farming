#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "time.h"
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Redmi27"
#define WIFI_PASSWORD "123456789"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAcBDGhLuPQKZ6exPPTEpC1HkyTR84ZUz4"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://bhumi-529da-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String soilMoistpath = "/soilmois";
String pHValpath = "/phval";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";


float temperature;
float humidity;
float pressure;

#define DHTPIN 17
#define DHTTYPE DHT22

// pH electrode sensor
const int pHPin = 32; // Define the pin for the pH sensor
const float pHOffset = 0.0; // Adjust this value based on your calibration

// Soil moisture sensor
const int soilMoisturePin = 34;

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);


// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
bool signUpStat=false;
unsigned long timerDelay = 1800;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup(){
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();
  // Initialize Soil Moisture sensor
  pinMode(soilMoisturePin, INPUT);

  //initialize WiFi
  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("SignupOK!");
    signUpStat=true;
  } else{
    Serial.println("Failed");
  } 

  config.token_status_callback = tokenStatusCallback;   
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop(){

  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    // Reading temperature or humidity takes about 250 milliseconds!
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int soilMoisture = analogRead(soilMoisturePin);
    float pHValue = readpH();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(), String(temperature));
    json.set(humPath.c_str(), String(humidity));
    json.set(soilMoistpath.c_str(), String(soilMoisture));
    json.set(pHValpath.c_str(), String(pHValue));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

    Serial.println("Data:");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C ");
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoisture);
    Serial.print("pH Value: ");
    Serial.println(pHValue);
  }
}



float readpH() {
  int pHRaw = analogRead(pHPin); // Read pH sensor using ESP32's built-in ADC
  float pHValue = map(pHRaw, 0, 4095, 0.0, 14.0); // Adjust the mapping based on your pH sensor's characteristics
  return pHValue;
}