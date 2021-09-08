#include <Arduino.h>

#include "AR488_Store_Tek_4924.h"



/***** AR488_Store_Tek_4924.cpp, ver. 0.05.38, 06/09/2021 *****/
/*
 * Tektronix 4924 Tape Storage functions implementation
 */

#ifdef EN_STORAGE


extern Stream& debugStream;

extern GPIBbus gpibBus;

extern ArduinoOutStream cout;

extern void printHex(char *buffr, int dsize);

CharStream charStream(LINELENGTH);
ArduinoOutStream gpibout(charStream);


/***** Tektronix file types *****/
alphaIndex tekFileTypes [] = {
  { 'A', "ASCII" },
  { 'B', "BINARY" },
  { 'N', "NEW" },
  { 'L', "LAST" }  
};


/***** Tektronix file usages *****/
alphaIndex tekFileUsages [] = {
  { 'P', "PROGRAM" },
  { 'D', "DATA" },
  { 'L', "LOG" },
  { 'T', "TEXT" }
};


/***** Index of accepted storage commands *****/
SDstorage::storeCmdRec SDstorage::storeCmdHidx [] = { 
  { 0x60, &SDstorage::stgc_0x60_h },  // STATUS
  { 0x61, &SDstorage::stgc_0x61_h },  // SAVE
  { 0x62, &SDstorage::stgc_0x62_h },  // CLOSE
  { 0x63, &SDstorage::stgc_0x63_h },  // OPEN
  { 0x64, &SDstorage::stgc_0x64_h },  // OLD/APPEND
  { 0x66, &SDstorage::stgc_0x66_h },  // TYPE
  { 0x67, &SDstorage::stgc_0x67_h },  // KILL
  { 0x69, &SDstorage::stgc_0x69_h },  // HEADER
  { 0x6C, &SDstorage::stgc_0x6C_h },  // PRINT
  { 0x6D, &SDstorage::stgc_0x6D_h },  // INPUT
  { 0x6E, &SDstorage::stgc_0x6E_h },  // READ
  { 0x6F, &SDstorage::stgc_0x6F_h },  // WRITE
  { 0x73, &SDstorage::stgc_0x73_h },  // CD
  { 0x7B, &SDstorage::stgc_0x7B_h },  // FIND
  { 0x7C, &SDstorage::stgc_0x7C_h },  // MARK
  { 0x7D, &SDstorage::stgc_0x7D_h },  // SECRET
  { 0x7E, &SDstorage::stgc_0x7E_h }   // ERROR
};


/**********************************************/
/***** Tek file header handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/


/***** Constructor *****/
TekFileInfo::TekFileInfo(){
  clear();
}


/***** Clear the file information record *****/
void TekFileInfo::clear(){
  fnum = 0;
  ftype = '\0';
  fusage = '\0';
  fsecret = false;
  frecords = 0;
  fsize = 0;
}


/***** Return file number value *****/
uint8_t TekFileInfo::getFnumVal(){
  return fnum;
}


/***** Return file number string*****/
void TekFileInfo::getFnumStr(char * numstr){
  sprintf(numstr, "%-3d", fnum);  // Left justified number
}


/***** Return file type string *****/
void TekFileInfo::getFtype(char * typestr){
  uint8_t i = 0;
  while (tekFileTypes[i].idx) {
    if (tekFileTypes[i].idx == ftype) {
      strcpy(typestr, tekFileTypes[i].desc);
      break;
    }
    i++;
  }
}


/***** Return file usage string *****/
void TekFileInfo::getFusage(char * usagestr){
  uint8_t i = 0;
  while (tekFileUsages[i].idx) {
    if (tekFileUsages[i].idx == fusage) {
      strcpy(usagestr, tekFileUsages[i].desc);
      break;
    }
    i++;
  }
}


/***** Return string containing assigned number of records *****/
void TekFileInfo::getFrecords(char * recordstr){
  sprintf(recordstr, "%d", frecords);
}


/***** Return string contaning the file size *****/
void TekFileInfo::getFsize(char * sizestr){
  sprintf(sizestr, "%-5d", fsize);   
}


/***** Return a file name from the stored file information *****/
void TekFileInfo::getFilename(char * filename){
  char * filenameptr = filename;
  memset(filename, '\0', 46);
  // File number
  getFnumStr(filenameptr);
  filenameptr = filenameptr + 7;
  // File type
  getFtype(filenameptr);
  filenameptr = filenameptr + 8;
  // File usage
  getFusage(filenameptr);
  filenameptr = filenameptr + 10;
  // Secret?
  if (fsecret) strcpy(filenameptr, "SECRET");
  filenameptr = filenameptr + 8;
  // Number of records
  getFrecords(filenameptr);
  // Up to char pos 34 replace NULL with space
  for (uint8_t i=0; i<34; i++) {
    if (filename[i] == 0x00) filename[i] = 0x20;
  }
}


