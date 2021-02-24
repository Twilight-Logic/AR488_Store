//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wtype-limits"
//#pragma GCC diagnostic ignored "-Wunused-variable"

#ifdef __AVR__
  #include <avr/wdt.h>
#endif

//#pragma GCC diagnostic pop

#include "AR488_Config.h"
#include "AR488_GPIB.h"
#include "AR488_Layouts.h"
#include "AR488_Eeprom.h"

#ifdef USE_INTERRUPTS
  #ifdef __AVR__
    #include <avr/interrupt.h>
  #endif
#endif

#ifdef E2END
  #include <EEPROM.h>
#endif

#ifdef AR_BT_EN
  #include "AR488_BT.h"
#endif

#ifdef EN_STORAGE
  #ifdef EN_TEK_4924
    #include "AR488_Store_Tek_4924.h"
  #endif
#endif


#ifdef SD_TEST
  #include <SPI.h>
  #include <SD.h>
  #define CHIP_SELECT_PIN 10
#endif




/***** FWVER "AR488 GPIB Storage, ver. 0.04.01, 23/02/2021" *****/

/*
  Arduino IEEE-488 implementation by John Chajecki

  Inspired by the original work of Emanuele Girlando, licensed under a Creative
  Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
  Any code in common with the original work is reproduced here with the explicit
  permission of Emanuele Girlando, who has kindly reviewed and tested this code.

  Thanks also to Luke Mester for comparison testing against the Prologix interface.
  AR488 is Licenced under the GNU Public licence.

  Thanks to 'maxwell3e10' on the EEVblog forum for suggesting additional auto mode
  settings and the macro feature.

  Thanks to 'artag' on the EEVblog forum for providing code for the 32u4.
*/

/*
   Implements most of the CONTROLLER functions;
   Substantially compatible with 'standard' Prologix "++" commands
   (see +savecfg command in the manual for differences)

   Principle of operation:
   - Commands received from USB are buffered and whole terminated lines processed
   - Interface commands prefixed with "++" are passed to the command handler
   - Instrument commands and data not prefixed with '++' are sent directly to the GPIB bus.
   - To receive from the instrument, issue a ++read command or put the controller in auto mode (++auto 1|2)
   - Characters received over the GPIB bus are unbuffered and sent directly to USB
   NOTES:
   - GPIB line in a HIGH state is un-asserted
   - GPIB line in a LOW state is asserted
   - The ATMega processor control pins have a high impedance when set as inputs
   - When set to INPUT_PULLUP, a 10k pull-up (to VCC) resistor is applied to the input
*/

/*
   Standard commands

   ++addr         - display/set device address
?   ++auto         - automatically request talk and read response
   ++eoi          - enable/disable assertion of EOI signal
   ++eos          - specify GPIB termination character
   ++eot_enable   - enable/disable appending user specified character to USB output on EOI detection
   ++eot_char     - set character to append to USB output when EOT enabled
   ++lon          - put controller in listen-only mode (listen to all traffic)
   ++read         - read data from instrument
   ++read_tmo_ms  - read timeout specified between 1 - 3000 milliseconds
   ++rst          - reset the controller
   ++savecfg      - save configration
   ++status       - set the status byte to be returned on being polled (bit 6 = RQS, i.e SRQ asserted)
   ++ver          - display firmware version
*/

/*
   Proprietry commands:

   ++default      - set configuration to controller default settings
   ++id name      - show/set the name of the interface
   ++id serial    - show/set the serial number of the interface
   ++id verstr    - show/set the version string (replaces setvstr)
   ++idn          - enable/disable reply to *idn? (disabled by default)
   ++setvstr      - set custom version string (to identify controller, e.g. "GPIB-USB"). Max 47 chars, excess truncated.
   ++ton          - put controller in talk-only mode (send data only)
   ++verbose      - verbose (human readable) mode
*/

/*
   NOT YET IMPLEMENTED

   ++help     - show summary of commands
*/

/*
   For information regarding the GPIB firmware by Emanualle Girlando see:
   http://egirland.blogspot.com/2014/03/arduino-uno-as-usb-to-gpib-controller.html
*/


/*
   Pin mapping between the Arduino pins and the GPIB connector.
   NOTE:
   GPIB pins 10 and 18-24 are connected to GND
   GPIB pin 12 should be connected to the cable shield (might be n/c)
   Pin mapping follows the layout originally used by Emanuelle Girlando, but adds
   the SRQ line (GPIB 10) on pin 2 and the REN line (GPIB 17) on pin 13. The program
   should therefore be compatible with the original interface design but for full
   functionality will need the remaining two pins to be connected.
   For further information about the AR488 see:
*/


/*********************************/
/***** CONFIGURATION SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvv *****/
// SEE >>>>> Config.h <<<<<
/***** ^^^^^^^^^^^^^^^^^^^^^ *****/
/***** CONFIGURATION SECTION *****/
/*********************************/


/***************************************/
/***** MACRO CONFIGURATION SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvvvv *****/
// SEE >>>>> Config.h <<<<<
/***** ^^^^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** MACRO CONFIGURATION SECTION *****/
/***************************************/


/*************************************/
/***** MACRO STRUCTRURES SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvv *****/
#ifdef USE_MACROS

/*** DO NOT MODIFY ***/
/*** vvvvvvvvvvvvv ***/

/***** STARTUP MACRO *****/
const char startup_macro[] PROGMEM = {MACRO_0};

/***** Consts holding USER MACROS 1 - 9 *****/
const char macro_1 [] PROGMEM = {MACRO_1};
const char macro_2 [] PROGMEM = {MACRO_2};
const char macro_3 [] PROGMEM = {MACRO_3};
const char macro_4 [] PROGMEM = {MACRO_4};
const char macro_5 [] PROGMEM = {MACRO_5};
const char macro_6 [] PROGMEM = {MACRO_6};
const char macro_7 [] PROGMEM = {MACRO_7};
const char macro_8 [] PROGMEM = {MACRO_8};
const char macro_9 [] PROGMEM = {MACRO_9};


/* Macro pointer array */
const char * const macros[] PROGMEM = {
  startup_macro,
  macro_1,
  macro_2,
  macro_3,
  macro_4,
  macro_5,
  macro_6,
  macro_7,
  macro_8,
  macro_9
};

/*** ^^^^^^^^^^^^^ ***/
/*** DO NOT MODIFY ***/

