#ifdef USES_C021
#ifdef ESP32
//#######################################################################################################
//######################## Controller Plugin 021: ESPEasy32 BLE P2P network #############################
//#######################################################################################################
//
// Based on C011 and C013 controller sources
// ESP32 BLE library is not stable enough for everyday usage :(

#define CPLUGIN_021
#define CPLUGIN_ID_021         21
#define CPLUGIN_NAME_021       "BLE Direct [Development]"

#undef CLASSIC_BT_ENABLED
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_gap_ble_api.h>

#include "src/DataStructs/P2PStructs.h"
#include "cdecode.h"  // BASE64 decoder

#define BLE_SERVICE_ID    "5032505f-5250-4945-6173-795f424c455f"
#define BLE_RECEIVER_CHAR "10000001-5250-4945-6173-795f424c455f"
#define BLE_INFO_CHAR     "10000002-5250-4945-6173-795f424c455f"
#define BLE_PACKET_SIZE   int((250*4)/3)   // warning: Base64 encoding requires more 4/3 x space!

#ifndef BLE_ESP32
#define BLE_ESP32
BLEServer* bleserv = NULL;
BLEClient* bleclient = NULL;
BLEService *pService = NULL;
#endif

uint8_t c_payload[BLE_PACKET_SIZE];
byte c021_capability = 0;
boolean c021_init = false;

struct C021_ConfigStruct
{
  void zero_last() {
    defaultdestination[23] = {0};
  }

  boolean  enablesend = true;
  boolean  enablerec = false;
  boolean  enabledsend = false;
  int8_t   defaultdestunit = 0;
  char     defaultdestination[24] = {0};
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
      pServer->updateConnParams(param->connect.remote_bda, 0x10, 0x40, 0, 200);
    };
    void onDisconnect(BLEServer* pServer) { }
};

class ReceiverCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();  // getData() ???
      int dlen = 0;
      if (rxValue.length() > 1) {
        dlen = b64decode((uint8_t*)rxValue.c_str(), rxValue.length(), c_payload);
/*        Serial.println("Received Value: "); // DEBUG
        for (int i = 0; i < dlen; i++) {
          Serial.print(c_payload[i]);
          Serial.print(", ");
        }
        Serial.println();*/
      } else
        return;
      if (c_payload[0] == 255) {
        //          memcpy(&c_payload, rxValue.c_str(), rxValue.length());
        struct EventStruct TempEvent;
        TempEvent.Data = (byte*)&c_payload;         // Payload
        TempEvent.Par1    = dlen;                   // data length
        TempEvent.String1 = "";
        TempEvent.String2 = "";
        byte ProtocolIndex = getProtocolIndex(CPLUGIN_ID_021);
        CPlugin_ptr[ProtocolIndex](CPLUGIN_PROTOCOL_RECV, &TempEvent, dummyString); // reroute incoming data to controller function
      }
    }
};

class InfoCallback: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) {
      base64 encoder;
      struct P2P_SysInfoStruct *dataReply;
      dataReply = new P2P_SysInfoStruct;
      int psize = BLE_create_sysinfo(dataReply);

      String encdata = encoder.encode((uint8_t*)dataReply, psize);
      pCharacteristic->setValue((uint8_t*)encdata.c_str(), encdata.length()); // return sysinfo if requested
      //pCharacteristic->setValue((byte*) &dataReply, psize);
      delete dataReply;
    }
};

