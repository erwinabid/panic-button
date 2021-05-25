#include <ESP8266WiFi.h>
//#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/kentaylor/WiFiManager
#include <DoubleResetDetector.h>  //https://github.com/datacute/DoubleResetDetector

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#define alarm D5
//#define buzer D6
const int PIN_LED = 2;  // D4 

#define DRD_TIMEOUT 10  // Number of seconds after reset during which a                      
#define DRD_ADDRESS 0   // RTC Memory Address for the DoubleResetDetector to use
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
   
/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "erwinabid"
#define AIO_KEY         "fc7fa68ca2344303b7fb9d2ee0e8ccd1"
/************ Global State (you don't need to change this!) ******************/
  WiFiClient client;
  Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
  Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/thb-002");
/*************************** Sketch Code ************************************/
void MQTT_connect();
bool initialConfig = false;
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(alarm,OUTPUT); 
  //pinMode(buzer,OUTPUT); 
  if (WiFi.SSID() == "") {
      Serial.println("Kami belum mendapatkan kredensial titik akses, jadi dapatkan sekarang");
     initialConfig = true;
     }
  if (drd.detectDoubleReset()) {
     Serial.println("Double Reset Detected");
     initialConfig = true;
     }
  if (initialConfig) {
     Serial.println("Mulai Konfigurasi WIFI di WEB.");
     digitalWrite(PIN_LED, HIGH); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
                                //Local intialization. Once its business is done, there is no need to keep it around
     WiFiManager wifiManager;
                     
    if (!wifiManager.startConfigPortal("MASTER 002")) {
      Serial.println("Tidak terhubung ke WiFi tetapi tetap melanjutkan.");
    } else {
                                  //jika Anda sampai di sini, Anda telah terhubung ke WiFi
      Serial.println("terhubung...WIFI :)");
    }
    digitalWrite(PIN_LED, LOW);  // Turn led off as we are not in configuration mode.
    ESP.reset();                  // Ini agak kasar. Untuk beberapa alasan yang tidak diketahui, server web hanya dapat dimulai sekali per boot up.
                                  // jadi mengatur ulang perangkat memungkinkan untuk kembali ke mode konfigurasi saat reboot
    delay(5000);
  }
  digitalWrite(PIN_LED, LOW);     // Turn led off as we are not in configuration mode.
  WiFi.mode(WIFI_STA);             //Paksa ke mode stasiun karena jika perangkat dimatikan saat berada dalam mode titik akses, perangkat akan-
                                   //memulai waktu berikutnya dalam mode titik akses.
  unsigned long startedAt = millis();
  Serial.print("After waiting ");
  int connRes = WiFi.waitForConnectResult();
  float waited = (millis() - startedAt);
  Serial.print(waited / 1000);
  Serial.print(" secs in setup() connection result is ");
  Serial.println(connRes);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("gagal terhubung WIFI, tetap menyelesaikan pengaturan");
  } else {
    Serial.print("local ip: ");Serial.println(WiFi.localIP());
  }


  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);
}

uint32_t x=0;

void loop() {
  drd.loop();  // Sebut metode loop reset ganda berulang setiap kali,
  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
      digitalWrite(alarm,HIGH);
     // digitalWrite(buzer,HIGH);
      delay(500);
      digitalWrite(alarm,LOW);
     // digitalWrite(buzer,LOW); 
    }
  }


}
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
