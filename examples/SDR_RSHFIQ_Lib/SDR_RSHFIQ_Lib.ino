//***************************************************************************************************
//
//    SDR_RSHFIQ_Lib.INO
//    Standalone Teensy 4 USB Host test program for RS-HFIQ transciever board
//    Shows how to use the Teensy4_USBHost_RS-HFIQ_Library
//
//    March 13, 2022 by K7MDL
//
//    Based on the Teensy 3.6/4.x USBHost.ino example and RS-HFIQ.ino code
//    Test set and query commands for the RS-HFIQ transceiver via the Teensy 4.x USB Host serial port.
//
//    NOTE: Configure your terminal to send CR at end of line.  
//    How to use: Open a terminal window.
//                Press H for the main menu and R for RS-HFIQ commands sub menu
//                Instructions are printed with the R sub menu. 
//                You can directly enter a VFO frequency or use one of the menu
//                  commands for mostly queries.
//                
//***************************************************************************************************

#include <Arduino.h>
#include <SDR_RS_HFIQ.h>          // https://github.com/K7MDL2/Teensy4_USB_Host_RS-HFIQ_Library
#include <InternalTemperature.h>  // V2.1.0 @ Github https://github.com/LAtimes2/InternalTemperature

SDR_RS_HFIQ RS_HFIQ;

void RS_HFIQ_Service(void);                     // commands the RS_HFIQ over USB Host serial port  
void printHelp(void);
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis);
void respondToByte(char c);

bool        enable_printCPUandMemory = false;   // CPU , memory and temperature
uint32_t    VFO = 7074000;                     // Dial frequency in Hz
uint8_t     curr_band=2;                        // Valid bands are 1 - 9 mapping to 80M to 10M.
int         block = 1;
 
void setup()
{
    while (!Serial && (millis() < 5000)) ;      // wait for Arduino Serial Monitor
    Serial.println("\n\nRS-HFIQ Teensy USB Host Port Library Test Program V1.0");
    
    // Something to do other than the RS-HFIQ for the Main Menu
    InternalTemperature.begin(TEMPERATURE_NO_ADC_SETTING_CHANGES);
    printHelp();
    
    RS_HFIQ.setup_RSHFIQ(block, VFO);  // initialize the RS-HFIQ radio hardware
    
    Serial.print(F("\nCurrent VFO is ")); Serial.println(VFO);
    Serial.print(F("Current Band is ")); Serial.println(curr_band);
}

