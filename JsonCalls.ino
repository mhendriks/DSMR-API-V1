/* 
***************************************************************************  
**  Program  : JsonCalls, part of DSMRloggerAPI
**  Version  : v2.0.0
**
**  Copyright (c) 2020 Martijn Hendriks
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/

// ******* Global Vars *******
uint32_t  antiWearTimer = 0;
bool onlyIfPresent  = false;

char fieldsArray[50][35] = {{0}}; // to lookup fields 
int  fieldsElements      = 0;

int  actualElements = 20;
char actualArray[][35] = { "timestamp"
                          ,"energy_delivered_tariff1","energy_delivered_tariff2"
                          ,"energy_returned_tariff1","energy_returned_tariff2"
                          ,"power_delivered","power_returned"
                          ,"voltage_l1","voltage_l2","voltage_l3"
                          ,"current_l1","current_l2","current_l3"
                          ,"power_delivered_l1","power_delivered_l2","power_delivered_l3"
                          ,"power_returned_l1","power_returned_l2","power_returned_l3"
                          ,"gas_delivered"
                          ,"\0"};
int  infoElements = 7;
char infoArray[][35]   = { "identification","p1_version","equipment_id","electricity_tariff","gas_device_type","gas_equipment_id", "\0" };

char OneRecord[2300]= "";

//=======================================================================
void processAPI() {
  char fName[40] = "";
  char URI[50]   = "";
  String words[10];

  strncpy( URI, httpServer.uri().c_str(), sizeof(URI) );

  if (httpServer.method() == HTTP_GET)
        DebugTf("from[%s] URI[%s] method[GET] \r\n"
                                  , httpServer.client().remoteIP().toString().c_str()
                                        , URI); 
  else  DebugTf("from[%s] URI[%s] method[PUT] \r\n" 
                                  , httpServer.client().remoteIP().toString().c_str()
                                        , URI); 

#ifdef USE_SYSLOGGER
  if (ESP.getFreeHeap() < 5000) // to prevent firmware from crashing!
#else
  if (ESP.getFreeHeap() < 8500) // to prevent firmware from crashing!
#endif
  {
      DebugTf("==> Bailout due to low heap (%d bytes))\r\n", ESP.getFreeHeap() );
      writeToSysLog("from[%s][%s] Bailout low heap (%d bytes)"
                                    , httpServer.client().remoteIP().toString().c_str()
                                    , URI
                                    , ESP.getFreeHeap() );
    httpServer.send(500, "text/plain", "500: internal server error (low heap)\r\n"); 
    return;
  }

  int8_t wc = splitString(URI, '/', words, 10);
  
  if (Verbose2) 
  {
    DebugT(">>");
    for (int w=0; w<wc; w++)
    {
      Debugf("word[%d] => [%s], ", w, words[w].c_str());
    }
    Debugln(" ");
  }

  /* code wordt nooit aabgesproken omdat <> api niet in dit proces komt
   if (words[1] != "api")
  {
    sendApiNotFound2(URI);
    return;
  }*/

  if (words[2] != "v1" && words[2] != "v2")
  {
    sendApiNotFound(URI);
    return;
  }

  if (words[3] == "dev")
  {
    if (words[2] == "v1") handleDevApi(URI, words[4].c_str(), words[5].c_str(), words[6].c_str()); 
    else handleDevApiV2(URI, words[4].c_str(), words[5].c_str(), words[6].c_str());
  }
  else if (words[3] == "hist")
  {
    if (words[2] == "v1") handleHistApi(URI, words[4].c_str(), words[5].c_str(), words[6].c_str());
    else handleHistApi2(URI, words[4].c_str(), words[5].c_str(), words[6].c_str());
  }
  else if (words[3] == "sm")
  {
    if (words[2] == "v1") handleSmApi(URI, words[4].c_str(), words[5].c_str(), words[6].c_str());
    else handleSmApi2(URI, words[4].c_str(), words[5].c_str(), words[6].c_str());
  }
  else sendApiNotFound(URI);
  
} // processAPI()

template <typename TSource>
void sendJson(const TSource &doc) 
{
  String JsonOutput;
  
  if (Verbose1) serializeJsonPretty(doc, JsonOutput); 
  else serializeJson(doc, JsonOutput);
  
  DebugTln(F("http: json sent .."));
  //Serial.print(F("Sending json: "));
  //Serial.println(JsonOutput);
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.setContentLength(measureJson(doc));
  httpServer.send(200, "application/json", JsonOutput);  
}

