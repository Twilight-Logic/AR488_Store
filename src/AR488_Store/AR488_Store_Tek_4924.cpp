#include <Arduino.h>

#include "AR488_Store_Tek_4924.h"



/***** AR488_Store_Tek_4924.cpp, ver. 0.05.31, 15/07/2021 *****/
/*
 * Tektronix 4924 Tape Storage functions implementation
 */

#ifdef EN_STORAGE


extern Stream& debugStream;

extern GPIBbus gpibBus;

extern ArduinoOutStream cout;

CharStream charStream(LINELENGTH);
ArduinoOutStream gpibout(charStream);


// Array containing index of accepted storage commands
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

TekFileInfo::TekFileInfo(){
  fnum = 0;
  ftype = '\0';
  fusage = '\0';
  fsecret = false;
  frecords = 0;
}


void TekFileInfo::getFnumber(char * numstr){
  itoa(fnum, numstr, 10);
}


void TekFileInfo::getFtype(char * typestr){
  switch(ftype){
    case 'A':
      strncpy(typestr, "ASCII", 5);
      break;
    case 'B':
      strncpy(typestr, "BINARY", 5);
      break;
    case 'N':
      strncpy(typestr, "NEW", 5);
      break;
    case 'L':
      strncpy(typestr, "LAST", 5);
      break;
  }
}


void TekFileInfo::getFusage(char * usagestr){
  switch(fusage){
    case 'P':
      strncpy(usagestr, "PROGRAM", 7);
      break;
    case 'D':
      strncpy(usagestr, "DATA", 4);
      break;
    case 'L':
      strncpy(usagestr, "LOG", 4);
      break;
    case 'T':
      strncpy(usagestr, "TEXT", 4);
      break;
  }
}


void TekFileInfo::getFrecords(char * recordstr){
  itoa(frecords, recordstr, 10);
}


void TekFileInfo::getFsize(char * sizestr){
  ltoa(fsize, sizestr, 10);   
}


void TekFileInfo::getFilename(char * filename){
  char * filenameptr = filename;
  memset(filename, '\0', 46);
  getFnumber(filenameptr);
  filename[7] = 0x32;
  filenameptr = filenameptr + 7;
  getFtype(filenameptr);
  filenameptr = filenameptr + 8;
  getFtype(filenameptr);
  filenameptr = filenameptr + 10;
  if (fsecret) strncpy(filenameptr, "SECRET", 8);
  filenameptr = filenameptr + 8;
  getFrecords(filenameptr);
}


void TekFileInfo::getTekHeader(char * header){
  char * headerptr = header;
  memset(header, '\0', 44);
  header[0] = 0x32;
  headerptr++;
  getFnumber(headerptr);
  header[7] = 0x32;
  headerptr = headerptr + 7;
  getFtype(headerptr);
  headerptr = headerptr + 8;
  getFtype(headerptr);
  headerptr = headerptr + 10;
  if (fsecret) strncpy(headerptr, "SECRET", 8);
  headerptr = headerptr + 8;
  getFrecords(headerptr);
  header[43] = 0x0D;
  header[44] = 0x13;
}

    
void TekFileInfo::setFnumber(char numstr[7]){
  fnum = atoi(numstr);
}


void TekFileInfo::setFtype(char * typestr){
  switch(typestr[0]){
    case 'A':
    case 'B':
    case 'L':
    case 'N':
      ftype == typestr[0];
      break;
  }    
}


void TekFileInfo::setFusage(char * usagestr){
  switch(usagestr[0]){
    case 'D':
    case 'L':
    case 'P':
    case 'T':
      fusage == usagestr[0];
      break;
  }  
}


void TekFileInfo::setFrecords(char * recordstr){
  frecords = atoi(recordstr);
}


void TekFileInfo::setFsize(char * sizestr){
  fsize = atol(sizestr);
  fsizeToRecords(fsize);
}


void TekFileInfo::setFsecret(bool isSecret){
  fsecret = isSecret;
}


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

void printHex2(char *buffr, int dsize) {
#ifdef DB_SERIAL_ENABLE
  for (int i = 0; i < dsize; i++) {
    debugStream.print(buffr[i], HEX); debugStream.print(" ");
  }
  debugStream.println();
#endif
}


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


/***** File search function *****/
/*
 * filenum 1-FILES_PER_DIRECTORY: search for the file number
 * filenum = 0: search for the last file
 */
