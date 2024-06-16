#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
//#include <SPI.h>
//#include "DFRobot_MAX17043.h" // https://github.com/DFRobot/DFRobot_MAX17043
//#include "Wire.h"
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLECharacteristic* pRxCharacteristic = NULL;
//CONFIG
#define bleAddress "1A7EEADFF188" // CONFIGURATION: < Use a real Lovense Edge BLE address here or use a Genertate one using a MAC address generator
#define Device_Type            "P:37:1A7EEADFF188" // < Toy Type:FW Version:bleAdress
#define Batch                  "220123" //Production Date, yymmdd, currently Set to my birthday day this year
//pins
#define LED  LED_BUILTIN
#define IN1   5  
#define IN3   6

//DFRobot_MAX17043 gauge;

// Don't touch these unless you know what you a doing
#define btname                 "LVS-Edge36"  
#define SERVICE_UUID           "50300001-0023-4bd4-bbd5-a6920e4c5653"
#define CHARACTERISTIC_RX_UUID "50300002-0023-4bd4-bbd5-a6920e4c5653"
#define CHARACTERISTIC_TX_UUID "50300003-0023-4bd4-bbd5-a6920e4c5653"
//END CONFIG

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
int vibration;
void V (int M)
{
    int Speed = round(12.75*vibration);
    if (M == 1) {
      ledcWrite(IN1,Speed);
    } else if (M == 2){
      ledcWrite(IN3,Speed);
    } else {
      ledcWrite(IN1,Speed);
      ledcWrite(IN3,Speed);
    }
}
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};
class MySerialCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      static uint8_t messageBuf[64];
      assert(pCharacteristic == pRxCharacteristic);
      std::string rxValue(pRxCharacteristic->getValue().c_str());
      
      //if (rxValue.length() > 0) {
        //Serial.println("*********");
        //Serial.print("Received Value: ");
        //for (int i = 0; i < rxValue.length(); i++)
          //Serial.print(rxValue[i]);

        //Serial.println();
        //Serial.println("*********");
      //}
      if (rxValue == "DeviceType;") { 
        memmove(messageBuf, Device_Type, 18);
        pTxCharacteristic->setValue(messageBuf, 18);
        pTxCharacteristic->notify();
      } else if (rxValue == "Battery;") {
        char bat[3];
        #ifdef BATT_GUAGE
          dtostrf(gauge.readPercentage(), 2,0, bat);
        #else
          bat[0] = '9';
          bat[1] = '9';        
        #endif
        bat[2] = ';';
        memmove(messageBuf, bat, 3);
        pTxCharacteristic->setValue(messageBuf, 3);
        pTxCharacteristic->notify();
      } else if (rxValue == "PowerOff;") {
        // Actually Doing this will require making a custom PCB
        memmove(messageBuf, "OK;", 3);
        pTxCharacteristic->setValue(messageBuf, 3);
        pTxCharacteristic->notify();
      } else if (rxValue == "GetBatch;") {
        memmove(messageBuf, Batch, 3);
        pTxCharacteristic->setValue(messageBuf, 3);
        pTxCharacteristic->notify();
      } else if (rxValue.rfind("Status:", 0) == 0) {
        memmove(messageBuf, "2;", 2);
        pTxCharacteristic->setValue(messageBuf, 3);
        pTxCharacteristic->notify();
      } else if (rxValue.rfind("Vibrate:",0) == 0) {
        memmove(messageBuf, "OK;", 3);
        pTxCharacteristic->setValue(messageBuf, 3);
        pTxCharacteristic->notify();
        vibration = std::atoi(rxValue.substr(8).c_str());
        V(3);
      }  else if (rxValue.rfind("Vibrate1:",0) == 0) {
        memmove(messageBuf, "OK;", 3);
        pTxCharacteristic->setValue(messageBuf, 3);
        pTxCharacteristic->notify();
        vibration = std::atoi(rxValue.substr(9).c_str());
        V(1);
      }  else if (rxValue.rfind("Vibrate2:",0) == 0) {
        memmove(messageBuf, "OK;", 3);
        pTxCharacteristic->setValue(messageBuf, 3);
        pTxCharacteristic->notify();
        vibration = std::atoi(rxValue.substr(9).c_str());
        V(2);
      }else {
        memmove(messageBuf, "ERR;", 4);
        pTxCharacteristic->setValue(messageBuf, 4);
        pTxCharacteristic->notify();
      }
    }
};

void setup() {
  //Serial.begin(115200);

    //SPI.begin();
    //while(gauge.begin() != 0) {
      //Serial.println("gauge begin faild!");
    //  delay(2000);
    //}
    //delay(2);
    //Serial.println("gauge begin successful!");
  ledcAttach(IN1,1000,8);
  ledcAttach(IN3,1000,8);
  //ledcSetup(1, 1000, 8);
  //ledcSetup(2, 1000, 8);
  //ledcAttachPin(MA_PWM, 1);
  //ledcAttachPin(MB_PWM, 2);
  pinMode(LED,OUTPUT);
  BLEDevice::init(btname);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_TX_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pTxCharacteristic->addDescriptor(new BLE2902());
  pRxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_RX_UUID,
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_WRITE_NR
                    );
  pRxCharacteristic->setCallbacks(new MySerialCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  //Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (!deviceConnected){
    digitalWrite(LED,HIGH);
    delay(1000);
    digitalWrite(LED,LOW);
    delay(1000);
  } 
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    digitalWrite(LED,HIGH);
  }
  if (!deviceConnected && oldDeviceConnected) {
    delay(100); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    //Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
    digitalWrite(LED,LOW);
  }
}