/***** Return a Tek header from the stored file information *****/
void TekFileInfo::getTekHeader(char * header){
  char * headerptr = header;
  memset(header, '\0', 44);
  header[0] = 0x20;
  headerptr++;
  getFnumStr(headerptr);
  header[6] = 0x20;
  headerptr = headerptr + 6;
  getFtype(headerptr);
  headerptr = headerptr + 8;
  getFusage(headerptr);
  headerptr = headerptr + 10;
  if (fsecret) strncpy(headerptr, "SECRET", 8);
  headerptr = headerptr + 8;
  getFrecords(headerptr);
  header[43] = 0x0D;
  header[44] = 0x13;
}


/***** Set file info from filename string *****/
void TekFileInfo::setFromFilename(char * filename){
  char * fptr = filename;
  fnum = (uint8_t)atoi(filename);   // fnum takes values 0 - 255
  ftype = filename[7];
  fusage = filename[15];
  if (filename[25] == 'S') fsecret = true;
  fptr = fptr + 32;
  frecords = (uint8_t)atoi(fptr);   // records can be from 1 - 255
}


/***** Set the file number *****/
bool TekFileInfo::setFnumber(uint8_t filenum ){
  if (filenum < FILES_PER_DIRECTORY) {
    fnum = filenum;
    return true;
  }
  return false;
}


/***** Set the file type *****/
void TekFileInfo::setFtype(char typechar){
  uint8_t i = 0;
  // typechar must match known types
  while (tekFileTypes[i].idx) {
    if (tekFileTypes[i].idx == typechar) {
      ftype = typechar;
      break;
    }
    i++;
  }
}


/***** Set the file usage *****/
void TekFileInfo::setFusage(char usagechar){
  uint8_t i = 0;
  // usagechar must match known usages
  while (tekFileUsages[i].idx) {
    if (tekFileUsages[i].idx == usagechar) {
      fusage = usagechar;
      break;
    }
    i++;
  }
}


/***** Set number of file records *****/
void TekFileInfo::setFrecords(uint16_t records){
  frecords = records;
}


/***** Set the file size *****/
void TekFileInfo::setFsize(size_t filesize){
  fsize = filesize;
  setFrecords(fsizeToRecords(fsize));
}


/***** Set status of SECRET *****/
void TekFileInfo::setFsecret(bool isSecret){
  fsecret = isSecret;
}


/***** Convert filesize to record count *****/
uint16_t TekFileInfo::fsizeToRecords(unsigned long fsize){
  frecords = (fsize/256) + 1;
  return frecords;
}


/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** Tek file header handling functions *****/
/**********************************************/




/*****************************/
/***** Utility functions *****/
/*****vvvvvvvvvvvvvvvvvvv*****/


/***** Convert 4 character hex string to 16-bit unsigned integer *****/
uint16_t SDstorage::hexToDataHeader(char * hexstr) {
  char asciistr[5] = {0};
  // Limit to 4 characters
  for (uint8_t i=0; i<4; i++) {
    asciistr[i] = hexstr[i];
  }
  // Return 16 bit value
  return (uint16_t)strtoul(asciistr, NULL, 16);
}


/*****^^^^^^^^^^^^^^^^^^^*****/
/***** Utility functions *****/
/*****************************/




/**************************************/
/***** SD Card handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/

/***** Contructor - initialise the SD Card object *****/
SDstorage::SDstorage(){
  // Set up CS pin
  pinMode(SDCARD_CS_PIN, OUTPUT);
  digitalWrite(SDCARD_CS_PIN, HIGH);
//  if (!sd.cardBegin(SD_CONFIG)) {
  // Initialise SD card object
  if (!sd.begin(SD_CONFIG)) {
    errorCode = 7;
#ifdef DEBUG_STORE
    debugStream.println(F("SD card initialisation failed!"));
#endif
  }
}


/***** Search for file by number function *****/
/*
 * filenum 1-FILES_PER_DIRECTORY: search for the file number
 * filenum = 0: search for the last file
 * returns true if the file is found
 */
bool SDstorage::searchForFile(uint8_t filenum, File& fileObj){

  File dirObj;
  char fname[file_header_size];

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("searchForFile: searching "));
  debugStream.print(directory);
  debugStream.println(F("..."));  
#endif

  // filenum is always 0 or more (uint8_t), but must be no larger than FILES_PER_DIRECTORY
  if (filenum <= FILES_PER_DIRECTORY){

    if (!dirObj.open(directory, O_RDONLY)) {
      errorCode = 3;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.println(F("searchForFile: failed to open directory!"));
#endif
      return false;
    }

    // Scan through each file in the directory
    while(fileObj.openNext(&dirObj, O_RDONLY)) {

      // Skip directories, hidden files, and null files
      if (!fileObj.isSubDir() && !fileObj.isHidden()) {

        // Retrieve file name
        fileObj.getName(fname, file_header_size);
        // Extract file number
        int num = atoi(fname);
        // Check file number against search parameter

        if (filenum == num) {
#ifdef DEBUG_STORE_COMMANDS
          debugStream.print(F("searchForFile: found file "));
          debugStream.println(filenum);
          debugStream.println(F("searchForFile: done."));
#endif
//          fileObj.close();
          dirObj.close();     
          return true;
        }
      }    
      fileObj.close(); 
    }

    dirObj.close();
    errorCode = 2;

#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("searchForFile: file not found!"));
#endif
  }


#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("searchForFile: done."));
#endif

  return false;
}


