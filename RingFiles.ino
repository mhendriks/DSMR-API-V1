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
void createRingFile(E_ringfiletype ringfiletype) 
{
  File RingFile = SPIFFS.open(RingFiles[ringfiletype].filename, "w"); // open for writing  
  if (!RingFiles) {
    DebugT(F("open ring file FAILED!!! --> Bailout\r\n"));
    Debugln(RingFiles[ringfiletype].filename);
    return;
  }
  //fill file with default values
  DynamicJsonDocument doc(9500);
  doc["actSlot"] = 0;
  for (int slot=0; slot < RingFiles[ringfiletype].slots; slot++ ) 
  { 
    //JsonArray data = doc.createNestedArray("data");
    //JsonObject Object = data.createNestedObject(String(slot));
    doc["data"][slot]["date"] = "00000000";
    doc["data"][slot]["values"][0] = 0;
    doc["data"][slot]["values"][1] = 0;
    doc["data"][slot]["values"][2] = 0;
    doc["data"][slot]["values"][3] = 0;
    doc["data"][slot]["values"][4] = 0;
  }
  //DebugT(F("RingFile output >"));serializeJson(doc, Serial);   Serial.println();
  //write json to ring file
  DebugT(F("write(): to ringfile ")); Debugln(RingFiles[ringfiletype].filename);
  
  writeToJsonFile(doc, RingFile);
}

uint8_t CalcSlot(E_ringfiletype ringfiletype) 
{
  //slot positie bepalen
  uint32_t  nr=0;
  time_t    t1 = epoch((char*)actTimestamp, strlen(actTimestamp), false);
  if (ringfiletype == RINGMONTHS ) nr = ( (year(t1) -1) * 12) + month(t1);    // eg: year(2023) * 12 = 24276 + month(9) = 202309
  else nr = t1 / RingFiles[ringfiletype].seconds;
  uint8_t slot = nr % RingFiles[ringfiletype].slots;
    DebugTf("slot: Slot is [%d]\r\n", slot);
    DebugTf("nr: nr is [%d]\r\n", nr);

  if (slot < 0 || slot >= RingFiles[ringfiletype].slots)
  {
    DebugTf("RINGFile: Some serious error! Slot is [%d]\r\n", slot);
    slot = RingFiles[ringfiletype].slots;
    slotErrors++;
    return 99;
  }
  return slot;
}


//---------
void RingFileToApi(E_ringfiletype ringfiletype) 
{  
  if (bailout()) return; //exit when heapsize is too small
  
  if (!SPIFFS.exists(RingFiles[ringfiletype].filename))
  {
    DebugT(F("read(): Ringfile doesn't exist: "));
    Debugln(RingFiles[ringfiletype].filename);
    createRingFile(ringfiletype);
    return;
    }
  
  DynamicJsonDocument doc(9500);
  File RingFile = SPIFFS.open(RingFiles[ringfiletype].filename, "r"); // open for reading
  
  DeserializationError error = deserializeJson(doc, RingFile);
  if (error) {
    DebugTln(F("read():Failed to read RING*.json file"));
  } else sendJson(doc); 
  
  RingFile.close();
}

//===========================================================================================
void writeRingFile(E_ringfiletype ringfiletype) 
{
  DynamicJsonDocument doc(9500);
  char key[10] = "";
  uint8_t slot = 0;
  slot = CalcSlot(ringfiletype);
  if (slot == 99 ) return;  // stop if slot is invalid

  DebugT("update timeslot: ");
  Debugln(slot);
  
  strncpy(key, actTimestamp, 8);
  DebugT("update date: ");
  Debugln(key);
  
  //json openen
  DebugT(F("read(): Ring file "));
  Debugln(RingFiles[ringfiletype].filename);
      
  File RingFile = SPIFFS.open(RingFiles[ringfiletype].filename, "r"); // open for reading  
  if (!RingFiles) {
    DebugT(F("open ring file FAILED!!! --> Bailout\r\n"));
    Debugln(RingFiles[ringfiletype].filename);
    return;
  }

  
  DeserializationError error = deserializeJson(doc, RingFile);
  if (error) {
    DebugT(F("read():Failed to deserialize ringfile"));
    Debugln(RingFiles[ringfiletype].filename);
    return;
  }
  
  DebugT(F("RingFile input <")); serializeJson(doc, Serial);   Serial.println();
  
  RingFile.close();

  // add/replace new value to json object
  doc["actSlot"] = slot;
  //JsonArray data = doc.createNestedArray("data");

  //JsonObject Object = data.createNestedObject(String(slot));
  doc["data"][slot]["date"] = key;
  doc["data"][slot]["values"][0] = (float)DSMRdata.energy_delivered_tariff1;
  doc["data"][slot]["values"][1] = (float)DSMRdata.energy_delivered_tariff2;
  doc["data"][slot]["values"][2] = (float)DSMRdata.energy_returned_tariff1;
  doc["data"][slot]["values"][3] = (float)DSMRdata.energy_returned_tariff2;
  #ifdef USE_PRE40_PROTOCOL
      doc["data"][slot]["values"][4] = (float)DSMRdata.gas_delivered2;
  #else
    doc["data"][slot]["values"][4] = (float)DSMRdata.gas_delivered;
  #endif
 
  DebugT(F("RingFile output >"));serializeJson(doc, Serial);   Serial.println();

  //write json to ring file
  DebugT(F("write(): to ringfile ")); Debugln(RingFiles[ringfiletype].filename);
  
  RingFile = SPIFFS.open(RingFiles[ringfiletype].filename, "w"); // open for writing  
  if (!RingFiles) {
    DebugT(F("writing (%s, 'w') FAILED!!! --> Bailout\r\n"));
    Debugln(RingFiles[ringfiletype].filename);
    return;
  }

  yield();
  writeToJsonFile(doc, RingFile);

} // writeRingFile()

//===========================================================================================
void writeRingFiles() 
{
  // update HOURS
  writeRingFile(RINGHOURS);

  // update DAYS
  writeRingFile(RINGDAYS);

  // update MONTHS
  writeRingFile(RINGMONTHS);

} // writeRingFiles()

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