#endif
/***** ^^^^^^^^^^^^^^^^^^^^ *****/
/***** MACRO CONFIG SECTION *****/
/********************************/



/**********************************/
/***** SERIAL PORT MANAGEMENT *****/
/***** vvvvvvvvvvvvvvvvvvvvvv *****/


/***** Default serial port *****/
#ifdef AR_CDC_SERIAL
  Serial_ *arSerial = &(AR_SERIAL_PORT);
  #ifndef DB_SERIAL_PORT
    Serial_ *dbSerial = arSerial;
  #endif
#endif
#ifdef AR_HW_SERIAL
  HardwareSerial *arSerial = &(AR_SERIAL_PORT);
  #ifndef DB_SERIAL_PORT
    HardwareSerial *dbSerial = arSerial;
  #endif
#endif
#ifdef AR_SW_SERIAL
  #include <SoftwareSerial.h>
  SoftwareSerial swArSerial(AR_SW_SERIAL_RX, AR_SW_SERIAL_TX);
  SoftwareSerial *arSerial = &swArSerial;
  #ifndef DB_SERIAL_PORT
    SoftwareSerial *dbSerial = arSerial;
  #endif
#endif


/***** Debug Port *****/
#ifdef DB_SERIAL_PORT
  #ifdef DB_CDC_SERIAL
    Serial_ *dbSerial = &(DB_SERIAL_PORT);
  #endif
  #ifdef DB_HW_SERIAL
    HardwareSerial *dbSerial = &(DB_SERIAL_PORT);
  #endif
  // Note: SoftwareSerial support conflicts with PCINT support
  #ifdef DB_SW_SERIAL
    #include <SoftwareSerial.h>
    SoftwareSerial swDbSerial(DB_SW_SERIAL_RX, DB_SW_SERIAL_TX);
    SoftwareSerial *dbSerial = &swDbSerial;
  #endif
#endif


/***** PARSE BUFFERS *****/
/*
 * Note: Ardiono serial input buffer is 64 
 */
// Serial input parsing buffer
static const uint8_t PBSIZE = 128;
char pBuf[PBSIZE];
uint8_t pbPtr = 0;


/***** ^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** SERIAL PORT MANAGEMENT *****/
/**********************************/



/************************************/
/***** COMMON VARIABLES SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvv *****/

/****** Process status values *****/
#define OK 0
#define ERR 1

/***** Control characters *****/
#define ESC  0x1B   // the USB escape char
#define CR   0xD    // Carriage return
#define LF   0xA    // Newline/linefeed
#define PLUS 0x2B   // '+' character



union AR488conf AR488;



/****** Global variables with volatile values related to controller state *****/

// GPIB control state
uint8_t cstate = 0;

// Verbose mode
bool isVerb = false;

// CR/LF terminated line ready to process
uint8_t lnRdy = 0;      

// GPIB data receive flags
//bool isReading = false; // Is a GPIB read in progress?
bool aRead = false;     // GPIB data read in progress
bool rEoi = false;      // Read eoi requested
bool rEbt = false;      // Read with specified terminator character
bool isQuery = false;   // Direct instrument command is a query
uint8_t tranBrk = 0;    // Transmission break on 1=++, 2=EOI, 3=ATN 4=UNL
uint8_t eByte = 0;      // Termination character

// Device mode - send data
bool snd = false;

// Escaped character flag
bool isEsc = false;           // Charcter escaped
bool isPlusEscaped = false;   // Plus escaped

// Read only mode flag
bool isRO = false;

// Talk only mode flag
bool isTO = false;

// GPIB command parser
bool aTt = false;
bool aTl = false;

// Data send mode flags
bool deviceAddressing = true;   // Suppress sending commands to address the instrument
bool dataBufferFull = false;    // Flag when parse buffer is full

// State flags set by interrupt being triggered
extern volatile bool isATN;  // has ATN been asserted?
extern volatile bool isSRQ;  // has SRQ been asserted?

// SRQ auto mode
bool isSrqa = false;

// Interrupt without handler fired
//volatile bool isBAD = false;

// Whether to run Macro 0 (macros must be enabled)
uint8_t runMacro = 0;

// Send response to *idn?
bool sendIdn = false;


#ifdef EN_STORAGE
  // SD card storage
SDstorage storage;
#endif

#ifdef SD_TEST
    Sd2Card sdcard;
    SdVolume sdvolume;
    SdFile sdroot;
#endif

/***** ^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** COMMON VARIABLES SECTION *****/
/************************************/



/*******************************/
/***** COMMON CODE SECTION *****/
/***** vvvvvvvvvvvvvvvvvvv *****/


/******  Arduino standard SETUP procedure *****/
void setup() {

  // Disable the watchdog (needed to prevent WDT reset loop)
#ifdef __AVR__
  wdt_disable();
#endif

  // Turn off internal LED (set OUPTUT/LOW)
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

#ifdef USE_INTERRUPTS
  // Turn on interrupts on port
  interruptsEn();
#endif

  // Initialise parse buffer
  flushPbuf();

// Initialise debug port
#ifdef DB_SERIAL_PORT
  if (dbSerial != arSerial) dbSerial->begin(DB_SERIAL_BAUD);
#endif

 // Initialise serial comms over USB or Bluetooth
#ifdef AR_BT_EN
  // Initialise Bluetooth  
  btInit();
  arSerial->begin(AR_BT_BAUD);
#else
  // Start the serial port
  arSerial->begin(AR_SERIAL_BAUD);
#endif

/*
// Enable storage device
#ifdef EN_STORAGE

#endif
*/

// Un-comment for diagnostic purposes
/* 
  #if defined(__AVR_ATmega32U4__)
    while(!*arSerial)
    ;
//    Serial.print(F("Starting "));
    for(int i = 0; i < 20; ++i) {  // this gives you 10 seconds to start programming before it crashes
      Serial.print(".");
      delay(500);
    }
    Serial.println("@>");
  #endif // __AVR_ATmega32U4__
*/
// Un-comment for diagnostic purposes


  // Initialise
  initAR488();

#ifdef E2END
  // Read data from non-volatile memory
  //(will only read if previous config has already been saved)
//  epGetCfg();
  if (!isEepromClear()) {
    if (!epReadData(AR488.db, AR_CFG_SIZE)) {
      // CRC check failed - config data does not match EEPROM
      epErase();
      initAR488();
      epWriteData(AR488.db, AR_CFG_SIZE);
    }
  }
#endif

  // SN7516x IC support
#ifdef SN7516X
  pinMode(SN7516X_TE, OUTPUT);
  #ifdef SN7516X_DC
    pinMode(SN7516X_DC, OUTPUT);
  #endif
  // Set listen mode on SN75161/2 (default)
  digitalWrite(SN7516X_TE, HIGH);
  #ifdef SN7516X_DC
    digitalWrite(SN7516X_DC, HIGH);
  #endif
  #ifdef SN7516X_SC
    digitalWrite(SN7516X_SC, LOW);
  #endif
#endif

  // Initialize the interface in device mode
  initDevice();

  isATN = false;
//  isSRQ = false;

#if defined(USE_MACROS) && defined(RUN_STARTUP)
  // Run startup macro
  execMacro(0);
#endif


#ifdef SD_TEST
if (sdcard.init(SPI_HALF_SPEED, CHIP_SELECT_PIN)) {
  Serial.println(F("SD card initialised."));
}else{
  Serial.println(F("SD card init failed!"));
}
#endif


#ifdef SAY_HELLO
  arSerial->println(F("AR488 ready."));
#endif

}
/****** End of Arduino standard SETUP procedure *****/


