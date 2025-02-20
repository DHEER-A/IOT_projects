#include <WiFi.h>
#include <esp_now.h>
#include "HX711.h"

#define LOADCELL_DOUT_PIN 6  // HX711 Data pin (ESP32-S3 GPIO)
#define LOADCELL_SCK_PIN 7   // HX711 Clock pin (ESP32-S3 GPIO)

HX711 scale;


// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 4

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure example to send data
typedef struct struct_message {
  int id;
  float Pitch;
  float Roll;
  float Yaw;
} struct_message;

struct_message myData;

float weight_grams;
long raw_value;
float calibration_factor = 183.9;

unsigned long previousMillis = 0;
const long interval = 100;


// Wi-Fi SSID (for scanning purposes)
constexpr char WIFI_SSID[] = "ESP_F7313D";

// Function to find the Wi-Fi channel of the desired SSID
int32_t getWiFiChannel(const char *ssid) {
  int32_t n = WiFi.scanNetworks();
  for (uint8_t i = 0; i < n; i++) {
    if (strcmp(ssid, WiFi.SSID(i).c_str()) == 0) {
      return WiFi.channel(6);
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
//  pinMode(LED_BUILTIN, OUTPUT);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare(); // Set zero point


  // Scan for networks and get the channel of the desired Wi-Fi network
  WiFi.mode(WIFI_STA);
  int32_t channel = getWiFiChannel(WIFI_SSID);

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

void loop() 
{
  
  unsigned long currentMillis = millis();
  if (scale.is_ready()) 
  {
      raw_value = scale.get_units(10);  // Get the raw reading
      Serial.print("Raw Value: ");
      Serial.println(raw_value);  // Print raw sensor output

      weight_grams = raw_value / calibration_factor;  // Convert to grams
      Serial.print("Measured Weight: ");
      Serial.print(weight_grams, 2);
      Serial.println(" g");
  } 
  else 
  {
      Serial.println("HX711 not found. Check wiring.");
  }

  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;

    // Set values to send
    myData.id = BOARD_ID;
    myData.Pitch = raw_value;
    myData.Roll = weight_grams;
    myData.Yaw = 369.20;
  //  digitalWrite(LED_BUILTIN, HIGH);


  // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK) 
    {

      Serial.println("Sent with success");
    } 
    else 
    {
      Serial.println("Error sending the data");
    }
  }
  delay(100);
//  digitalWrite(LED_BUILTIN, LOW);
  delay(100);

}
