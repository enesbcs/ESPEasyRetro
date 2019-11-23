/********************************************************************************************\
* Initialize specific hardware settings (only global ones, others are set through devices)
\*********************************************************************************************/

void hardwareInit()
{

  // set GPIO pins state if not set to default
  for (byte x=0; x < 17; x++)
    if (Settings.PinBootStates[x] != 0)
      switch(Settings.PinBootStates[x])
      {
        case 1:
          pinMode(x,OUTPUT);
          digitalWrite(x,LOW);
          setPinState(1, x, PIN_MODE_OUTPUT, LOW);
          break;
        case 2:
          pinMode(x,OUTPUT);
          digitalWrite(x,HIGH);
          setPinState(1, x, PIN_MODE_OUTPUT, HIGH);
          break;
        case 3:
          pinMode(x,INPUT_PULLUP);
          setPinState(1, x, PIN_MODE_INPUT, 0);
          break;
      }

  if (Settings.Pin_Reset != -1)
    pinMode(Settings.Pin_Reset,INPUT_PULLUP);
 
  // configure hardware pins according to eeprom settings.
  if (Settings.Pin_i2c_sda != -1)
  {
    String log = F("INIT : I2C");
    addLog(LOG_LEVEL_INFO, log);
    Wire.begin(Settings.Pin_i2c_sda, Settings.Pin_i2c_scl);
      if(Settings.WireClockStretchLimit)
      {
        String log = F("INIT : I2C custom clockstretchlimit:");
        log += Settings.WireClockStretchLimit;
        addLog(LOG_LEVEL_INFO, log);
        #if defined(ESP8266)
          Wire.setClockStretchLimit(Settings.WireClockStretchLimit);
        #endif        
      }
  }

#if ENABLE_I2C_WATCHDOG
  // I2C Watchdog boot status check
  if (Settings.WDI2CAddress != 0)
  {
    delay(500);
    Wire.beginTransmission(Settings.WDI2CAddress);
    Wire.write(0x83);             // command to set pointer
    Wire.write(17);               // pointer value to status byte
    Wire.endTransmission();

    Wire.requestFrom(Settings.WDI2CAddress, (uint8_t)1);
    if (Wire.available())
    {
      byte status = Wire.read();
      if (status & 0x1)
      {
        String log = F("INIT : Reset by WD!");
        addLog(LOG_LEVEL_ERROR, log);
        lastBootCause = BOOT_CAUSE_EXT_WD;
      }
    }
  }
#endif  

  // SPI Init
  if (Settings.InitSPI)
  {
    SPI.setHwCs(false);
    SPI.begin();
    String log = F("INIT : SPI Init (without CS)");
    addLog(LOG_LEVEL_INFO, log);
  }
  else
  {
    String log = F("INIT : SPI not enabled");
    addLog(LOG_LEVEL_INFO, log);
  }

#ifdef FEATURE_SD
  if (Settings.Pin_sd_cs >= 0)
  {
    if (SD.begin(Settings.Pin_sd_cs))
    {
      String log = F("SD   : Init OK");
      addLog(LOG_LEVEL_INFO, log);
    }
    else
    {
      String log = F("SD   : Init failed");
      addLog(LOG_LEVEL_ERROR, log);
    }
  }
#endif

}

void checkResetFactoryPin(){
  static byte factoryResetCounter=0;
  if (Settings.Pin_Reset == -1)
    return;

  if (digitalRead(Settings.Pin_Reset) == 0){ // active low reset pin  
    factoryResetCounter++; // just count every second
  }
  else
  { // reset pin released
    if (factoryResetCounter > 9) // factory reset and reboot
      ResetFactory();
    if (factoryResetCounter > 3) // normal reboot
    #if defined(ESP8266)
      ESP.reset();
    #endif
    #if defined(ESP32)
      ESP.restart();
    #endif    

    factoryResetCounter = 0; // count was < 3, reset counter
  }
}

#ifdef ESP32

// ********************************************************************************
// Get info of a specific GPIO pin.
// ********************************************************************************
bool getGpioInfo(int gpio, int& pinnr, bool& input, bool& output, bool& warning) {
  pinnr = -1; // ESP32 does not label the pins, they just use the GPIO number.

  // Input GPIOs:  0-19, 21-23, 25-27, 32-39
  // Output GPIOs: 0-19, 21-23, 25-27, 32-33
  input  = gpio <= 39;
  output = gpio <= 33;

  if ((gpio < 0) || (gpio == 20) || (gpio == 24) || ((gpio > 27) && (gpio < 32))) {
    input  = false;
    output = false;
  }

  if ((input == false) && (output == false)) {
    return false;
  }

  // GPIO 0 & 2 can't be used as an input. State during boot is dependent on boot mode.
  warning = (gpio == 0 || gpio == 2);

  if (gpio == 12) {
    // If driven High, flash voltage (VDD_SDIO) is 1.8V not default 3.3V.
    // Has internal pull-down, so unconnected = Low = 3.3V.
    // May prevent flashing and/or booting if 3.3V flash is used and this pin is
    // pulled high, causing the flash to brownout.
    // See the ESP32 datasheet for more details.
    warning = true;
  }

  if (gpio == 15) {
    // If driven Low, silences boot messages printed by the ROM bootloader.
    // Has an internal pull-up, so unconnected = High = normal output.
    warning = true;
  }
  return true;
}

#else // ifdef ESP32

// return true when pin can be used.
bool getGpioInfo(int gpio, int& pinnr, bool& input, bool& output, bool& warning) {
  pinnr  = -1;
  input  = true;
  output = true;

  // GPIO 0, 2 & 15 can't be used as an input. State during boot is dependent on boot mode.
  warning = (gpio == 0 || gpio == 2 || gpio == 15);

  switch (gpio) {
    case  0: pinnr =  3; break;
    case  1: pinnr = 10; break;
    case  2: pinnr =  4; break;
    case  3: pinnr =  9; break;
    case  4: pinnr =  2; break;
    case  5: pinnr =  1; break;
    case  6:                    // GPIO 6 .. 8  is used for flash
    case  7:
    case  8: pinnr = -1; break;
    case  9: pinnr = 11; break; // On ESP8266 used for flash
    case 10: pinnr = 12; break; // On ESP8266 used for flash
    case 11: pinnr = -1; break;
    case 12: pinnr =  6; break;
    case 13: pinnr =  7; break;
    case 14: pinnr =  5; break;

    // GPIO-15 Can't be used as an input. There is an external pull-down on this pin.
    case 15: pinnr =  8; input = false; break;
    case 16: pinnr =  0; break; // This is used by the deep-sleep mechanism
  }
  # ifndef ESP8285

  if ((gpio == 9) || (gpio == 10)) {
    // On ESP8266 used for flash
    warning = true;
  }
  # endif // ifndef ESP8285

  if (pinnr < 0) {
    input  = false;
    output = false;
    return false;
  }
  return true;
}

#endif // ifdef ESP32