/***** Get the number of the last file *****/
uint8_t SDstorage::getLastFile(File& fileObj) {
  File dirObj;
  char fname[file_header_size];

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("getLastFileNum: searching "));
  debugStream.print(directory);
  debugStream.println(F("..."));  
#endif

  if (!dirObj.open(directory, O_RDONLY)) {
    errorCode = 3;
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("getLastFileNum: failed to open directory!"));
#endif
    return 0;
  }

  // Scan through the files in the directory
  while(fileObj.openNext(&dirObj, O_RDONLY)) {

    // Skip directories, hidden files, and null files
    if (!fileObj.isSubDir() && !fileObj.isHidden()) {
      // Retrieve file name
      fileObj.getName(fname, file_header_size);
      // Extract file type
      if (fname[7] == 'L') {
        // Extract file number
        int num = atoi(fname);
        dirObj.close();
#ifdef DEBUG_STORE_COMMANDS
        debugStream.print(F("getLastFileNum: LAST is file number: "));
        debugStream.println(num);
        debugStream.println(F("getLastFileNum: done."));
#endif
        return num;
      }
    }
    fileObj.close();
  }
  dirObj.close();
  errorCode = 2;

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("searchForFile: file not found!"));
  debugStream.println(F("getLastFileNum: done."));
#endif

  return 0;
}


/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** SD Card handling functions *****/
/**************************************/




/**************************************/
/***** Command handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/


/***** Command handler interface *****/
void SDstorage::storeExecCmd(uint8_t cmd) {
  uint8_t i = 0;
  while (storeCmdHidx[i].cmdByte) {
    if (storeCmdHidx[i].cmdByte == cmd) {
#ifdef DEBUG_STORE
      debugStream.print(F("Executing secondary address command: "));
      debugStream.println(cmd, HEX);
#endif
      // Call handler
      (this->*storeCmdHidx[i].handler)();
      return;
    }
    i++;
  }
#ifdef DEBUG_STORE
  debugStream.print(F("Secondary command: "));
  debugStream.print(cmd, HEX);
  debugStream.println(F(" not found!"));
#endif
}


/***** COMMAND HANDLERS *****/

/***** STATUS command *****/
void SDstorage::stgc_0x60_h(){
  char statstr[5] = {0};
#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x60_h: sending status byte..."));
  debugStream.println(errorCode);
#endif
  itoa(gpibBus.cfg.stat, statstr, 10);
  // Send info to GPIB bus
  gpibBus.sendData(statstr, strlen(statstr), DATA_COMPLETE);
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x60_h: done."));
#endif  
}


/***** SAVE command *****/
void SDstorage::stgc_0x61_h(){

  TekFileInfo fileinfo;
  char fname[file_header_size];
  uint32_t fsize = 0;
  uint8_t r = 0;
  File dirObj;
  
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x61_h: started SAVE handler..."));
#endif

  if (dirObj.open(directory, O_RDONLY)) {
    if ((f_type == 'N') || (f_type == 'P')) {  // OR 'P' ?
      // Read data into the file, re-writing its contents
      sdinout.rewind();
      r = gpibBus.receiveToFile(sdinout, true, false, 0);
      if (r == 0) {
        // End the file here
        sdinout.truncate();
        // Make sure file is makrked as ASCII PROG, with appropriate length and rename
        sdinout.getName(fname, file_header_size);
        fileinfo.setFromFilename(fname);
        // Set file parameters
        fileinfo.setFtype('A');
        fileinfo.setFusage('P');
        fileinfo.setFsize(sdinout.fileSize());
        // Read back new filename
        fileinfo.getFilename(fname);
        // Rename the file
        if (sdinout.rename(&dirObj, fname)) {
#ifdef DEBUG_STORE_COMMANDS
          debugStream.println(F("stgc_061_h: filename updated."));
        }else{
          debugStream.println(F("stgc_061_h: failed to update filename!"));           
#endif
        }
//        sdinout.close(); // Finished with file
#ifdef DEBUG_STORE_COMMANDS
      }else{
        debugStream.println(F("stgc_0x61_h: receive failed!"));
#endif
      }
#ifdef DEBUG_STORE_COMMANDS
    }else{
      errorCode = 2;
      debugStream.println(F("stgc_0x61_h: incorrect file type!"));
#endif
    }

  }else{
    errorCode = 3;
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("searchForFile: failed to open directory!"));
#endif      
  }

#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x61_h: done."));
#endif  
}


/***** CLOSE command *****/
void SDstorage::stgc_0x62_h(){
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x62_h: started CLOSE handler..."));
  debugStream.print(F("stgc_0x62_h: closing: "));
  debugStream.print(f_name);
  debugStream.println(F("..."));
#endif
  // Close file handle
  sdinout.close();
  // Clear f_name and f_type
  memset(f_name, '\0', file_header_size);
  f_type = '\0';
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x62_h: done."));
#endif
}


/***** OPEN command *****/
void SDstorage::stgc_0x63_h(){
  
}


