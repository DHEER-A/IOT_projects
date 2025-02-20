
#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>

// Replace with your network credentials (STATION)
const char* ssid = "Galaxy F62";
const char* password = "12345678910abcd";

float Pitch_1 = 0.0, Pitch_2 = 0.0, Pitch_3 = 0.0;
float Roll_1 = 0.0, Roll_2 = 0.0, Roll_3 = 0.0;
float Yaw_1 = 0.0, Yaw_2 = 0.0, Yaw_3 = 0.0;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  float Pitch;
  float Roll;
  float Yaw;
} struct_message;

struct_message incomingReadings;

JSONVar board;

AsyncWebServer server(80);
AsyncEventSource events("/events");

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  
  board["id"] = incomingReadings.id;
  board["Pitch"] = incomingReadings.Pitch;
  board["Roll"] = incomingReadings.Roll;
  board["Yaw"] = incomingReadings.Yaw;
  /*float rollAngle = atan2(incomingReadings.y, incomingReadings.z) * 180.0 / PI;
  board["roll"] = rollAngle;*/
  if (incomingReadings.id == 1)
  {
    Pitch_1 = incomingReadings.Pitch;
    Roll_1 = incomingReadings.Roll;
    Yaw_1 = incomingReadings.Yaw;
  }
  else if (incomingReadings.id == 2)
  {
    Pitch_2 = incomingReadings.Pitch;
    Roll_2 = incomingReadings.Roll;
    Yaw_2 = incomingReadings.Yaw;
  }
  else if (incomingReadings.id == 3)
  {
    Pitch_3 = incomingReadings.Pitch;
    Roll_3 = incomingReadings.Roll;
    Yaw_3 = incomingReadings.Yaw;
  }


  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
  
  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf("y value: %4.2f \n", incomingReadings.Pitch);
  Serial.printf("z value: %4.2f \n", incomingReadings.Roll);
  Serial.printf("Roll angle: %4.2f degrees\n", incomingReadings.Yaw);

  Serial.println();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p { font-size: 1.2rem;}
    body { margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 900px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.0rem; }
    .packet { color: #bebebe; }
    .card.roll { color: #fd7e14; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
      
      
      <!-- Board 0 -->
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 0 - PITCH</h4>
        <p><span class="reading"><span id="b2_pitch"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 0 - ROLL</h4>
        <p><span class="reading"><span id="b2_roll"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 0 - YAW</h4>
        <p><span class="reading"><span id="b2_yaw"></span> &deg;</span></p>
      </div>
      
      
      
      <!-- Board 1 -->
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 1 - PITCH</h4>
        <p><span class="reading"><span id="b1_pitch"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 1 - ROLL</h4>
        <p><span class="reading"><span id="b1_roll"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 1 - YAW</h4>
        <p><span class="reading"><span id="b1_yaw"></span> &deg;</span></p>
      </div>



      <!-- Board 2 -->
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 2 - PITCH</h4>
        <p><span class="reading"><span id="b2_pitch"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 2 - ROLL</h4>
        <p><span class="reading"><span id="b2_roll"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 2 - YAW</h4>
        <p><span class="reading"><span id="b2_yaw"></span> &deg;</span></p>
      </div>



      <!-- Board 3 -->
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 3 - PITCH</h4>
        <p><span class="reading"><span id="b3_pitch"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 3 - ROLL</h4>
        <p><span class="reading"><span id="b3_roll"></span> &deg;</span></p>
      </div>
      <div class="card roll">
        <h4><i class="fas fa-ruler"></i> BOARD 3 - YAW</h4>
        <p><span class="reading"><span id="b3_yaw"></span> &deg;</span></p>
      </div>


    </div>

  </div>

<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("b" + obj.id + "_pitch").innerHTML = obj.pitch.toFixed(2);
  document.getElementById("b" + obj.id + "_roll").innerHTML = obj.roll.toFixed(2);
  document.getElementById("b" + obj.id + "_yaw").innerHTML = obj.yaw.toFixed(2);
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
   
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    //client->send("hello!", NULL, millis(), 10000);
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
