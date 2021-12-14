#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

Servo myservo;

// BLE charactieristics
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
int txValue = 0;
char txString[2];

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define FORCE_SENSOR_PIN 36 // GIOP36 (ADC0)
#define SERVO_PIN 18
#define LED_PIN 0  // alert
#define THUMB_LED 4  // thumb
#define SECOND_LED 16  // 2nd metatarsal head
#define THIRD_LED 17  // 3rd metatarsal head
#define FORTH_LED 5  // 4th metatatsal head
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "828917c1-ea55-4d4a-a66e-fd202cea0645"

// Class for servercallbacks (Wether the device is connected or not)
class MyServerCallbacks: public BLEServerCallbacks{
  void onConnect(BLEServer *pServer){
    deviceConnected = true;
  }

  void onDisconnect(BLEServer *pServer){
    deviceConnected = false;
  }
};

// Check if there has incoming data or not
class MyCallbacks: public BLECharacteristicCallbacks{
  
  void motor_start(int LED_POINTER){
    digitalWrite(LED_POINTER, HIGH);
    
    if(txValue == 0){
      txValue = 1;  // motor is working
    }
    
    for(int pos = 0; pos <= 90; pos+=2)
    { 
      int forceSensorValue = analogRead(FORCE_SENSOR_PIN);
  
      Serial.print("Force sensor value: ");
      Serial.print(forceSensorValue);
      Serial.print("\t");
      Serial.print("Servo position: ");
      Serial.print(pos);
      Serial.print("\t");
      
      myservo.write(pos);
      
      if(forceSensorValue > 160)
      {
        Serial.println("LED ON");
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Hold for 1.5 sec");
        delay(1500);
        break;
      } else {
        Serial.println("LED OFF");
        digitalWrite(LED_PIN, LOW);
      }
      delay(200);
    }
  
    Serial.println("#SEROFF");
    myservo.write(0);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    digitalWrite(LED_POINTER, LOW);
    delay(2000);
  }
  
  void turn_off_all_leds(){
    digitalWrite(THUMB_LED, LOW);
    digitalWrite(SECOND_LED, LOW);
    digitalWrite(THIRD_LED, LOW);
    digitalWrite(FORTH_LED, LOW);
  }
  
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string rxValue = pCharacteristic->getValue();

    if(rxValue.length() > 0){
      Serial.println("==== RECEIVE DATA ====");
      Serial.print("Received Value: ");

      for(int i = 0; i < rxValue.length(); i++){
        Serial.print(rxValue[i]);
      }

      Serial.println();
      
      turn_off_all_leds();
      digitalWrite(THUMB_LED, HIGH);
      if(rxValue.find("1") != -1){
        turn_off_all_leds();
        Serial.println("Testing on the Thumb");
        motor_start(THUMB_LED);
        digitalWrite(SECOND_LED, HIGH);
      } else if(rxValue.find("2") != -1){
        turn_off_all_leds();
        Serial.println("Testing on the Second metatarsal head");
        motor_start(SECOND_LED);
        digitalWrite(THIRD_LED, HIGH);
      } else if(rxValue.find("3") != -1){
        turn_off_all_leds();
        Serial.println("Testing on the Third metatarsal head");
        motor_start(THIRD_LED);
        digitalWrite(FORTH_LED, HIGH);
      } else if(rxValue.find("4") != -1){
        turn_off_all_leds();
        Serial.println("Testing on the Forth metatarsal head");
        motor_start(FORTH_LED);
        turn_off_all_leds();
      }

      Serial.println("==== END RECEIVE DATA ====");
      Serial.println();

      if(txValue == 1){
        txValue = 0;  // the motor is ready
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  myservo.attach(SERVO_PIN);
  myservo.write(0);
  pinMode(LED_PIN, OUTPUT);
  pinMode(THUMB_LED, OUTPUT);
  pinMode(SECOND_LED, OUTPUT);
  pinMode(THIRD_LED, OUTPUT);
  pinMode(FORTH_LED, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(THUMB_LED, LOW);
  digitalWrite(SECOND_LED, LOW);
  digitalWrite(THIRD_LED, LOW);
  digitalWrite(FORTH_LED, LOW);
  Serial.println("Starting BLE work!");

  // Create the BLE Device
  BLEDevice::init("Foot Nerve Tester"); // Bluetooth name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_TX,
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  // BLE2902 needed to notify
  pCharacteristic->addDescriptor(new BLE2902());

  // Characteristic for the receiving end
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  
  // Start the service
  pService->start();
  
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);

  // Start adverising (showing BLE name to connect to)
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");
  digitalWrite(THUMB_LED, HIGH);
}

void loop() {
  if(deviceConnected){
    // Conversion of txValue
    itoa(txValue, txString, 10);
  
    // Setting the value to the characteristic
    pCharacteristic->setValue(txString);
  
    // Notify the connected client
    pCharacteristic->notify();
    Serial.println("Sent value: " + String(txString));
    delay(7000);
  }
}
