#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 3

MPU9250_asukiaaa mpu;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure example to send data
typedef struct struct_message {
  int id;
  float Pitch;
  float Roll;
  float Yaw;
} struct_message;

struct_message myData;

unsigned long previousMillis = 0;
const long interval = 100;
unsigned int readingId = 0;


float aX, aY, aZ, aSqrt, gX, gY, gZ;
float pitch, roll, yaw = 0;
unsigned long prevTime = 0;
float gyroZ_bias = 0;
bool calibrating = true;
int calibrationSamples = 500;



// Wi-Fi SSID (for scanning purposes)
constexpr char WIFI_SSID[] = "ESP_F7313D";

// Function to find the Wi-Fi channel of the desired SSID
int32_t getWiFiChannel(const char *ssid) {
  int32_t n = WiFi.scanNetworks();
  for (uint8_t i = 0; i < n; i++) {
    if (strcmp(ssid, WiFi.SSID(i).c_str()) == 0) {
      Serial.println(i);
      return WiFi.channel(i);
    }
  }
  return 0;
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  mpu.setWire(&Wire);
  mpu.beginAccel();  // Initialize accelerometer
  mpu.beginGyro();   // Initialize gyroscope
  calibrateGyro();

  Serial.println("MPU9250 initialized successfully!");

  // Scan for networks and get the channel of the desired Wi-Fi network

  int32_t channel = getWiFiChannel(WIFI_SSID);
  //int32_t channel = 0;
  WiFi.disconnect();
  Serial.printf("Connected to channel %d of SSID: %s\n", channel, WIFI_SSID);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Add peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = channel;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  

  Serial.println("ESP32 setup completed.");
}

void loop() {
  
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

  yaw += gZ * deltaTime;  // Integrating gyroscope Z-axis

 // Print values
  Serial.print("Pitch: "); Serial.print(pitch, 2);
  Serial.print("  Roll: "); Serial.print(roll, 2);
  Serial.print("  Yaw: "); Serial.println(yaw, 2);

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Set values to send
    myData.id = BOARD_ID;
    myData.Pitch = pitch;
    myData.Roll = roll;
    myData.Yaw = yaw;
    digitalWrite(LED_BUILTIN, HIGH);


    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK) {

      Serial.println("Sent with success");
      Serial.println(esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData)));
    } else {
      Serial.println("Error sending the data");
      Serial.println(esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData)));
    }
  }
  delay(75);
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