/***** OLD/APPEND command (tek_OLD) *****/
void SDstorage::stgc_0x64_h() {
  char path[full_path_size] = {0};
  char linebuffer[line_buffer_size];

  // line_buffer_size = 72 char line max in Tek plus CR plus NULL

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x64_h: started OLD/APPEND handler..."));
#endif

  if (f_type == 'P') {

    strncpy(path, directory, strlen(directory));
    strncat(path, f_name, strlen(f_name));

#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x64_h: reading "));
    debugStream.print(path);
    debugStream.println(F("..."));
#endif

    ifstream sdin(path);

    while (sdin.getline(linebuffer, line_buffer_size, '\r')) {
      // getline() discards CR so add it back on
      strncat(linebuffer, "\r\0", 2);
    
      if (sdin.peek() == EOF) {
        // Last line was read so send data with EOI on last character
        gpibBus.sendData(linebuffer, strlen(linebuffer), DATA_COMPLETE);
      }else{
        // Send line of data to the GPIB bus
        gpibBus.sendData(linebuffer, strlen(linebuffer), DATA_CONTINUE);
      }
    }

    // Close the file
    stgc_0x62_h();  // Close function
    
  }else{
    gpibBus.writeByte(0xFF, true);  // Send FF and signal EOI
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x64_h: incorrect file type!"));
#endif
  }

#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x64_h: done."));
#endif


}


/***** TYPE command *****/
/*
 * 0 = Empty file or file not open 
 * 1 = End of file
 * 2 = ASCII data
 * 3 = Binary numeric data
 * 4 = Binary character string
 */
void SDstorage::stgc_0x66_h() {
  char dtype = 0;
#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x66_h: sending data type..."));
  debugStream.println(errorCode);
#endif

/*
 * Extract type of the next DATA on tape 
 * 0 : Empty (NEW or not open)
 * 1 : End-of-file character
 * 2 : ASCII data (numaric or character sring)
 * 3 : Binary numeric data
 * 4 : Binary character string
 * Note: this is not about the file type but the data in the file
 * 
 * Empty might be indicated by file size = 0
 * Otherwise perhaps need to scan ahead using seek() ?
 */

  // Send info to GPIB bus
  gpibBus.sendData(&dtype, 1, DATA_COMPLETE);
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x66_h: done."));
#endif    
}


/***** KILL command *****/
void SDstorage::stgc_0x67_h(){
  char kbuffer[5] = {0};

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x67_h: started KILL handler..."));
#endif
  
  // Read the directory name data
  gpibBus.receiveParams(false, kbuffer, 12);   // Limit to 12 characters

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x67_h: received parameters: "));
  debugStream.println(kbuffer);
//  printHex(mpbuffer, 12);
#endif

/*

  - find the file to kill (or report file not found error)
  - delete file contents (delete and recreate?)
  - mark file as NEW 

 */


#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x67_h: done."));
#endif
}


/***** HEADER command *****/
void SDstorage::stgc_0x69_h() {
//  char path[full_path_size] = {0};
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x69_h: started HEADER handler..."));
#endif   
//  strcpy(path, directory);
//  strcat(path, f_name);
  gpibBus.sendData(f_name, strlen(f_name), DATA_COMPLETE);

//  Extract filetype and datatype to use for subsequent Tektronix commands.

//  Extract filetype and datatype to use for subsequent Tektronix commands.

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x69_h: done."));
#endif

}


/***** PRINT command *****/
void SDstorage::stgc_0x6C_h() {
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x6C_h: started PRINT handler..."));
#endif
  if (f_type == 'D') {
    gpibBus.receiveToFile(sdinout, true, false, 0);
#ifdef DEBUG_STORE_COMMANDS
  }else{
    debugStream.println(F("stgc_0x6C_h: incorrect file type!"));
#endif
  }
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x6C_h: done."));
#endif
}


/***** INPUT command *****/
/*
 * Reads text files and returns the next item (line at present)
 * Note: EOF character = FF
 */

 /*
  * Working on 4051 but not on 4052/4054
  */

void SDstorage::stgc_0x6D_h() {

  int16_t c;
  uint8_t err = false;

  // line_buffer_size = 72 char line max in Tek plus CR plus NULL

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x6D_h: started INPUT handler..."));
#endif

  if ((f_type == 'D') || (f_type == 'P')) {


#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x6D_h: reading "));
    debugStream.print(directory);
    debugStream.print(f_name);
    debugStream.println(F("..."));
#endif

    while (sdinout.available()) {

      // Read a byte
      c = sdinout.read();
    
      if (c == EOF) {  // Reached EOF
        // Send EOI + 0xFF to indicate EOF reached
        err = gpibBus.writeByte(0xFF, DATA_COMPLETE);
#ifdef DEBUG_STORE_COMMANDS
        debugStream.println(F("\nEOF reached!"));
#endif
      }else{
        // Send byte to the GPIB bus
        err = gpibBus.writeByte((uint8_t)c, DATA_CONTINUE);
#ifdef DEBUG_STORE_COMMANDS
        if (!err) debugStream.print((char)c);
#endif
      }

      // Exit on ATN or receiver request to stop (NDAC HIGH)
      if (err) {
#ifdef DEBUG_STORE_COMMANDS
        if (err == 1) debugStream.println(F("\r\nreceiver requested stop!"));
        if (err == 2) debugStream.println(F("\r\nATN detected."));
#endif
        // Rewind file read by a character (current character has already been read)
        sdinout.seekCur(-1);
        break;
      }

    }

  }else{
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x6D_h: incorrect file type!"));
#endif
  }

#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x6D_h: done."));
#endif

}



