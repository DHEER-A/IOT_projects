#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>

const char* ssid = "Galaxy F62";
const char* password = "12345678910abcd";

float pitchC[4] = {0.0}, rollC[4] = {0.0}, yawC[4] = {0.0}, loadC = 0;
// Store Pitch, Roll, Yaw values from 3 ESP32 Senders + Load value
float pitch[3] = {0}, roll[3] = {0}, yaw[3] = {0}, load = 0;
bool calibrate[4] = {false};

typedef struct struct_message {
  int id;
  float pitch;
  float roll;
  float yaw;
} struct_message;

struct_message incomingReadings;
JSONVar board;

AsyncWebServer server(80);
AsyncEventSource events("/events");


// UUIDs for the 3 sender boards (Pitch/Roll/Yaw)
#define SERVICE_UUID_1        "12345678-1234-5678-1234-56789abcdef0"
#define SERVICE_UUID_2        "23456789-2345-6789-2345-6789abcdef01"
#define SERVICE_UUID_3        "34567890-3456-789a-3456-789abcdef012"
#define CHARACTERISTIC_UUID   "abcdef12-3456-789a-bcde-f01234567890"

// UUID for Load data sender
#define LOAD_SERVICE_UUID     "45678901-4567-890b-4567-890abcdef013"
#define LOAD_CHARACTERISTIC_UUID "bcdef123-4567-890b-cdef-123456789012"

// BLE Clients & Characteristics for 3 Pitch/Roll/Yaw Senders and 1 Load Sender
BLEAdvertisedDevice* myDevices[4];
BLEClient* pClients[4] = {nullptr, nullptr, nullptr, nullptr};
BLERemoteCharacteristic* pRemoteCharacteristics[4] = {nullptr, nullptr, nullptr, nullptr};
bool doConnect[4] = {false, false, false, false};



// Parse incoming Pitch/Roll/Yaw data
void parseSensorData(const char* data, int index) {
    sscanf(data, "%f,%f,%f", &pitch[index], &roll[index], &yaw[index]);
    Serial.printf("Board %d -> Pitch: %d, Roll: %d, Yaw: %d\n", index + 1, int(pitch[index]), int(roll[index]), int(yaw[index]));
    Serial.printf("Received Raw Data: %s\n", data);



  board["id"] = index + 1;
  board["pitch"] = int(abs(pitch[index]-pitchC[index]+360))%360;
  board["roll"] = int(abs(roll[index] -rollC[index]+360))%360;
  board["yaw"] = int(abs(yaw[index] - yawC[index]+360))%360;
  events.send(JSON.stringify(board).c_str(), "new_readings", millis());

  if (calibrate[0]) {
    calibrate[0] = false;
    pitchC[0]= int(pitch[0]);
    rollC[0] = int(roll[0]);
    yawC[0] = int(yaw[0]);  
    Serial.println("Calibration done for board 1");
  } 
  else if (calibrate[1]) {
    calibrate[1] = false;
    pitchC[1]= int(pitch[1]);
    rollC[1] = int(roll[1]);
    yawC[1] = int(yaw[1]);
    Serial.println("Calibration done for board 2");
  } 
  else if (calibrate[2]) {
    calibrate[2] = false;
    pitchC[2]= int(pitch[2]);
    rollC[2] = int(roll[2]);
    yawC[2] = int(yaw[2]);
    Serial.println("Calibration done for board 3");
  }
}

