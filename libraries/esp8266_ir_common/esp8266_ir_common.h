#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>

ESP8266WiFiMulti wifiMulti;

void try_connect_real(){
    while(wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Not connected to wifi");
      delay(500);
    }
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.localIP());

    while(!MDNS.begin(HOSTNAME)){
      Serial.println("MDNS failed");
      delay(500);
    }
    Serial.println("MDNS ok");
    
    #ifdef SERVER
    MDNS.addService("esp", "tcp", 80);
    #endif
}

void try_connect(){
    if(WiFi.status() == WL_CONNECTED)
        return;
    
    try_connect_real();
}

void init_wifi(){
  // Connect WiFi
  Serial.println("Connecting to WiFi");
  
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setPhyMode(WIFI_PHY_MODE_11B); // connects faster
  //WiFi.setOutputPower(20.5);

  // Connect WiFi
  wifiMulti.addAP(YOUR_SSID, YOUR_PASSWORD);

  try_connect_real();
}
