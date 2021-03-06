#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8x8lib.h>
#include "cipher.h"

#define SENDDELAY 8000
#define CLIENTID "LORACLIENT01" //define your client name

#define ENCRYPTIONKEY "yvRBAECu22YbdNpO" //define your encryption key
#define SS 18
#define IO0 26

#define SDA 21
#define SCL 22
#define RST 23

#define SCK 5
#define MISO 19
#define MOSI 27

#define DELIMITER ";"

String clientid = CLIENTID; 
long sendcount=0;

U8X8_SSD1306_128X64_NONAME_SW_I2C displaydevice(SCL, SDA, RST); 

Cipher * cipher = new Cipher();

void sendPacket() 
{
  String message="Hello World!";
  String packet=clientid + DELIMITER +  message + DELIMITER;

  String encryptedPacket = cipher->encryptString(packet);
    
  LoRa.beginPacket();
  LoRa.print(encryptedPacket);
  LoRa.endPacket();

  sendcount++;

  displaydevice.clearDisplay();
  displaydevice.setCursor(0,1);
  displaydevice.print("TX encrypted");
  displaydevice.setCursor(0,3);
  displaydevice.print(message);
  displaydevice.setCursor(0,5);
  displaydevice.print("Message count:");
  displaydevice.setCursor(0,7);
  displaydevice.print(sendcount);
}

void setup() {
 Serial.begin(9600);
  //OLED INIT
  displaydevice.begin();
  displaydevice.setFont(u8x8_font_pressstart2p_r);
  displaydevice.clearDisplay();
  displaydevice.drawString(0, 1, "LoRa IoT Client");
  displaydevice.drawString(0, 3, clientid.c_str());
  //OLED INIT
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
  sendPacket();  
  delay(SENDDELAY);

}
