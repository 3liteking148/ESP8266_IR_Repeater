#include <Arduino.h>
#include <assert.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#define HOSTNAME "ir-rx0"
#include <esp8266_ir_common.h>

#define IR_RX_PIN 4 // D2 in NodeMCU v3
#define IR_BUFFER_SIZE 1024
#define IR_TIMEOUT 100

#define HEARTBEAT_SEND_INTERVAL_MS 2500
#define HEARTBEAT_TIMEOUT_MS 5000

IRrecv irrecv(IR_RX_PIN, IR_BUFFER_SIZE, IR_TIMEOUT, true);
decode_results results;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  init_wifi();
  
  assert(irutils::lowLevelSanityCheck() == 0);
  irrecv.enableIRIn();
}

unsigned long lastClientHeartbeatMs;
unsigned long lastServerHeartbeatMs;
uint8_t terminator[] = {0xFF, 0xFF};
void loop() {
  // put your main code here, to run repeatedly:
  try_connect();
  MDNS.update();
  yield();
  
  // check if server is still alive
  if(client.connected() && millis() - lastServerHeartbeatMs >= HEARTBEAT_TIMEOUT_MS){
    Serial.println("No heartbeat received");
    client.stop();
  }
  yield();

  // try getting a connection
  IPAddress ipaddr = IPADDR_NONE;
  if(!client.connected()){
    int n = MDNS.queryService("esp", "tcp");  // Send out query for esp tcp services
    
    if (n >= 1) { //todo
       ipaddr = MDNS.IP(0);
    }
    
    client.connect(ipaddr, 80);
    client.setNoDelay(true);
    client.setTimeout(500);

    Serial.print("Connected to IP: ");
    Serial.println(ipaddr);
    
    lastServerHeartbeatMs = millis();
    lastClientHeartbeatMs = millis();
  }
  yield();

  // send either data or a heartbeat to the server
  if (irrecv.decode(&results)) {
    uint16_t* rawData = resultToRawArray(&results);
    uint16_t rawSize = getCorrectedRawLength(&results);
    
    int bytesToWrite = rawSize * sizeof(uint16_t) + 2;
    int bytesWritten = 0;
    
    if(client.connected()){
      // 0xFFFF is the terminator, remove any instances of it from the IR sequences
      for(int i=0; i<rawSize; i++){
        if(rawData[i] == 0xFFFF)
          rawData[i] = 0xFFFE; 
      }

      bytesWritten += client.write((uint8_t*)rawData, rawSize * sizeof(uint16_t));
      bytesWritten += client.write(terminator, sizeof(terminator));
      
      if(bytesWritten == bytesToWrite){
        Serial.println("Heartbeat sent");
        lastClientHeartbeatMs = millis();
      } else {
        Serial.println("Data transmission failed");
        client.stop();
      }
    }
    Serial.println("Wrote " + String(bytesWritten) + "/" +  String(bytesToWrite) + " bytes");
    Serial.println("Free heap: " + String(ESP.getFreeHeap()));
    free(rawData);
  } else if(client.connected() && millis() - lastClientHeartbeatMs > HEARTBEAT_SEND_INTERVAL_MS){ 
    // send heartbeat to server instead if there is no data
    client.write(terminator, sizeof(terminator));
    Serial.println("Heartbeat sent");
    lastClientHeartbeatMs = millis();
  }
  yied();
  
  // handle heartbeat from server
  if(client.available()){
    Serial.println("Heartbeat received");
    while(client.available()) client.read();
    lastServerHeartbeatMs = millis();
  }
}
