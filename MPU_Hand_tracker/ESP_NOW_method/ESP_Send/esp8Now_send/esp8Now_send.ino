/* This is the program for esp8266 which is programmed to send the y and z values from 
mpu6050 to an esp32 with espNOW protocol*/



#include <ESP8266WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <espnow.h>

Adafruit_MPU6050 mpu;
AsyncWebServer server(80);

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t broadcastAddress[] = {0xC8, 0x2E, 0x18, 0xF7, 0x31, 0x3C}; // MAC Address of the receiver

// Structure to send data
typedef struct struct_message {
    int id; // must be unique for each sender board
    float y;
    float z;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(sendStatus == 0 ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 initialized successfully!");

  // Optional: Configure accelerometer range
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  // Optional: Configure gyroscope range
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // Set the data rate for better sampling (optional)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);



  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback for sending data
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER); // Set ESP8266 as a controller
  esp_now_register_send_cb(OnDataSent);

  // Add peer (receiver)
  if (esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0) != 0) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Set values to send
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  myData.id = 2;
  myData.y = ay;
  Serial.println(ay);
  myData.z = az;
  Serial.println(az);

  // Send message via ESP-NOW
  int result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == 0) {
 //   Serial.println("Sent with success");
  } else {
  //  Serial.println("Error sending the data");
  }
  delay(100); // Delay between transmissions
}