// Parse incoming Load data
void parseLoadData(const char* data) {
    sscanf(data, "Load:%f", &load);
    Serial.printf("Load Sensor -> Load: %.2f\n", load);

    board["id"] = 4;
    board["load"] = int(load) - int(loadC);
    events.send(JSON.stringify(board).c_str(), "new_readings", millis());


  if (calibrate[3]) {
    calibrate[3] = false;
    loadC = int(load);
    Serial.println("Calibration done for board 4");
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP-NOW DASHBOARD</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: 'Arial', sans-serif;
    }

    body {
      background: linear-gradient(135deg, #1e3c72, #2a5298);
      color: #fff;
      text-align: center;
      padding: 20px;
    }

    h2 {
      font-size: 2.5rem;
      margin-bottom: 20px;
      font-weight: bold;
    }

    .container {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 20px;
    }

    .card {
      background: rgba(255, 255, 255, 0.1);
      padding: 25px;
      border-radius: 15px;
      width: 350px;
      box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.3);
      transition: transform 0.3s;
      position: relative;
    }

    .card:hover {
      transform: translateY(-5px);
    }

    h3 {
      font-size: 2rem;
      margin-bottom: 15px;
      font-weight: bold;
      text-transform: uppercase;
      letter-spacing: 1px;
    }

    /* Bigger Labels for Pitch, Roll, Yaw */
    .label {
      font-size: 2.3rem; /* BIGGER FONT FOR LABELS */
      font-weight: bold;
      display: block;
      margin-top: 10px;
    }

    /* Bigger Values */
    .reading {
      font-size: 2.5rem; /* Values Stay Big */
      font-weight: bold;
      display: block;
    }

    .btn {
      background: #ff9800;
      border: none;
      padding: 12px 20px;
      font-size: 1.4rem;
      font-weight: bold;
      border-radius: 8px;
      cursor: pointer;
      margin-top: 15px;
      transition: background 0.3s ease-in-out;
    }

    .btn:hover {
      background: #e68900;
    }

    /* Connection Status Indicator */
    .status {
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: red;
      position: absolute;
      top: 15px;
      right: 15px;
      border: 3px solid #fff;
    }

    /* Responsive Design */
    @media (max-width: 768px) {
      .container {
        flex-direction: column;
        align-items: center;
      }
    }
  </style>
</head>
<body>

  <h2>ESP-NOW DASHBOARD</h2>

  <div class="container">
    <div class="card">
      <div class="status" id="status1"></div>
      <h3>MODULE 1</h3>
      <p><span class="label">Pitch: <span id="pitch1"></span></span></p>
      <p><span class="label">Roll: <span id="roll1"></span></span></p>
      <p><span class="label">Yaw: <span id="yaw1"></span></span></p>
      <button class="btn" onclick="calibrate(1)">Calibrate</button>
    </div>

    <div class="card">
      <div class="status" id="status2"></div>
      <h3>MODULE 2</h3>
      <p><span class="label">Pitch: <span id="pitch2"></span></span></p>
      <p><span class="label">Roll: <span id="roll2"></span></span></p>
      <p><span class="label">Yaw: <span id="yaw2"></span></span></p>
      <button class="btn" onclick="calibrate(2)">Calibrate</button>
    </div>

    <div class="card">
      <div class="status" id="status3"></div>
      <h3>MODULE 3</h3>
      <p><span class="label">Pitch: <span id="pitch3"></span></span></p>
      <p><span class="label">Roll: <span id="roll3"></span></span></p>
      <p><span class="label">Yaw: <span id="yaw3" ></span></span></p>
      <button class="btn" onclick="calibrate(3)">Calibrate</button>
    </div>

    <div class="card">
      <div class="status" id="status4"></div>
      <h3>LOAD MODULE</h3>
      <p><span class="label">Load: <span id="load" >g</span></span></p>
      <button class="btn" onclick="calibrate(4)">Calibrate</button>
    </div>
  </div>

  <script>
    // Track whether data has been received from each module
    let receivedData = { 1: false, 2: false, 3: false, 4: false };

    if (!!window.EventSource) {
      var source = new EventSource('/events');

      source.addEventListener('new_readings', function(e) {
        var data = JSON.parse(e.data);
        document.getElementById("load").innerHTML = data.load;
        document.getElementById("pitch" + data.id).innerHTML = data.pitch;
        document.getElementById("roll" + data.id).innerHTML = data.roll;
        document.getElementById("yaw" + data.id).innerHTML = data.yaw;

        // Set connection status to green once data is received
        if (!receivedData[data.id]) {
          document.getElementById("status" + data.id).style.background = "green";
          receivedData[data.id] = true;
        }
      }, false);
    }

    function calibrate(id) {
      fetch('/calibrate?id=' + id);
    }
  </script>

</body>
</html>
)rawliteral";