boolean CPlugin_021(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  BLECharacteristic *pCharacteristicRX;
  BLECharacteristic *pCharacteristicTX;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
      {
        Protocol[++protocolCount].Number = CPLUGIN_ID_021;
        Protocol[protocolCount].usesMQTT = false;
        Protocol[protocolCount].usesTemplate = false;
        Protocol[protocolCount].usesAccount = false;
        Protocol[protocolCount].usesPassword = false;
        Protocol[protocolCount].defaultPort = 1024;
        Protocol[protocolCount].usesID = true;
        Protocol[protocolCount].Custom = true;
        break;
      }

    case CPLUGIN_GET_DEVICENAME:
      {
        string = F(CPLUGIN_NAME_021);
        break;
      }

    case CPLUGIN_INIT:
      {
        C021_ConfigStruct customConfig;
        LoadCustomControllerSettings(event->ControllerIndex, (byte*)&customConfig, sizeof(customConfig));        
        customConfig.zero_last();
        Serial.print("Init BLE Controller ");
        if (c021_init == false) {
          if (!BLEDevice::getInitialized())
          {
            BLEDevice::init(Settings.Name);
          }
          BLEDevice::setMTU(BLE_PACKET_SIZE);
          //          esp_bt_controller_enable(ESP_BT_MODE_BLE);
          //          esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
          c021_init = true;
        }
        Serial.println(BLEDevice::getAddress().toString().c_str());

        if (customConfig.enablesend || customConfig.enabledsend) {
          if (bleclient == NULL) {
            c021_capability += 1;
            Serial.println("Init BLE send");
            bleclient  = BLEDevice::createClient();
            esp_ble_gap_set_prefer_conn_params(*BLEDevice::getAddress().getNative(), 0x10, 0x20, 0, 200);
            delay(200);
            if (customConfig.enablesend) {
              Serial.println("Sending sysinfo");
              C021_send_sysinfo(customConfig.defaultdestination, false);
              if (!customConfig.enablerec) {
                delay(500);
                Serial.println("Getting sysinfo");
                C021_get_sysinfo(customConfig.defaultdestination, false);
              }
            }
            Serial.println("BLE Sender initialized");
          }
        }

        if (customConfig.enablerec) {
          if (bleserv == NULL) {
            c021_capability += 2;
            Serial.println("Init BLE receiver");
            bleserv = BLEDevice::createServer();
            delay(200);
            bleserv->setCallbacks(new MyServerCallbacks());
            // Create the BLE Device
            pService = bleserv->createService(BLE_SERVICE_ID);
            delay(200);
            pCharacteristicRX = pService->createCharacteristic(
                                  BLE_RECEIVER_CHAR,
                                  BLECharacteristic::PROPERTY_WRITE
                                );
            pCharacteristicRX->addDescriptor(new BLE2902());
            pCharacteristicRX->setCallbacks(new ReceiverCallback());
            pCharacteristicTX = pService->createCharacteristic(
                                  BLE_INFO_CHAR,
                                  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                                );
            pCharacteristicTX->addDescriptor(new BLE2902());
            pCharacteristicTX->setCallbacks(new InfoCallback());
            Serial.println("BLE Receiver service started");
          }
          startbleadvertise();
        }

        break;
      }

    case CPLUGIN_PROTOCOL_SEND:
      {
        C021_ConfigStruct customConfig;
        if (c021_init == false) {
          return false;
        }
        LoadCustomControllerSettings(event->ControllerIndex, (byte*)&customConfig, sizeof(customConfig));
        customConfig.zero_last();

        if ( (event->idx != 0) && (customConfig.enablesend) )
        {
          struct P2P_SensorDataStruct_Raw *dataReply;
          dataReply = new P2P_SensorDataStruct_Raw;

          dataReply->sourcelUnit = byte(Settings.Unit);
          dataReply->destUnit = byte(customConfig.defaultdestunit);

          dataReply->plugin_id = (uint16_t)(Settings.TaskDeviceNumber[event->TaskIndex]);
          dataReply->idx = (uint16_t)(event->idx);
          dataReply->valuecount = byte(getValueCountFromSensorType(event->sensorType));

          for (byte x = 0; x < dataReply->valuecount; x++)
            dataReply->Values[x] = (float)UserVar[event->BaseVarIndex + x];

          byte psize = sizeof(P2P_SensorDataStruct_Raw);
          if (dataReply->valuecount < 4) { // do not send zeroes
            psize = psize - ((4 - dataReply->valuecount) * sizeof(float));
          }
          /*          Serial.println(customConfig.defaultdestination);
                    Serial.println(event->idx);
                    Serial.println(psize);*/
          BLE_senddata(customConfig.defaultdestination, (uint8_t*)dataReply, psize, customConfig.enablerec);
          delete dataReply;
        }
        break;
      }

    case CPLUGIN_PROTOCOL_RECV:
      {
        if (event->Data[0] == 255) {
          /*          Serial.println("PKTOK");
                    Serial.println(event->Par1);*/

          if (event->Data[1] == 1) { // infopacket
            struct P2P_SysInfoStruct *dataReply;
            dataReply = (P2P_SysInfoStruct*)event->Data;
            byte namelen = dataReply->NameLength;
            if (namelen < 25) {
              dataReply->Name[namelen] = 0; // terminate with zero
            } else {
              namelen = 25;
            }
            if (dataReply->sourcelUnit < UNIT_MAX) {
              Nodes[dataReply->sourcelUnit].age = 0;
              Nodes[dataReply->sourcelUnit].build = dataReply->build;
              Nodes[dataReply->sourcelUnit].ip[0] = 127;
              for (byte x = 3; x < 6; x++)  {
                Nodes[dataReply->sourcelUnit].ip[x - 2] = byte(dataReply->mac[x]);
              }
              Nodes[dataReply->sourcelUnit].nodeType = byte(dataReply->nodeType);
              if (Nodes[dataReply->sourcelUnit].nodeName == 0) {
                Nodes[dataReply->sourcelUnit].nodeName = (char *)malloc(26);
                memcpy(Nodes[dataReply->sourcelUnit].nodeName, dataReply->Name, dataReply->NameLength); ///???
              } else {
                for (byte x = 0; x < namelen; x++)  {
                  if (dataReply->Name[x]==0)
                   break;
                  Nodes[dataReply->sourcelUnit].nodeName[x] = dataReply->Name[x];
                }
              }
              //strncpy(Nodes[dataReply->sourcelUnit].nodeName, dataReply->Name, namelen); ///???              
              Nodes[dataReply->sourcelUnit].nodeName[namelen] = 0;
            }
            break;
          }

          if (event->Data[1] == 5) { // data arrived
            struct P2P_SensorDataStruct_Raw *dataReply;
            dataReply = (P2P_SensorDataStruct_Raw*)event->Data;
            if ( (byte(dataReply->destUnit) != byte(Settings.Unit)) && byte(dataReply->destUnit) != 0) { // exit if not broadcasted and we are not the destination
              break;
            }
            if (dataReply->sourcelUnit < UNIT_MAX) {
              Nodes[dataReply->sourcelUnit].age = 0;
              if (Nodes[dataReply->sourcelUnit].ip[0] == 0) {
                for (byte x = 0; x < 4; x++)
                  Nodes[dataReply->sourcelUnit].ip[0] = 127;
                Nodes[dataReply->sourcelUnit].ip[3] = dataReply->sourcelUnit;
              }
              if (Nodes[dataReply->sourcelUnit].nodeName == 0) {
                Nodes[dataReply->sourcelUnit].nodeName =  (char *)malloc(26);
                Nodes[dataReply->sourcelUnit].nodeName = "BLENode";
              }
            }
            byte ControllerID = findFirstEnabledControllerWithId(CPLUGIN_ID_021);
            if (ControllerID == CONTROLLER_MAX) // controller not valid, hard pass
              return false;

            int fid = -1;
            for (byte t = 0; t < TASKS_MAX; t++) { // search for idx
              if (Settings.TaskDeviceID[ControllerID][t] == dataReply->idx) {
                fid = t;
                break;
              }
            }
            if (fid == -1) { // idx not found, create it
              for (byte t = 0; t < TASKS_MAX; t++) { // find an empty task
                if (Settings.TaskDeviceNumber[t] == 0) {
                  fid = t;
                  break;
                }
              }
              if (fid > -1) { //create new task
                taskClear(fid, false);
                LoadTaskSettings(fid);
                struct EventStruct *TempEvent;
                TempEvent = new EventStruct;
                TempEvent->TaskIndex = fid;
                int pid = dataReply->plugin_id;
                if (pid > 255)
                  pid = 33; // in case pluginid is larger than 255 set dummy
                Settings.TaskDeviceNumber[fid] = byte(pid);
                Settings.TaskDeviceDataFeed[fid] = dataReply->sourcelUnit;  // remote feed
                Settings.TaskDeviceID[ControllerID][fid] = dataReply->idx;  // set idx
                for (byte x = 0; x < CONTROLLER_MAX; x++)
                  Settings.TaskDeviceSendData[x][fid] = false;

                String deviceName = "";
                byte DeviceIndex = getDeviceIndex(pid);
                Plugin_ptr[DeviceIndex](PLUGIN_GET_DEVICENAME, 0, deviceName);
                deviceName += String(fid);
                deviceName.replace(" ", "");
                deviceName.toCharArray(ExtraTaskSettings.TaskDeviceName, deviceName.length());
                deviceName = "";
                PluginCall(PLUGIN_GET_DEVICEVALUENAMES, TempEvent, dummyString);
                byte maxvars = Device[DeviceIndex].ValueCount;
                if (dataReply->valuecount < maxvars)
                  maxvars = dataReply->valuecount;
                for (byte x = 0; x < maxvars; x++)
                {
                  deviceName = String(ExtraTaskSettings.TaskDeviceValueNames[x]);
                  deviceName.replace(" ", "");
                  if (deviceName.length() < 1) {
                    deviceName = "Value" + String(x);
                  }
                  deviceName.toCharArray(ExtraTaskSettings.TaskDeviceValueNames[x], deviceName.length());
                }
                PluginCall(PLUGIN_INIT, TempEvent, dummyString);
                delete TempEvent;
                Settings.TaskDeviceEnabled[fid] = true;
                ExtraTaskSettings.TaskIndex = fid;
                SaveTaskSettings(fid);
                SaveSettings();
              }
            }
            if (fid == -1) { //neither found nor created, hard pass
              return false;
            }
            // idx found, update
            if (Settings.TaskDeviceDataFeed[fid] > 0)
            {
              byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[fid]);
              byte maxvars = Device[DeviceIndex].ValueCount;
              if (dataReply->valuecount < maxvars)
                maxvars = dataReply->valuecount;
              //              Serial.println(maxvars);
              for (byte x = 0; x < maxvars; x++)
              {
                UserVar[fid * VARS_PER_TASK + x] = dataReply->Values[x];
                //                Serial.println(dataReply->Values[x]); // debug
              }
              if (Settings.UseRules) {
                createRuleEvents(fid);
              }
            }
            event->Data[1] = 0;
          }

          if (event->Data[1] == 7) { // commandpacket
            struct P2P_CommandDataStruct *dataReply;
            dataReply = (P2P_CommandDataStruct*)event->Data;
            if ( (byte(dataReply->destUnit) != byte(Settings.Unit)) and (byte(dataReply->destUnit) != 0) ) { // exit if not broadcasted and we are not the destination
              break;
            }
            dataReply->CommandLine[dataReply->CommandLength] = 0; // make it zero terminated
            String request = (char*)dataReply->CommandLine;

            if (dataReply->sourcelUnit < UNIT_MAX) {
              Nodes[dataReply->sourcelUnit].age = 0;
              if (Nodes[dataReply->sourcelUnit].ip[0] == 0) {
                for (byte x = 0; x < 4; x++)
                  Nodes[dataReply->sourcelUnit].ip[0] = 127;
                Nodes[dataReply->sourcelUnit].ip[3] = dataReply->sourcelUnit;
              }
              if (Nodes[dataReply->sourcelUnit].nodeName == 0) {
                Nodes[dataReply->sourcelUnit].nodeName =  (char *)malloc(26);
                Nodes[dataReply->sourcelUnit].nodeName = "BLENode";
              }
            }

            struct EventStruct *TempEvent;
            TempEvent = new EventStruct;
            parseCommandString(TempEvent, request);
            TempEvent->Source = VALUE_SOURCE_SYSTEM;
            if (!PluginCall(PLUGIN_WRITE, TempEvent, request)) {           // call plugins
              ExecuteCommand(VALUE_SOURCE_SYSTEM, dataReply->CommandLine);     // or execute system command
            } // VALUE_SOURCE_BLE not defined yet
            delete TempEvent;
          }

          if (event->Data[1] == 8) {
            struct P2P_TextDataStruct *dataReply;
            dataReply = (P2P_TextDataStruct*)event->Data;
            if ( (byte(dataReply->destUnit) != byte(Settings.Unit)) && byte(dataReply->destUnit) != 0) { // exit if not broadcasted and we are not the destination
              break;
            }
            if (dataReply->sourcelUnit < UNIT_MAX) {
              Nodes[dataReply->sourcelUnit].age = 0;
              if (Nodes[dataReply->sourcelUnit].ip[0] == 0) {
                for (byte x = 0; x < 4; x++)
                  Nodes[dataReply->sourcelUnit].ip[0] = 127;
                Nodes[dataReply->sourcelUnit].ip[3] = dataReply->sourcelUnit;
              }
              if (Nodes[dataReply->sourcelUnit].nodeName == 0) {
                Nodes[dataReply->sourcelUnit].nodeName =  (char *)malloc(26);
                Nodes[dataReply->sourcelUnit].nodeName = "BLENode";
              }
            }

            String logs = F("UNIT");
            logs += String(dataReply->sourcelUnit);
            logs += F(">");
            logs += String(dataReply->TextLine);
            addLog(LOG_LEVEL_INFO, logs);
          }
        }

        break;
      }

    case CPLUGIN_WEBFORM_LOAD:
      {
        C021_ConfigStruct customConfig;

        LoadCustomControllerSettings(event->ControllerIndex, (byte*)&customConfig, sizeof(customConfig));
        customConfig.zero_last();
        if (c021_init) {
          String tstr = F("Own BLE address: ");
          tstr += String(BLEDevice::getAddress().toString().c_str());
          addFormNote(tstr);
        }
        addFormCheckBox(F("Enable receiver"), F("c011rec"), customConfig.enablerec);
        addFormNote(F("Disable in case of sleep mode usage!"));
        addFormCheckBox(F("Enable sending to MASTER"), F("c011send"), customConfig.enablesend);
        //        addFormCheckBox(F("Enable Direct sending"), F("c011dsend"), customConfig.enabledsend);
        addFormTextBox( F("Default BLE MASTER unit MAC"), F("c011master"), customConfig.defaultdestination, 24);
        addFormNumericBox( F("Default destination node Number"), F("c011destunit"), customConfig.defaultdestunit, 0, 255);
        addFormNote(F("Default node index for data sending, only used when Master Unit address is setted."));
        break;
      }

    case CPLUGIN_WEBFORM_SAVE:
      {
        C021_ConfigStruct customConfig;

        customConfig.enablerec   = isFormItemChecked(F("c011rec"));
        customConfig.enablesend  = isFormItemChecked(F("c011send"));
        customConfig.enabledsend = isFormItemChecked(F("c011dsend"));
        customConfig.defaultdestunit = getFormItemInt(F("c011destunit"));
        String master   = WebServer.arg(F("c011master"));
        strlcpy(customConfig.defaultdestination, master.c_str(), sizeof(customConfig.defaultdestination));
        customConfig.zero_last();
        if (master.length() < 11) {
          customConfig.enablesend = false;
        }
        SaveCustomControllerSettings(event->ControllerIndex, (byte*)&customConfig, sizeof(customConfig));
        break;
      }

  }
  return success;
}

