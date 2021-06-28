# AR488 Arduino GPIB Interface


The AR488 GPIB controller is an Arduino-based controller for interfacing with IEEE488 GPIB devices via USB. This work was inspired by and was based on the work originally released by Emanuele Girlando and released with his permission.

AR488_Store is a device mode GPIB interface designed primarily for the emmulation of legacy storage devices. Currently supported deices include:

Tektronix 4924 Tape Drive

The sketch is based on the AR488 code and implements a sub-set of Prologix ++ device mode commands. Controller commands are not supported but a number of custom commands have been implemented, for example to provide storage management. Primary and secondary addressing is supported as well as interfacing with SN75160 and SN75161 GPIB transceiver integrated circuits.

To build an interface, at least one Arduino board will be required to act as the interface hardware. An SD Card reader and an SD card will also be required to provide the neccessary data storgage. The SD card reader connects to the ICSP bus and uses the SPI library. 

Arduino boards provide a low cost alternative to other commercial interfaces. Currently the following boards are supported:

<table>
<tr><td><i>MCU</i></td><td><i>Board</i></td><td><i>Serial Ports</i></td><td><i>Layouts</i></td></tr>
<tr><td>644</td><td>Mega 2560</td><td>1 x UART, Serial0 shared with USB</td><td>Panduino or Sanguino board</td></tr>
<tr><td>1284</td><td>Mega 2560</td><td>1 x UART, Serial0 shared with USB</td><td>Pandauino or Sanguino board</td></tr>
<tr><td>2560</td><td>Mega 2560</td><td>4 x UART, Serial0 shared with USB</td><td>D - (default) using pins on either side of board<br>E1 - using the first row of end connector<br>E2 - using the second row of end connector</td></tr>
</table>

Uno, Nano,Micro Pro and Leonardo Boards have insufficient memory to handle the SdFat library so boards with MCU's having greater capacity such as the 644, 1284 and Mega 2560 have been used for this project. Currently, 3 layouts are provided for the AtMega2650 using either the lower numbered pins on the sides of the board (<D>efault), the first row of pins of the two row header at the end of the board (E1), or the second row of the same header (E2). This provides some flexibility and allows various displays and other devices to be connected if desired. Please be aware that when using the <D>efault layout, pins 16 and 17 that correspond to TXD2 and RXD2 (Serial2) cannot be used for serial communication as they are used to drive GPIB signals, however serial ports 0, 1 and 3 remain available for use. Layouts E1 and E2 do not have the same restriction.

Including the SN7516x chipset into the interface design will naturally add to the cost, but has the advantage of providing the full 48mA drive current capacity regardless of the capability of the Arduino board being used, as well as providing proper tri-state output with Hi-Z when the board is powered down. The latter isolates the Arduino micro-controller from the GPIB bus when the interface is powered down, preventing GPIB bus communication problems due to 'parasitic power' from signals present on the GPIB bus, thereby allowing the interface to be safely powered down while not in use. Construction will involve adding a daughter-board between the Arduino GPIO pins and the GPIB bus. This could be constructed using prototyping board or shield, or custom designed using KiCad or other PCB layout design software.

To use the sketch, create a new directory, and then unpack the .zip file into this location. Open the main sketch, AR488_Store.ino, in the Arduino IDE. This should also load all of the linked .h and .cpp files. Review Config.h and make any configuration adjustment required (see the 'Configuration' section of the AR488 manual for details), including the selcetion of the board layout selection appropriate to the Arduino board that you are using. Set the target board in Board Manager within the Arduino IDE (Tools => Board:), and then compile and upload the sketch. There should be no need to make any changes to any other files. Once uploaded, the firmware should respond to the ++ver command with its version information.

Unless some form of shield or custom design with integral IEEE488 connector is used, connecting to an instrument will require a 16 core cable and a suitable IEEE488 connector. This can be salvaged from an old GPIB cable or purchased from various electronics parts suppliers. Searching for a 'centronics 24-way connector' sometimes yields better results than searching for 'IEEE 488 connector' or 'GPIB connector'. Details of interface construction and the mapping of Arduino pins to GPIB control signals and data bus are explained in the "Building an AR488 GPIB Interface" section of the <a href="https://github.com/Twilight-Logic/AR488/blob/master/AR488-manual.pdf">AR488 Manual</a>.
 
Commands generally adhere closely to the Prologix syntax, however there are some minor differences, additions and enhancements. For example, due to issues with longevity of the Arduino EEPROM memory, the ++savecfg command has been implemented differently to save EEPROM wear. Some commands have been enhanced with additional options and a number of new custom commands have been added to provide new features that are not found in the standard Prologix implementation. Details of all commands and features can be found in the Command Reference section of the <a href="https://github.com/Twilight-Logic/AR488/blob/master/AR488-manual.pdf">AR488 Manual</a>.

Once uploaded, the firmware should respond to the ++ver command with its version information.

<b><i>Obtaining support:</i></b>

In the event that a problem is found, this can be logged via the Issues feature on the AR488 GitHub page. Please provide at minimum:

- the firmware version number
- the type of board being used
- the make and model of instrument you are trying to control
- a description of the issue including;
- what steps are required to reproduce the issue

Comments and feedback can be provided here:<BR>
https://www.eevblog.com/forum/projects/tektronix-4924-tape-drive-emulator/

<b><i>Acknowledgements:</i></b>
<table>
<tr><td>Emanuelle Girlando</td><td>Original GPIB interface project for the Arduino Uno</td></tr>
<tr><td>Monty McGraw</td><td>Storage functions</td></tr>
</table>

Also, thank you to all the contributors to the Emulator EEVblog thread for their suggestions and support.

