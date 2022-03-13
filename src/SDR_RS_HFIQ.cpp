//***************************************************************************************************
//
//      SDR_RS_HFIQ.cpp 
//
//      USB host control for RS-HFIQ 5W transciever board
//
//      Placed in the Public Domain
//
//***************************************************************************************************

#include <Arduino.h>
#include <USBHost_t36.h>
#include <SDR_RS_HFIQ.h>

#define USBBAUD 57600   // RS-HFIQ uses 57600 baud
uint32_t baud = USBBAUD;
uint32_t format = USBHOST_SERIAL_8N1;
USBHost RSHFIQ;
USBHub hub1(RSHFIQ);
USBHub hub2(RSHFIQ);
USBHIDParser hid1(RSHFIQ);
USBHIDParser hid2(RSHFIQ);
USBHIDParser hid3(RSHFIQ);

static uint32_t rs_freq;
int  counter  = 0;
int  blocking = 1;  // 0 means do not wait for serial response from RS-HFIQ - for testing only.  1 is normal

// External program stuff uique to the calling program.  Move this into the class init.
//extern uint32_t VFOA;  // 0 value should never be used more than 1st boot before EEPROM since init should read last used from table.
//extern uint32_t VFOB;
//extern uint8_t  curr_band;   // global tracks our current band setting.
///extern struct   Band_Memory bandmem[];
//extern void     displayFreq();


// There is now two versions of the USBSerial class, that are both derived from a common Base class
// The difference is on how large of transfers that it can handle.  This is controlled by
// the device descriptor, where up to now we handled those up to 64 byte USB transfers.
// But there are now new devices that support larger transfer like 512 bytes.  This for example
// includes the Teensy 4.x boards.  For these we need the big buffer version. 
// uncomment one of the following defines for userial
USBSerial userial(RSHFIQ);  // works only for those Serial devices who transfer <=64 bytes (like T3.x, FTDI...)
//USBSerial_BigBuffer userial(myusb, 1); // Handles anything up to 512 bytes
//USBSerial_BigBuffer userial(myusb); // Handles up to 512 but by default only for those > 64 bytes

USBDriver *drivers[] = {&hub1, &hub2, &hid1, &hid2, &hid3, &userial};
#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
const char * driver_names[CNT_DEVICES] = {"Hub1", "Hub2",  "HID1", "HID2", "HID3", "USERIAL1" };
bool driver_active[CNT_DEVICES] = {false, false, false, false};

// ************************************************* Setup *****************************************
//
// *************************************************************************************************
void SDR_RS_HFIQ::setup_RSHFIQ(int _blocking, uint32_t VFO)  // 0 non block, 1 blocking
{   
    Serial.println("Start of RS-HFIQ Setup");
    rs_freq = VFO;
    blocking = _blocking;
    Serial.println(F("\n\nUSB Host Testing - Serial"));
    RSHFIQ.begin();
    delay(50);
    Serial.println(F("Waiting for RS-HFIQ device to register on USB Host port"));
    while (!refresh_RSHFIQ())  // observed about 500ms required.
    {        
        if (!blocking) break;
        // wait until we have a valid USB 
        Serial.print(F("Retry (500ms) = ")); Serial.println(counter++);
    }
    delay(1500);  // about 1-2 seconds needed before RS-HFIQ ready to receive commands over USB
    
    send_fixed_cmd_to_RSHFIQ(q_dev_name); // get our device ID name
    Serial.print(F("Device Name: "));print_RSHFIQ(blocking);  // waits for serial available (BLOCKING call);
    
    send_fixed_cmd_to_RSHFIQ(q_ver_num);
    Serial.print(F("Version: "));print_RSHFIQ(blocking);  // waits for serial available (BLOCKING call);

    send_fixed_cmd_to_RSHFIQ(q_Temp);
    Serial.print(F("Temp: ")); print_RSHFIQ(blocking);   // Print our query results
    
    send_fixed_cmd_to_RSHFIQ(s_initPLL);  // Turn on the LO clock source  
    Serial.println(F("Initializing PLL"));
    
    send_fixed_cmd_to_RSHFIQ(q_Analog_Read);
    Serial.print(F("Analog Read: ")); print_RSHFIQ(blocking);   // Print our query results
    
    send_fixed_cmd_to_RSHFIQ(q_BIT_freq);
    Serial.print(F("Built-in Test Frequency: ")); print_RSHFIQ(blocking);   // Print our query results
    
    send_fixed_cmd_to_RSHFIQ(q_clip_on);
    Serial.print(F("Clipping (0 No Clipping, 1 Clipping): ")); print_RSHFIQ(blocking);   // Print our query results

    send_variable_cmd_to_RSHFIQ(s_freq, convert_freq_to_Str(rs_freq));
    Serial.print(F("Starting Frequency (Hz): ")); Serial.println(convert_freq_to_Str(rs_freq));
    delay(25);

    send_fixed_cmd_to_RSHFIQ(q_F_Offset);
    Serial.print(F("F_Offset (Hz): ")); print_RSHFIQ(blocking);   // Print our query result

    send_fixed_cmd_to_RSHFIQ(q_freq);  // query the current frequency.
    Serial.print(F("Reported Frequency (Hz): ")); print_RSHFIQ(blocking);   // Print our query results
    
    Serial.println(F("End of RS-HFIQ Setup"));
    
    counter = 0;
    disp_Menu();
}