void endbleadvertise() {
  if (pService != NULL) {
    pService->stop();
  }
}


void startbleadvertise() {
  if (pService != NULL) {
    pService->start();
    //    bleserv->getAdvertising()->start();
    BLEAdvertising *pAdvertising = bleserv->getAdvertising();
/*    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);*/
    pAdvertising->start();
  }
}
/*
  void taskAdvertise( void * parameter ){
  vTaskDelete( NULL );
  }
*/
int BLE_create_sysinfo(struct P2P_SysInfoStruct *dataReply) {
  memcpy(dataReply->mac, BLEDevice::getAddress().getNative(), 6);
  memset(dataReply->Name, 0, 24);
  strncpy(dataReply->Name, Settings.Name, 24);
  dataReply->NameLength = strlen(Settings.Name);

  dataReply->sourcelUnit = byte(Settings.Unit);
  dataReply->build = uint16_t(BUILD);
  dataReply->nodeType = byte(NODE_TYPE_ID);
  dataReply->cap = c021_capability;

  // send packet
  byte psize = sizeof(P2P_SysInfoStruct);
  if (dataReply->NameLength < 24) { // do not send zeroes
    psize = psize - (24 - dataReply->NameLength);
  }
  /*  Serial.println(dataReply->Name); // debug
    Serial.println(dataReply->sourcelUnit); // debug
    Serial.println(dataReply->nodeType); // debug*/

  return psize;
}