void sendDeviceTime2() 
{
  // Allocate a temporary JsonDocument
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument doc(110);
  
  doc["timestamp"] = actTimestamp;
  doc["time"] = buildDateTimeString(actTimestamp, sizeof(actTimestamp));
  doc["epoch"] = (int)now();
  
  sendJson(doc);

} // sendDeviceTime()

void sendDeviceInfo2() 
{
  char compileOptions[200] = "";
  DynamicJsonDocument doc(1500);

#ifdef USE_REQUEST_PIN
    strConcat(compileOptions, sizeof(compileOptions), "[USE_REQUEST_PIN]");
#endif
#if defined( USE_PRE40_PROTOCOL )
    strConcat(compileOptions, sizeof(compileOptions), "[USE_PRE40_PROTOCOL]");
#elif defined( USE_BELGIUM_PROTOCOL )
    strConcat(compileOptions, sizeof(compileOptions), "[USE_BELGIUM_PROTOCOL]");
#else
    strConcat(compileOptions, sizeof(compileOptions), "[USE_DUTCH_PROTOCOL]");
#endif
#ifdef USE_UPDATE_SERVER
    strConcat(compileOptions, sizeof(compileOptions), "[USE_UPDATE_SERVER]");
#endif
#ifdef USE_MQTT
    strConcat(compileOptions, sizeof(compileOptions), "[USE_MQTT]");
#endif
#ifdef USE_MINDERGAS
    strConcat(compileOptions, sizeof(compileOptions), "[USE_MINDERGAS]");
#endif
#ifdef USE_SYSLOGGER
    strConcat(compileOptions, sizeof(compileOptions), "[USE_SYSLOGGER]");
#endif
#ifdef USE_NTP_TIME
    strConcat(compileOptions, sizeof(compileOptions), "[USE_NTP_TIME]");
#endif

  //doc["author"] = F("Willem Aandewiel / Martijn Hendriks (www.aandewiel.nl)");
  doc["fwversion"] = _FW_VERSION;
  snprintf(cMsg, sizeof(cMsg), "%s %s", __DATE__, __TIME__);
  doc["compiled"] = cMsg;
  doc["hostname"] = settingHostname;
  doc["ipaddress"] = WiFi.localIP().toString();
  doc["indexfile"] = settingIndexPage;
  doc["macaddress"] = WiFi.macAddress();
  doc["freeheap"] ["value"] = ESP.getFreeHeap();
  doc["freeheap"]["unit"] = "bytes";
  doc["maxfreeblock"] ["value"] = ESP.getMaxFreeBlockSize();
  doc["maxfreeblock"]["unit"] = "bytes";
  doc["chipid"] = String( ESP.getChipId(), HEX );
  doc["coreversion"] = String( ESP.getCoreVersion() );
  doc["sdkversion"] = String( ESP.getSdkVersion() );
  doc["cpufreq"] ["value"] = ESP.getCpuFreqMHz();
  doc["cpufreq"]["unit"] = "MHz";
  doc["sketchsize"] ["value"] = formatFloat( (ESP.getSketchSize() / 1024.0), 3);
  doc["sketchsize"]["unit"] = "kB";
  doc["freesketchspace"] ["value"] = formatFloat( (ESP.getFreeSketchSpace() / 1024.0), 3);
  doc["freesketchspace"]["unit"] = "kB";
    
  if ((ESP.getFlashChipId() & 0x000000ff) == 0x85) 
        snprintf(cMsg, sizeof(cMsg), "%08X (PUYA)", ESP.getFlashChipId());
  else  snprintf(cMsg, sizeof(cMsg), "%08X", ESP.getFlashChipId());
  doc["flashchipid"] = cMsg;
  doc["flashchipsize"] ["value"] = formatFloat((ESP.getFlashChipSize() / 1024.0 / 1024.0), 3);
  doc["flashchipsize"]["unit"] = "MB";
  doc["flashchiprealsize"] ["value"] = formatFloat((ESP.getFlashChipRealSize() / 1024.0 / 1024.0), 3);
  doc["flashchiprealsize"]["unit"] = "MB";

  SPIFFS.info(SPIFFSinfo);
  doc["spiffssize"] ["value"] = formatFloat( (SPIFFSinfo.totalBytes / (1024.0 * 1024.0)), 0);
  doc["spiffssize"]["unit"] = "MB";
  doc["flashchipspeed"] ["value"] = formatFloat((ESP.getFlashChipSpeed() / 1000.0 / 1000.0), 0);
  doc["flashchipspeed"]["unit"] = "MHz";
 
  FlashMode_t ideMode = ESP.getFlashChipMode();
  doc["flashchipmode"] = flashMode[ideMode];
  doc["boardtype"] = ARDUINO_BOARD;
  doc["compileoptions"] = compileOptions;
  doc["ssid"] = WiFi.SSID();

#ifdef SHOW_PASSWRDS
  doc["pskkey"] = WiFi.psk();
#endif
  doc["wifirssi"] = WiFi.RSSI();
  doc["uptime"] = upTime();
  doc["smhasfaseinfo"] = (int)settingSmHasFaseInfo;
  doc["telegraminterval"] = (int)settingTelegramInterval;
  doc["telegramcount"] = (int)telegramCount;
  doc["telegramerrors"] = (int)telegramErrors;

#ifdef USE_MQTT
  snprintf(cMsg, sizeof(cMsg), "%s:%04d", settingMQTTbroker, settingMQTTbrokerPort);
  doc["mqttbroker"] = cMsg;
  doc["mqttinterval"] = settingMQTTinterval;
  if (mqttIsConnected)
        doc["mqttbroker_connected"] = "yes";
  else  doc["mqttbroker_connected"] = "no";
#endif

#ifdef USE_MINDERGAS
  snprintf(cMsg, sizeof(cMsg), "%s:%d", timeLastResponse, intStatuscodeMindergas);
  doc["mindergas_response"] = txtResponseMindergas;
  doc["mindergas_status"] = cMsg;
#endif

  doc["reboots"] = (int)nrReboots;
  doc["lastreset"] = lastReset;  

  sendJson(doc);
 
} // sendDeviceInfo()

