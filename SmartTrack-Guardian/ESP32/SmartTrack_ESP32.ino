#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4A5B0001-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_TX   "4A5B0002-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_RX   "4A5B0003-0000-1000-8000-00805F9B34FB"

#define BUZZER_PIN 25
#define LED_PIN 2
#define WATER_PIN 34
#define WATER_THRESHOLD 500

BLECharacteristic *txCharacteristic;
bool deviceConnected = false;
bool ringing = false;
bool waterDetected = false;
unsigned long lastStatusTime = 0;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    deviceConnected = true;
    Serial.println("Phone connected");
  }

  void onDisconnect(BLEServer *server) override {
    deviceConnected = false;
    Serial.println("Phone disconnected");
    BLEDevice::startAdvertising();
  }
};

class RXCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    std::string raw = characteristic->getValue();
    String command = String(raw.c_str());
    command.trim();

    if (command == "RING_ON") {
      ringing = true;
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      Serial.println("RING_ON received");
    } else if (command == "RING_OFF") {
      ringing = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      Serial.println("RING_OFF received");
    }
  }
};

void sendStatus(int waterValue) {
  waterDetected = waterValue > WATER_THRESHOLD;

  String status = "{\"device\":\"SmartTrack_Wallet\",\"status\":\"";
  status += deviceConnected ? "CONNECTED" : "DISCONNECTED";
  status += "\",\"ringing\":";
  status += ringing ? "true" : "false";
  status += ",\"water\":";
  status += waterDetected ? "true" : "false";
  status += ",\"waterValue\":" + String(waterValue);
  status += ",\"battery\":null}";

  txCharacteristic->setValue(status.c_str());
  if (deviceConnected) txCharacteristic->notify();
  Serial.println(status);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init("SmartTrack_Wallet");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(SERVICE_UUID);
  txCharacteristic = service->createCharacteristic(
    CHARACTERISTIC_TX,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  txCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *rxCharacteristic = service->createCharacteristic(
    CHARACTERISTIC_RX,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  rxCharacteristic->setCallbacks(new RXCallbacks());

  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("SmartTrack Guardian BLE started");
}

void loop() {
  int waterValue = analogRead(WATER_PIN);
  waterDetected = waterValue > WATER_THRESHOLD;

  if (millis() - lastStatusTime >= 5000) {
    lastStatusTime = millis();
    sendStatus(waterValue);
  }
  delay(50);
}