// BLE Notification Callback for Pitch/Roll/Yaw
void notifyCallback(BLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify, int index) {
    char receivedData[50];
    strncpy(receivedData, (char*)pData, length);
    receivedData[length] = '\0';
    parseSensorData(receivedData, index);
}

// BLE Notification Callback for Load Sensor
void notifyLoadCallback(BLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    char receivedData[20];
    strncpy(receivedData, (char*)pData, length);
    receivedData[length] = '\0';
    parseLoadData(receivedData);
}

// Custom Callback Wrapper for each board
void notifyCallbackWrapper1(BLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    notifyCallback(pCharacteristic, pData, length, isNotify, 0);
}
void notifyCallbackWrapper2(BLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    notifyCallback(pCharacteristic, pData, length, isNotify, 1);
}
void notifyCallbackWrapper3(BLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    notifyCallback(pCharacteristic, pData, length, isNotify, 2);
}

// BLE Client Callbacks
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) { Serial.println("Connected to BLE server"); }
    void onDisconnect(BLEClient* pclient) { Serial.println("Disconnected. Reconnecting..."); }
};

// BLE Scan Callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID()) {
            String uuid = advertisedDevice.getServiceUUID().toString().c_str();
            if (uuid == SERVICE_UUID_1) { myDevices[0] = new BLEAdvertisedDevice(advertisedDevice); doConnect[0] = true; }
            if (uuid == SERVICE_UUID_2) { myDevices[1] = new BLEAdvertisedDevice(advertisedDevice); doConnect[1] = true; }
            if (uuid == SERVICE_UUID_3) { myDevices[2] = new BLEAdvertisedDevice(advertisedDevice); doConnect[2] = true; }
            if (uuid == LOAD_SERVICE_UUID) { myDevices[3] = new BLEAdvertisedDevice(advertisedDevice); doConnect[3] = true; }
        }
    }
};

// Connect to a BLE Server
void connectToServer(int index, String uuid, String characteristicUUID) {
    if (!pClients[index]) {
        pClients[index] = BLEDevice::createClient();
        pClients[index]->setClientCallbacks(new MyClientCallback());
    }

    if (!pClients[index]->isConnected() && pClients[index]->connect(myDevices[index])) {
        Serial.printf("Connected to BLE Server %d\n", index + 1);
        BLERemoteService* pRemoteService = pClients[index]->getService(BLEUUID(uuid.c_str()));
        if (pRemoteService) {
            pRemoteCharacteristics[index] = pRemoteService->getCharacteristic(BLEUUID(characteristicUUID.c_str()));
            if (pRemoteCharacteristics[index]) {
                if (index == 0) pRemoteCharacteristics[index]->registerForNotify(notifyCallbackWrapper1);
                if (index == 1) pRemoteCharacteristics[index]->registerForNotify(notifyCallbackWrapper2);
                if (index == 2) pRemoteCharacteristics[index]->registerForNotify(notifyCallbackWrapper3);
                if (index == 3) pRemoteCharacteristics[index]->registerForNotify(notifyLoadCallback);
                Serial.printf("Subscribed to BLE notifications from Board %d\n", index + 1);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected!");


    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html);
    });

    server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (request->hasParam("id")) {
        int id = request->getParam("id")->value().toInt() - 1;
        calibrate[id] = true;
      }
      request->send(200, "text/plain", "OK");
    });

    events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    });
    server.addHandler(&events);
    server.begin();

    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10, false);
}

void loop() {
    for (int i = 0; i < 4; i++) {
        if (doConnect[i]) {
            if (i < 3) {
                connectToServer(i, i == 0 ? SERVICE_UUID_1 : (i == 1 ? SERVICE_UUID_2 : SERVICE_UUID_3), CHARACTERISTIC_UUID);
            } else {
                connectToServer(i, LOAD_SERVICE_UUID, LOAD_CHARACTERISTIC_UUID);
            }
            doConnect[i] = false;
        }
    }

  static unsigned long lastEventTime = millis();
    static const unsigned long EVENT_INTERVAL_MS = 5000;
    if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
      events.send("ping",NULL,millis());
      lastEventTime = millis();
    }
}