//=======================================================================
void sendDeviceSettings2() 
{
  DebugTln("sending device settings ...\r");
  DynamicJsonDocument doc(1500);
  
  doc["hostname"]["value"] = settingHostname;
  doc["hostname"]["type"] = "s";
  doc["hostname"]["maxlen"] = sizeof(settingHostname) -1;
  
  doc["ed_tariff1"]["value"] = settingEDT1;
  doc["ed_tariff1"]["type"] = "f";
  doc["ed_tariff1"]["min"] = 0;
  doc["ed_tariff1"]["max"] = 10;
  
  doc["ed_tariff2"]["value"] = settingEDT2;
  doc["ed_tariff2"]["type"] = "f";
  doc["ed_tariff2"]["min"] = 0;
  doc["ed_tariff2"]["max"] = 10;
  
  doc["er_tariff1"]["value"] = settingERT1;
  doc["er_tariff1"]["type"] = "f";
  doc["er_tariff1"]["min"] = 0;
  doc["er_tariff1"]["max"] = 10;
  
  doc["er_tariff2"]["value"] = settingERT2;
  doc["er_tariff2"]["type"] = "f";
  doc["er_tariff2"]["min"] = 0;
  doc["er_tariff2"]["max"] = 10;
  
  doc["gd_tariff"]["value"] = settingGDT;
  doc["gd_tariff"]["type"] = "f";
  doc["gd_tariff"]["min"] = 0;
  doc["gd_tariff"]["max"] = 10;
  
  doc["electr_netw_costs"]["value"] = settingENBK;
  doc["electr_netw_costs"]["type"] = "f";
  doc["electr_netw_costs"]["min"] = 0;
  doc["electr_netw_costs"]["max"] = 100;
  
  doc["gas_netw_costs"]["value"] = settingGNBK;
  doc["gas_netw_costs"]["type"] = "f";
  doc["gas_netw_costs"]["min"] = 0;
  doc["gas_netw_costs"]["max"] = 100;
  
  doc["sm_has_fase_info"]["value"] = settingSmHasFaseInfo;
  doc["sm_has_fase_info"]["type"] = "i";
  doc["sm_has_fase_info"]["min"] = 0;
  doc["sm_has_fase_info"]["max"] = 1;
  
  doc["tlgrm_interval"]["value"] = settingTelegramInterval;
  doc["tlgrm_interval"]["type"] = "i";
  doc["tlgrm_interval"]["min"] = 2;
  doc["tlgrm_interval"]["max"] = 60;
  
  doc["index_page"]["value"] = settingIndexPage;
  doc["index_page"]["type"] = "s";
  doc["index_page"]["maxlen"] = sizeof(settingIndexPage) -1;
  
  doc["mqtt_broker"]["value"]  = settingMQTTbroker;
  doc["mqtt_broker"]["type"] = "s";
  doc["mqtt_broker"]["maxlen"] = sizeof(settingIndexPage) -1;
  
  doc["mqtt_broker_port"]["value"] = settingMQTTbrokerPort;
  doc["mqtt_broker_port"]["type"] = "i";
  doc["mqtt_broker_port"]["min"] = 1;
  doc["mqtt_broker_port"]["max"] = 9999;
  
  doc["mqtt_user"]["value"] = settingMQTTuser;
  doc["mqtt_user"]["type"] = "s";
  doc["mqtt_user"]["maxlen"] = sizeof(settingMQTTuser) -1;
  
  doc["mqtt_passwd"]["value"] = settingMQTTpasswd;
  doc["mqtt_passwd"]["type"] = "s";
  doc["mqtt_passwd"]["maxlen"] = sizeof(settingMQTTpasswd) -1;
  
  doc["mqtt_toptopic"]["value"] = settingMQTTtopTopic;
  doc["mqtt_toptopic"]["type"] = "s";
  doc["mqtt_toptopic"]["maxlen"] = sizeof(settingMQTTtopTopic) -1;
  
  doc["mqtt_interval"]["value"] = settingMQTTinterval;
  doc["mqtt_interval"]["type"] = "i";
  doc["mqtt_interval"]["min"] = 0;
  doc["mqtt_interval"]["max"] = 600;
  
  #if defined (USE_MINDERGAS )
    doc["mindergastoken"]["value"] = settingMindergasToken;
    doc["mindergastoken"]["type"] = "s";
    doc["mindergastoken"]["maxlen"] = sizeof(settingMindergasToken) -1;
  #endif

  sendJson(doc);

} // sendDeviceSettings()

