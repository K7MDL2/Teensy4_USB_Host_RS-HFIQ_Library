//
//      SDR_RS_HFIQ.h
//
//      Teensy 4 USB host port serial control library for RS-HFIQ SDR 5W transceiver board
//      March 12, 2022 by K7MDL
//
//      Placed in the Public Domain
//
//
#ifndef _SDR_RS_HFIQ_SERIAL_H_
#define _SDR_RS_HFIQ_SERIAL_H_

#include <Arduino.h>

// ToDo: pass the band edges in function args and remove this extern call
extern uint32_t find_new_band(uint32_t new_frequency);  // lookup function in main pgrogam for band edge validation, returns valid freq to keep in band or 0;

/*
//  Here is an example of the external band limit validation taken from my SDR_RA8875 program (found in Controls.cpp).
//  Your version of this wodu reside in the main program.  I am looking to make an internal table with defaults that 
//    can be overwritten by a new util;ity function.
//  Uses a table of band information which includes upper and lower frequencies for each band.  
//  If a band change is detected it calls changeBands(0) to apply the newly set VFO freqency (A or B which ever is active).
//  If any other serial command is supplied from the terminal it is skipped.
//  If the VFO is changed but the band remains the same, the changeBands(0) is skipped but the VFOs are updates and displayed.


// For RS-HFIQ free-form frequency entry validation but can be useful for external program CAT control such as a logger program.
// Changes to the correct band settings for the new target frequency.  
// The active VFO will become the new frequency, the other VFO will come from the database last used frequency for that band.
// If the new frequency is below or above the band limits it returns a value of 0 and skips any updates.
//
uint32_t find_new_band(uint32_t new_frequency)
{
    int i;

    for (i=BAND10; i> BAND1; i--)    // start at the top and look for first band that VFOA fits under bandmem[i].edge_upper
    {
        if (new_frequency >= bandmem[i].edge_lower && new_frequency <= bandmem[i].edge_upper)  // found a band lower than new_frequency so search has ended
        {
            //Serial.print("Edge_Lower = "); Serial.println(bandmem[i].edge_lower);
            curr_band = bandmem[i].band_num;
            if (bandmem[curr_band].VFO_AB_Active == VFO_A)
            {
                VFOA = bandmem[curr_band].vfo_A_last = new_frequency;  // up the last used frequencies
                VFOB = bandmem[curr_band].vfo_B_last;
            }
            else
            {
                VFOA = bandmem[curr_band].vfo_A_last;
                VFOB = bandmem[curr_band].vfo_B_last = new_frequency;
            }
            //Serial.print("New Band = "); Serial.println(curr_band);
            //changeBands(0);
            return new_frequency;
        }
    }
    Serial.println("Invalid Frequency Requested");
    return 0;
}
*/

class SDR_RS_HFIQ
{
    public:
        SDR_RS_HFIQ()  // -- Place any args here --          
        //  Place functions here if needed    ---
        {}  // Copy arguments to local variables
        // publish externally available functions
        uint32_t cmd_console(uint32_t VFO);  // active VFO value to possible change, returns new or unchanged VFO value
        void setup_RSHFIQ(int _blocking, uint32_t VFO);
        void send_variable_cmd_to_RSHFIQ(const char * str, char * cmd_str);
        char * convert_freq_to_Str(uint32_t freq);
        void send_fixed_cmd_to_RSHFIQ(const char * str);
        
    private:  
        char freq_str[15] = "7074000";  // *Fxxxx command to set LO freq, PLL Clock 0
        const char s_initPLL[5]       = "*OF1";   // turns on LO clock0 output and sets drive current level to 4ma.
        const char q_freq[4]          = "*F?";    // returns current LO frequency
        const char s_freq[3]          = "*F";     // set LO frequency template.  3 to 30Mhz range
        const char q_dev_name[3]      = "*?";     // example "RSHFIQ"
        const char q_ver_num[3]       = "*W";     // example "RS-HFIQ FW 2.4a"
        const char s_TX_OFF[4]        = "*X0";    // Transmit OFF 
        const char s_TX_ON[4]         = "*X1";    // Transmit ON - power is controlled via audio input level
        const char q_Temp[3]          = "*T";     // Temp on board in degrees C
        const char q_Analog_Read[3]   = "*L";    // analog read
        const char q_EXT_freq[4]      = "*E?";    // query the setting for PLL Clock 1 frequency presented on EX-RF jack or used for CW
        const char s_EXT_freq[15]     = "*E";// sets PLL Clock 1.  4KHz to 225Mhz range
        const char q_F_Offset[4]      = "*D?";    // Query Offset added to LO, BIT, or EXT frequency
        const char s_F_Offset[15]     = "*D";// Sets Offset to add to LO, BIT, or EXT frequency
        const char q_clip_on[3]       = "*C";     // clipping occuring, add external attenuation
        const char q_BIT_freq[4]      = "*B?";    // Built In Test. Uses PLL clock 2

        void print_RSHFIQ(int flag);
        bool refresh_RSHFIQ(void);
        void disp_Menu(void);
        void init_PLL(void);
        void wait_reply(int blocking); // BLOCKING CALL!  Use with care       
        void update_VFOs(uint32_t newfreq);
        void write_RSHFIQ(int ch);
        int  read_RSHFIQ(void);

};
#endif   // _SDR_RS_HFIQ_SERIAL_H_