/***** ARDUINO MAIN LOOP *****/
void loop() {

/*** Macros ***/
/*
 * Run the startup macro if enabled
 */
#ifdef USE_MACROS
  // Run user macro if flagged
  if (runMacro > 0) {
    execMacro(runMacro);
    runMacro = 0;
  }
#endif

/*** Pin Hooks ***/
/*
 * Not all boards support interrupts or have PCINTs. In this
 * case, use in-loop checking to detect when SRQ and ATN have 
 * been signalled
 */
#ifndef USE_INTERRUPTS
  isATN = (digitalRead(ATN)==LOW ? true : false);
  isSRQ = (digitalRead(SRQ)==LOW ? true : false);
#endif

/*** Process the buffer ***/
/* Each received char is passed through parser until an un-escaped 
 * CR is encountered. If we have a command then parse and execute.
 * If the line is data (inclding direct instrument commands) then
 * send it to the instrument.
 * NOTE: parseInput() sets lnRdy in serialEvent, readBreak or in the
 * above loop
 * lnRdy=1: process command;
 * lnRdy=2: send data to Gpib
 */

  // lnRdy=1: received a command so execute it...
  if (lnRdy == 1) {
    execCmd(pBuf, pbPtr);
  }

  // Device mode:
  if (isTO) {
    if (lnRdy == 2) sendToInstrument(pBuf, pbPtr);
  }else if (isRO) {
    lonMode();
  }else{
    if (isATN) attnRequired();
    if (lnRdy == 2) sendToInstrument(pBuf, pbPtr);
  }

  // Reset line ready flag
//  lnRdy = 0;

  // IDN query ?
  if (sendIdn) {
    if (AR488.idn==1) arSerial->println(AR488.sname);
    if (AR488.idn==2) {arSerial->print(AR488.sname);arSerial->print("-");arSerial->println(AR488.serial);}
    sendIdn = false;
  }

  // Check serial buffer
  lnRdy = serialIn_h();
  
  delayMicroseconds(5);
}
/***** END MAIN LOOP *****/


/***** Initialise the interface *****/
void initAR488() {
  // Set default values ({'\0'} sets version string array to null)
//  AR488 = {false, false, 1, 0, 1, 0, 0, 0, 0, 1200, 0, {'\0'}, 0, 0, {'\0'}, 0, 0};
  AR488 = {false, false, 0, 1, 0, 0, 0, 0, 1200, 0, {'\0'}, 0, 0, {'\0'}, 0, 0};
}


/***** Initialise device mode *****/
void initDevice() {
  // Set GPIB control bus to device idle mode
  setGpibControls(DINI);

  // Initialise GPIB data lines (sets to INPUT_PULLUP)
  readGpibDbus();
}


/***** Serial event handler *****/
/*
 * Note: the Arduino serial buffer is 64 characters long. Characters are stored in
 * this buffer until serialEvent_h() is called. parsedInput() takes a character at 
 * a time and places it into the 256 character parse buffer whereupon it is parsed
 * to determine whether a command or data are present.
 * lnRdy=0: terminator not detected yet
 * lnRdy=1: terminator detected, sequence in parse buffer is a ++ command
 * lnRdy=2: terminator detected, sequence in parse buffer is data or direct instrument command
 */ 
uint8_t serialIn_h() {
  uint8_t bufferStatus = 0;
  // Parse serial input until we have detected a line terminator
  while (arSerial->available() && bufferStatus==0) {   // Parse while characters available and line is not complete
    bufferStatus = parseInput(arSerial->read());
  }

#ifdef DEBUG1
  if (bufferStatus) {
    dbSerial->print(F("BufferStatus: "));
    dbSerial->println(bufferStatus);  
  }
#endif

  return bufferStatus;
}


/*************************************/
/***** Device operation routines *****/
/*************************************/


/***** Unrecognized command *****/
void errBadCmd() {
  arSerial->println(F("Unrecognized command"));
}


