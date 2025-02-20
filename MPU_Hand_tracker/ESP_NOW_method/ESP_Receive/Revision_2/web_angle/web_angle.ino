#include <ESP8266WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

const char* ssid = "Galaxy F62";  // Set your Wi-Fi SSID
const char* password = "12345678910abcd";  // Set your Wi-Fi password



Adafruit_MPU6050 mpu;
AsyncWebServer server(80);

// Function to get the roll angle from the MPU6050 sensor
float getRollAngle() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // Calculate Roll (rotation around X-axis)
  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  float roll = atan2(ay, az) * 180 / PI;
  return roll;
}

// Plain text response with roll angle value
String generatePlainText() {
  String text = "Roll Angle: " + String(getRollAngle()) + " degrees";
  return text;
}

void setup() {
  Serial.begin(115200);


  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("Connected to Wi-Fi, IP address: ");
  Serial.println(WiFi.localIP());  // Print the ESP IP address

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found");
    while (1);  // Halt if MPU6050 is not found
  }

  // Handle root URL and send plain text response
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String text = generatePlainText();  // Generate the plain text content with the roll angle
    request->send(200, "text/plain", text);  // Send the plain text as the response
  });

  // Start the server
  server.begin();
}

void loop() {
  // Nothing to do in loop, the server runs asynchronously
}