bool SDstorage::searchForFile(uint8_t filenum, SdFile *fileobjptr){
  SdFile dirObj;
  SdFile fileObj;
  char fname[file_header_size];

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("searchForFile: searching..."));
#endif

  if (filenum < FILES_PER_DIRECTORY){

    if (!dirObj.open(directory, O_RDONLY)) {
      errorCode = 3;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.print(F("searchForFile: opening directory "));
      debugStream.print(directory);
      debugStream.println(F(" failed!"));
#endif
      return false;    
    }

    for (uint8_t i=0; i<FILES_PER_DIRECTORY; i++) {

debugStream.print(F("loop cnt: "));
debugStream.println(i);

      // Open the next file
      fileObj.openNext(&dirObj, O_RDONLY);



      // Skip directories, hidden files, and null files
      if (!fileObj.isSubDir() && !fileObj.isHidden()) {

debugStream.println(F("notdir"));

        
        // Retrieve file name
        fileObj.getName(fname, file_header_size);
debugStream.println(fname);

        if (filenum > 0) {
          // Extract file number
          int num = atoi(fname);
          // Check file number against search parameter
          if (filenum == num) {
          fileobjptr = &fileObj;
#ifdef DEBUG_STORE_COMMANDS
          debugStream.print(F("searchForFile: found file "));
          debugStream.println(filenum);
          debugStream.println(F("searchForFile: done."));
#endif         
            return true;
          }
        }else{
debugStream.println(F("chklast"));

          // Find the last file
          if (fname[7] == 'L') {
            fileobjptr = &fileObj;
#ifdef DEBUG_STORE_COMMANDS
            debugStream.println(F("searchForFile: found LAST."));
            debugStream.println(F("searchForFile: done."));
#endif            
            return true;
          }
        }
        
      }
       
    }
    
  }
  errorCode = 2;
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("searchForFile: not found!"));
  debugStream.println(F("searchForFile: done."));
#endif
  
  return false;
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
#ifdef DEBUG_STORE
//    debugStream.print(storeCmdHidx[i].cmdByte, HEX);
//    debugStream.print(" ");
#endif
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
  char path[60] = {0};
  char linebuffer[line_buffer_size];
  char c;
  uint8_t i = 0;

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
#ifdef DEBUG_STORE_COMMANDS
    debugStream.println(F("stgc_0x64_h: incorrect file type!"));
#endif
  }

    // cout << '\r'; //add one CR to last line processed to end Tek BASIC program output

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

// Extract type

// Extract type

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
//  printHex2(mpbuffer, 12);
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
  char path[60] = {0};
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
  
}


/***** INPUT command *****/
/*
 * Reads text files and returns the next item (line at present)
 */
