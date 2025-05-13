/*#############################
  SW_Version 1.2
  SW_Version_Date: 12.05.2025
  createdBy: Smartflow-rws.com
  #############################*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>  // For HTTPS
#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include "smartflow_config.h"

//######################################################################################
//Config Settings
String deviceName = DEVICE_NAME;
double sensorDivisionFactor = CALIBRATION_VALUE; //Assuming sensor dipped in a 10cm water container
float lowerValue = LOWER_VALUE;
int upperValue = UPPER_VALUE;
int maxWaterDurationTime = MAX_WATER_DURATION_VALUE * 30;

//######################################################################################
/////// Auth Settings ///////
char authPost[] = SECRET_AUTH_POST;
char authGet[] = SECRET_AUTH_GET;
char authPut[] = SECRET_AUTH_PUT;
//######################################################################################

// Network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Server URLs
const char* serverNamePost = "https://www.smartflow-rws.com/_functions/postData/liveSensorData/";
const char* serverNameGet = "https://www.smartflow-rws.com/";

// BME280 Variables decleration
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

// Variables Declarations
bool flag = true;
bool batteryMode = false;
const int bufferSize = 256;
int flushWaterDuration = 60; // 1 minute valve is open in order to flush roof from water when max water level is reached
int State = 0;
int sensorPin = A0; // Analog reading of water sensor
int IsPowerOnPin = A3;
int sensorValue = 0; // initialization
int powerModeValue = 0;
int PowerControl = 14; // Power control (Battery/PS) - Relay 1
int WaterValve = 25; // Water valve contorl pin - Relay 2
int counter = 3600; // `time to send periodic data in seconds
int counterUpperValue = 0;
int counterWaterFillingUp = 0;
int numSamples = 100; // Number of samples to take
int forcedEmptyCounter = 0;
int resistorRatio = 6; //1K and 5K resistors
String valveStatus = "Auto";
String valveCurrentStatus = "Open";
String statusCode = "Open";
String humidity = "0";
String pressure = "0";
String temperature = "0";
double batteryLevel = 0;
double waterHeight = 0;
float totalValue = 0;
float averageSensorValue = 0;


// WiFi and HTTPClient objects
WiFiClientSecure client;
HTTPClient http;
String response = ""; // Global for storing HTTP responses

// Setup function to initialize WiFi and set pin modes
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000); // Set a timeout for Serial read operations
  delay(10);

  // Set relay control pins (valve + pwr control)
  pinMode(WaterValve, OUTPUT);
  pinMode(PowerControl, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  if (! bme.begin(0x77, &Wire)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    //while (1);
  }

}

void loop() {
  readPowerPin();
  readBmeValus();
  readAnalogSensor();
  if(batteryMode){
    ValveControl("Open");
    counter++;
    valveCurrentStatus = "Open";
    if (counter > 300) {
      counter = 0;
      sendPOSTRequest();
      sendPUTRequest();
    }
  }
  else{
    checkWaterLevel();
    sendGETRequest();
    counter++;
    counterUpperValue++;
    counterWaterFillingUp++;
    Serial.print("counter value: ");
    Serial.println(counter);
    Serial.print("counterUpperValue value: ");
    Serial.println(counterUpperValue);
    Serial.print("counterWaterFillingUp value: ");
    Serial.println(counterWaterFillingUp);
  }
}

void readBmeValus() {
  temperature = bme.readTemperature();
  pressure = bme.readPressure() / 100.0F;
  humidity = bme.readHumidity();
}

void readPowerPin() {
  powerModeValue = analogRead(IsPowerOnPin);
  Serial.print("Voltage on pin: ");
  batteryLevel = powerModeValue * resistorRatio * 3.3 / 4095;
  Serial.println(batteryLevel);
  Serial.print("Raw Value is: ");
  Serial.println(powerModeValue);
  if (powerModeValue < 1650) { // Battery is less than ~9V
    digitalWrite(PowerControl, LOW);
    batteryMode = true;
  } else {
    digitalWrite(PowerControl, HIGH);
    batteryMode = false;

  }
}

void readAnalogSensor() {
  for (int i = 0; i < numSamples; i++) {
    sensorValue = analogRead(sensorPin) / sensorDivisionFactor;
    totalValue += sensorValue; // Accumulate the sensor value
    delay(10); // Optional delay to stabilize readings if needed
  }
  averageSensorValue = totalValue / numSamples;
  waterHeight = averageSensorValue;
  Serial.print("Average Water Level in cm: ");
  Serial.println(waterHeight);
  averageSensorValue = 0;
  sensorValue = 0;
  totalValue = 0;

}

void checkWaterLevel() {
  Serial.print("Valve Mode is: ");
  Serial.println(valveStatus);
  Serial.print("Valve Current Status is: ");
  Serial.println(valveCurrentStatus);
  if (waterHeight < lowerValue) {
    if (valveStatus == "Auto") {
      ValveControl("Open");
      valveCurrentStatus = "Open";
      if (counter > 3600) {
        counter = 0;
        sendPOSTRequest();
        sendPUTRequest();
      }
    }
  }

  if (waterHeight > upperValue) {
    ValveControl("Open");
    valveCurrentStatus = "MaxWaterLevel-Open";
    if (counterUpperValue > 10) {
      counterUpperValue = 0;
      sendPOSTRequest();
      sendPUTRequest();
    }
  }

  if (waterHeight > lowerValue && waterHeight < upperValue) {
    if (valveStatus == "Auto") {
      ValveControl("Close");
      valveCurrentStatus = "Close";
      forcedEmptyCounter++;
      if (forcedEmptyCounter > maxWaterDurationTime) { // Empty all water on the roof and leave valve open
        forcedEmptyCounter = 0;
        ValveControl("Open");
        for (int i = 0; i < flushWaterDuration; i++) {
          sendPOSTRequest();
          sendPUTRequest();
          valveCurrentStatus = "EmptyWater-Open";
          delay(1000);      // Wait for 1 second (1000 milliseconds)
        }
      }
      if (counterWaterFillingUp > 10) {
        counterWaterFillingUp = 0;
        sendPOSTRequest();
        sendPUTRequest();
      }
    }
  }
}


// Send POST request
void sendPOSTRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    client.setInsecure(); // Disable SSL certificate validation
    http.begin(client, serverNamePost); // Specify the URL
    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    http.addHeader("auth", authPost); // Add authentication header

    // Create the JSON payload for POST message
    String payloadPost = "{\"title\":\"" + deviceName
                         + "\",\"temperature\":\"" + temperature
                         + "\",\"pressure\":\"" + pressure
                         + "\",\"humidity\":\"" + humidity
                         + "\",\"waterLevel\":\"" + waterHeight
                         + "\",\"status\":\"" + valveCurrentStatus
                         + "\",\"batteryLevel\":\"" + batteryLevel 
                         + "\"}";

    // Send POST request
    int httpResponseCode = http.POST(payloadPost);

    // Check the response code
    if (httpResponseCode > 0) {
      response = http.getString(); // Save response in global variable
      Serial.println("POST Response code: " + String(httpResponseCode));
      //Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST request: " + String(httpResponseCode));
    }

    // End the HTTP request
    http.end();
  }
}

// Send PUT request with the same payload as POST
void sendPUTRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    String serverNamePut = "https://www.smartflow-rws.com/_functions/putData/dataCollection=liveStatusSensorData&fieldKey=serialName&keysValue=" + deviceName;
    client.setInsecure(); // Disable SSL certificate validation
    String payloadPut = "{\"temperature\":\"" + temperature
                        + "\",\"pressure\":\"" + pressure
                        + "\",\"humidity\":\"" + humidity
                        + "\",\"waterLevel\":\"" + waterHeight
                        + "\",\"batteryLevel\":\"" + batteryLevel 
                        + "\",\"valveCurrentStatus\":\"" + valveCurrentStatus.substring(0, 13)
                        + "\"}";

    http.begin(client, serverNamePut); // Specify the PUT URL
    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    http.addHeader("auth", authPut); // Add authentication header

    // Send PUT request with the same payload
    int httpResponseCode = http.PUT(payloadPut);

    // Check the PUT response code
    if (httpResponseCode > 0) {
      response = http.getString(); // Save response in global variable
      Serial.println("PUT Response code: " + String(httpResponseCode));
      //Serial.println("PUT Response: " + response);
    } else {
      Serial.println("Error on sending PUT request: " + String(httpResponseCode));
    }

    // End the PUT request
    http.end();
  }
}

// Function to send a GET request with parameters in the URL
void sendGETRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    client.setInsecure(); // Disable SSL certificate validation

    // Create the GET request URL with parameters
    String url = String(serverNameGet) + "_functions/getData/dataCollection=liveStatusSensorData&fieldKey=serialName&condition=eq&keysValue=" + deviceName;

    http.begin(client, url); // Specify the full URL including parameters
    http.addHeader("auth", authGet); // Add authentication header

    // Send GET request
    int httpResponseCode = http.GET();

    // Check the response code
    if (httpResponseCode > 0) {
      response = http.getString(); // Save response in global variable
      Serial.println("GET Response code: " + String(httpResponseCode));
      //Serial.println("GET Response: " + response);

      // Parse the JSON response
      const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(13) + response.length() * sizeof(char);
      DynamicJsonDocument doc(capacity);
      DeserializationError error = deserializeJson(doc, response);

      // Check if parsing succeeded
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // Extract the first element of the array
      JsonObject obj = doc[0];
      statusCode = obj["status"].as<String>();
      lowerValue = obj["lowerValue"].as<String>().toFloat();
      upperValue = obj["upperValue"].as<String>().toInt();
      maxWaterDurationTime = obj["maxWaterDurationTime"].as<String>().toInt() * 30;
      flushWaterDuration = obj["flushWaterDuration"].as<String>().toInt();

      // Set State based on status
      if (statusCode == "Auto") {
        valveStatus = "Auto";
        flag = true;
      } else if (statusCode == "Manual_Open") {
        valveStatus = "Manual_Open";
        ValveControl("Open");
        flag = true;
      } else if (statusCode == "Manual_Close") {
        valveStatus = "Manual_Close";
        if (flag) {
          flag = false;
          ValveControl("Close");
          delay(10000);
          ValveControl("Open");
        }
      }

      Serial.print("State: ");
      Serial.println(State);
    } else {
      Serial.println("Error on sending GET request: " + String(httpResponseCode));
    }

    // End the HTTP request
    http.end();
  }
}

void ValveControl(String mode) {
  if (mode == "Open") {
    digitalWrite(WaterValve, LOW);
  }

  if (mode == "Close") {
    digitalWrite(WaterValve, HIGH);
  }
}