/***** Add character to the buffer and parse *****/
uint8_t parseInput(char c) {

  uint8_t r = 0;

  // Read until buffer full (buffer-size - 2 characters)
  if (pbPtr < PBSIZE) {
    if (isVerb) arSerial->print(c);  // Humans like to see what they are typing...
    // Actions on specific characters
    switch (c) {
      // Carriage return or newline? Then process the line
      case CR:
      case LF:
        // If escaped just add to buffer
        if (isEsc) {
          addPbuf(c);
          isEsc = false;
        } else {
          // Carriage return on blank line?
          // Note: for data CR and LF will always be escaped
          if (pbPtr == 0) {
            flushPbuf();
            if (isVerb) showPrompt();
            return 0;
          } else {
            if (isVerb) arSerial->println();  // Move to new line
#ifdef DEBUG1
            dbSerial->print(F("parseInput: Received ")); dbSerial->println(pBuf);
#endif
            // Buffer starts with ++ and contains at least 3 characters - command?
            if (pbPtr>2 && isCmd(pBuf) && !isPlusEscaped) {
              // Exclamation mark (break read loop command)
              if (pBuf[2]==0x21) {
                r = 3;
                flushPbuf();
              // Otherwise flag command received and ready to process 
              }else{
                r = 1;
              }
            // Buffer contains *idn? query and interface to respond
            }else if (pbPtr>3 && AR488.idn>0 && isIdnQuery(pBuf)){
              sendIdn = true;
              flushPbuf();
            // Buffer has at least 1 character = instrument data to send to gpib bus
            }else if (pbPtr > 0) {
              r = 2;
            }
            isPlusEscaped = false;
#ifdef DEBUG1
            dbSerial->print(F("R: "));dbSerial->println(r);
#endif
//            return r;
          }
        }
        break;
      case ESC:
        // Handle the escape character
        if (isEsc) {
          // Add character to buffer and cancel escape
          addPbuf(c);
          isEsc = false;
        } else {
          // Set escape flag
          isEsc  = true;  // Set escape flag
        }
        break;
      case PLUS:
        if (isEsc) {
          isEsc = false;
          if (pbPtr < 2) isPlusEscaped = true;
        }
        addPbuf(c);
//        if (isVerb) arSerial->print(c);
        break;
      // Something else?
      default: // any char other than defined above
//        if (isVerb) arSerial->print(c);  // Humans like to see what they are typing...
        // Buffer contains '++' (start of command). Stop sending data to serial port by halting GPIB receive.
        addPbuf(c);
        isEsc = false;
    }
  }
  if (pbPtr >= PBSIZE) {
    if (isCmd(pBuf) && !r) {  // Command without terminator and buffer full
      if (isVerb) {
        arSerial->println(F("ERROR - Command buffer overflow!"));
      }
      flushPbuf();
    }else{  // Buffer contains data and is full, so process the buffer (send data via GPIB)
      dataBufferFull = true;
      r = 2;
    }
  }
  return r;
}


/***** Is this a command? *****/
bool isCmd(char *buffr) {
  if (buffr[0] == PLUS && buffr[1] == PLUS) {
#ifdef DEBUG1
    dbSerial->println(F("isCmd: Command detected."));
#endif
    return true;
  }
  return false;
}


/***** Is this an *idn? query? *****/
bool isIdnQuery(char *buffr) {
//  if (buffr[0] == PLUS && buffr[1] == PLUS) {
  if (strncmp(buffr, "*idn?", 5)==0) {
#ifdef DEBUG1
    dbSerial->println(F("isIdnQuery: Detected IDN query."));
#endif
    return true;
  }
  return false;
}


/***** ++read command detected? *****/
bool isRead(char *buffr) {
  char cmd[4];
  // Copy 2nd to 5th character
  for (int i = 2; i < 6; i++) {
    cmd[i - 2] = buffr[i];
  }
  // Compare with 'read'
  if (strncmp(cmd, "read", 4) == 0) return true;
  return false;
}


/***** Add character to the buffer *****/
void addPbuf(char c) {
  pBuf[pbPtr] = c;
  pbPtr++;
}


/***** Clear the parse buffer *****/
void flushPbuf() {
  memset(pBuf, '\0', PBSIZE);
  pbPtr = 0;
}


struct cmdRec { 
  const char* token; 
  void (*handler)(char *);
};


static cmdRec cmdHidx [] = { 
 
  { "addr",        addr_h      }, 
  { "auto",        amode_h     },
  { "default",     (void(*)(char*)) default_h },
  { "eoi",         eoi_h       },
  { "eor",         eor_h       },
  { "eos",         eos_h       },
  { "eot_char",    eot_char_h  },
  { "eot_enable",  eot_en_h    },
  { "id",          id_h        },
  { "idn",         idn_h       },
  { "lon",         lon_h       },
  { "macro",       macro_h     },
  { "read",        read_h      },
  { "read_tmo_ms", rtmo_h      },
  { "repeat",      repeat_h    },
  { "rst",         (void(*)(char*)) rst_h     },
  { "savecfg",     (void(*)(char*)) save_h    },
  { "setvstr",     setvstr_h   },
  { "status",      stat_h      },
#ifdef EN_STORAGE
  { "storage",     store_h     },
#endif
  { "ton",         ton_h       },
  { "ver",         ver_h       },
  { "verbose",     (void(*)(char*)) verb_h    },
  { "tmbus",       tmbus_h     },
  { "xdiag",       xdiag_h     }
};



/***** Show a prompt *****/
void showPrompt() {
  // Print prompt
  arSerial->println();
  arSerial->print("> ");
}


/****** Send data to instrument *****/
/* Processes the parse buffer whenever a full CR or LF
 * and sends data to the instrument
 */
void sendToInstrument(char *buffr, uint8_t dsize) {

#ifdef DEBUG1
  dbSerial->print(F("sendToInstrument: Received for sending: ")); printHex(buffr, dsize);
#endif

  // Is this an instrument query command (string ending with ?)
  if (buffr[dsize-1] == '?') isQuery = true;

  // Send string to instrument
  gpibSendData(buffr, dsize);
  // Clear data buffer full flag
  if (dataBufferFull) dataBufferFull = false;

  // Show a prompt on completion?
  if (isVerb) showPrompt();

  // Flush the parse buffer
  flushPbuf();
  lnRdy = 0;

#ifdef DEBUG1
  dbSerial->println(F("sendToInstrument: Sent."));
#endif

}


/***** Execute a command *****/
void execCmd(char *buffr, uint8_t dsize) {
  char line[PBSIZE];

  // Copy collected chars to line buffer
  memcpy(line, buffr, dsize);

  // Flush the parse buffer
  flushPbuf();
  lnRdy = 0;

#ifdef DEBUG1
  dbSerial->print(F("execCmd: Command received: ")); printHex(line, dsize);
#endif

  // Its a ++command so shift everything two bytes left (ignore ++) and parse
  for (int i = 0; i < dsize-2; i++) {
    line[i] = line[i + 2];
  }
  // Replace last two bytes with a null (\0) character
  line[dsize - 2] = '\0';
  line[dsize - 1] = '\0';
#ifdef DEBUG1
  dbSerial->print(F("execCmd: Sent to the command processor: ")); printHex(line, dsize-2);
#endif
  // Execute the command
  if (isVerb) arSerial->println(); // Shift output to next line
  getCmd(line);

  // Show a prompt on completion?
  if (isVerb) showPrompt();
}


