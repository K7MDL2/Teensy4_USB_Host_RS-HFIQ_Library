# Teensy4_USB_Host_RS-HFIQ_Library

Teensy 4.0 or 4.1 USB Host Port serial control for the 5W RS-HFIQ SDR transceiver placed into the Public Domain.

This is the initial version designed orignally to work with my SDR_RA887x Teensy-based SDR radio (https://github.com/K7MDL2/KEITHSDR).  

I am using this library today supporting normal tuning and also free-form command entry from the USB serial terminal.  It should be easily usable by any Teensy 4 based Arduino program.

You can see how I use this library in my Teensy SDR by searching my SDR_887x code for "$define USE_RS_HFIQ" in the code.  

Uses the Teensy 4 CPU USB host port. The RS-HFIQ uses 150ma over that USB host port so you need a decent 5V supply to account for this.

Depends (today) on 1 external program function to validate frequency entries sourced from the USB serial terminal.

Contains 1 Arduino terminal example program to do all commands the RS-HFIQ supports.

In theory this should also permit control via Omni-Rig2 interface from PC or Linux SDR programs because the serial interface will accept all commands in the same format the RS-HFIQ accepts. (Not tested yet)

The B, D and E set frequency offset comamnds likely need more work.
