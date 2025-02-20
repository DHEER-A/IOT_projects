#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "HX711.h"

#define LOADCELL_DOUT_PIN 6  // HX711 Data pin (ESP32-S3 GPIO)
#define LOADCELL_SCK_PIN 7   // HX711 Clock pin (ESP32-S3 GPIO)

#define SERVICE_UUID        "45678901-4567-890b-4567-890abcdef013"  // Unique Load Data Service UUID
#define CHARACTERISTIC_UUID "bcdef123-4567-890b-cdef-123456789012"  // Unique Load Data Characteristic UUID

float calibration_factor =183.9;

BLECharacteristic *pCharacteristic;

HX711 scale;

// Function to simulate Load Data
void sendLoadData() {
    if (scale.is_ready()) {
        long raw_value = scale.get_units(10);  // Get the raw reading
        Serial.print("Raw Value: ");
        Serial.println(raw_value);  // Print raw sensor output

        float load = raw_value / calibration_factor;  // Convert to grams

        String loadData = "Load:" + String(load, 2);

        Serial.println("Sending: " + loadData);

    // Send data via BLE notification
        pCharacteristic->setValue(loadData.c_str());
        pCharacteristic->notify();

      } else {
        Serial.println("HX711 not found. Check wiring.");
      }
  
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
    Serial.println("ESP32-S3 HX711 Weight Calibration");

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    Serial.println("Remove all weight and taring scale...");
    delay(3000);
    
    scale.tare(); // Set zero point
    Serial.println("Scale tared. Now place a known weight.");
    BLEDevice::init("ESP32-Load-Sensor");

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

    Serial.println("BLE Load Data Sender Started!");
}

void loop() {
    sendLoadData();  // Send new data every 100ms
    delay(250);
}
