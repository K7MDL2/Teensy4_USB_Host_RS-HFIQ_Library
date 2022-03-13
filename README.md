# Teensy4_USB_Host_RS-HFIQ_Library

Teensy 4.0 or 4.1 USB Host Port serial control for the 5W RS-HFIQ SDR transceiver placed into the Public Domain.

This is the initial version designed originally to work with my SDR_RA887x Teensy-based SDR radio (https://github.com/K7MDL2/KEITHSDR) but can be used easily by any other Arduino Teensy 4 app.  

I am using this library today for normal tuning and also free-form command entry from the USB serial terminal. 

You can see how I use this library in my Teensy SDR by searching my SDR_887x code for "$define USE_RS_HFIQ" in the code at  https://github.com/K7MDL2/KEITHSDR/tree/main/SDR_RA8875 

Uses the Teensy 4 CPU USB host port. The RS-HFIQ uses 150ma over that USB host port so you need a decent 5V supply to account for this.

Contains 1 Arduino terminal program example to do all commands the RS-HFIQ supports.  It is not yet using the new library, it was the test program used to figure out how it all worked.

In theory this should also permit control via Omni-Rig2 interface from PC or Linux SDR programs because the serial interface will accept all commands in the same format the RS-HFIQ accepts. (Not tested yet)

Tis version uses an internal band map table that ensures any serial terminal request frequency is within the band limits.  Invalid requests are ignored and returns a value of 0 to the caller.  Otherwise it retunsdd the new valid frequency and a band number.  The band number is useful for forcing updates on band changes when multiple VFOs are involved.

Known Issues:
The B, D and E set frequency offset commands need more work.

