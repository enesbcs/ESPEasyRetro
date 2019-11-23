#ifdef USES_P104
#ifdef ESP32
//#######################################################################################################
//#################################### Plugin 104: BLE Mi Flora #########################################
//#######################################################################################################
//
// Based on https://github.com/sidddy/flora/blob/master/flora/flora.ino
//
#define PLUGIN_104
#define PLUGIN_ID_104         104
#define PLUGIN_NAME_104       "Environment - BLE Mi Flora"
#define PLUGIN_VALUENAME1_104 "Temperature"
#define PLUGIN_VALUENAME2_104 "Moisture"
#define PLUGIN_VALUENAME3_104 "Light"
#define PLUGIN_VALUENAME4_104 "Conductivity"

#include "BLEDevice.h"
#include <esp_gap_ble_api.h>

#define MIFLORA_DATA_SERVICE    "00001204-0000-1000-8000-00805f9b34fb"
#define MIFLORA_DATA_CHAR       "00001a01-0000-1000-8000-00805f9b34fb"
#define MIFLORA_WRITE_CHAR      "00001a00-0000-1000-8000-00805f9b34fb"
#define MIFLORA_BATTERY_CHAR    "00001a02-0000-1000-8000-00805f9b34fb"

#ifndef BLE_ESP32_MULTI
#define BLE_ESP32_MULTI
BLEClient* bleclients[TASKS_MAX];
#endif

#ifndef BLE_NOTIFY_TASK
#define BLE_NOTIFY_TASK
static int ble_notify_task = -1;
#endif

String Plugin_104_valuename(byte value_nr, bool displayString) {
  switch (value_nr) {
    case 0:  return displayString ? F("None") : F("");
    case 1:  return displayString ? F("Battery") : F("batt");
    case 2:  return displayString ? F("Temperature") : F("temp");
    case 3:  return displayString ? F("Moisture") : F("moist");
    case 4:  return displayString ? F("Light") : F("light");
    case 5:  return displayString ? F("Conductivity") : F("conduct");
    default:
      break;
  }
  return "";
}

boolean Plugin_104(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_104;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_104);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_104));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_104));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_104));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_104));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        char deviceAddress[18];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceAddress, sizeof(deviceAddress));
        addFormTextBox( F("BLE Address"), F("p104_addr"), deviceAddress, 18);
        String options[6];
        for (byte i = 0; i < 6; ++i) {
          options[i] = Plugin_104_valuename(i, true);
        }
        String label;
        String id;
        for (byte i = 0; i < 4; ++i) {
          byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][i];
          label = F("Indicator ");
          label += (i + 1);
          id = "p104_";
          id += (i + 1);
          addFormSelector(label, id, 6, options, NULL, choice);
        }
#ifdef BLE_BATTERY_PATCH
        addFormCheckBox(F("Try to provide battery field to Domoticz"), F("p104_battery"), Settings.TaskDevicePluginConfig[event->TaskIndex][4]);
#endif
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        char deviceAddress[18];
        String tmpString = WebServer.arg(F("p104_addr"));
        strncpy(deviceAddress, tmpString.c_str(), sizeof(deviceAddress));
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceAddress, sizeof(deviceAddress));
        String id;
        for (byte i = 0; i < 4; ++i) {
          id = "p104_";
          id += (i + 1);
          Settings.TaskDevicePluginConfig[event->TaskIndex][i] = getFormItemInt(id);
        }
#ifdef BLE_BATTERY_PATCH
        Settings.TaskDevicePluginConfig[event->TaskIndex][4] = isFormItemChecked(F("p104_battery"));
#else
        Settings.TaskDevicePluginConfig[event->TaskIndex][4] = 0;