void SDstorage::stgc_0x6D_h() {

  char linebuffer[line_buffer_size];
  char c;
  uint8_t i = 0;
  bool err = false;

  // line_buffer_size = 72 char line max in Tek plus CR plus NULL

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x6D_h: started INPUT handler..."));
#endif

  if (f_type == 'D') {


#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x6D_h: reading "));
    debugStream.print(directory);
    debugStream.print(f_name);
    debugStream.println(F("..."));
#endif

    while (sdinout.available()) {

      // Read a byte
      c = sdinout.read();
    
      if (sdinout.peek() == EOF) {
        // Send byte with EOI on last character
        err = gpibBus.writeByte(c, DATA_COMPLETE);
        debugStream.println(F("EOF reached!"));
        // Close file
//        stgc_0x62_h();  // Close function
      }else{
        // Send byte to the GPIB bus
        err = gpibBus.writeByte(c, DATA_CONTINUE);
        debugStream.print(c);
      }

      // Exit on ATN or error (both NDAC and NRFD high)
      if (err) {
        debugStream.println(F("\nSend error!!"));
        // Set lines to listen ?
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

//  char path[60] = {0};
  char buffr[line_buffer_size];
  char buffr2[bin_buffer_size];

  uint16_t dataHeaderValue = 0;
  uint16_t dataLength = 0;
  uint8_t dataType = 0;

  char hexbyte[3] = {0}; 

  bool isHexFormat = true;


#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x6E_h: started READ handler..."));
#endif

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x6E_h: reading "));
  debugStream.print(directory);
  debugStream.print(f_name);
  debugStream.println(F("..."));
#endif

  while (sdinout.available()) {
//  if (sdinout.available()) {
    memset(buffr, '\0', line_buffer_size);
    // Read the binary header (first 2 bytes or 4 hex encoded bytes)
    if (isHexFormat){
      sdinout.read(buffr, 4);
      dataHeaderValue = hexToDataHeader(buffr);
    }else{
      sdinout.read(buffr, 2);
      dataHeaderValue = (buffr[0] * 256) + buffr[1];
    }

    dataType = (uint8_t)(dataHeaderValue >> 13);
    dataLength = (dataHeaderValue & 0x1FFF) - 2; // Reduce by the two header bytes

/*
    debugStream.println(dataHeaderValue, HEX);
    debugStream.println(dataHeaderValue, BIN);

    if (dataType == 1) debugStream.println(F("- binary number"));
    if (dataType == 2) debugStream.println(F("- binary string"));
    if (dataType == 7) debugStream.println(F("- EOF"));  
    debugStream.print(F("Data length: "));
    debugStream.println(dataLength);
*/

    memset(buffr, '\0', line_buffer_size);
    if (isHexFormat) {
      for (uint8_t i=0; i<dataLength; i++){
        sdinout.read(hexbyte, 2);
        buffr[i] = (char)strtoul(hexbyte, NULL, 16);              
      }      
    }else{
      sdinout.read(buffr, dataLength);
    }

    debugStream.println(buffr);


  }






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

}


/***** DIRECTORY (CD) command *****/
void SDstorage::stgc_0x73_h(){
  // limit the emulator dir name to single level "/" plus 8 characters trailing "/" plus NULL
  char dnbuffer[13] = {0};
  uint8_t j = 1;
  
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_073_h: started CD handler..."));
#endif
  
  // Read the directory name data
  gpibBus.receiveParams(false, dnbuffer, 12);   // Limit to 12 characters

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x73_h: received directory name: "));
  debugStream.println(dnbuffer);
//  printHex2(dnbuffer, 12);
#endif
  
  // set global directory = dir
  // directory = dir;

  // Set directory path
  directory[0] = '/';
  for (uint8_t i=0; i<strlen(dnbuffer); i++) {
    // Ignore slashes and line terminators (CR or LF)
    if ((dnbuffer[i] != 0x2F) && (dnbuffer[i] != 0x5C) && (dnbuffer[i] != 0x0A) && (dnbuffer[i] != 0x0D)) {
      // Add character to directory name
      if (j < 11) directory[j] = dnbuffer[i]; // Copy max 10 characters
      j++;
    }
  }
  // Add terminating slash and NULL
  directory[j] = '/';
  directory[j+1] = '\0';

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x73_h: set directory name: "));
  debugStream.println(directory);
//  printHex2(directory, 12);
#endif

  // Close any previous file that was open
  stgc_0x62_h();  // Close function
  
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x73_h: end CD handler."));  
#endif

}


/***** FIND command *****/
void SDstorage::stgc_0x7B_h(){
  char receiveBuffer[83] = {0};   // *0 chars + CR/LF + NULL
  char path[60] = {0};
  uint8_t paramLen = 0;
  uint8_t num = 0;
  char fname[46];
  
  SdFile dirFile;
  SdFile file;

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7B_h: started FIND handler..."));
#endif

  // Close currently open work file (clears handle, f_name and f_type)
//  if (sdinout.is_open()) stgc_0x62_h(); // Close function
  if (sdinout) stgc_0x62_h(); // Close function

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

    if (!dirFile.open(directory, O_RDONLY)) {
      errorCode = 3;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.print(F("stgc_0x7B_h: opening directory "));
      debugStream.print(directory);
      debugStream.println(F(" failed!"));
#endif
      return;    
    }

    for (int index = 0; index < nMax; index++) { //SdFat file index number

      // while (n < nMax ) {
      file.openNext(&dirFile, O_RDONLY);
    
      // Skip directories, hidden files, and null files
      if (!file.isSubDir() && !file.isHidden()) {

        file.getName(fname, 46);

        int filenumber = atoi(fname);

        // We have matched the file number)
        if (filenumber == num) {
          strncpy(f_name, fname, 46);
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

          file.close();     // end of iteration close **this** file handle
          dirFile.close();  // end of iteration through all files, close directory
          
          // File type matched
          if (f_type) {
            
#ifdef DEBUG_STORE_COMMANDS
            debugStream.print(F("stgc_0x7B_h: found: "));
            debugStream.println(f_name);
            debugStream.print(F("stgc_0x7B_h: type:  "));
            debugStream.println(f_type);
#endif
            // Compose the full file path
            strncpy(path, directory, strlen(directory));
            strncat(path, f_name, strlen(f_name));
            // Open found file           
            sdinout.open(path);
          }else{
            // Type undetemined
#ifdef DEBUG_STORE_COMMANDS
            debugStream.println(F("Unknown type!"));
          }
#endif
          debugStream.println(F("stgc_0x7B_h: done."));
          return;
        }

#ifdef DEBUG_STORE_COMMANDS
        // Indicate a directory.
        if (file.isDir()) {
          debugStream.println(F("stgc_0x7B_h: /"));
          return;
        }
#endif

      }
      file.close();  // end of iteration close **this** file
    }
    dirFile.close();  // end of iteration through all files, close directory
    errorCode = 2;
#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x7B_h: file "));
    debugStream.print(num);
    debugStream.println(F(" not found!"));
#endif

  }
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7B_h: done."));
#endif  
}


/***** MARK command *****/
void SDstorage::stgc_0x7C_h(){
//  char path[60] = {0};
  char fname[file_header_size];
  char mpbuffer[13] = {0};
  char *token;
  uint8_t numfiles = 0;
  uint8_t filenum = 0;
  SdFile lastFile;

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_07C_h: started MARK handler..."));
#endif
  
  // Read the directory name data
  gpibBus.receiveParams(false, mpbuffer, 12);   // Limit to 12 characters

  token = strtok(mpbuffer, " \t");
  numfiles = atoi(token);

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_07C_h: Number of files requested: "));
  debugStream.println(numfiles);
#endif

  if (numfiles > 0) {
    // Search for the last file
    if (searchForFile(0, &lastFile)){

      // Retrieve file name
      lastFile.getName(fname, file_header_size);
      filenum = atoi(fname);

#ifdef DEBUG_STORE_COMMANDS
      debugStream.print(F("stgc_07C_h: LAST is file: "));
      debugStream.println(filenum);
#endif

      // Renumber LAST to current number + numfiles
      // Create files and mark as NEW (ignore length)
      
    }else{
      errorCode = 2;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.println(F("stgc_07C_h: LAST not found!"));
#endif
    }
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
