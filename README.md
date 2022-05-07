# AR488 Store


This project is based on the AR488 GPIB controller, an Arduino-based controller for interfacing with IEEE488 GPIB devices via USB. This original AR488 project was inspired by and was based on the work originally released by Emanuele Girlando and was released with his permission.

AR488_Store (a.k.a. "flash drive") implements a GPIB device designed primarily to emulate legacy GPIB connected storage devices. Currently supported deices include:

- Tektronix 4924 tape drive

Tektronix 4924 emulation, a.k.a "4050 flash drive" supports most of the original 4924 commands, including OLD, FIND, INPUT, PRINT, READ, WRITE, MARK, KILL, BSAVE/BREAD. BSAVE/BREAD require the R05 Binary Program Loader or a Multifunction Device or Maxirom which contain the R05 ROM. The "flash drive" also supports multiple directories (in place of tapes) and custom commands for switching to and listing files in a directory (using a BASIC program). The flash drive has been tested on a Tektronix 4051, 4052 and 4054 computer.

The sketch is based, in part, on the original AR488 code and supports Device mode only. It does NOT support controller mode. It implements a sub-set of Prologix ++ style commands, primarily for debug/testing purposes and to view storage contents. Primary and secondary addressing is supported as well as interfacing with SN75160 and SN75161 GPIB transceiver integrated circuits. In normal use, serial ports are disabled and all activity ocurrs between the GPIB bus and SD storage.

To build an interface, at least one Arduino board with sufficient memory will be required to act as the interface hardware. An SD Card reader and an SD card will also be required to provide the neccessary data storgage. The SD card reader connects to the ICSP bus and is dependent on the SPI library. 

Arduino boards generally provide a low cost alternative to other commercial interfaces. Currently the following boards are supported:

<table>
<tr><td><i>MCU</i></td><td><i>Board</i></td><td><i>Serial Ports</i></td><td><i>Layouts</i></td></tr>
<tr><td>644</td><td>MightyCore ATmega 644</td><td>1 x UART, Serial0 shared with USB</td><td>Panduino/Sanguino board</td></tr>
<tr><td>1284</td><td>MightyCore ATmega 1284</td><td>1 x UART, Serial0 shared with USB</td><td>Pandauino/Sanguino board</td></tr>
<tr><td>2560</td><td>Arduino Mega 2560</td><td>4 x UART, Serial0 shared with USB</td><td>D - (default) using pins on either side of board<br>E1 - using the first row of end connector<br>E2 - using the second row of end connector</td></tr>
</table>

Uno, Nano,Micro Pro and Leonardo Boards have insufficient memory to handle the SdFat library so boards with MCU's having greater capacity such as the 644, 1284 and Mega 2560 have been used for this project. Currently, 3 layouts are provided for the AtMega2650 using either the lower numbered pins on the sides of the board (<D>efault), the first row of pins of the two row header at the end of the board (E1), or the second row of the same header (E2). This provides some flexibility and allows various displays and other devices to be connected if desired. Please be aware that when using the <D>efault layout, pins 16 and 17 that correspond to TXD2 and RXD2 (Serial2) cannot be used for serial communication as they are used to drive GPIB signals, however serial ports 0, 1 and 3 remain available for use. Layouts E1 and E2 do not have the same restriction.

Including the SN7516x chipset into the interface design will naturally add to the cost, but has the advantage of providing the full 48mA drive current capacity regardless of the capability of the Arduino board being used, as well as providing proper tri-state output with Hi-Z when the board is powered down. The latter isolates the Arduino micro-controller from the GPIB bus when the interface is powered down, preventing GPIB bus communication problems due to 'parasitic power' from signals present on the GPIB bus, thereby allowing the interface to be safely powered down while not in use. Construction will involve adding a daughter-board between the Arduino GPIO pins and the GPIB bus. This could be constructed using prototyping board or shield, or custom designed using KiCad or other PCB layout design software.

To use the sketch, create a new directory, and then unpack the .zip file into this location. Open the main sketch, AR488_Store.ino, in the Arduino IDE. This should also load all of the linked .h and .cpp files. Review Config.h and make any configuration adjustments required (see the 'Configuration' section of the AR488 manual for details), including the selcetion of the board layout selection appropriate to the Arduino board that you are using. Set the target board in Board Manager within the Arduino IDE (Tools => Board:), and then compile and upload the sketch. There should be no need to make any changes to any other files. Once uploaded, the firmware should respond to the ++ver command with its version information.

Unless some form of shield or custom design with integral IEEE488 connector is used, connecting to an instrument will require a 16 core cable and a suitable IEEE488 connector. This can be salvaged from an old GPIB cable or purchased from various electronics parts suppliers. Searching for a 'centronics 24-way connector' sometimes yields better results than searching for 'IEEE 488 connector' or 'GPIB connector'. Details of interface construction and the mapping of Arduino pins to GPIB control signals and data bus are explained in the "Building an AR488 GPIB Interface" section of the <a href="https://github.com/Twilight-Logic/AR488/blob/master/AR488-manual.pdf">AR488 Manual</a>.

Once uploaded, and with a serial port enabled, the firmware should respond to the ++ver command with its version information. Since the emulator is normally run with serial ports disabled, there will be no output on the serial port.

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
<tr><td>Monty McGraw</td><td>Storage functions, Tektronix content, testing/debugging</td></tr>
<tr><td>Tom Stephenson</td><td>Testing and debugging</td></tr>
</table>

Also, thank you to all the contributors to the Emulator EEVblog thread for their suggestions and support.