/***** READ command (tek_READ_one) *****/
void SDstorage::stgc_0x6E_h() {

// Note: Reads BINARY files

// READ command test

  int16_t c;
  uint8_t err = 0;

  // line_buffer_size = 72 char line max in Tek plus CR plus NULL

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x6E_h: started READ handler..."));
#endif

  if (f_type == 'H') {


#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x6E_h: reading "));
    debugStream.print(directory);
    debugStream.print(f_name);
    debugStream.println(F("..."));
#endif

    while (sdinout.available()) {

      // Read a byte
      c = sdinout.read();
    
      if (sdinout.peek() == EOF) {  // Look ahead for EOF
        // Reached EOF - send last byte with EOI
        err = gpibBus.writeByte(c, DATA_COMPLETE);
//        err = gpibBus.writeByte(0xFF, DATA_COMPLETE);
        debugStream.println(F("\nEOF reached!"));
      }else{
        // Send byte to the GPIB bus
        err = gpibBus.writeByte(c, DATA_CONTINUE);
#ifdef DEBUG_STORE_COMMANDS
        if (!err) {
          char x[4] = {0};
          sprintf(x,"%02X ",c);    
          debugStream.print(x);
        }
#endif
      }

      // Exit on ATN or receiver request to stop (NDAC HIGH)
      if (err) {
#ifdef DEBUG_STORE_COMMANDS
        if (err == 1) debugStream.println(F("\r\nreceiver requested stop!"));
        if (err == 2) debugStream.println(F("\r\nATN detected."));
#endif
        // Set lines to listen ?
        // Rewind file read by a character (current character has already been read)
        sdinout.seekCur(-1);
        break;
      }
      
    }
    
  }else{
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x6E_h: incorrect file type!"));
#endif
  }

#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x6E_h: done."));
#endif

// READ command test




    /*
        Tektronix 4050 DATA Types: ASCII and BINARY
        ASCII DATA
        - ASCII Program lines are delimited with CR (unless changed) and files delimited by FF for EOF
        - ASCII Program DATA numeric items are delimited with Space, Comma, Semicolon, Colon, or CR
        - ASCII Program DATA string items are delimited by leading and trailing Quotation Marks or CR
        - To use a Quotation mark inside a character string requires double quotes: ""Help""
        BINARY DATA
        - Each data item contains its own header identifying the length of the item and the type
        - This two byte Header is generated by the talker sending the binary data
        - The two byte data header has the following form:
        MSB                            LSB   MSB                        LSB
        |T3  T2  T1  L13  L12  L11  L10  L9 | L8  L7  L6  L5  L4  L3  L2  L1|
        |           BYTE 1                  |             BYTE 2            |
        L13 to L1 comprise the length of the item including the two header bytes.
        Since all numbers in Tek 4050 BASIC are 8-byte floating point, the binary number data length = 10
        BINARY data types for READ and WRITE commands
        ----------
        T3, T2, T1 are the types of BINARY data
        0   0   0 Unassigned
        0   0   1 Binary number
        0   1   0 Binary string
        0   1   1 Unassigned
        1   0   0 Unassigned
        1   0   1 Unassigned
        1   1   0 Unassigned
        1   1   1 EOF
        Tek 4050 BASIC "TYP" command returns a value based on next item in an open file:
        TYP    Description
        ---    -----------
        0      Empty File or File Not Open
        1      End of File Character
        2      ASCII Numeric Data or Character String
        3      BINARY Numeric Data
        4      BINARY Character String
    */

/*
  
  switch (f_type) {

    case 'P':
    case 'D':

#ifdef DEBUG_STORE_COMMANDS
//            cout << F("ASCII Data or Program\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("stgc_0x6E_h:  ASCII data or program [type P or D]"));
#endif

        while (sdin.getline(buffr, line_buffer_size, '\r')) {
          // getline() discards CR so add it back on
          strncat(buffr, "\r\0", 2);
    
          if (sdin.peek() == EOF) {
            // Last line was read so send data with EOI on last character
            gpibBus.sendData(buffr, strlen(buffr), DATA_COMPLETE);
          }else{
            // Send line of data to the GPIB bus
            gpibBus.sendData(buffr, strlen(buffr), DATA_CONTINUE);
          }
        }
*/

/*
        sdin.getline(buffr, line_buffer_size, '\r');          // fetch one CR terminated line of the program
//        cout << buffr << '\r';                                // send that line to the serial port

            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, line_buffer_size, '\r');          // fetch one CR terminated line of the program
                        if (sdin.eof()) {
                            cout << "__EOF__" << endl;

                            return;
                        }
                        cout << buffr << '\r';                                // send that line to the serial port

                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;
*/

/*
        case 'B':
#ifdef DEBUG_STORE_COMMANDS
//            cout << F("BINARY PROGRAM\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("BINARY PROGRAM"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif
            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, bin_buffer_size, '\r');
                        if (strlen(buffr) == 0) {
                            cout << "__EOF__" << endl;
                            return;
                        } else {
                            strcpy(buffr2, buffr);
                            strcat(buffr2, "\r"); //add CR back to buffer, this line was CR terminated;
                        }
                        //convert HEX to binary, output HEX then binary for each buffer

                        int i = 0, len = strlen(buffr2), n = 0;
                        //cout << "Buffer= " << len << " \n";
                        char byt = {0};
                        char tmp[3] = {0, 0, 0};

                        //cout << F("\nHEX: ") << buffr << " \n";
                        //cout << " \n";
                        for (i = 0; i < len - 1; i += 2)
                        {
                            tmp[0] = buffr2[i];
                            tmp[1] = buffr2[i + 1];
                            byt = strtoul(tmp, NULL, 16);
                            cout << byt;
                            //fputc(byt, stdout);
                        }
                        cout << "\r";
                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;

        case 'H':
#ifdef DEBUG_STORE_COMMANDS
//            cout << F("BINARY DATA\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("BINARY DATA"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif

            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, bin_buffer_size, '\r');
                        if (strlen(buffr) == 0) {
                            cout << "__EOF__" << endl;
                            return;
                        } else {
                            strcpy(buffr2, buffr);
                            strcat(buffr2, "\r"); //add CR back to buffer, this line was CR terminated;
                        }
                        //convert HEX to binary, output HEX then binary for each buffer

                        int i = 0, len = strlen(buffr2), n = 0;
                        //cout << "Buffer= " << len << " \n";
                        char byt = {0};
                        char tmp[3] = {0, 0, 0};

                        //cout << F("\nHEX: ") << buffr << " \n";
                        //cout << " \n";
                        for (i = 0; i < len - 1; i += 2)
                        {
                            tmp[0] = buffr2[i];
                            tmp[1] = buffr2[i + 1];
                            byt = strtoul(tmp, NULL, 16);
                            cout << byt;
                            //fputc(byt, stdout);
                        }
                        cout << "\r";
                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;

        case 'S':
#ifdef DEBUG_STORE_COMMANDS
//            cout << F("SECRET ASCII PROGRAM\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("SECRET ASCII PROGRAM"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif
            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, bin_buffer_size, '\r');
*/
                        /*
                            if (strlen(buffr) == 0) {
                            cout << "__EOF__" << endl;
                            return;
                            } else { */
/*
                        strcpy(buffr2, buffr);
                        strcat(buffr2, "\r"); //add CR back to buffer, this line was CR terminated;

                        //convert HEX to binary, output HEX then binary for each buffer

                        int i = 0, len = strlen(buffr2), n = 0;
                        //cout << "Buffer= " << len << " \n";
                        char byt = {0};
                        char tmp[3] = {0, 0, 0};

                        //cout << F("\nHEX: ") << buffr << " \n";
                        //cout << " \n";
                        for (i = 0; i < len - 1; i += 2)
                        {
                            tmp[0] = buffr2[i];
                            tmp[1] = buffr2[i + 1];
                            byt = strtoul(tmp, NULL, 16);
                            cout << byt;
                            //fputc(byt, stdout);
                        }
                        cout << "\r";
                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;

        case 'L':
            cout << F("LAST file is EMPTY\n");
            break;

        case 'N':
            cout << F("New file - no data\n");
            break;

        case 'O':
            cout << F("No file is open\n");
            break;
    }
    sdin.close();              //close input stream as we have reached EOF
    f_type = 'O';              //set file type to "File Not Open"

  // Send EOI to end transmission
//  gpibBus.sendEOI();
*/

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x6E_h: done."));
#endif


}