/***** Extract command and pass to handler *****/
void getCmd(char *buffr) {

  char *token;  // Pointer to command token
  char *params; // Pointer to parameters (remaining buffer characters)
  
  int casize = sizeof(cmdHidx) / sizeof(cmdHidx[0]);
  int i = 0;

#ifdef DEBUG1
  dbSerial->print("getCmd: ");
  dbSerial->print(buffr); dbSerial->print(F(" - length:")); dbSerial->println(strlen(buffr));
#endif

  // If terminator on blank line then return immediately without processing anything 
  if (buffr[0] == 0x00) return;
  if (buffr[0] == CR) return;
  if (buffr[0] == LF) return;

  // Get the first token
  token = strtok(buffr, " \t");

#ifdef DEBUG1
  dbSerial->print("getCmd: process token: "); dbSerial->println(token);
#endif

  // Check whether it is a valid command token
  i = 0;
  do {
    if (strcasecmp(cmdHidx[i].token, token) == 0) break;
    i++;
  } while (i < casize);

  if (i < casize) {
    // We have found a valid command and handler
#ifdef DEBUG1
    dbSerial->print("getCmd: found handler for: "); dbSerial->println(cmdHidx[i].token);
#endif
    // If command is found then execute it
    // If its a command with parameters
    // Copy command parameters to params and call handler with parameters
    params = token + strlen(token) + 1;
  
    // If command parameters were specified
    if (strlen(params) > 0) {
#ifdef DEBUG1
      dbSerial->print(F("Calling handler with parameters: ")); dbSerial->println(params);
#endif
      // Call handler with parameters specified
      cmdHidx[i].handler(params);
        
    }else{
      // Call handler without parameters
      cmdHidx[i].handler(NULL);
    }
  } else {
    // No valid command found
    errBadCmd();
  }
}


/***** Prints charaters as hex bytes *****/
void printHex(char *buffr, int dsize) {
  for (int i = 0; i < dsize; i++) {
    dbSerial->print(buffr[i], HEX); dbSerial->print(" ");
  }
  dbSerial->println();
}


/***** Check whether a parameter is in range *****/
/* Convert string to integer and check whether value is within
 * lowl to higl inclusive. Also returns converted text in param
 * to a uint16_t integer in rval. Returns true if successful, 
 * false if not
*/
bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval) {

  // Null string passed?
  if (strlen(param) == 0) return true;

  // Convert to integer
  rval = 0;
  rval = atoi(param);

  // Check range
  if (rval < lowl || rval > higl) {
    errBadCmd();
    if (isVerb) {
      arSerial->print(F("Valid range is between "));
      arSerial->print(lowl);
      arSerial->print(F(" and "));
      arSerial->println(higl);
    }
    return true;
  }
  return false;
}


/***** If enabled, executes a macro *****/
#ifdef USE_MACROS
void execMacro(uint8_t idx) {
  char c;
  const char * macro = pgm_read_word(macros + idx);
  int ssize = strlen_P(macro);

  // Read characters from macro character array
  for (int i = 0; i < ssize; i++) {
    c = pgm_read_byte_near(macro + i);
    if (c == CR || c == LF || i == (ssize - 1)) {
      // Reached last character before NL. Add to buffer before processing
      if (i == ssize-1) {
        // Check buffer and add character
        if (pbPtr < (PBSIZE - 2)){
          addPbuf(c);
        }else{
          // Buffer full - clear and exit
          flushPbuf();
          return;
        }
      }
      if (isCmd(pBuf)){
        execCmd(pBuf, strlen(pBuf));
      }else{
        sendToInstrument(pBuf, strlen(pBuf));
      }
      // Done - clear the buffer
      flushPbuf();
    } else {
      // Check buffer and add character
      if (pbPtr < (PBSIZE - 2)) {
        addPbuf(c);
      } else {
        // Exceeds buffer size - clear buffer and exit
        i = ssize;
        return;
      }
    }
  }

  // Clear the buffer ready for serial input
  flushPbuf();
}
#endif


/*************************************/
/***** STANDARD COMMAND HANDLERS *****/
/*************************************/

/***** Show or change device address *****/
void addr_h(char *params) {
  //  char *param, *stat;
  char *param;
  uint16_t val;
  if (params != NULL) {

    // Primary address
    param = strtok(params, " \t");
    if (notInRange(param, 1, 30, val)) return;
    if (val == AR488.caddr) {
      errBadCmd();
      if (isVerb) arSerial->println(F("That is my address! Address of a remote device is required."));
      return;
    }
    AR488.paddr = val;
    if (isVerb) {
      arSerial->print(F("Set device primary address to: "));
      arSerial->println(val);
    }

    // Secondary address
    AR488.saddr = 0;
    val = 0;
    param = strtok(NULL, " \t");
    if (param != NULL) {
      if (notInRange(param, 96, 126, val)) return;
      AR488.saddr = val;
      if (isVerb) {
        arSerial->print("Set device secondary address to: ");
        arSerial->println(val);
      }
    }

  } else {
    arSerial->print(AR488.paddr);
    if (AR488.saddr > 0) {
      arSerial->print(F(" "));
      arSerial->print(AR488.saddr);
    }
    arSerial->println();
  }
}


/***** Show or set read timout *****/
void rtmo_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 1, 32000, val)) return;
    AR488.rtmo = val;
    if (isVerb) {
      arSerial->print(F("Set [read_tmo_ms] to: "));
      arSerial->print(val);
      arSerial->println(F(" milliseconds"));
    }
  } else {
    arSerial->println(AR488.rtmo);
  }
}


/***** Show or set end of send character *****/
void eos_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val)) return;
    AR488.eos = (uint8_t)val;
    if (isVerb) {
      arSerial->print(F("Set EOS to: "));
      arSerial->println(val);
    };
  } else {
    arSerial->println(AR488.eos);
  }
}


/***** Show or set EOI assertion on/off *****/
void eoi_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    AR488.eoi = val ? true : false;
    if (isVerb) {
      arSerial->print(F("Set EOI assertion: "));
      arSerial->println(val ? "ON" : "OFF");
    };
  } else {
    arSerial->println(AR488.eoi);
  }
}


/***** Show or enable/disable sending of end of transmission character *****/
void eot_en_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    AR488.eot_en = val ? true : false;
    if (isVerb) {
      arSerial->print(F("Appending of EOT character: "));
      arSerial->println(val ? "ON" : "OFF");
    }
  } else {
    arSerial->println(AR488.eot_en);
  }
}