//====================================================
void sendApiNotFound(const char *URI)
{
  DebugTln("sending device settings ...\r");
  char JsonOutput[200];
  DynamicJsonDocument doc(200);
  
  doc["error"]["url"] = URI;
  doc["error"]["message"] = "not valid url";

  serializeJson(doc, JsonOutput);
  //Serial.print(F("Sending json: "));
  //Serial.println(JsonOutput);
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.setContentLength(measureJson(doc));
  httpServer.send(404, "application/json", JsonOutput); 
  
  
} // sendApiNotFound()
//---------------------------------------------------------------
void FillJsonRec(const char *cName, String sValue)
{
  char noUnit[] = {'\0'};
 
  if (sValue.length() > (JSON_BUFF_MAX - 60) )
  {
    FillJsonRec(cName, sValue.substring(0,(JSON_BUFF_MAX - (strlen(cName) + 30))), noUnit);
  }
  else
  {
    FillJsonRec(cName, sValue, noUnit);
  }
  
} // FillJsonRec(*char, String)

//=======================================================================
void FillJsonRec(const char *cName, const char *cValue, const char *cUnit)
{
    DebugTln("const char *cName, const char *cValue, const char *cUnit");

  if (strlen(cUnit) == 0)
  {
    snprintf(OneRecord, sizeof(OneRecord)-1, "%s\"%s\":{\"value\":\"%s\"},"
                                      ,OneRecord, cName, cValue);
  }
  else
  {
    snprintf(OneRecord, sizeof(OneRecord)-1, "%s\"%s\":{\"value\":\"%s\",\"unit\":\"%s\"},"
                                      ,OneRecord, cName, cValue, cUnit);
  }

} // FillJsonRec(*char, *char, *char)

//---------------------------------------------------------------
void FillJsonRec(const char *cName, const char *cValue)
{
    DebugTln("const char *cName, const char *cValue");
    char noUnit[] = {'\0'};

  FillJsonRec(cName, cValue, noUnit);
  
} // FillJsonRec(*char, *char)