/***** WRITE command *****/
void SDstorage::stgc_0x6F_h() {
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x6F_h: started WRITE handler..."));
#endif
  if (f_type == 'H') {
    gpibBus.receiveToFile(sdinout, true, false, 0); 
#ifdef DEBUG_STORE_COMMANDS
  }else{
    debugStream.println(F("stgc_0x6F_h: incorrect file type!"));
#endif
  }
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x6F_h: done."));
#endif
}


/***** DIRECTORY (CD) command *****/
void SDstorage::stgc_0x73_h(){
/*
 * There is no CD command on the 4051 or 4924
 */
}


/***** FIND command *****/
void SDstorage::stgc_0x7B_h(){
  char receiveBuffer[83] = {0};   // *0 chars + CR/LF + NULL
  char path[full_path_size] = {0};
  uint8_t paramLen = 0;
  uint8_t num = 0;
  char fname[46];
  File findfile;

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7B_h: started FIND handler..."));
#endif

  // Close currently open work file (clears handle, f_name and f_type)
  if (sdinout.isOpen()) stgc_0x62_h(); // Close function

  // Read file number parameter from GPIB
  paramLen = gpibBus.receiveParams(false, receiveBuffer, 83);

  // If we have received parameter data
  if (paramLen > 0) {

#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x7B_h: received parameter: "));
    debugStream.println(receiveBuffer);
#endif

    num = atoi(receiveBuffer);

    /*  FINDs and OPENs file (num)
        Returns: string result= "A" for ASCII, "H" for HEX (Binary or Secret file), "N" for Not Found
        FIND iterates through each file in a directory until filenumber matches num
        since SdFat file index is not sequential with Tek 4050 filenames
    */

    if (searchForFile(num, findfile)) { // Returns filehandle if found
      // Get filename from file handle and store in f_name
      findfile.getName(fname, 46);
      strncpy(f_name, fname, 46);
      findfile.close();
      
      // all BINARY files are in HEX format
      if ((f_name[7] == 'B') && (f_name[15] == 'P')) {
        f_type = 'B';    // BINARY PROGRAM file
      } else if ((f_name[7] == 'B') && (f_name[15] == 'D')) {
        f_type = 'H';    // BINARY DATA file ** to read - parse the data_type
      } else if (f_name[25] == 'S') {  // f_name[25] is location of file type SECRET ASCII PROGRAM
        f_type = 'S';    // SECRET ASCII PROGRAM file
      } else if (f_name[15] == 'P') {  // f_name[15] is location of file type (PROG, DATA,...)
        f_type = 'P';    // ASCII PROGRAM file
      } else if (f_name[7] == 'N') {  // f_name[7] is location of file type (ASCII,BINARY,NEW, or LAST)
        f_type = 'N';    // ASCII LAST file
      } else if (f_name[7] == 'L') {  // f_name[7] is location of file type (ASCII,BINARY,LAST)
        f_type = 'L';    // ASCII LAST file
      } else {
        f_type = 'D';    // ASCII DATA file, also allows other types like TEXT and LOG to be treated as DATA
      }

      if (f_type) {
        strncpy(path, directory, strlen(directory));
        strncat(path, f_name, strlen(f_name));
        sdinout.open(path, O_RDWR); // Open file for reading and writing

#ifdef DEBUG_STORE_COMMANDS
        debugStream.print(F("stgc_0x7B_h: found: "));
        debugStream.println(f_name);
        debugStream.print(F("stgc_0x7B_h: type:  "));
        debugStream.println(f_type);
#endif
      }else{
        // Type undetemined
#ifdef DEBUG_STORE_COMMANDS
        debugStream.println(F("Unknown type!"));
        debugStream.println(F("stgc_0x7B_h: done."));
      }
#endif
      return;      
    }else{
      errorCode = 2;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.print(F("stgc_0x7B_h: file "));
      debugStream.print(num);
      debugStream.println(F(" not found!"));
#endif
    }
#ifdef DEBUG_STORE_COMMANDS
  }else{
    debugStream.println(F("no search criteria!"));
#endif
  }

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7B_h: done."));
#endif  
}


