#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>

#define SERVICE_UUID        "34567890-3456-789a-3456-789abcdef012"
#define CHARACTERISTIC_UUID "abcdef12-3456-789a-bcde-f01234567890"

MPU9250_asukiaaa mpu;

BLECharacteristic *pCharacteristic;

unsigned long previousMillis = 0;
const long interval = 100;
unsigned int readingId = 0;


float aX, aY, aZ, aSqrt, gX, gY, gZ;
float pitch, roll, yaw = 0;
unsigned long prevTime = 0;
float gyroZ_bias = 0;
bool calibrating = true;
int calibrationSamples = 100;



// Function to simulate pitch, roll, and yaw values
void sendSensorData() {
    unsigned long currentMillis = millis();


  mpu.accelUpdate();
  aX = mpu.accelX();
  aY = mpu.accelY();
  aZ = mpu.accelZ();

  mpu.gyroUpdate();
  gX = mpu.gyroX();
  gY = mpu.gyroY();
  gZ = mpu.gyroZ() - gyroZ_bias;

    // Calculate pitch and roll using accelerometer
  pitch = atan2(aY, sqrt(aX * aX + aZ * aZ)) * 180.0 / PI;
  roll = atan2(-aX, aZ) * 180.0 / PI;

  // Calculate yaw using gyroscope integration
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - prevTime) / 1000.0;  // Convert ms to seconds
  prevTime = currentTime;

  yaw += gZ * deltaTime;
    // Format data as "Pitch:x,Roll:y,Yaw:z"
    String sensorData = "" + String(abs(pitch), 2) + 
                        "," + String(abs(roll), 2) + 
                        "," + String(abs(yaw), 2);

    Serial.println("Sending: " + sensorData);

    // Send data via BLE notification
    pCharacteristic->setValue(sensorData.c_str());
    pCharacteristic->notify();
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Client Connected");
    }
    void onDisconnect(BLEServer* pServer) {
        Serial.println("Client Disconnected. Restarting advertising...");
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->start();
    }
};

void setup() {
    Serial.begin(115200);
     Wire.begin();
    pinMode(LED_BUILTIN, OUTPUT);
    mpu.setWire(&Wire);
    mpu.beginAccel();  // Initialize accelerometer
    mpu.beginGyro();   // Initialize gyroscope
    calibrateGyro();

    Serial.println("MPU9250 initialized successfully!");
    BLEDevice::init("ESP32-BLE-Sensor");

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                          CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_READ   |
                          BLECharacteristic::PROPERTY_NOTIFY
                      );

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();

    Serial.println("BLE Server Started!");
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(75);
    sendSensorData();  // Send new data every 100ms
    digitalWrite(LED_BUILTIN, LOW);
    delay(75);
}

void calibrateGyro() {
  float sum = 0;
  for (int i = 0; i < calibrationSamples; i++) {
    mpu.gyroUpdate();
    sum += mpu.gyroZ();  // Convert to degrees
    delay(5);
  }
  gyroZ_bias = sum / calibrationSamples;
  calibrating = false;
}