//=======================================================================
void FillJsonRec(const char *cName, String sValue, const char *cUnit)
{
  DebugTln("const char *cName, String sValue, const char *cUnit ");

  if (sValue.length() > (JSON_BUFF_MAX - 65) )
  {
    DebugTf("[2] sValue.length() [%d]\r\n", sValue.length());
  }
  
  if (strlen(cUnit) == 0)
  {
    snprintf(OneRecord, sizeof(OneRecord)-1, "%s\"%s\":{\"value\":\"%s\"},"
                                      ,OneRecord, cName, sValue.c_str());
  }
  else
  {
    snprintf(OneRecord, sizeof(OneRecord)-1, "%s\"%s\":{\"value\":\"%s\",\"unit\":\"%s\"},"
                                      ,OneRecord, cName, sValue.c_str(), cUnit);
  }

} // FillJsonRec(*char, String, *char)

//---------------------------------------------------------------
void FillJsonRec(const char *cName, float fValue)
{
        DebugTln("const char *cName, float fValue");

  char noUnit[] = {'\0'};

  FillJsonRec(cName, fValue, noUnit);
  
} // FillJsonRec(*char, float)

//=======================================================================
void FillJsonRec(const char *cName, float fValue, const char *cUnit)
{  
      DebugTln("const char *cName, float fValue, const char *cUnit");

  if (strlen(cUnit) == 0)
  {
    snprintf(OneRecord, sizeof(OneRecord)-1, "%s\"%s\":{\"value\":%.3f},"
                                      ,OneRecord, cName, fValue);
  }
  else
  {
    snprintf(OneRecord, sizeof(OneRecord)-1, "%s \"%s\":{\"value\":%.3f,\"unit\":\"%s\"},"
                                      ,OneRecord, cName, fValue, cUnit);
  }

} // FillJsonRec(*char, float, *char)

//=======================================================================

struct buildJsonApi2 {
    bool  skip = false;

    template<typename Item>
    void apply(Item &i) {
      skip = false;
      String Name = Item::name;
      //-- for dsmr30 -----------------------------------------------
#if defined( USE_PRE40_PROTOCOL )
      if (Name.indexOf("gas_delivered2") == 0) Name = "gas_delivered";
#endif
      if (!isInFieldsArray(Name.c_str(), fieldsElements))
      {
        skip = true;
      }
      if (!skip)
      {
        if (i.present()) 
        {
          String Unit = Item::unit();
        
          if (Unit.length() > 0)
          {
            FillJsonRec(Name.c_str(), typecastValue(i.val()), Unit.c_str());
          }
          else 
          {
            FillJsonRec(Name.c_str(), typecastValue(i.val()));
          }
        }
        else if (!onlyIfPresent)
        {
          FillJsonRec(Name.c_str(), "-");
 
        }
        //printf("OneRecord: %s \n\r", OneRecord);
      }
  }

};  // buildJsonApi2()

