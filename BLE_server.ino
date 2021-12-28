#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

Servo myservo;

BLEServer *pServer; // BLE Server
BLECharacteristic *pCharacteristic; // BLE Charactieristics

bool deviceConnected = false;
String txValue = "0";
int runningFlag = -1; // -1: let motor stop working, 0: let motor start working

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define FORCE_SENSOR_PIN 36 // GIOP36 (ADC0)
#define SERVO_PIN 18
#define LED_PIN 0     // alert
#define FIRST_LED 4   // left thumb
#define SECOND_LED 2  // left 2nd metatarsal head
#define THIRD_LED 17  // left and right 3rd metatarsal head
#define FORTH_LED 5   // left 4th metatatsal head
#define FIFTH_LED 12  // right thumb
#define SIXTH_LED 14  // right 2nd metatarsal head
#define SEVENTH_LED 27  // right 4th metatarsal head
#define STATUS_LED 19 // status led
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "828917c1-ea55-4d4a-a66e-fd202cea0645"

// Class for servercallbacks (Wether the device is connected or not)
class MyServerCallbacks: public BLEServerCallbacks{
  void onConnect(BLEServer *pServer){
    deviceConnected = true;
    Serial.println("Connected");
  }

  void onDisconnect(BLEServer *pServer){
    Serial.println("Disconnectd");
    deviceConnected = false;
    pServer->getAdvertising()->start();
  }
};

// Check if there has incoming data or not
class MyCallbacks: public BLECharacteristicCallbacks{

  void onRead(BLECharacteristic *pCharacteristic){
    pCharacteristic->setValue(txValue.c_str());
    Serial.println("onRead Value: " + txValue);
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

      if(rxValue.find("0") != -1){
        runningFlag = 0;  // let motor run
      } else if(rxValue.find("1") != -1){
        Serial.println("Testing on the Left thumb");
        digitalWrite(FIRST_LED, HIGH);
      } else if(rxValue.find("2") != -1){
        Serial.println("Testing on the Left second metatarsal head");
        digitalWrite(SECOND_LED, HIGH);
      } else if(rxValue.find("3") != -1){
        Serial.println("Testing on the Left or Right third metatarsal head");
        digitalWrite(THIRD_LED, HIGH);
      } else if(rxValue.find("4") != -1){
        Serial.println("Testing on the Left forth metatarsal head");
        digitalWrite(FORTH_LED, HIGH);
      } else if(rxValue.find("5") != -1){
       Serial.println("Testing on the Right thumb");
        digitalWrite(FIFTH_LED, HIGH);
      } else if(rxValue.find("6") != -1){
        Serial.println("Testing on the Right second metatarsal head");
        digitalWrite(SIXTH_LED, HIGH);
      } else if(rxValue.find("7") != -1){
        Serial.println("Testing on the Right forth metatarsal head");
        digitalWrite(SEVENTH_LED, HIGH);
      } else if(rxValue.find("8") != -1){
        // turn off all leds
        digitalWrite(FIRST_LED, LOW);
        digitalWrite(SECOND_LED, LOW);
        digitalWrite(THIRD_LED, LOW);
        digitalWrite(FORTH_LED, LOW);
        digitalWrite(FIFTH_LED, LOW);
        digitalWrite(SIXTH_LED, LOW);
        digitalWrite(SEVENTH_LED, LOW);
      }
      Serial.println("==== END RECEIVE DATA ====");
      Serial.println();
    }
  }
};

void setup() {
  Serial.begin(115200);
  myservo.attach(SERVO_PIN);
  myservo.write(0);
  pinMode(LED_PIN, OUTPUT);
  pinMode(FIRST_LED, OUTPUT);
  pinMode(SECOND_LED, OUTPUT);
  pinMode(THIRD_LED, OUTPUT);
  pinMode(FORTH_LED, OUTPUT);
  pinMode(FIFTH_LED, OUTPUT);
  pinMode(SIXTH_LED, OUTPUT);
  pinMode(SEVENTH_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(FIRST_LED, LOW);
  digitalWrite(SECOND_LED, LOW);
  digitalWrite(THIRD_LED, LOW);
  digitalWrite(FORTH_LED, LOW);
  digitalWrite(FIFTH_LED, LOW);
  digitalWrite(SIXTH_LED, LOW);
  digitalWrite(SEVENTH_LED, LOW);
  digitalWrite(STATUS_LED, LOW);
  Serial.println("Starting BLE work!");

  // Create the BLE Device
  BLEDevice::init("Foot Nerve Tester"); // Bluetooth name

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Characteristic for Notification
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_TX,
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  // BLE2902 needed to notify
  pCharacteristic->addDescriptor(new BLE2902());

  // Characteristic for Writing and Reading data
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
  digitalWrite(STATUS_LED, HIGH);
}

void loop() {
  if(deviceConnected){
    if(runningFlag == 0){
      motor_start();
    }

    Serial.println("Running Flag: " + String(runningFlag));
  }
  delay(2000);
}

void motor_start(){

  if(txValue == "0"){
    txValue = "1";  // motor is working
  }
  notifyValue();  // notify motor status
  
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

  Serial.println("Finish");
  myservo.write(0);
  
  if(txValue == "1"){
    txValue = "0";  // the motor is ready
    runningFlag = -1;
  }
  notifyValue();
  digitalWrite(LED_PIN, LOW);
  turn_off_all_leds();
  delay(1000);
}

void turn_off_all_leds(){
  digitalWrite(FIRST_LED, LOW);
  digitalWrite(SECOND_LED, LOW);
  digitalWrite(THIRD_LED, LOW);
  digitalWrite(FORTH_LED, LOW);
  digitalWrite(FIFTH_LED, LOW);
  digitalWrite(SIXTH_LED, LOW);
  digitalWrite(SEVENTH_LED, LOW);
}

void notifyValue(){
  // Setting the value to the characteristic
  pCharacteristic->setValue(txValue.c_str());

  // Notify the connected client
  pCharacteristic->notify();
  Serial.println("Notify Value: " + txValue);
}