uint32_t SDR_RS_HFIQ::cmd_console(uint32_t VFO)  // active VFOA value to possible change, returns new or unchanged VFO value
{
    char c;
    unsigned char Ser_Flag = 0, Ser_NDX = 0;
    char S_Input[16];

    rs_freq = VFO;

    while (Serial.available() > 0)    // Process any and all characters in the buffer
    {
        c = Serial.read(); 
        c = toupper(c);
        //Serial.print(c);
        if (c == '*')                   // No matter where we are in the state machine, a '*' say clear the buffer and start over.
        {
            Ser_NDX = 0;                   // character index = 0
            Ser_Flag = 1;                  // State 1 means the '*' has been received and we are collecting characters for processing
            for (int z = 0; z < 16; z++)  // Fill the buffer with spaces
            {
                S_Input[z] = ' ';
            }
            Serial.print(F("Start Cmd string "));
        }
        else 
        {
          if (Ser_Flag == 1 && c != 13) S_Input[Ser_NDX++] = c;  // If we are in state 1 and the character isn't a <CR>, put it in the buffer
          if (Ser_Flag == 1 && c == 13)                         // If it is a <CR> ...
          {
              S_Input[Ser_NDX] = 0;                                // terminate the input string with a null (0)
              Ser_Flag = 3;                                        // Set state to 3 to indicate a command is ready for processing
              Serial.println(S_Input);
          }
          if (Ser_NDX > 15) Ser_NDX = 15;   // If we've received more than 15 characters without a <CR> just keep overwriting the last character
        }
        if (S_Input[0] != '*' and  Ser_Flag == 0 && Ser_NDX < 2)  // process menu pick list
        {
            int32_t fr_adj = 0;     

            Ser_NDX = 0;

            switch (c)
            {
                case '1': Serial.println(F("TX OFF")); send_fixed_cmd_to_RSHFIQ(s_TX_OFF); break; 
                case '2': Serial.println(F("TX ON")); send_fixed_cmd_to_RSHFIQ(s_TX_ON); break; 
                case '3': Serial.print(F("Query Frequency: ")); send_fixed_cmd_to_RSHFIQ(q_freq); print_RSHFIQ(blocking); break;
                case '4': Serial.print(F("Query Frequency Offset: ")); send_fixed_cmd_to_RSHFIQ(q_F_Offset); print_RSHFIQ(blocking); break;
                case '5': Serial.print(F("Query Built-in Test Frequency: ")); send_fixed_cmd_to_RSHFIQ(q_BIT_freq); print_RSHFIQ(blocking); break;
                case '6': Serial.print(F("Query Analog Read: ")); send_fixed_cmd_to_RSHFIQ(q_Analog_Read); print_RSHFIQ(blocking); break;
                case '7': Serial.print(F("Query Ext Frequency or CW: ")); send_fixed_cmd_to_RSHFIQ(q_EXT_freq); print_RSHFIQ(blocking); break;
                case '8': Serial.print(F("Query Temp: ")); send_fixed_cmd_to_RSHFIQ(q_Temp); print_RSHFIQ(blocking); break;
                case '9': Serial.println(F("Initialize PLL Clock0 (LO)")); send_fixed_cmd_to_RSHFIQ(s_initPLL); break;
                case '0': Serial.print(F("Query Clipping: ")); send_fixed_cmd_to_RSHFIQ(q_clip_on); print_RSHFIQ(blocking); break;
                case 'K': Serial.println(F("Move Down 1000Hz ")); fr_adj = -1000; break;
                case 'L': Serial.println(F("Move Up 1000Hz ")); fr_adj = 1000; break;
                case '<': Serial.println(F("Move Down 100Hz ")); fr_adj = -100; break;
                case '>': Serial.println(F("Move Up 100Hz ")); fr_adj = 100; break;  
                case ',': Serial.println(F("Move Down 10Hz ")); fr_adj = -100; break;
                case '.': Serial.println(F("Move Up 10Hz ")); fr_adj = 100; break; 
                case 'R': disp_Menu(); break;
                default: Serial.write(c); userial.write(c); break;
                break;  
            }
            if (fr_adj != 0)  // Move up or down
            {
                rs_freq += fr_adj;
                Serial.print(F("Target Freq = ")); Serial.println(rs_freq);
                send_variable_cmd_to_RSHFIQ(s_freq, convert_freq_to_Str(rs_freq));
                fr_adj = 0;
                return rs_freq;
            } 
        }
    }

    // If a complete command is received, process it
    if (Ser_Flag == 3) 
    {
        Serial.print(F("Send Cmd String : *"));Serial.println(S_Input);  
        if ((S_Input[0] == 'F' || S_Input[0] == 'D' || S_Input[0] == 'E') && S_Input[1] != '?') // Some commands do not issue a response
                                                                                                // so do not do a blocking print that waits
                                                                                                // for a response that will never come.
        {
            // convert string to number and update the rs_freq variable
            rs_freq = atoi(&S_Input[1]);   // skip the first letter 'F' and convert the number   
            if (S_Input[0] == 'F')
                rs_freq = find_new_band(rs_freq);  // set the correct index and changeBands() to match for possible band change
            if (rs_freq == 0)
                {
                    Serial.print(F("RS-HFIQ: Invalid Frequency = ")); Serial.println(S_Input);
                    Ser_Flag = 0;
                    return rs_freq;
                }
            //update_VFOs(rs_freq);
            Serial.print(F("RS_HFIQ Frequency Change: ")); Serial.println(rs_freq);
            send_variable_cmd_to_RSHFIQ(s_freq, convert_freq_to_Str(rs_freq));    
        }
        else
        {
          Serial.print(F("RS_HFIQ Command: ")); Serial.println(S_Input);
          send_fixed_cmd_to_RSHFIQ(S_Input);
          delay(5);
          print_RSHFIQ(0);  // do not print this for freq changes, causes a hang since there is no response and this is a blocking call 
                            // Use non blocking since user input could be in error and a response may not be returned
        }
        Ser_Flag = 0;
    }
    return rs_freq;
}