/***** Show or set end of transmission character *****/
void eot_char_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 255, val)) return;
    AR488.eot_ch = (uint8_t)val;
    if (isVerb) {
      arSerial->print(F("EOT set to ASCII character: "));
      arSerial->println(val);
    };
  } else {
    arSerial->println(AR488.eot_ch, DEC);
  }
}


/***** Show or enable/disable auto mode *****/
void amode_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val)) return;
    if (val > 0 && isVerb) {
      arSerial->println(F("WARNING: automode ON can cause some devices to generate"));
      arSerial->println(F("         'addressed to talk but nothing to say' errors"));
    }
    AR488.amode = (uint8_t)val;
    if (AR488.amode < 3) aRead = false;
    if (isVerb) {
      arSerial->print(F("Auto mode: "));
      arSerial->println(AR488.amode);
    }
  } else {
    arSerial->println(AR488.amode);
  }
}


/***** Display the controller version string *****/
void ver_h(char *params) {
  // If "real" requested
  if (params != NULL && strncmp(params, "real", 3) == 0) {
    arSerial->println(F(FWVER));
    // Otherwise depends on whether we have a custom string set
  } else {
    if (strlen(AR488.vstr) > 0) {
      arSerial->println(AR488.vstr);
    } else {
      arSerial->println(F(FWVER));
    }
  }
}


/***** Address device to talk and read the sent data *****/
void read_h(char *params) {
  // Clear read flags
  rEoi = false;
  rEbt = false;
  // Read any parameters
  if (params != NULL) {
    if (strlen(params) > 3) {
      if (isVerb) arSerial->println(F("Invalid termination character - ignored!")); void addr_h(char *params);
    } else if (strncmp(params, "eoi", 3) == 0) { // Read with eoi detection
      rEoi = true;
    } else { // Assume ASCII character given and convert to an 8 bit byte
      rEbt = true;
      eByte = atoi(params);
    }
  }
  if (AR488.amode == 3) {
    // In auto continuous mode we set this flag to indicate we are ready for continuous read
    aRead = true;
  } else {
    // If auto mode is disabled we do a single read
    gpibReceiveData();
  }
}


/***** Reset the controller *****/
/*
 * Arduinos can use the watchdog timer to reset the MCU
 * For other devices, we restart the program instead by
 * jumping to address 0x0000. This is not a hardware reset
 * and will not reset a crashed MCU, but it will re-start
 * the interface program and re-initialise all parameters. 
 */
void rst_h() {
#ifdef WDTO_1S
  // Where defined, reset controller using watchdog timeout
  unsigned long tout;
  tout = millis() + 2000;
  wdt_enable(WDTO_1S);
  while (millis() < tout) {};
  // Should never reach here....
  if (isVerb) {
    arSerial->println(F("Reset FAILED."));
  };
#else
  // Otherwise restart program (soft reset)
  asm volatile ("  jmp 0");
#endif
}


/***** Set the status byte (device mode) *****/
void stat_h(char *params) {
  uint16_t val = 0;
  // A parameter given?
  if (params != NULL) {
    // Byte value given?
    if (notInRange(params, 0, 255, val)) return;
    AR488.stat = (uint8_t)val;
    if (val & 0x40) {
      setSrqSig();
      if (isVerb) arSerial->println(F("SRQ asserted."));
    } else {
      clrSrqSig();
      if (isVerb) arSerial->println(F("SRQ un-asserted."));
    }
  } else {
    // Return the currently set status byte
    arSerial->println(AR488.stat);
  }
}


/***** Save controller configuration *****/
void save_h() {
#ifdef E2END
  epWriteData(AR488.db, AR_CFG_SIZE);
  if (isVerb) arSerial->println(F("Settings saved."));
#else
  arSerial->println(F("EEPROM not supported."));
#endif
}


/***** Show state or enable/disable listen only mode *****/
void lon_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    isRO = val ? true : false;
    if (isTO) isTO = false; // Talk-only mode must be disabled!
    if (isVerb) {
      arSerial->print(F("LON: "));
      arSerial->println(val ? "ON" : "OFF") ;
    }
  } else {
    arSerial->println(isRO);
  }
}


/***** Set the SRQ signal *****/
void setSrqSig() {
  // Set SRQ line to OUTPUT HIGH (asserted)
  setGpibState(0b01000000, 0b01000000, 1);
  setGpibState(0b00000000, 0b01000000, 0);
}


/***** Clear the SRQ signal *****/
void clrSrqSig() {
  // Set SRQ line to INPUT_PULLUP (un-asserted)
  setGpibState(0b00000000, 0b01000000, 1);
  setGpibState(0b01000000, 0b01000000, 0);
}



/***********************************/
/***** CUSTOM COMMAND HANDLERS *****/
/***********************************/

/***** Re-load default configuration *****/
void default_h() {
  initAR488();
}


/***** Show or set end of receive character(s) *****/
void eor_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 15, val)) return;
    AR488.eor = (uint8_t)val;
    if (isVerb) {
      arSerial->print(F("Set EOR to: "));
      arSerial->println(val);
    };
  } else {
    if (AR488.eor>7) AR488.eor = 0;  // Needed to reset FF read from EEPROM after FW upgrade
    arSerial->println(AR488.eor);
  }
}


/***** Enable verbose mode 0=OFF; 1=ON *****/
void verb_h() {
  isVerb = !isVerb;
  arSerial->print("Verbose: ");
  arSerial->println(isVerb ? "ON" : "OFF");
}


/***** Set version string *****/
/* Replace the standard AR488 version string with something else
 *  NOTE: some instrument software requires a sepcific version string to ID the interface
 */
void setvstr_h(char *params) {
  uint8_t plen;
  char idparams[64];
  plen = strlen(params);
  memset(idparams, '\0', 64);
  strncpy(idparams, "verstr ", 7);
  strncat(idparams, params, plen);


  id_h(idparams);

}


/***** Talk only mode *****/
void ton_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    isTO = val ? true : false;
    if (isTO) isRO = false; // Read-only mode must be disabled in TO mode!
    if (isVerb) {
      arSerial->print(F("TON: "));
      arSerial->println(val ? "ON" : "OFF") ;
    }
  } else {
    arSerial->println(isTO);
  }
}


