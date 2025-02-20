#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>

const char* ssid = "Galaxy F62";
const char* password = "12345678910abcd";

float pitchC[4] = {0.0}, rollC[4] = {0.0}, yawC[4] = {0.0};
float pitch[4] = {0.0}, roll[4] = {0.0}, yaw[4] = {0.0};
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

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  int index = incomingReadings.id - 1;
  pitch[index] = incomingReadings.pitch;
  roll[index] = incomingReadings.roll;
  yaw[index] = incomingReadings.yaw;

  board["id"] = incomingReadings.id;
  board["pitch"] = pitch[index] - pitchC[index];
  board["roll"] = roll[index] - rollC[index];
  board["yaw"] = yaw[index] - yawC[index];
  events.send(JSON.stringify(board).c_str(), "new_readings", millis());

  if (calibrate[0]) {
    calibrate[0] = false;
    pitchC[0]= pitch[0];
    rollC[0] = roll[0];
    yawC[0] = yaw[0];  
    Serial.println("Calibration done for board 1");
  } else if (calibrate[1]) {
    calibrate[1] = false;
    pitchC[1]= pitch[1];
    rollC[1] = roll[1];
    yawC[1] = yaw[1];
    Serial.println("Calibration done for board 2");
  } else if (calibrate[2]) {
    calibrate[2] = false;
    pitchC[2]= pitch[2];
    rollC[2] = roll[2];
    yawC[2] = yaw[2];
    Serial.println("Calibration done for board 3");
  } else if (calibrate[3]) {
    calibrate[3] = false;
    pitchC[3]= pitch[3];
    rollC[3] = roll[3];
    yawC[3] = yaw[3];
    Serial.println("Calibration done for board 4");
  }
}



const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; }
    .card { background: #fff; padding: 20px; margin: 20px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
    .reading { font-size: 2rem; }
    .btn { padding: 10px 20px; font-size: 1.2rem; cursor: pointer; }
  </style>
</head>
<body>
  <h2>ESP-NOW DASHBOARD</h2>
  <div class="card">
    <h3>MODULE 1</h3>
    <p>Pitch: <span id="pitch1">0</span> Roll: <span id="roll1">0</span> Yaw: <span id="yaw1">0</span></p>
    <button class="btn" onclick="calibrate(1)">Calibrate</button>
  </div>
  <div class="card">
    <h3>MODULE 2</h3>
    <p>Pitch: <span id="pitch2">0</span> Roll: <span id="roll2">0</span> Yaw: <span id="yaw2">0</span></p>
    <button class="btn" onclick="calibrate(2)">Calibrate</button>
  </div>
  <div class="card">
    <h3>MODULE 3</h3>
    <p>Pitch: <span id="pitch3">0</span> Roll: <span id="roll3">0</span> Yaw: <span id="yaw3">0</span></p>
    <button class="btn" onclick="calibrate(3)">Calibrate</button>
  </div>
  <div class="card">
    <h3>LOAD MODULE</h3>
    <p>Pitch: <span id="pitch4">0</span> Roll: <span id="roll4">0</span> Yaw: <span id="yaw4">0</span></p>
    <button class="btn" onclick="calibrate(4)">Calibrate</button>
  </div>
<script>
if (!!window.EventSource) {
  var source = new EventSource('/events');
  source.addEventListener('new_readings', function(e) {
    var data = JSON.parse(e.data);
    document.getElementById("pitch" + data.id).innerHTML = data.pitch.toFixed(2);
    document.getElementById("roll" + data.id).innerHTML = data.roll.toFixed(2);
    document.getElementById("yaw" + data.id).innerHTML = data.yaw.toFixed(2);
  }, false);
}
function calibrate(id) {
  fetch('/calibrate?id=' + id);
}
</script>
</body>
</html>
)rawliteral";


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected!");
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());


  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

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
}

void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping",NULL,millis());
    lastEventTime = millis();
  }
}