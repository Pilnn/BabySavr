#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "esp_adc_cal.h"
 
#define LM35_Sensor1    35
 
int LM35_Raw_Sensor1 = 0;
float LM35_TempC_Sensor1 = 0.0;
float LM35_TempF_Sensor1 = 0.0;
float Voltage = 0.0;

BLEServer* pServer= NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks: public BLEServerCallbacks {

  void onConnect(BLEServer* pServer){
    deviceConnected = true;
  };

    void onDisconnect(BLEServer* pServer){
      deviceConnected = false;
    }
};





// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);

  BLEDevice::init("Magnet Sensor");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for a client connection to notify");

}

// the loop routine runs over and over again forever:
void loop() {
  // Read LM35_Sensor1 ADC Pin
  LM35_Raw_Sensor1 = analogRead(LM35_Sensor1);
  // Calibrate ADC & Get Voltage (in mV)
  Voltage = readADC_Cal(LM35_Raw_Sensor1);
  Serial.println(Voltage);
  // TempC = Voltage(mV) / 10
  LM35_TempC_Sensor1 = Voltage / 10;
  LM35_TempF_Sensor1 = (LM35_TempC_Sensor1 * 1.8) + 32;
 
  // Print The Readings
  Serial.print("Temperature = ");
  Serial.print(LM35_TempC_Sensor1);
  Serial.print(" °C , ");
  Serial.print("Temperature = ");
  Serial.print(LM35_TempF_Sensor1);
  Serial.println(" °F");

  delay(100);
//
 
  if(deviceConnected){
    // pCharacteristic->setValue((uint8_t*)&LM35_TempF_Sensor1,4);
    uint8_t temp_to_send = (uint8_t)LM35_TempF_Sensor1; // Assuming temperature fits in a byte
    pCharacteristic->setValue(&temp_to_send, sizeof(temp_to_send));

    pCharacteristic->notify();
    
    delay(3);
  }
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;    
  }

  if (deviceConnected && !oldDeviceConnected){
    oldDeviceConnected = deviceConnected;
  }
}

uint8_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