/***** MARK command *****/
void SDstorage::stgc_0x7C_h(){
  char fname[file_header_size] = {0};
  char mpbuffer[13] = {0};
//  char *token;
  uint8_t numfiles = 0;
  uint8_t filenum = 0;
  uint16_t flen = 0;
  File dirObj;
  File markFile;
  TekFileInfo fileinfo;

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_07C_h: started MARK handler..."));
#endif

  if (dirObj.open(directory, O_RDONLY)) {

    // Read the requested number of files
    gpibBus.receiveParams(false, mpbuffer, 12);   // Limit to 12 characters
    numfiles = atoi(mpbuffer);
    // read the requested file size
    gpibBus.receiveParams(false, mpbuffer, 12);   // Limit to 12 characters
    flen = atoi(mpbuffer);

#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_07C_h: Number of files requested: "));
    debugStream.println(numfiles);
    debugStream.print(F("File length (bytes):   "));
    debugStream.println(flen); 
#endif

//    flen = (flen/256) + 1;
#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("File length (records): "));
    debugStream.println(flen); 
#endif

    // Get the number of the LAST file
    filenum = getLastFile(markFile);

#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_07C_h: LAST file no.: "));
    debugStream.println(filenum);
#endif

    if ((filenum > 0) && (numfiles > 0)) {

      // Retrieve file name
      markFile.getName(fname, file_header_size);
      fileinfo.setFromFilename(fname);

      // Renumber LAST to current number + numfiles
      if (fileinfo.setFnumber(filenum + numfiles)) {

        // Construct the new file name
        fileinfo.setFrecords(1);
        fileinfo.getFilename(fname);

        // Rename the file on the SD card
        if (markFile.rename(&dirObj, fname)) {

          markFile.close(); // Finished with LAST file

#ifdef DEBUG_STORE_COMMANDS
          debugStream.print(F("stgc_07C_h: renamed LAST to: "));
          debugStream.println(fname);
#endif
     
          // Create new files and mark them as NEW
          for (uint8_t i=filenum; i<(filenum + numfiles); i++) {
            fileinfo.clear();
            fileinfo.setFnumber(i);
            fileinfo.setFtype('N');
            fileinfo.setFsize(flen);
            fileinfo.getFilename(fname);

debugStream.print(F("Filename bytes:"));
printHex(fname, strlen(fname));
debugStream.println();

            // Create a NEW file with the generated name
            if (markFile.open(&dirObj,fname,(O_RDWR | O_CREAT | O_AT_END))) { // Create new file if it doesn't already exist
//              markFile.sync();
              markFile.close();
#ifdef DEBUG_STORE_COMMANDS
              debugStream.print(F("stgc_07C_h: created: "));
              debugStream.print(directory);
              debugStream.println(fname);
#endif
            }else{
#ifdef DEBUG_STORE_COMMANDS
              debugStream.print(F("Failed to create: "));
              debugStream.println(fname);
#endif
            }

          }
        }
      }
      dirObj.close();
    }else{
      errorCode = 2;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.println(F("stgc_07C_h: LAST not found!"));
#endif
    }  

  }else{
    errorCode = 3;
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("searchForFile: failed to open directory!"));
#endif
  }