#endif

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        if (!BLEDevice::getInitialized())
        {
          BLEDevice::init(Settings.Name);
          delay(200);
        }
        bleclients[event->TaskIndex] = BLEDevice::createClient();
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] = 0;
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] = 0;

        success = true;
        break;
      }


    case PLUGIN_READ:
      {
        uint8_t p104_payload[200];
        BLERemoteCharacteristic* pRemoteCharacteristic;
        boolean isbattery = false;
        boolean isdata    = false;
        char deviceAddress[18];
        boolean success = false;

        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] == 1 ||
            Settings.TaskDevicePluginConfig[event->TaskIndex][1] == 1 ||
            Settings.TaskDevicePluginConfig[event->TaskIndex][2] == 1 ||
            Settings.TaskDevicePluginConfig[event->TaskIndex][3] == 1) {
          isbattery = true;
        }
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][0] > 1 ||
            Settings.TaskDevicePluginConfig[event->TaskIndex][1] > 1 ||
            Settings.TaskDevicePluginConfig[event->TaskIndex][2] > 1 ||
            Settings.TaskDevicePluginConfig[event->TaskIndex][3] > 1) {
          isdata    = true;
        }
        if (!isbattery && !isdata) {
          return false;
        }
        if (ble_notify_task == -1) {
          if (Settings.TaskDevicePluginConfig[event->TaskIndex][4] != 0) {
            isbattery = true;
          }
          unsigned long *ptimer;
          ptimer = (unsigned long *)&Settings.TaskDevicePluginConfigLong[event->TaskIndex][0]; // last battery req
          // Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] // last battery val
          if (isbattery && !isdata && (Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] > 0 && (millis() < *ptimer))) {
            UserVar[event->BaseVarIndex] = Settings.TaskDevicePluginConfigLong[event->TaskIndex][1];
            return true;
          }
          ble_notify_task = event->TaskIndex;
          LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceAddress, sizeof(deviceAddress));

          String log = F("BLE connection to ");
          log += String(deviceAddress);
          addLog(LOG_LEVEL_DEBUG, log);
          esp_ble_gap_set_prefer_conn_params(*BLEDevice::getAddress().getNative(), 0x10, 0x40, 3, 400);
          success = bleclients[event->TaskIndex]->connect(BLEAddress(deviceAddress));
          if (success) {
//            delay(100);
            BLERemoteService* pRemoteService = bleclients[event->TaskIndex]->getService(BLEUUID(MIFLORA_DATA_SERVICE));
//            delay(100);
            if (isdata) {
              if (pRemoteService != nullptr) {
                Serial.println(" - Found our service");
                pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(MIFLORA_WRITE_CHAR));
                if (pRemoteCharacteristic != nullptr) {
                  uint8_t buf[2] = {0xA0, 0x1F};
                  pRemoteCharacteristic->writeValue(buf, 2, true);
                }
  //              delay(100);
                Serial.println(" - Get value");
                pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(MIFLORA_DATA_CHAR));
                if (pRemoteCharacteristic != nullptr) {
                  std::string rxValue = pRemoteCharacteristic->readValue();
                  Serial.print("The characteristic value was: ");
                  if (rxValue.length() < 200) {
                    memcpy(&p104_payload, rxValue.c_str(), rxValue.length());
                    Serial.print("Hex: ");
                    for (int i = 0; i < 16; i++) {
                      Serial.print((int)p104_payload[i], HEX);
                      Serial.print(" ");
                    }
                    Serial.println(" ");
                  }
                } else {
                  Serial.println("flora char is null");
                  success = false;
                }
              } else {
                Serial.println("flora serv is null");
                success = false;
              }
            }
            if (isbattery && success) {
              if (pRemoteService != nullptr) { //Serial.println(" - Found our service");
                if (Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] < 1 || (millis() > *ptimer)) // request battery status one time per hour
                {
                  delay(100);
                  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(MIFLORA_BATTERY_CHAR));
                  if (pRemoteCharacteristic != nullptr) {
                    std::string rValue = pRemoteCharacteristic->readValue();
                    int battery = 0;
                    if (rValue.length() > 0) {
                      battery = int((char)rValue[0]);
                    }
                    Serial.print("battery ");
                    Serial.println(battery);
                    Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] = battery;
                    *ptimer = millis() + 3600000;
                  }
                }
              }
            }

            if (success) {
              for (byte i = 0; i < 4; ++i) {
                if (Settings.TaskDevicePluginConfig[event->TaskIndex][i] > 0) {
                  UserVar[event->BaseVarIndex + i] = P104_get_value(Settings.TaskDevicePluginConfig[event->TaskIndex][i], event->TaskIndex, (uint8_t*)&p104_payload);
                }
              }
            }
            delay(200);
          }
          bleclients[event->TaskIndex]->disconnect();
          //esp_ble_gap_disconnect(*bleclient->getPeerAddress().getNative());  // OMG just disconnect please

          if (success) {
            log += " success.";
            addLog(LOG_LEVEL_DEBUG, log);
            sendData(event);
          } else {
            log += " failed.";
            addLog(LOG_LEVEL_ERROR, log);
          }
          ble_notify_task = -1;
          break;
        }
      }
  }
  return success;
}

float P104_get_value(int type, byte taskindex, uint8_t* val)
{
  float value = 0;
  int16_t* temp_raw = (int16_t*)val;
  float temperature = (*temp_raw) / ((float)10.0);
  if ( type > 1 && (temperature > 200) ) {
    return value;
  }
  switch (type)
  {
    case 0:
      {
        value = 0;
        break;
      }
    case 1:
      {
        value = Settings.TaskDevicePluginConfigLong[taskindex][1];
        break;
      }
    case 2:
      {
        value = temperature;
        break;
      }
    case 3:
      {
        value = val[7];
        break;
      }
    case 4:
      {
        value = val[3] + val[4] * 256;
        break;
      }
    case 5:
      {
        value = val[8] + val[9] * 256;
        break;
      }
  }
  return value;
}

#endif
#endif // USES_P104
