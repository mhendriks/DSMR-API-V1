/* 
***************************************************************************  
**  Program  : convertPRD2RING, part of DSMRloggerAPI
**  Version  : v2.0.0
**
**  Copyright (c) 2020 Willem Aandewiel / Martijn Hendriks
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/
//===========================================================================================
void fillRecord(char *record, int8_t len) 
{
  int8_t s = 0, l = 0;
  while (record[s] != '\0' && record[s]  != '\n') {s++;}
  if (Verbose1) DebugTf("Length of record is [%d] bytes\r\n", s);
  for (l = s; l < (len - 2); l++) {
    record[l] = ' ';
  }
  record[l]   = ';';
  record[l+1] = '\n';
  record[len] = '\0';

  while (record[l] != '\0') {l++;}
  if (Verbose1) DebugTf("Length of record is now [%d] bytes\r\n", l);
  
} // fillRecord()

//=====================================================================
void convertPRD2RING()
{
    if (DSMRfileExist(RingFiles[RINGHOURS].filename,  false) )
    {
      SPIFFS.remove(RingFiles[RINGHOURS].filename);
      convertPRDfile(RINGHOURS);
    }
    if (DSMRfileExist(RingFiles[RINGDAYS].filename,   false) )
    {
      SPIFFS.remove(RingFiles[RINGDAYS].filename);
      convertPRDfile(RINGDAYS);
    }
    if (DSMRfileExist(RingFiles[RINGMONTHS].filename, false) )
    {
      SPIFFS.remove(RingFiles[RINGMONTHS].filename);
      convertPRDfile(RINGMONTHS);
    }
    SPIFFS.remove("/!PRDconvert");

} // convertPRD2RING()

//=====================================================================
void convertPRDfile(E_ringfiletype ringfiletype)
{
  char  PRDfileName[30];
  char  buffer[200];
  char  recKey[15];
  float EDT1, EDT2, ERT1, ERT2, GDT;
  int   offSet = 0, maxRecs = RingFiles[ringfiletype].slots;

  Debugln("convertPRDfile() =============================================\r\n");
 
  strCopy(PRDfileName, sizeof(PRDfileName), RingFiles[ringfiletype].filename);
 
  File PRDfile  = SPIFFS.open(PRDfileName, "r");    // open for Read 
  if (!PRDfile) 
  {
    DebugTf("File [%s] does not exist, skip\r\n", PRDfileName);
    return;
  } // if (!dataFile)

  int recLen = PRDfile.readBytesUntil('\n', buffer, sizeof(buffer)) +1;
  DebugTf("recLen[%02d]\r\n", recLen);

  for(int r=maxRecs; r>0; r--)
  {
      offSet = r * recLen;
      DebugTf("offSet[%4d] => ", offSet);
      PRDfile.seek(offSet, SeekSet);
      int l = PRDfile.readBytesUntil('\n', buffer, sizeof(buffer));
      buffer[l] = 0;
      sscanf(buffer, "%[^;]; %f; %f; %f; %f; %f;", recKey, &EDT1, &EDT2, &ERT1, &ERT2, &GDT);
      /*
      DebugTf("values key[%s], EDT1[%8.3f], ERT2[%8.3f], EDT1[%8.3f[, EDT2[%8.3f], GDT[%8.3f]\r\n"
                                                    , recKey
                                                    , EDT1, EDT2
                                                    , ERT1, ERT2
                                                    , GDT);
      */
      Debugf("recKey[%s] --> \r\n", recKey);
      if (isNumericp(recKey, strlen(recKey)))
      {
       // writeToRINGfile(fileType, recKey, EDT1, EDT2, ERT1, ERT2, GDT);
      }
      yield();
  } // for r ..
  
  PRDfile.close();  

} // convertPRDfile()

//=====================================================================
void writeToRINGfile(int8_t fileType, const char *key, float EDT1, float EDT2
                                      , float ERT1, float ERT2, float GDT)
{
  char record[DATA_RECLEN + 1] = "";
  char newKey[15];
  uint16_t recSlot;

  // key is:
  //   hours:  YYMMDDHH concat mmssX
  //    days:  YYMMDD   concat HHmmssX
  //  months:  YYMM     concat DDHHmmssX
  strCopy(newKey, 14, key);
  
  switch(fileType)
  {
    case HOURS:   strConcat(newKey, 14, "0101X");
                  recSlot = CalcSlot(RINGHOURS,newKey);//timestampToHourSlot(newKey,  strlen(newKey));
                  break;
    case DAYS:    strConcat(newKey, 14, "230101X");
                  recSlot = CalcSlot(RINGDAYS,newKey);//timestampToDaySlot(newKey,   strlen(newKey));
                  break;
    case MONTHS:  strConcat(newKey, 14, "01230101X");
                  recSlot = CalcSlot(RINGMONTHS,newKey);//timestampToMonthSlot(newKey, strlen(newKey));
                  break;
                  
  } // switch()

  snprintf(record, sizeof(record), (char*)DATA_FORMAT, newKey , (float)EDT1
                                             , (float)EDT2
                                             , (float)ERT1
                                             , (float)ERT2
                                             , (float)GDT);

  // DATA + \n + \0                                        
  fillRecord(record, DATA_RECLEN);

  if (Verbose2) Debugf("key[%s], slot[%02d] %s\r", newKey, recSlot, record);
  
  switch(fileType)
  {
    case HOURS:   //writeDataToFile(HOURS_FILE,  record, recSlot, HOURS);
                  break;
    case DAYS:    //writeDataToFile(DAYS_FILE,   record, recSlot, DAYS);
                  break;
    case MONTHS:  //writeDataToFile(MONTHS_FILE, record, recSlot, MONTHS);
                  break;
                  
  } // switch()

} // writeToRINGfile()


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
****************************************************************************
*/
