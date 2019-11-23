#ifdef USES_P103
#ifdef ESP32
//#######################################################################################################
//######################## Plugin 103: Temperature and Humidity sensor BLE Mijia ########################
//#######################################################################################################
//
// Based on https://github.com/dtony/Mijia-ESP32-Bridge/blob/master/src/main.cpp
//
#define PLUGIN_103
#define PLUGIN_ID_103         103
#define PLUGIN_NAME_103       "Environment - BLE Mijia"
#define PLUGIN_VALUENAME1_103 "Temperature"
#define PLUGIN_VALUENAME2_103 "Humidity"
#define PLUGIN_VALUENAME3_103 "Battery"

#include "BLEDevice.h"
#include "BLERemoteCharacteristic.h"
#include <esp_gap_ble_api.h>

#define MIJIA_DATA_SERVICE    "226c0000-6476-4566-7562-66734470666d"
#define MIJIA_DATA_CHAR       "226caa55-6476-4566-7562-66734470666d"
#define MIJIA_BATTERY_SERVICE "0000180f-0000-1000-8000-00805f9b34fb"
#define MIJIA_BATTERY_CHAR    "00002a19-0000-1000-8000-00805f9b34fb"

#ifndef BLE_ESP32_MULTI
#define BLE_ESP32_MULTI
BLEClient* bleclients[TASKS_MAX];
#endif

#ifndef BLE_NOTIFY_TASK
#define BLE_NOTIFY_TASK
static int ble_notify_task = -1;
#endif

//static void p103_notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
static void p103_notifyCallback(
  void* locBLERemoteCharacteristic,
  unsigned char* tData,
  unsigned int plength,
  bool pisNotify) {

  BLERemoteCharacteristic* pBLERemoteCharacteristic = (BLERemoteCharacteristic*)locBLERemoteCharacteristic;

  if (ble_notify_task > -1) {
/*    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(plength);*/
    String data = String((char *)tData);
    float temperature = data.substring(2, 6).toDouble();
    float humidity = data.substring(9, 13).toDouble();
    byte varIndex = (ble_notify_task * VARS_PER_TASK);
    UserVar[varIndex] = temperature;
    UserVar[varIndex + 1] = humidity;
/*    Serial.println(pBLERemoteCharacteristic->toString().c_str());
    Serial.printf("%.1f / %.1f\n", temperature, humidity);*/
    // Disconnect from BT
    //esp_ble_gap_disconnect(*bleclient->getPeerAddress().getNative());  // OMG just disconnect please
    bleclients[ble_notify_task]->disconnect();
    struct EventStruct *TempEvent;
    TempEvent = new EventStruct;
    TempEvent->TaskIndex = ble_notify_task;
    TempEvent->sensorType = SENSOR_TYPE_TEMP_HUM;
    sendData(TempEvent);
    delete TempEvent;
    ble_notify_task = -1;
//    delay(100);
  }
}

boolean Plugin_103(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_103;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3; //3rd is the battery
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_103);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_103));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_103));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_103));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        char deviceAddress[18];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceAddress, sizeof(deviceAddress));
        addFormTextBox( F("BLE Address"), F("p103_addr"), deviceAddress, 18);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        char deviceAddress[18];
        String tmpString = WebServer.arg(F("p103_addr"));
        strncpy(deviceAddress, tmpString.c_str(), sizeof(deviceAddress));
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceAddress, sizeof(deviceAddress));
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        if (!BLEDevice::getInitialized())
        {
          BLEDevice::init(Settings.Name);
//          delay(200);
        }
        bleclients[event->TaskIndex] = BLEDevice::createClient();
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] = 0;
        success = true;
        break;
      }


    case PLUGIN_READ:
      {
        char deviceAddress[18];
        boolean notifysetted = false;
        boolean success = false;
        unsigned long *ptimer;
        ptimer = (unsigned long *)&Settings.TaskDevicePluginConfigLong[event->TaskIndex][0];

        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceAddress, sizeof(deviceAddress));

        String log = F("BLE connection to ");
        log += String(deviceAddress);
        addLog(LOG_LEVEL_DEBUG, log);
        if (ble_notify_task == -1) {

          if (UserVar[event->BaseVarIndex + 2] < 1 || (millis() > *ptimer)) // request battery status one time per hour
          {
//            Serial.println("BLE Battery requesting");
            esp_ble_gap_set_prefer_conn_params(*BLEDevice::getAddress().getNative(), 0x10, 0x20, 3, 300);
            success = bleclients[event->TaskIndex]->connect(BLEAddress(deviceAddress));
            if (success) {
//              Serial.println("connected");
//              delay(100);
              std::string rValue = bleclients[event->TaskIndex]->getValue(BLEUUID(MIJIA_BATTERY_SERVICE), BLEUUID(MIJIA_BATTERY_CHAR));
              int battery = 0;
              if (rValue.length() > 0) {
                battery = int((char)rValue[0]);
              }
              UserVar[event->BaseVarIndex + 2] = int(battery);
//              Serial.println(battery);
              *ptimer = millis() + 3600000;
            }
          }
          // Connect to the remote BLE Server
          if (!success) {
            Serial.println("reconnect");
            esp_ble_gap_set_prefer_conn_params(*BLEDevice::getAddress().getNative(), 0x10, 0x20, 0, 100);
            success = bleclients[event->TaskIndex]->connect(BLEAddress(deviceAddress));
          }
          if (success) {
//            Serial.println("register notify");
            ble_notify_task = event->TaskIndex;           // Serial.println(" - Connected to server");
            // Obtain a reference to the service we are after in the remote BLE server.
            BLERemoteService* pRemoteService = bleclients[event->TaskIndex]->getService(BLEUUID(MIJIA_DATA_SERVICE));
//            delay(100);
            if (pRemoteService != nullptr) {
//              Serial.println(" - Found BLE service");
              BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(MIJIA_DATA_CHAR));
              if (pRemoteCharacteristic != nullptr) {
//                Serial.println(" - Found our characteristic");
                if (pRemoteCharacteristic->canNotify()) {
                  Serial.println(" - Register for notifications");
                  pRemoteCharacteristic->registerForNotify((notify_callback)p103_notifyCallback);
                  notifysetted = true;
                }
              }
            }
          }

          if (!notifysetted) {
            Serial.println("force disconnect");
            bleclients[event->TaskIndex]->disconnect();
            ble_notify_task = -1; //release the line
            //esp_ble_gap_disconnect(*bleclient->getPeerAddress().getNative());  // OMG just disconnect please
            //delete bleclient;
            //bleclient = NULL;
          }

        }
//        Serial.println("end of read");
        if (success && notifysetted) {
          log += " success.";
          addLog(LOG_LEVEL_DEBUG, log);
        } else {
          log += " failed.";
          addLog(LOG_LEVEL_ERROR, log);
          success = false;
        }
        break;
      }
  }
  return success;
}


#endif
#endif // USES_P103
