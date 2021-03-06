#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <LoRa.h>
#include <U8x8lib.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"
#include <ArduinoJson.h>
#include "Cipher.h"

#define SS 18
#define RST 23
#define IO0 26

#define SDA 21
#define SCL 22

#define SCK 5
#define MISO 19
#define MOSI 27

String gatewayid = "LORAGATE01";           

U8X8_SSD1306_128X64_NONAME_SW_I2C displaydevice(SCL, SDA, RST); 

WiFiClient mainESP;
PubSubClient MQTT(mainESP);

Cipher * cipher = new Cipher();

String delimeter = ";";
String RxString;

int RxRSSI; boolean rx_fp;
long lastrx,crontimer,rxcount=0;

String StringSplit(String s, char parser, int index) {
  String rs="";
  int parserIndex = index;
  int parserCnt=0;
  int rFromIndex=0, rToIndex=-1;
  while (index >= parserCnt) {
    rFromIndex = rToIndex+1;
    rToIndex = s.indexOf(parser,rFromIndex);
    if (index == parserCnt) {
      if (rToIndex == 0 || rToIndex == -1) return "";
      return s.substring(rFromIndex,rToIndex);
    } else parserCnt++;
  }
  return rs;
}

void get_lora_data() {
  boolean clr = 1;
  rx_fp = true;
  while (LoRa.available()) {
    if (clr == 1) {
      RxString = "";
      clr = 0;
    }
    RxString += (char)LoRa.read();
  }

  //Decrypt Message
  RxString=cipher->decryptString(RxString);
  
  RxRSSI = LoRa.packetRssi();
  Serial.print("Received Packet: ");
  Serial.println(RxString);
  lastrx = millis() / 1000;
  clr = 1;
}

void display_status() {
  displaydevice.clearDisplay();
  long lastdata = ((millis() / 1000) - lastrx);
  displaydevice.setCursor(0, 1);
  displaydevice.print("Lastrx: ");
  if(rx_fp == true){

  rxcount++;
    
  displaydevice.print(lastdata);
  displaydevice.print("s");
  displaydevice.setCursor(0, 3);

  String clientId = StringSplit(RxString,';',0);
  String message = StringSplit(RxString,';',1);

  displaydevice.print("S:");
  displaydevice.print(clientId);
  displaydevice.setCursor(0, 5);
  displaydevice.print("M:");
  displaydevice.print(message);

  displaydevice.setCursor(0,7);
  displaydevice.print("RX: ");
  displaydevice.print(rxcount);
  }
  else
  {
    displaydevice.print("none");  
    displaydevice.setCursor(0, 5);
    displaydevice.print("RSSI: ");
    displaydevice.print(RxRSSI);
    displaydevice.print(" dBm");
  }
}
void startWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }
  WiFi.setHostname(mqtt_name);
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  MQTT.setServer(mqtt_server, 1883);
}

void reconnect() {
  while (!MQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (MQTT.connect(mqtt_name)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(MQTT.state());
      Serial.println(" try again in 5 seconds");
      for (int i = 0; i < 5000; i++) {
        delay(1);
      }
    }
  }
}

void setup() {
  Serial.begin(9600);

  displaydevice.begin();
  displaydevice.setFont(u8x8_font_pressstart2p_r);
  displaydevice.clearDisplay();
  displaydevice.drawString(0, 1, "LoRa IoT Gateway");
 
  Serial.println("Starting LoRa...");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, IO0);
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa started");

  cipher->setKey(ENCRYPTIONKEY);
 
}

void loop() {

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("Receive LoRa Packet...");
    get_lora_data();

    char buffer[100];
  
    if (WiFi.status() != WL_CONNECTED) startWiFi();

    if (!MQTT.connected()) reconnect();

     MQTT.publish(mqtt_topic, RxString.c_str());    
  }


  if (millis() / 1000 > crontimer + 5) {
    crontimer = millis() / 1000;
    display_status();
  }

  if (millis() / 1000 > 90000) {
    ESP.restart();
  }
 
}
