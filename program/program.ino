
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <string.h>
#include <time.h>
#include <M5StickCPlus.h>

#include <time.h>
#include <stdlib.h>



//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

//include for bluetooth
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <driver/i2s.h>
#include <driver/rmt.h>
#include <rom/crc.h>
#include "esp_pm.h"

// Insert your network credentials
#define WIFI_SSID "Redmi Note 10S"
#define WIFI_PASSWORD "yoannclochard"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAn6G1EesNzTpYfjF7YPJvJK-9XQrJiL2M"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://iotproject-e9405-default-rtdb.europe-west1.firebasedatabase.app/" 

//UID for bluetooth
#define SERVICE_UUID           "1bc68b2a-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_RX_UUID "1bc68da0-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_TX_UUID "1bc68efe-f3e3-11e9-81b4-2a2ae2dbcce4"

//bluetooth variable
BLEServer *pServer   = NULL;
BLEService *pService = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected    = false;

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
String macAddress = ""; 

//Gyro 
float gyroX = 0;
float gyroY = 0;
float gyroZ = 0;

//heures 
const char* ntpServer = "fr.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

//comparaison heure
struct tm timeStart;
time_t lastPlayed;
bool playing = false;

void setup(){
  Serial.begin(115200);

  setupFirebase();
//gyro
  M5.begin();
  M5.Imu.Init(); 

  //heure 
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 

  srand(time(NULL));   // Initialization, should only be called once.


}

void loop(){

  M5.Imu.getGyroData(&gyroX, &gyroY, &gyroZ);

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    if ((gyroX + gyroY + gyroZ)/3 > 10) {
      struct tm timeinfo;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
      }
      
      if(!playing) {
        //Firebase.RTDB.setString(&fbdo, macAddress + "/test/heure", (String) (timeinfo.tm_year + 1900) + ":" + (String) (timeinfo.tm_mon + 1)
        //+ ":" + (String) timeinfo.tm_mday + ":" + (String) timeinfo.tm_hour + ":" + (String) timeinfo.tm_min + ":" + (String) timeinfo.tm_sec );
        timeStart = timeinfo;
        playing = true;
      }

      lastPlayed = mktime(&timeinfo);

      M5.Lcd.print("Moving");
    } else {
      if(playing) {
        struct tm timestill;
        if(!getLocalTime(&timestill)){
          Serial.println("Failed to obtain time");
          return;
        }
        time_t currenttime;
        currenttime = mktime(&timestill);

        double diff;
        diff = difftime(currenttime, lastPlayed);

        if(diff > 180) {
          int r = rand();  

          Firebase.RTDB.setString(&fbdo, macAddress + "/test/" + r + "/heure", (String) (timeStart.tm_year + 1900) + ":" + (String) (timeStart.tm_mon + 1)
          + ":" + (String) timeStart.tm_mday + ":" + (String) timeStart.tm_hour + ":" + (String) timeStart.tm_min + ":" + (String) timeStart.tm_sec );

          Firebase.RTDB.setString(&fbdo, macAddress + "/test/" + r + "/heureFin", (String) (timestill.tm_year + 1900) + ":" + (String) (timestill.tm_mon + 1)
          + ":" + (String) timestill.tm_mday + ":" + (String) timestill.tm_hour + ":" + (String) timestill.tm_min + ":" + (String) timestill.tm_sec );

          playing = false;
        }

      }
  
      M5.Lcd.print("Still");
    }
  }
}

void setupFirebase () {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  auth.user.email = "esp32@esp32.fr";
  auth.user.password = "esp32.fr";
  Serial.println("auth config");

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  macAddress = WiFi.macAddress();
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