#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7C_h: done."));
#endif
}


/***** SECRET command *****/
void SDstorage::stgc_0x7D_h(){
  
}


/***** ERROR command *****/
/*
 *  1 - Domain error / invalid argument
 *  2 - File not found
 *  3 - Mag tape format error
 *  4 - Illegal access
 *  5 - File not open
 *  6 - Read error
 *  7 - No cartridge inserted
 *  8 - Over read (illegal tape length)
 *  9 - Write protected
 * 10 - Read after write error
 * 11 - End of medium
 * 12 - End of file
 */
void SDstorage::stgc_0x7E_h(){
  char errstr[5] = {0};
#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x7E_h: sending error code "));
  debugStream.println(errorCode);
#endif
//  charStream.flush();
//  gpibout << errorCode;
  itoa(errorCode, errstr, 10);
  // Send info to GPIB bus
//  gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
//  gpibBus.sendRawData(errstr, strlen(errstr));
  gpibBus.sendData(errstr, strlen(errstr), DATA_COMPLETE);
  // Signal end of data
//  gpibBus.sendEOI();
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7E_h: done."));
#endif
}



/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** Command handling functions *****/
/**************************************/




/***** TLIST command *****/
/*
  uint8_t number = 1;
  uint8_t index = 0;
  bool lastfile = false;

  SdFile dirFile;
  SdFile file;

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7C_h: started TLIST handler..."));
#endif

  // match the specific Tek4924 tape file number from f_name
//  for (int number = 1; number < nMax; number++) { // find each file in # sequence
//  for (uint8_t number = 1; number < nMax; number++) { // find each file in # sequence
  while ((!lastfile) && (number < nMax)) {

    //cout << "Directory: " << directory << '\r\n';
    f_name[0] = 0;  // clear f_name, otherwise f_name=last filename
    file.close();  //  close any file that could be open (from previous FIND for example)

    if (!dirFile.open(directory, O_RDONLY)) {
//      sd.errorHalt("Open directory failed - possible invalid directory name");
      errorCode = 1;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.print(F("stgc_0x7C_h: open directory '"));
      debugStream.print(directory);
      debugStream.println(F("' failed - name may be invalid"));

//      debugStream << F("This is a ") << number << F(" test!");
#endif
      return;
    }
    file.rewind();

//    for (int index = 0; index < nMax; index++) { //SdFat file index number
//    for (uint8_t index = 0; index < nMax; index++) { //SdFat file index number

    while ((file.openNext(&dirFile, O_RDONLY)) && (index < nMax)) {

      // while (n < nMax ) {
//      file.openNext(&dirFile, O_RDONLY);
      // Skip directories, hidden files, and null files
      if (!file.isSubDir() && !file.isHidden()) {

        file.getName(f_name, 46);

//        int filenumber = atoi(f_name);
        uint8_t filenumber = atoi(f_name);

        if (filenumber == number) {
          // print the entire file 'header' with leading space, and CR + DC3 delimiters
          // Note \x019 = end of medium
//          cout << F(" ") << f_name << '\r' << '\x019';

          // Stream info to buffer 
          charStream.flush();
//          gpibout.flush();
//          charStream << F(" ") << f_name << '\r' << '\x019';
//charStream.print("Test");
        
          gpibout << F(" ") << f_name << '\r' << '\x019';
          
          // Send info to GPIB bus
          gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
//          gpibBus.sendRawData(gpibout.toCharArray(), gpibout.length());
        
        }
        
        if ((f_name[7] == 'L') && (filenumber == number)) {
          // then this is the LAST file - end of TLIST
//          file.close();  // end of iteration close **this** file
//          f_name[0] = 0;  // clear f_name, otherwise f_name=last filename
//          f_type = 'O';    // set file type to "O" for NOT OPEN
//          dirFile.close();  // end of iteration through all files, close directory

#ifdef DEBUG_STORE_COMMANDS
          debugStream.println(F("stgc_0x7C_h: reached last file."));  
#endif    
          lastfile = true;
          break;
//          return;

        } else if (file.isDir()) {
          // Indicate a directory.
//          cout << '/' << endl;

          // Stream info to buffer
          charStream.flush();
//          gpibout.flush();
          gpibout << '/' << endl;
          // Send info to GPIB bus
          gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
//          gpibBus.sendRawData(gpibout.toCharArray(), gpibout.length());

        }

      }
      index++;
      file.close();  // end of iteration close **this** file
    }

    number++;
    f_name[0] = 0;  // clear f_name, otherwise f_name=last filename
    f_type = 'O';    // set file type to "O" for NOT OPEN
    dirFile.close();  // end of iteration through all files, close directory
  } 

  // Send EOI to end transmission
  gpibBus.sendEOI();

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7C_h: end TLIST handler."));  
#endif
*/
/***** TLIST command *****/



#endif // EN_STORAGE