void SDR_RS_HFIQ::send_fixed_cmd_to_RSHFIQ(const char * str)
{
    userial.printf("*%s\r", str);
    delay(5);
}

void SDR_RS_HFIQ::send_variable_cmd_to_RSHFIQ(const char * str, char * cmd_str)
{
    userial.printf("%s%s\r", str, cmd_str);
    delay(5);
}

void SDR_RS_HFIQ::init_PLL(void)
{
  Serial.println("init_PLL: Begin");
  send_fixed_cmd_to_RSHFIQ(s_initPLL);
  delay(2000);   // delay needed to turn on PLL clock0
  Serial.println("init_PLL: End");
}

char * SDR_RS_HFIQ::convert_freq_to_Str(uint32_t rs_freq)
{
  sprintf(freq_str, "%lu", rs_freq);
  send_fixed_cmd_to_RSHFIQ(freq_str);
  return freq_str;
}

void SDR_RS_HFIQ::write_RSHFIQ(int ch)
{   
    userial.write(ch);
} 

int SDR_RS_HFIQ::read_RSHFIQ(void)
{
    while (userial.available()) 
    {
        //Serial.println("USerial Available");
        return userial.read();
    }
    return 0;
}

void SDR_RS_HFIQ::wait_reply(int blocking)
{
    if (!blocking)
        return;
    while (!userial.available());
}

