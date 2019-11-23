#ifndef COMMAND_UDP_H
#define COMMAND_UDP_H


String Command_UDP_Test (struct EventStruct *event, const char* Line)
{
  for (byte x = 0; x < event->Par2; x++)
  {
    String eventName = "Test ";
    eventName += x;
    SendUDPCommand(event->Par1, (char*)eventName.c_str(), eventName.length());
  }
  return return_command_success();
}

String Command_UDP_Port(struct EventStruct *event, const char* Line)
{
  return Command_GetORSetBool(event, F("UDPPort:"),
   Line,
   (bool *)&Settings.UDPPort,
   1);
}

String Command_UPD_SendTo(struct EventStruct *event, const char* Line)
{
  String eventName = Line;
  eventName = eventName.substring(7);
  int index = eventName.indexOf(',');
  if (index > 0)
  {
    eventName = eventName.substring(index + 1);
    SendUDPCommand(event->Par1, (char*)eventName.c_str(), eventName.length());
  }
  return return_command_success();
}


String Command_UDP_SendToUPD(struct EventStruct *event, const char* Line)
{
  if (wifiStatus == ESPEASY_WIFI_SERVICES_INITIALIZED) {
    String strLine = Line;
    String ip = parseString(strLine, 2);
    String port = parseString(strLine, 3);
    if (!isInt(port)) return return_command_failed();
    String message = parseStringToEndKeepCase(strLine, 4);
    IPAddress UDP_IP;
    if(UDP_IP.fromString(ip)) {
      portUDP.beginPacket(UDP_IP, port.toInt());
      #if defined(ESP8266)
        portUDP.write(message.c_str(), message.length());
      #endif
      #if defined(ESP32)
        portUDP.write((uint8_t*)message.c_str(), message.length());
      #endif
      portUDP.endPacket();
    }
    return return_command_success();
  }
  return return_not_connected();
}

#endif // COMMAND_UDP_H