/***** Repeat a given command and return result *****/
void repeat_h(char *params) {

  uint16_t count;
  uint16_t tmdly;
  char *param;

  if (params != NULL) {
    // Count (number of repetitions)
    param = strtok(params, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 2, 255, count)) return;
    }
    // Time delay (milliseconds)
    param = strtok(NULL, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 0, 30000, tmdly)) return;
    }

    // Pointer to remainder of parameters string
    param = strtok(NULL, "\n\r");
    if (strlen(param) > 0) {
      for (uint16_t i = 0; i < count; i++) {
        // Send string to instrument
        gpibSendData(param, strlen(param));
        delay(tmdly);
        gpibReceiveData();
      }
    } else {
      errBadCmd();
      if (isVerb) arSerial->println(F("Missing parameter"));
      return;
    }
  } else {
    errBadCmd();
    if (isVerb) arSerial->println(F("Missing parameters"));
  }

}


/***** Run a macro *****/
void macro_h(char *params) {
#ifdef USE_MACROS
  uint16_t val;
  const char * macro;

  if (params != NULL) {
    if (notInRange(params, 0, 9, val)) return;
    //    execMacro((uint8_t)val);
    runMacro = (uint8_t)val;
  } else {
    for (int i = 0; i < 10; i++) {
      macro = (pgm_read_word(macros + i));
      //      arSerial->print(i);arSerial->print(F(": "));
      if (strlen_P(macro) > 0) {
        arSerial->print(i);
        arSerial->print(" ");
      }
    }
    arSerial->println();
  }
#else
  memset(params, '\0', 5);
  arSerial->println(F("Disabled"));
#endif
}


/***** Storage management *****/
#ifdef EN_STORAGE
void store_h(char *params){
  char *keyword;
  char *param1;
  char *param2;
//  uint8_t mode = 0;
//  uint8_t val = 0;
  
  // Get first parameter (action)
  keyword = strtok(params, " \t");
  param1 = strtok(NULL, " \t");
  param2 = strtok(NULL, " \t");
  
  if (keyword != NULL) {
    if (strncmp(keyword, "info", 4)==0) {
      arSerial->print(F("SDcard initialised:\t"));
      if (storage.isInit()){
        arSerial->println(F("YES"));
        storage.showSDInfo(arSerial);
        arSerial->print(F("Volume mounted:\t\t"));
        if (storage.isVolumeMounted()){
          arSerial->println(F("YES"));
          storage.showSdVolumeInfo(arSerial);
        }else{
          arSerial->println(F("NO"));
        }
      }else{
        arSerial->println(F("NO"));
      }
    }

    if (strncmp(keyword, "dir", 4)==0) {
      storage.listSdFiles(arSerial);
    }

    if (strncmp(keyword, "fmt", 3)==0) {

    }

    if (strncmp(keyword, "tape", 4)==0) {
      tape_h(param1, param2);
    }
  }


}


void tape_h(char *cmd, char *param){
  
}


#endif




/***** Bus diagnostics *****/
/*
 * Usage: xdiag mode byte
 * mode: 0=data bus; 1=control bus
 * byte: byte to write on the bus
 * Note: values to switch individual bits = 1,2,4,8,10,20,40,80
 */
void xdiag_h(char *params){
  char *param;
  uint8_t mode = 0;
  uint8_t val = 0;
  
  // Get first parameter (mode = 0 or 1)
  param = strtok(params, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      mode = atoi(param);
      if (mode>2) {
        arSerial->println(F("Invalid: 0=data bus; 1=control bus"));
        return;
      }
    }
  }
  // Get second parameter (8 bit byte)
  param = strtok(NULL, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      val = atoi(param);
    }

    if (mode) {   // Control bus
      // Set to required state
      setGpibState(0xFF, 0xFF, 1);  // Set direction
      setGpibState(~val, 0xFF, 0);  // Set state (low=asserted so must be inverse of value)
      // Reset after 10 seconds
      delay(10000);
/*      
      if (AR488.cmode==2) {
        setGpibControls(CINI);
      }else{
*/      
        setGpibControls(DINI);
//      }
    }else{        // Data bus
      // Set to required value
      setGpibDbus(val);
      // Reset after 10 seconds
      delay(10000);
      setGpibDbus(0);
    }
  }

}


/****** Timing parameters ******/

void tmbus_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 30000, val)) return;
    AR488.tmbus = val;
    if (isVerb) {
      arSerial->print(F("TmBus set to: "));
      arSerial->println(val);
    };
  } else {
    arSerial->println(AR488.tmbus, DEC);
  }
}


/***** Set device ID *****/
/*
 * Sets the device ID parameters including:
 * ++id verstr - version string (same as ++setvstr)
 * ++id name   - short name of device (e.g. HP3478A) up to 15 characters
 * ++id serial - serial number up to 9 digits long
 */
void id_h(char *params) {
  uint8_t dlen = 0;
  char * keyword; // Pointer to keyword following ++id
  char * datastr; // Pointer to supplied data (remaining characters in buffer)
  char serialStr[10];

#ifdef DEBUG10
  arSerial->print(F("Params: "));
  arSerial->println(params);
#endif

  if (params != NULL) {
    keyword = strtok(params, " \t");
    datastr = keyword + strlen(keyword) + 1;
    dlen = strlen(datastr);
    if (dlen) {
      if (strncmp(keyword, "verstr", 6)==0) {
#ifdef DEBUG10       
        arSerial->print(F("Keyword: "));
        arSerial->println(keyword);
        arSerial->print(F("DataStr: "));
        arSerial->println(datastr);
#endif
        if (dlen>0 && dlen<48) {
#ifdef DEBUG10
        arSerial->println(F("Length OK"));
#endif
          memset(AR488.vstr, '\0', 48);
          strncpy(AR488.vstr, datastr, dlen);
          if (isVerb) arSerial->print(F("VerStr: "));arSerial->println(AR488.vstr);
        }else{
          if (isVerb) arSerial->println(F("Length of version string must not exceed 48 characters!"));
          errBadCmd();
        }
        return;
      }
      if (strncmp(keyword, "name", 4)==0) {
        if (dlen>0 && dlen<16) {
          memset(AR488.sname, '\0', 16);
          strncpy(AR488.sname, datastr, dlen);
        }else{
          if (isVerb) arSerial->println(F("Length of name must not exceed 15 characters!"));
          errBadCmd();
        }
        return;
      }
      if (strncmp(keyword, "serial", 6)==0) {
        if (dlen < 10) {
          AR488.serial = atol(datastr);
        }else{
          if (isVerb) arSerial->println(F("Serial number must not exceed 9 characters!"));
          errBadCmd();
        }
        return;
      }
//      errBadCmd();
    }else{
      if (strncmp(keyword, "verstr", 6)==0) {
        arSerial->println(AR488.vstr);
        return;
      }     
      if (strncmp(keyword, "name", 4)==0) {
        arSerial->println(AR488.sname);
        return;      
      }
      if (strncmp(keyword, "serial", 6)==0) {
        memset(serialStr, '\0', 10);
        snprintf(serialStr, 10, "%09lu", AR488.serial);  // Max str length = 10-1 i.e 9 digits + null terminator 
        arSerial->println(serialStr);
        return;    
      }
    }
  }
  errBadCmd();
}