void loop()
{
    // For test purposes hit 'U'  or 'Y' to update the RS-HFIQ frequency to hard coded values emulating a VFO encoder
    // Your encoder tuning process calls the function 
    //    RS_HFIQ.send_variable_cmd_to_RSHFIQ("*F", RS_HFIQ.convert_freq_to_Str(VFO));  
    // directly for VFO updates. 
    // There are several other commands. They can be viewed in the libary source code.

    //respond to Serial commands
    while (Serial.available())
    {
        char ch = (Serial.peek());
        ch = toupper(ch);

        switch (ch)
        {
            case 'U':   Serial.read();  // Set VFO to something new.  Can insert touch or encoder driven frequency here
                        VFO = 14074000;    
                        //curr_band = 4;      // start off with a valid band and VFO
                        RS_HFIQ.send_variable_cmd_to_RSHFIQ("*F", RS_HFIQ.convert_freq_to_Str(VFO));
                        delay(5);  // query sometimes returns with freq out of range, not sure why yet. Possibly cannot update quick enough. adding delay here helps
                        RS_HFIQ.send_fixed_cmd_to_RSHFIQ("*F?");                     
                        RS_HFIQ.print_RSHFIQ(block);
                        break;
            case 'Y':   Serial.read();  // Set VFO to something new.  Can insert touch or encoder driven frequency here
                        VFO = 21074000;    
                        //curr_band = 6;      // start off with a valid band and VFO
                        RS_HFIQ.send_variable_cmd_to_RSHFIQ("*F", RS_HFIQ.convert_freq_to_Str(VFO));
                        delay(5);  // sometimes returns with freq out of range, not sure why yet. Possibly cannot update quick enough.
                        RS_HFIQ.send_fixed_cmd_to_RSHFIQ("*F?");                        
                        RS_HFIQ.print_RSHFIQ(block);
                        break;
            case 'C':
            case 'H':   respondToByte((char)Serial.read());   // pick off these 2 for a main menu.  
                                                              // Must use letters not used by the library
                        break;
            default:    RS_HFIQ_Service(); // all other characters pass through to the library for processing
                        break;
        }
    }
    //check to see whether to print the CPU and Memory Usage
    if (enable_printCPUandMemory)
        printCPUandMemory(millis(), 3000); //print every 3000 msec
}
//
// Main service interface to the library
// Called from the main loop when there is a new frequency to be tuned
// Send the requested frequency and applies the validated results to the VFO
// Non VFO related commands go direct.
//
void RS_HFIQ_Service(void)
{
    uint32_t temp_freq;
    static int8_t last_curr_band = curr_band;
    static uint32_t last_VFO = VFO;
    
    // Returns the requested frequency or 0 if OOB.
    // curr_band is passed and modified if a new (valid) band is determined. 
    // The library has an internal band map table covering ham bands 80-10M matching the RS-HFIQ filter set.
    // You can use this band info or ignore it.
    
    if ((VFO = RS_HFIQ.cmd_console(VFO, &curr_band)) != 0)  // feed in desired frequency and current band
    {
        if (last_curr_band != curr_band)  // If we get a validated frequency back the hrdware is now using it
                                          //  and the band map will give us a new band index we can use for 
                                          //  recalling other per band related settings.
        {
            Serial.print(F("New Band = ")); Serial.println(curr_band); 
        }   
    }
    if (temp_freq != 0)
    {
        if (last_VFO != VFO)  // only act on frequency changes, skip other things like queries
        {                      
            Serial.print(F("New VFO Frequency = ")); Serial.println(VFO); 
            last_VFO = VFO;
        }
    }
}

// Utility functions for demo

void togglePrintMemoryAndCPU(void) 
{ 
  enable_printCPUandMemory = !enable_printCPUandMemory; 
}

// _______________________________________ Print CPU Stats, Adjsut Dial Freq ____________________________
//
//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis)
{
    //static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
    static unsigned long lastUpdate_millis = 0;

    //has enough time passed to update everything?
    if (curTime_millis < lastUpdate_millis)
        lastUpdate_millis = 0; //handle wrap-around of the clock
    if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis)
    {
        Serial.print(F("CPU Temperature:"));
        Serial.print(InternalTemperature.readTemperatureF(), 1);
        Serial.print(F("F "));
        Serial.print(InternalTemperature.readTemperatureC(), 1);
        Serial.println(F("C"));
        Serial.println(F("*** End of Report ***"));

        lastUpdate_millis = curTime_millis; //we will use this value the next time around.
    }
}

//
// _______________________________________ Console Parser ____________________________________
//
//switch yard to determine the desired action for non RS-HFIQ commands
void respondToByte(char c)
{
    char s[2];
    s[0] = c;
    s[1] = 0;
    if (!isalpha(c) && c != '?' && c != '*')
        return;
    switch (c)
    {
    case 'h':
    case 'H':
    case '?':   printHelp();
                break;
    case 'C':
    case 'c':   Serial.println(F("Toggle printing CPU temperature."));
                togglePrintMemoryAndCPU();
                break;
    default:
                Serial.print(F("You typed "));
                Serial.print(s);
                Serial.println(F(".  What command?"));
                break;
    }
}

//
// _______________________________________ Print Help Menu ____________________________________
//
void printHelp(void)
{
    Serial.println();
    Serial.println(F("Help: Available Commands:"));
    Serial.println(F("   h: Print this help"));
    Serial.println(F("   C: Toggle printing of CPU and Memory usage"));
    Serial.println(F("   Y: Update VFO to 21074KHz (hard coded to emulate a VFO encoder update"));
    Serial.println(F("   U: Update VFO to 14074KHz (hard coded to emulate a VFO encoder update"));
    Serial.println(F("   R to display the RS-HFIQ Menu"));
}
