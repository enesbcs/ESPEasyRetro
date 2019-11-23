/*
#################################################
 This is the place where plugins are registered
#################################################

To create/register a plugin, you have to :
- find an available number, ie 777.
- Create your own plugin, ie as "_P777_myfunction.ino"
- be sure it starts with ""#ifdef USES_P777", and ends with "#endif"
- then register it into the PLUGIN_SET_EXPERIMENTAL block (see below)
 #ifdef PLUGIN_SET_EXPERIMENTAL
     #define USES_P777   // MYsuperPlugin
 #endif
 - you can from now on test it by compiling using the PLUGIN_BUILD_DEV flag
 either by adding "-DPLUGIN_BUILD_DEV" when compiling, or by momentarly
 adding "#define PLUGIN_BUILD_DEV" at the top of the ESPEasy.ino file
 - You will then have to push a PR including your plugin + the corret line (#define USES_P777) added to this file

 When found stable enought, the maintainer (and only him) will choose to move it to TESTING or STABLE
*/


/******************************************************************************\
 * BUILD Configs *******************************************************************
\******************************************************************************/

#define ENABLE_BLYNK  false

#ifdef PLUGIN_BUILD_CUSTOM
//## PLUGINS ##
    #define USES_P001   // Switch
    #define USES_P002   // ADC
    #define USES_P003   // Pulse
    #define USES_P004   // Dallas
    #define USES_P005   // DHT
    #define USES_P006   // BMP085
    #define USES_P007   // PCF8591
    #define USES_P008   // RFID
    #define USES_P009   // MCP

    #define USES_P010   // BH1750
    #define USES_P011   // PME
    #define USES_P012   // LCD
    #define USES_P013   // HCSR04
    #define USES_P014   // SI7021
    #define USES_P015   // TSL2561
    #define USES_P016   // IR
    #define USES_P017   // PN532
    #define USES_P018   // Dust
    #define USES_P019   // PCF8574

    #define USES_P020   // Ser2Net
    #define USES_P021   // Level
    #define USES_P022   // PCA9685
    #define USES_P023   // OLED
    #define USES_P024   // MLX90614
    #define USES_P025   // ADS1115
    #define USES_P026   // SysInfo
    #define USES_P027   // INA219
    #define USES_P028   // BME280
    #define USES_P029   // Output

    #define USES_P030   // BMP280
    #define USES_P031   // SHT1X
    #define USES_P032   // MS5611
    #define USES_P033   // Dummy
    #define USES_P034   // DHT12
    #define USES_P035   // IRTX
    #define USES_P036   // FrameOLED -- OLED_SSD1306_SH1106_images.h
    #define USES_P037   // MQTTImport
    #define USES_P038   // NeoPixel
    #define USES_P039   // Thermo
    #define USES_P040   // ID12
    
    #define USES_P041   // NeoClock
    #define USES_P042   // Candle
    #define USES_P043   // ClkOutput
//    #define USES_P044   // P1WifiGateway
    #define USES_P045   // MPU6050
//    #define USES_P046   // VentusW266
    #define USES_P047   // I2C_soil_misture
    #define USES_P048   // Motoshield_v2
    #define USES_P049   // MHZ19

//    #define USES_P050   // TCS34725 RGB Color Sensor with IR filter and White LED -- Library?
    #define USES_P051   // AM2320
    #define USES_P052   // SenseAir
    #define USES_P053   // PMSx003
    #define USES_P054   // DMX512
    #define USES_P055   // Chiming
    #define USES_P056   // SDS011-Dust
    #define USES_P057   // HT16K33_LED
    #define USES_P058   // HT16K33_KeyPad
    #define USES_P059   // Encoder

    #define USES_P060   // MCP3221
    #define USES_P061   // Keypad
    #define USES_P062   // MPR121_KeyPad
    #define USES_P063   // TTP229_KeyPad
    #define USES_P064   // APDS9960 Gesture
    #define USES_P065   // DRF0299
    #define USES_P066   // VEML6040
    #define USES_P067   // HX711_Load_Cell
    #define USES_P068   // SHT3x
    #define USES_P069   // LM75A

    #define USES_P070   // NeoPixel_Clock
    #define USES_P071   // Kamstrup401
    #define USES_P072   // HDC1080
    #define USES_P073   // 7DG
//    #define USES_P074   // TSL2561 -- Library?
//    #define USES_P075   // Nextion
//    #define USES_P076   // sonoff -- Library?
//    #define USES_P077  // CSE7766   Was P134 on Playground sonoff
    #define USES_P078   // Eastron Modbus Energy meters
    #define USES_P079   // Wemos Motoshield
//    #define USES_P080   // iButton Sensor  DS1990A

    #define USES_P102   // Analog32
    #define USES_P103   // BLE Mijia
    #define USES_P104   // BLE Flora      
      
    #define USES_P111 // RF decode
    #define USES_P126 // Ping
    #define USES_P133 // VL53L0X
    #define USES_P144 // RF Tx
    #define USES_P157 // DS3231
    #define USES_P159 // Pushbutton
    #define USES_P160 // Output
    #define USES_P161 // Switchboard
    #define USES_P166 // WifiMan
//## CONTROLLERS ##
    #define USES_C001   // Domoticz HTTP
    #define USES_C002   // Domoticz MQTT
    #define USES_C003   // Nodo telnet
    #define USES_C004   // ThingSpeak
    #define USES_C005   // OpenHAB MQTT
    #define USES_C006   // PiDome MQTT
    #define USES_C007   // Emoncms
    #define USES_C008   // Generic HTTP
    #define USES_C009   // FHEM HTTP
    #define USES_C010   // Generic UDP
    #define USES_C011   // HTTP Adv
#if ENABLE_BLYNK    
    #define USES_C012   // Blynk HTTP
#else
    #undef USES_C012
#endif   
    #define USES_C013   // ESPEasy P2P network
    #define USES_C021   // ESPEasy32 BLE P2P network    
//## NOTIFICATIONS ##
    #define USES_N001   // Email
    #define USES_N002   // Buzzer
#endif

#ifdef ESP32 // remove non-working plugins
 #undef USES_P010 // as_bh1750
 #undef USES_P049 //easysoftwareserial  
 #undef USES_P052 //easysoftwareserial  
 #undef USES_P053 //easysoftwareserial 
 #undef USES_P065 //easysoftwareserial
 #undef USES_P071 //easysoftwareserial 
 #undef USES_P075 //easysoftwareserial  
 #undef USES_P078 //easysoftwareserial  
 #undef USES_P126
#else
 #undef USES_P102  // only for ESP32
 #undef USES_P103  // only for ESP32 
 #undef USES_P104  // only for ESP32 
 #undef USES_C021  // only for ESP32
#endif

/******************************************************************************\
 * Libraries dependencies *****************************************************
\******************************************************************************/

/*
#if defined(USES_P00x) || defined(USES_P00y)
#include <the_required_lib.h>
#endif
*/