void ArrayToJson(){
  //send json output
  
  if (strlen(OneRecord) < 4 ) { //some relevant data should be available
    OneRecord[1] = '}';
    OneRecord[2] = '\0';
  } else OneRecord[strlen(OneRecord)-1] = '}'; //replace "," with json terminating char
  
  DebugTln(F("http: json sent .."));
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.setContentLength(strlen(OneRecord));
  httpServer.send(200, "application/json", OneRecord);  
  
  
}    
//====================================================
void handleSmApi2(const char *URI, const char *word4, const char *word5, const char *word6)
{
  char    tlgrm[1200] = "";
  uint8_t p=0;  
  bool    stopParsingTelegram = false;
  memset(OneRecord,0,sizeof(OneRecord));//clear onerecord
  OneRecord[0] = '{'; //start Json
  
  //DebugTf("word4[%s], word5[%s], word6[%s]\r\n", word4, word5, word6);
  char type;
  switch (word4[0]) {
    
  case 'i': //info
    onlyIfPresent = false;
    copyToFieldsArray(infoArray, infoElements);
    DSMRdata.applyEach(buildJsonApi2());
    ArrayToJson() ;
  break;
  
  case 'a': //actual
    onlyIfPresent = true;
    copyToFieldsArray(actualArray, actualElements);
    DSMRdata.applyEach(buildJsonApi2());
    ArrayToJson();
  break;
  
  case 'f': //fields
    fieldsElements = 0;
    onlyIfPresent = false;
    if (strlen(word5) > 0)
    {
       memset(fieldsArray,0,sizeof(fieldsArray));
       strCopy(fieldsArray[0], 34,"timestamp");
       strCopy(fieldsArray[1], 34, word5);
       fieldsElements = 2;
    }
    //sendJsonFields(word4);
    DSMRdata.applyEach(buildJsonApi2());
    ArrayToJson();
  break;  
  case 't': //telegramm 
  {
    showRaw = true;
    slimmeMeter.enable(true);
    swSer1.setTimeout(5000);  // 5 seconds must be enough ..
    memset(tlgrm, 0, sizeof(tlgrm));
    int l = 0;
    // The terminator character is discarded from the serial buffer.
    l = swSer1.readBytesUntil('/', tlgrm, sizeof(tlgrm));
    // now read from '/' to '!'
    // The terminator character is discarded from the serial buffer.
    l = swSer1.readBytesUntil('!', tlgrm, sizeof(tlgrm));
    swSer1.setTimeout(1000);  // seems to be the default ..
    DebugTf("read [%d] bytes\r\n", l);
    if (l == 0) 
    {
      httpServer.send(200, "application/plain", "no telegram received");
      showRaw = false;
      return;
    }

    tlgrm[l++] = '!';
#if !defined( USE_PRE40_PROTOCOL )
    // next 6 bytes are "<CRC>\r\n"
    for (int i=0; ( i<6 && (i<(sizeof(tlgrm)-7)) ); i++)
    {
      tlgrm[l++] = (char)swSer1.read();
    }
#else
    tlgrm[l++]    = '\r';
    tlgrm[l++]    = '\n';
#endif
    tlgrm[(l +1)] = '\0';
    // shift telegram 1 char to the right (make room at pos [0] for '/')
    for (int i=strlen(tlgrm); i>=0; i--) { tlgrm[i+1] = tlgrm[i]; yield(); }
    tlgrm[0] = '/'; 
    showRaw = false;
    if (Verbose1) Debugf("Telegram (%d chars):\r\n/%s", strlen(tlgrm), tlgrm);
    httpServer.send(200, "application/plain", tlgrm);
    break; 
    } 
  default:
    sendApiNotFound(URI);
    break;
  }
  
} // handleSmApi2()
//====================================================

void handleDevApiV2(const char *URI, const char *word4, const char *word5, const char *word6)
{
  //DebugTf("word4[%s], word5[%s], word6[%s]\r\n", word4, word5, word6);
  if (strcmp(word4, "info") == 0)
  {
    sendDeviceInfo2();
  }
  else if (strcmp(word4, "time") == 0)
  {
    sendDeviceTime2();
  }
  else if (strcmp(word4, "settings") == 0)
  {
    if (httpServer.method() == HTTP_PUT || httpServer.method() == HTTP_POST)
    {
      //------------------------------------------------------------ 
      // json string: {"name":"settingInterval","value":9}  
      // json string: {"name":"settingTelegramInterval","value":123.45}  
      // json string: {"name":"settingTelegramInterval","value":"abc"}  
      //------------------------------------------------------------ 
      // so, why not use ArduinoJSON library?
      // I say: try it yourself ;-) It won't be easy
      String wOut[5];
      String wPair[5];
      String jsonIn  = httpServer.arg(0).c_str();
      char field[25] = "";
      char newValue[101]="";
      jsonIn.replace("{", "");
      jsonIn.replace("}", "");
      jsonIn.replace("\"", "");
      int8_t wp = splitString(jsonIn.c_str(), ',',  wPair, 5) ;
      for (int i=0; i<wp; i++)
      {
        //DebugTf("[%d] -> pair[%s]\r\n", i, wPair[i].c_str());
        int8_t wc = splitString(wPair[i].c_str(), ':',  wOut, 5) ;
        //DebugTf("==> [%s] -> field[%s]->val[%s]\r\n", wPair[i].c_str(), wOut[0].c_str(), wOut[1].c_str());
        if (wOut[0].equalsIgnoreCase("name"))  strCopy(field, sizeof(field), wOut[1].c_str());
        if (wOut[0].equalsIgnoreCase("value")) strCopy(newValue, sizeof(newValue), wOut[1].c_str());
      }
      //DebugTf("--> field[%s] => newValue[%s]\r\n", field, newValue);
      updateSetting(field, newValue);
      httpServer.send(200, "application/json", httpServer.arg(0));
      writeToSysLog("DSMReditor: Field[%s] changed to [%s]", field, newValue);
    }
    else
    {
      sendDeviceSettings2();
    }
  }
  else if (strcmp(word4, "debug") == 0)
  {
    sendDeviceDebug(URI, word5);
  }
  else sendApiNotFound(URI);
  
} // handleDevApiV2()

