#include <Arduino.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>

#define HOSTNAME "ir-tx0"
#define SERVER
#include <esp8266_ir_common.h>

#define IR_TX_PIN 4 // D2 in NodeMCU v3
#define SEQ_END_MAGIC 0xFF

#define HEARTBEAT_SEND_INTERVAL_MS 2500
#define HEARTBEAT_TIMEOUT_MS 5000
#define RECV_TIMEOUT_MS 5000

IRsend irsend(IR_TX_PIN);
uint16_t rawData[2048];

WiFiClient client;
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  init_wifi();
  
  server.begin();
  irsend.begin();
}

unsigned long lastClientHeartbeatMs;
unsigned long lastServerHeartbeatMs;
void loop() {
  // put your main code here, to run repeatedly:
  try_connect();
  MDNS.update();
  yield();

  // check if client is still alive
  if(client.connected() && millis() - lastClientHeartbeatMs >= HEARTBEAT_TIMEOUT_MS){    
    Serial.println("No heartbeat received");
    client.stop();
  }
  yield();

  // try getting a connection
  if(!client.connected()){
    client = server.available();
    if(client){
      Serial.println("Client connected");
      client.setNoDelay(true);
      client.setTimeout(500);

      lastServerHeartbeatMs = millis();
      lastClientHeartbeatMs = millis();
    }
  }
  yield();

  // send heartbeat to client
  if(client.connected() && millis() - lastServerHeartbeatMs >= HEARTBEAT_SEND_INTERVAL_MS){    
    client.write(0xFF);
    Serial.println("Heartbeat sent");
    lastServerHeartbeatMs = millis();
  }
  yield();

  // handle data and heartbeat from client
  if(client.connected() && client.available()){
    int i = 0;
    
    unsigned long lastDownloadMs = millis();
    while(millis() - lastDownloadMs < RECV_TIMEOUT_MS){
        if(client.available() >= 2){
          uint16_t a = client.read();
          uint16_t b = client.read();

          lastClientHeartbeatMs = millis();
          lastDownloadMs = millis();

          if(a == SEQ_END_MAGIC && b == SEQ_END_MAGIC) break;
        
          rawData[i++] = b * 256 + a; // little endian
          
          if(i >= 2048) break;
        }
        yield();
    }

    if(i > 0){
      Serial.println("Received " + String(i * 2) + " bytes of raw data");
      irsend.sendRaw(rawData, i, 38);
    } else{
      Serial.println("Heartbeat received");
    }
  }
}
