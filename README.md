# Teensy4_USB_Host_RS-HFIQ_Library

Teensy 4.0 or 4.1 USB Host Port serial control for the 5W RS-HFIQ SDR transceiver placed into the Public Domain.

This is the initial version designed originally to work with my SDR_RA887x Teensy-based SDR radio (https://github.com/K7MDL2/KEITHSDR) but can be used easily by any other Arduino Teensy 4 app.  

I am using this library today for normal tuning and also free-form command entry from the USB serial terminal. 

You can see how I use this library in my Teensy SDR by searching my SDR_887x code for "$define USE_RS_HFIQ" in the code at  https://github.com/K7MDL2/KEITHSDR/tree/main/SDR_RA8875 

Uses the Teensy 4 CPU USB host port. The RS-HFIQ uses 150ma over that USB host port so you need a decent 5V supply to account for this.

Contains 2 Arduino terminal program examples to do all commands the RS-HFIQ supports.

  a. SDR_RSHFIQ_.ino was the test program used to figure out how it all worked and is self-contained.  It does not use the library.
  b. SDR_RSHFIQ_Lib.ino shows how to use the library and does the same thing as the non-library version.

In theory this should also permit control via Omni-Rig2 interface from PC or Linux SDR programs because the serial interface will accept all commands in the same format the RS-HFIQ accepts. (Not tested yet)

This version uses an internal band map table that ensures any serial terminal request frequency is within the band limits.  Invalid requests are ignored and returns a value of 0 to the caller.  Otherwise it returns the new valid frequency and a band number.  The band number is useful for forcing updates on band changes when multiple VFOs are involved.  Can do your own band number lookup based on the returned frequency and ignore the one supplied. 

The initialization process has some pecularities that have been worked out by trial and error.  The library and now the test program now both share the same startup code which has shown to be reliable.  Once in a while a query following a setting returns the wrong or previous query value.  2nd attempt usually get the right answer.

The B, D and E set frequency offset commands are now supported as of April 18,, 2022.  The B (BIT - Built-In-Test signal) accepts and reports the right vaules but I have yet to hear the test signal.  D is offset, and and E is external RF signal on the ext SMA jack near an endplate (jack is unpopulated).  These values are stored in EEPROM so take that into account during reboots and such.