boolean BLE_senddata(const char *bleaddress, uint8_t* bdata, int data_len, boolean enablerec) {
  base64 encoder;
  if (bleclient == NULL) {
    return false;
  }
  if (enablerec) {
    endbleadvertise();
  }
  esp_ble_gap_set_prefer_conn_params(*BLEDevice::getAddress().getNative(), 0x10, 0x20, 0, 100);
  boolean success = bleclient->connect(BLEAddress(bleaddress));
  delay(200);
  if (success) {
    BLERemoteService* pRemoteService = bleclient->getService(BLEUUID(BLE_SERVICE_ID));
    delay(200);
    if (pRemoteService != nullptr) {
      BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(BLE_RECEIVER_CHAR));
      if (pRemoteCharacteristic != nullptr) {
        String encdata = encoder.encode(bdata, data_len); // encode data to base64 as current esp32 ble implementation is not playing nicely with binary data
//        Serial.print("Sending: ");
//        Serial.println(encdata); // DEBUG
        pRemoteCharacteristic->writeValue((uint8_t*)encdata.c_str(), encdata.length(), true);
      } else {
        success = false;
      }
    } else {
      success = false;
    }
    //bleclient->disconnect(); // HEAP error ????
    esp_ble_gap_disconnect(*bleclient->getPeerAddress().getNative());  // OMG just disconnect please
    if (enablerec) {
      startbleadvertise(); // restart advertisement if needed
    }
  }
  return success;
}

