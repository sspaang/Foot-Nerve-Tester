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
int runningFlag = -1; // -1: let motor to stop running, 0: let motor to run

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define FORCE_SENSOR_PIN 36 // GIOP36 (ADC0)
#define SERVO_PIN 18
#define LED_PIN 0  // alert
#define THUMB_LED 4  // thumb
#define SECOND_LED 2  // 2nd metatarsal head
#define THIRD_LED 17  // 3rd metatarsal head
#define FORTH_LED 5  // 4th metatatsal head
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
        runningFlag = 1;  // turn on LED to let user know where to lay the foot
      } else if(rxValue.find("2") != -1){
        runningFlag = 2;  // turn on LED to let user know where to lay the foot
      } else if(rxValue.find("3") != -1){
        runningFlag = 3;  // turn on LED to let user know where to lay the foot
      } else if(rxValue.find("4") != -1){
        runningFlag = 4;  // turn on LED to let user know where to lay the foot
      } else if(rxValue.find("5") != -1){
        runningFlag = -1;
      }
      Serial.println("RunningFlag = " + String(runningFlag));
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
  pinMode(THUMB_LED, OUTPUT);
  pinMode(SECOND_LED, OUTPUT);
  pinMode(THIRD_LED, OUTPUT);
  pinMode(FORTH_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(THUMB_LED, LOW);
  digitalWrite(SECOND_LED, LOW);
  digitalWrite(THIRD_LED, LOW);
  digitalWrite(FORTH_LED, LOW);
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
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_READ
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue(txValue.c_str()); // inform client that motor is ready
  
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
    } else if(runningFlag == 1){
      turn_off_all_leds();
      Serial.println("Testing on the Thumb");
      digitalWrite(THUMB_LED, HIGH);
      runningFlag = -1;
    } else if(runningFlag == 2){
      turn_off_all_leds();
      Serial.println("Testing on the Second metatarsal head");
      digitalWrite(SECOND_LED, HIGH);
      runningFlag = -1;
    } else if(runningFlag == 3){
      turn_off_all_leds();
      Serial.println("Testing on the Third metatarsal head");
      digitalWrite(THIRD_LED, HIGH);
      runningFlag = -1;
    } else if(runningFlag == 4){
      turn_off_all_leds();
      Serial.println("Testing on the Forth metatarsal head");
      digitalWrite(FORTH_LED, HIGH);
      runningFlag = -1;
    } else if(runningFlag == -1){
      turn_off_all_leds();
    }
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
  digitalWrite(THUMB_LED, LOW);
  digitalWrite(SECOND_LED, LOW);
  digitalWrite(THIRD_LED, LOW);
  digitalWrite(FORTH_LED, LOW);
}

void notifyValue(){
  // Setting the value to the characteristic
  pCharacteristic->setValue(txValue.c_str());

  // Notify the connected client
  pCharacteristic->notify();
  Serial.println("Notify Value: " + txValue);
}
