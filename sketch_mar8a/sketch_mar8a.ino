#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <driver/i2s.h>
#include <driver/rmt.h>
#include <rom/crc.h>

#include "M5StickCPlus.h"
#include "esp_pm.h"

#define SERVICE_UUID           "1bc68b2a-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_RX_UUID "1bc68da0-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_TX_UUID "1bc68efe-f3e3-11e9-81b4-2a2ae2dbcce4"

BLEServer *pServer   = NULL;
BLEService *pService = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected    = false;
bool oldDeviceConnected = false;
char idtest[50];

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        pTxCharacteristic->setValue(idtest);
        pTxCharacteristic->notify();
    };

    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
    }
};

uint8_t *data = new uint8_t[128];

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        data = pCharacteristic->getData();
        String test;
        test = (char *)data; 
        Serial.println(test);
    }
};

bool InitBLEServer() {
    uint64_t chipid = ESP.getEfuseMac();
    String blename  = "Aniball";

    BLEDevice::init(blename.c_str());
    // BLEDevice::setPower(ESP_PWR_LVL_N12);
    pServer = BLEDevice::createServer();

    pServer->setCallbacks(new MyServerCallbacks());
    pService          = pServer->createService(SERVICE_UUID);
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_RX_UUID, BLECharacteristic::PROPERTY_NOTIFY);

    pTxCharacteristic->addDescriptor(new BLE2902());
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_TX_UUID, BLECharacteristic::PROPERTY_WRITE);
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    return true;
}

void DisPlayBLESend() {
    uint8_t senddata[2] = {0};

    pService->start();
    pServer->getAdvertising()->start();

    //String blename  = "M5-" + String((uint32_t)(chipid >> 32), HEX);

    while ((!M5.BtnA.isPressed()) && (!M5.BtnB.isPressed())) {
        if (deviceConnected) {
            //pTxCharacteristic->setValue(idtest);
            //pTxCharacteristic->notify();
        } else {
        }

        M5.update();
        delay(100);
    }
    while ((M5.BtnA.isPressed()) || (M5.BtnB.isPressed())) {
        M5.update();
        M5.Beep.tone(4000);
        delay(10);
    }
    delay(50);
    M5.Beep.mute();
    pService->stop();
    pServer->getAdvertising()->stop();
}


void setup() {
  uint64_t chipid = ESP.getEfuseMac();
  *(int64_t *)idtest = chipid; 

  M5.begin();
  InitBLEServer();
}

void loop() {
  DisPlayBLESend();
  M5.update();
  delay(50);
}