void C021_send_sysinfo(const char *bleaddress, boolean enablerec) {
  struct P2P_SysInfoStruct *dataReply;
  dataReply = new P2P_SysInfoStruct;
  int psize = 0;
  psize = BLE_create_sysinfo(dataReply);
  delay(random(10, 200));
  BLE_senddata(bleaddress, (uint8_t*)dataReply, psize, enablerec);
  delete dataReply;
}

void C021_get_sysinfo(const char *bleaddress, boolean enablerec) {
  if (enablerec) {
    endbleadvertise();
  }
  esp_ble_gap_set_prefer_conn_params(*BLEDevice::getAddress().getNative(), 0x10, 0x20, 1, 200);
  boolean success = bleclient->connect(BLEAddress(bleaddress));
  if (success) {
    std::string rxValue = bleclient->getValue(BLEUUID(BLE_SERVICE_ID), BLEUUID(BLE_INFO_CHAR));
    int dlen = 0;
    if (rxValue.length() > 1) {
      dlen = b64decode((uint8_t*)rxValue.c_str(), rxValue.length(), c_payload); // decode incoming base64 data
      //memcpy(&c_payload, rxValue.c_str(), rxValue.length());      
      struct EventStruct TempEvent;
      TempEvent.Data = (byte*)&c_payload;         // Payload
      TempEvent.Par1    = dlen;       // data length
      TempEvent.String1 = "";
      TempEvent.String2 = "";
      byte ProtocolIndex = getProtocolIndex(CPLUGIN_ID_021);
      CPlugin_ptr[ProtocolIndex](CPLUGIN_PROTOCOL_RECV, &TempEvent, dummyString); // reroute incoming data to controller function

      //bleclient->disconnect(); // HEAP error ????
      esp_ble_gap_disconnect(*bleclient->getPeerAddress().getNative());  // OMG just disconnect please
      if (enablerec) {
        startbleadvertise();
      }
    }
  }
}