void idn_h(char * params){
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 2, val)) return;
    AR488.idn = (uint8_t)val;
    if (isVerb) {
      arSerial->print(F("Sending IDN: "));
      arSerial->print(val ? "Enabled" : "Disabled"); 
      if (val==2) arSerial->print(F(" with serial number"));
      arSerial->println();
    };
  } else {
    arSerial->println(AR488.idn, DEC);
  }  
}


/******************************************************/
/***** Device mode GPIB command handling routines *****/
/******************************************************/

/***** Attention handling routine *****/
/*
 * In device mode is invoked whenever ATN is asserted
 */
void attnRequired() {

  uint8_t db = 0;
  uint8_t stat = 0;
  uint8_t saddr = 0;
  bool mla = false;
  bool mta = false;
  bool spe = false;
  bool spd = false;
  bool eoiDetected = false;
  
  // Set device listner active state (assert NDAC+NRFD (low), DAV=INPUT_PULLUP)
  setGpibControls(DLAS);

#ifdef DEBUG5
  dbSerial->println(F("Answering attention!"));
#endif

  // Read bytes
//  while (isATN) {
  while (digitalRead(ATN)==LOW) {
    stat = gpibReadByte(&db, &eoiDetected);
    if (!stat) {

#ifdef DEBUG5
      dbSerial->println(db, HEX);
#endif

      // Device is addressed to listen
      if (AR488.paddr == (db ^ 0x20)) { // MLA = db^0x20
#ifdef DEBUG5
        dbSerial->println(F("attnRequired: Controller wants me to data accept data <<<"));
#endif
        mla = true;
      }

      // Device is addressed to talk
      if (AR488.paddr == (db ^ 0x40)) { // MLA = db^0x40
          // Call talk handler to send data
          mta = true;
#ifdef DEBUG5
          if (!spe) dbSerial->println(F("attnRequired: Controller wants me to send data >>>"));
#endif
      }

      // Secondary addressing
      if (db>0x5F && db<0x80) {
        saddr = db;
      }


      // Serial poll enable request
      if (db==GC_SPE) spe = true;

      // Serial poll disable request
      if (db==GC_SPD) spd = true;
 
      // Unlisten
      if (db==GC_UNL) unl_h();

      // Untalk
      if (db==GC_UNT) unt_h();

    }
  
  }

#ifdef DEBUG5
  dbSerial->println(F("End ATN loop."));
#endif


  if (saddr && (mla||mta)){

#ifdef DEBUG5
    dbSerial->print(F("Secondary address: "));dbSerial->println(db);
#endif

#ifdef EN_STORAGE
//    storeExecCmd(saddr);
#else
  if (isVerb) arSerial->println(F("Storage device not enabled!"));
#endif

  }



  if (mla) { 
#ifdef DEBUG5
    dbSerial->println(F("Listening..."));
#endif
    // Call listen handler (receive data)
    mla_h();
    mla = false;
  }

  // Addressed to listen?
  if (mta) {
    // Serial poll enabled
    if (spe) {
#ifdef DEBUG5
      dbSerial->println(F("attnRequired: Received serial poll enable."));
#endif
      spe_h();
      spe = false;
    // Otherwise just send data
    }else{
      mta_h();
      mta = false;
    }
  }

  // Serial poll disable received
  if (spd) {
#ifdef DEBUG5
    dbSerial->println(F("attnRequired: Received serial poll disable."));
#endif
    spd_h();
    mta = false;
    spe = false;
    spd = false;
  }

  // Finished attention - set controls to idle
  setGpibControls(DIDS);

#ifdef DEBUG5
  dbSerial->println(F("attnRequired: END attnReceived."));
#endif

}


/***** Device is addressed to listen - so listen *****/
void mla_h(){
  gpibReceiveData();
}


/***** Device is addressed to talk - so send data *****/
void mta_h(){
  if (lnRdy == 2) sendToInstrument(pBuf, pbPtr);
}


/***** Selected Device Clear *****/
void sdc_h() {
  // If being addressed then reset
  if (isVerb) arSerial->println(F("Resetting..."));
#ifdef DEBUG5
  dbSerial->print(F("Reset adressed to me: ")); dbSerial->println(aTl);
#endif
  if (aTl) rst_h();
  if (isVerb) arSerial->println(F("Reset failed."));
}


/***** Serial Poll Disable *****/
void spd_h() {
  if (isVerb) arSerial->println(F("<- serial poll request ended."));
}


/***** Serial Poll Enable *****/
void spe_h() {
  if (isVerb) arSerial->println(F("Serial poll request received from controller ->"));
  gpibSendStatus();
  if (isVerb) arSerial->println(F("Status sent."));
  // Clear the SRQ bit
  AR488.stat = AR488.stat & ~0x40;
  // Clear the SRQ signal
  clrSrqSig();
  if (isVerb) arSerial->println(F("SRQ bit cleared (if set)."));
}


/***** Unlisten *****/
void unl_h() {
  // Stop receiving and go to idle
#ifdef DEBUG5
  dbSerial->println(F("Unlisten received."));
#endif
  rEoi = false;
  tranBrk = 3;  // Stop receving transmission
}


/***** Untalk *****/
void unt_h() {
  // Stop sending data and go to idle
#ifdef DEBUG5
  dbSerial->println(F("Untalk received."));
#endif
}


void lonMode(){

  gpibReceiveData();

  // Clear the buffer to prevent it getting blocked
  if (lnRdy==2) flushPbuf();
  
}
