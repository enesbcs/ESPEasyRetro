#ifndef DATASTRUCTS_P2PSTRUCTS
#define DATASTRUCTS_P2PSTRUCTS

struct P2P_SysInfoStruct
{
  // header field
  byte header = 255;
  byte ID = 1;
  byte sourcelUnit;
  byte destUnit=0; // default destination is to broadcast
  // start of payload
  byte mac[6];
  uint16_t build = BUILD;
  byte nodeType = NODE_TYPE_ID;
  byte cap;
  byte NameLength; // length of the following char array to send
  char Name[25];
};

struct P2P_SensorDataStruct_Raw
{
  // header field
  byte header = 255;
  byte ID = 5;
  byte sourcelUnit;
  byte destUnit;
  // start of payload
  uint16_t plugin_id;
  uint16_t idx;
  byte samplesetcount=0;
  byte valuecount;
  float Values[VARS_PER_TASK];  // use original float data
};

struct P2P_SensorDataStruct_TTN
{
  // header field
  byte header = 255;
  byte ID = 6;
  byte sourcelUnit;
  byte destUnit;
  // start of payload
  uint16_t plugin_id;
  uint16_t idx;
  byte samplesetcount=0;
  byte valuecount;
  byte data[8];    // implement base16Encode thing if you like
};

struct P2P_CommandDataStruct
{
  // header field
  byte header = 255;
  byte ID = 7;
  byte sourcelUnit;
  byte destUnit;
  // start of payload
  byte CommandLength; // length of the following char array to send
  char CommandLine[80];
};

struct P2P_TextDataStruct
{
  // header field
  byte header = 255;
  byte ID = 8;
  byte sourcelUnit;
  byte destUnit;
  // start of payload
  byte TextLength; // length of the following char array to send
  char TextLine[80];
};

#endif
