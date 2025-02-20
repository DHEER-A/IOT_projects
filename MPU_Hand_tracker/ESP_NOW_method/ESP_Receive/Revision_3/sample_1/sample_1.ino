#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include "MPU9250_asukiaaa.h"

// MPU9250 sensor object for server ESP32 (board 3)
MPU9250_asukiaaa mpu;

// Arrays to store pitch, roll, and yaw for all 4 boards (including server as board 3)
float pitch[4] = {0}, roll[4] = {0}, yaw[4] = {0};
float pitchOffset[4] = {0}, rollOffset[4] = {0}, yawOffset[4] = {0};

AsyncWebServer server(80);
AsyncEventSource events("/events");

// WiFi credentials
const char* ssid = "Galaxy F62";
const char* password = "12345678910abcd";

// ESP-NOW receive callback function
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
    if (len == sizeof(pitch) + sizeof(roll) + sizeof(yaw)) {
        memcpy(pitch + 1, data, sizeof(pitch) - sizeof(pitch[0]));  // Receiving pitch for boards 1,2,4
        memcpy(roll + 1, data + sizeof(pitch) - sizeof(pitch[0]), sizeof(roll) - sizeof(roll[0]));
        memcpy(yaw + 1, data + sizeof(pitch) + sizeof(roll) - sizeof(pitch[0]) - sizeof(roll[0]), sizeof(yaw) - sizeof(yaw[0]));
        Serial.println("Data received from other boards.");
    }
}

// Calibration function
void calibrateModule(int module) {
    float sumPitch = 0, sumRoll = 0, sumYaw = 0;
    for (int i = 0; i < 300; i++) {
        mpu.magUpdate();
        sumPitch += mpu.magX();
        sumRoll += mpu.magY();
        sumYaw += mpu.magZ();
        delay(10);
    }
    pitchOffset[module] = sumPitch / 300;
    rollOffset[module] = sumRoll / 300;
    yawOffset[module] = sumYaw / 300;
    Serial.printf("Calibration done for Board %d\n", module + 1);
}

// HTML for web interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial; text-align: center; }
    .topnav { background-color: #2f4468; color: white; padding: 20px; }
    .content { padding: 20px; }
    .card { background-color: white; padding: 20px; margin: 10px; }
    .reading { font-size: 2.8rem; }
    .button { padding: 10px 20px; background-color: #008CBA; color: white; border: none; cursor: pointer; }
  </style>
</head>
<body>
  <div class="topnav"><h3>ESP-NOW DASHBOARD</h3></div>
  <div class="content">%s</div>
  <script>
    function calibrate(board) { fetch('/calibrate?module=' + board); }
    if (!!window.EventSource) {
      var source = new EventSource('/events');
      source.addEventListener('new_readings', function(e) {
        var obj = JSON.parse(e.data);
        for (var i = 0; i < 4; i++) {
          document.getElementById("pitch" + (i+1)).innerHTML = obj.pitch[i].toFixed(2);
          document.getElementById("roll" + (i+1)).innerHTML = obj.roll[i].toFixed(2);
          document.getElementById("yaw" + (i+1)).innerHTML = obj.yaw[i].toFixed(2);
        }
      }, false);
    }
  </script>
</body>
</html>)rawliteral";

// Generate HTML content dynamically
String generateHTML() {
    String html = "";
    for (int i = 0; i < 4; i++) {
        html += "<div class='card'>";
        html += "<h4>Board " + String(i + 1) + " - Pitch</h4><p><span class='reading' id='pitch" + String(i + 1) + "'></span> &deg;</p>";
        html += "<h4>Board " + String(i + 1) + " - Roll</h4><p><span class='reading' id='roll" + String(i + 1) + "'></span> &deg;</p>";
        html += "<h4>Board " + String(i + 1) + " - Yaw</h4><p><span class='reading' id='yaw" + String(i + 1) + "'></span> &deg;</p>";
        html += "<button class='button' onclick='calibrate(" + String(i) + ")'>Calibrate Board " + String(i + 1) + "</button>";
        html += "</div>";
    }
    return html;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi. IP Address: " + WiFi.localIP().toString());
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);

    mpu.setWire(&Wire);
    mpu.beginMag();
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = String(index_html);
        html.replace("%s", generateHTML());
        request->send(200, "text/html", html);
    });

    server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("module")) {
            int module = request->getParam("module")->value().toInt();
            calibrateModule(module);
            request->send(200, "text/plain", "Calibration done for module " + String(module + 1));
        }
    });

    events.onConnect([](AsyncEventSourceClient *client) {
        client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);
    server.begin();
}

void sendSensorData() {
    String json = "{";
    json += "\"pitch\":[" + String(pitch[0], 2) + "," + String(pitch[1], 2) + "," + String(pitch[2], 2) + "," + String(pitch[3], 2) + "],";
    json += "\"roll\":[" + String(roll[0], 2) + "," + String(roll[1], 2) + "," + String(roll[2], 2) + "," + String(roll[3], 2) + "],";
    json += "\"yaw\":[" + String(yaw[0], 2) + "," + String(yaw[1], 2) + "," + String(yaw[2], 2) + "," + String(yaw[3], 2) + "]";
    json += "}";
    events.send(json.c_str(), "new_readings", millis());
}

void loop() {
    mpu.magUpdate();
    pitch[0] = mpu.magX() - pitchOffset[0];
    roll[0] = mpu.magY() - rollOffset[0];
    yaw[0] = mpu.magZ() - yawOffset[0];

    Serial.printf("Board 3 - Pitch: %.2f, Roll: %.2f, Yaw: %.2f\n", pitch[0], roll[0], yaw[0]);

    sendSensorData();
    delay(50);
}