boolean C021_sendcmd(const char *bleaddress, byte destunit, const char *line, boolean enablerec) {
  struct P2P_CommandDataStruct *dataReply;
  dataReply = new P2P_CommandDataStruct;

  dataReply->CommandLength = strlen(line);
  strncpy(dataReply->CommandLine, line, dataReply->CommandLength);

  dataReply->sourcelUnit = byte(Settings.Unit);
  dataReply->destUnit = byte(destunit);

  // send packet
  byte psize = sizeof(P2P_CommandDataStruct);
  if (dataReply->CommandLength < 80) { // do not send zeroes
    psize = psize - (80 - dataReply->CommandLength);
  }
  BLE_senddata(bleaddress, (uint8_t*)dataReply, psize, enablerec);
  delete dataReply;
}

String Command_BLEExec(struct EventStruct *event, const char* Line)
{
  C021_ConfigStruct customConfig;
  String eventName = Line;
  eventName = eventName.substring(11);
  int index = eventName.indexOf(',');
  if (index > 0)
  {
    eventName = eventName.substring(index + 1);
/*    Serial.println("ble_exec");
    Serial.println(event->Par1);
    Serial.println(eventName);*/
    byte ControllerIndex = 0;
    for (byte x = 0; x < CONTROLLER_MAX; x++) {
     if (Settings.Protocol[x] == CPLUGIN_ID_021)
      ControllerIndex = x;
      break;
    }
    if (Settings.Protocol[ControllerIndex] != CPLUGIN_ID_021)
     return return_command_failed();
    LoadCustomControllerSettings(ControllerIndex, (byte*)&customConfig, sizeof(customConfig));  
    customConfig.zero_last();
    C021_sendcmd(customConfig.defaultdestination, event->Par1, eventName.c_str(), customConfig.enablerec);
  }
  return return_command_success();
  
/*   customConfig.enablesend = false;
   customConfig.enablerec = false;
   SaveCustomControllerSettings(event->ControllerIndex, (byte*)&customConfig, sizeof(customConfig));
   delay(100);
   reboot();*/
}

int b64decode(uint8_t* b64Text, int b64Len, uint8_t* output) {
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block((const char*)b64Text, b64Len, (char*)output, &s);
  return cnt;
}
#endif
#endif