// flag = 0 do not Block
// flag = 1, block while waiting for a serial char
void SDR_RS_HFIQ::print_RSHFIQ(int flag)
{
  if (flag) wait_reply(1);  // we are waiting for a reply (BLOCKING)
  //Serial.print("print_RSHFIQ: ");
  while ( int c = read_RSHFIQ())
  {
    Serial.write(c);
  }
}

void SDR_RS_HFIQ::disp_Menu(void)
{
    Serial.println(F("\n\n ***** Control Menu *****\n"));
    Serial.println(F(" H - Main Menu"));
    Serial.println(F(" 1 - TX OFF"));
    Serial.println(F(" 2 - TX ON"));
    Serial.println(F(" 3 - Query Frequency"));
    Serial.println(F(" 4 - Query Frequency Offset"));
    Serial.println(F(" 5 - Query Built-in Test Frequency"));
    Serial.println(F(" 6 - Query Analog Read"));
    Serial.println(F(" 7 - Query External Frequency or CW"));
    Serial.println(F(" 8 - Query Temp"));
    Serial.println(F(" 9 - Initialize PLL Clock0 (LO)"));
    Serial.println(F(" 0 - Query Clipping"));
    
    Serial.println(F("\n Can use upper or lower case letters"));
    Serial.println(F(" Can use multiple frequency shift actions such as "));
    Serial.println(F("   <<<< for 4*100Hz  to move down 400Hz"));
    Serial.println(F(" K - Move Down 1000Hz "));
    Serial.println(F(" L - Move Up 1000Hz "));
    Serial.println(F(" < - Move Down 100Hz "));
    Serial.println(F(" > - Move Up 100Hz "));
    Serial.println(F(" , - Move Down 10Hz "));
    Serial.println(F(" . - Move Up 10Hz "));

    Serial.println(F("\n Enter any native command string"));
    Serial.println(F("    Example: *F5000000 will change frequency to WWV at 5MHz"));
    Serial.println(F("    Minimum is 3.0MHz or *F3000000, max is 30.0MHz or *F30000000"));
    Serial.println(F("    Band filters and attenuation are automatically set"));
    Serial.println(F("\n ***** End of Menu *****"));
}

bool SDR_RS_HFIQ::refresh_RSHFIQ(void)
{
    bool Proceed;

    Proceed = false;
    
    RSHFIQ.Task();
    
    // Print out information about different devices.
    for (uint8_t i = 0; i < CNT_DEVICES; i++) 
    {
        if (*drivers[i] != driver_active[i]) 
        {
            if (driver_active[i]) 
            {
                Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
                driver_active[i] = false;
                Proceed = false;
            } 
            else 
            {
                Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
                driver_active[i] = true;
                Proceed = true;

                const uint8_t *psz = drivers[i]->manufacturer();
                if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
                psz = drivers[i]->product();
                if (psz && *psz) Serial.printf("  product: %s\n", psz);
                psz = drivers[i]->serialNumber();
                if (psz && *psz) Serial.printf("  Serial: %s\n", psz);

                // If this is a new Serial device.
                if (drivers[i] == &userial) 
                {
                    // Lets try first outputting something to our USerial to see if it will go out...
                    userial.begin(baud);
                }
            }
        }
    }
    delay (500);
    return Proceed;
}

//#endif // RS_HFIQ