//====================================================
void handleHistApi2(const char *URI, const char *word4, const char *word5, const char *word6)
{
  int8_t  fileType     = 0;
  char    fileName[20] = "";
  
  //DebugTf("word4[%s], word5[%s], word6[%s]\r\n", word4, word5, word6);
  if (   strcmp(word4, "hours") == 0 )
  {
    //fileType = HOURS;
    //strCopy(fileName, sizeof(fileName), HOURS_FILE);
    RingFileToApi(RINGHOURS);
    return;
  }
  else if (strcmp(word4, "days") == 0 )
  {
    //fileType = DAYS;
    //strCopy(fileName, sizeof(fileName), DAYS_FILE);
    RingFileToApi(RINGDAYS);
    return;

  }
  else if (strcmp(word4, "months") == 0)
  {
    fileType = MONTHS;
    if (httpServer.method() == HTTP_PUT || httpServer.method() == HTTP_POST)
    {
      //------------------------------------------------------------ 
      // json string: {"recid":"29013023"
      //               ,"edt1":2601.146,"edt2":"9535.555"
      //               ,"ert1":378.074,"ert2":208.746
      //               ,"gdt":3314.404}
      //------------------------------------------------------------ 
      char      record[DATA_RECLEN + 1] = "";
      uint16_t  recSlot;

      String jsonIn  = httpServer.arg(0).c_str();
      DebugTln(jsonIn);
      
      recSlot = buildDataRecordFromJson(record, jsonIn);
      
      //--- update MONTHS
      writeDataToFile(MONTHS_FILE, record, recSlot, MONTHS);
      //--- send OK response --
      httpServer.send(200, "application/json", httpServer.arg(0));
      
      return;
    }
    else 
    {
      //strCopy(fileName, sizeof(fileName), MONTHS_FILE);
      RingFileToApi(RINGMONTHS);
      return;
    }
  }
  else 
  {
    sendApiNotFound(URI);
    return;
  }
  /*if (strcmp(word5, "desc") == 0)
        sendJsonHist2(fileType, fileName, actTimestamp, true);
  else  sendJsonHist2(fileType, fileName, actTimestamp, false);
*/
} // handleHistApi2()

//=======================================================================
void sendJsonHist2(int8_t fileType, const char *fileName, const char *timeStamp, bool desc) 
{
  uint8_t startSlot, nrSlots, recNr  = 0;
  char    typeApi[10];
  DynamicJsonDocument doc(9500);
  
  if (DUE(antiWearTimer))
  {
    writeDataToFiles();
    writeRingFiles();
    writeLastStatus();
  }
    
  switch(fileType) {
    case HOURS:   startSlot       = timestampToHourSlot(timeStamp, strlen(timeStamp));
                  nrSlots         = _NO_HOUR_SLOTS_;
                  strCopy(typeApi, 9, "hours");
                  break;
    case DAYS:    startSlot       = timestampToDaySlot(timeStamp, strlen(timeStamp));
                  nrSlots         = _NO_DAY_SLOTS_;
                  strCopy(typeApi, 9, "days");
                  break;
    case MONTHS:  startSlot       = timestampToMonthSlot(timeStamp, strlen(timeStamp));
                  nrSlots         = _NO_MONTH_SLOTS_;
                  strCopy(typeApi, 9, "months");
                  break;
  }

  if (desc)
        startSlot += nrSlots +1; // <==== voorbij actuele slot!
  else  startSlot += nrSlots;    // <==== start met actuele slot!

  DebugTf("sendJsonHist startSlot[%02d]\r\n", (startSlot % nrSlots));
  
  for (uint8_t s = 0; s < nrSlots; s++)
  { 
    JsonObject obj = doc.createNestedObject();
    if (desc)
          readOneSlot(fileType, fileName, s, (s +startSlot), true, "hist", obj);
    else  readOneSlot(fileType, fileName, s, (startSlot -s), true, "hist", obj);
  }

  //DebugTf("sendJson V2 HistoryData");
  sendJson(doc);
    
} // sendJsonHist2()

/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
***************************************************************************/
