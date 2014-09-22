#include <LowPower.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <WString.h>
#include <avr/power.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/sleep.h>
#include <Time.h>  
#include <stdlib.h> 
#include <MemoryFree.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <E24C1024.h>

#define BATPIN 3              //Analog input for battery voltage
#define EEPROM_size 131072UL 
//Digital Pin Assignements
// 0 Reserved for TTL RX
// 1 Reserved for TTL TX
#define TB 2
#define IRRIDIUMENABLE 9    // 9 for v3+ boards
#define LDOENABLE 5
#define SENSOR_BUS 4
// Pin 6 available
#define ONE_WIRE_BUS 7
#define MAXENABLE 8
#define MAXPOWER 9        //Only required for v2 boards
#define MAX_SERIAL A0
#define TXPIN  12
#define RXPIN  11
#define LEDPIN 13
#define MAX_STRING_LEN  40       //Max length for str split on deliminator
#define DEBOUNCE_DELAY 50    // the debounce time in ms; decrease if quick button presses are ignored, increase

//Version History
// V2.02  - Trimmed the data packet down to 10 bytes by removing status byte
//        - Added a timeout parameter to the interactive_mode function
//          In Version 2 pins where mapped:
//            IRRIDIUMENABLE 5
//            MAXPOWER 8
//            MAXENABEL 9
//
// V3.01 
//        - Major upgrade....board now has seperate enable pins for 
//            LDO
//            Maxbotix
//            Iridium Modem
// V3.02  
//        - Added a watchdog to catch the modem from transmitting too often or not often enough
//        - Modified the iridium retry interval to equal 1/10 of the irdium interval
//        - Modified maxbotix routine to take the first or last reading if the median is max/min range
//        - Fixed bug with getting iridium time which reported a time if the network was not available
//        - Moved iridium time set to just after a succesful message is sent
//        - Fixed two way commands....modem can accept commands in txt.sbd file (no trailing newline)
// V3.03
//        - Cleaned up the iridium send subroutine.
//        - Retry delay is adjusted upward to the iridium interval if retries is greater than 120
//        - Modifieid the read maxbotix sensor to take readings until 10 good readings are recorded, or until 5 second duration is exceeded.
// V3.04  
//        - Removed the automatic time set.  Logger will only set the time with menu or 2-way interactive commande.
//        - Pulled the setpins() function out of the logging loop into the main loop of the program.   
//        - Added the setpinslow() function to the main loop of the program after everything is complete.
//        - Updated the iridium EPOCH time which changed on June 17th 2014.  New epoch: 11May2014 14:23:55.
//        - fixed 2-way bug, should work now.
// V3.05 
//        - Added the setting the time 1 once per day via iridium
//        - Moved the menu information from SRAM to PROGMEM
//        - Change variable 'time' in the send data subroutine from a long to an unsigned long type
// V3.06  
//        - Updated the 'Time' library with a newer version 24 Mar 2012
// V3.07  
//        - Made the iridium base epoch time a configurable setting that is stored in EEPROM.
//        - Included a flag 'TIME_AUTO_SET' to enable iridium time setting
//        - Added the iridium epoch to the modem status string
//        - Fixed bug in time calculation...not all 8 hex characters were getting processed
//        -Incocorporated both V2,V3 and V4 boards into the same software package.  The board type is selected by setting the 
//          corrext 'board_version' prior to uploading it to the IC.
//        -Moved the code over to github repository with version control



//Version
char version[5] = "3.07";

//This variable needs to match the appropriate board....check the google doc spreadsheet for board version number
byte board_version = 4;

// Gloabl variable counter
int i;

//logging interval in seconds
unsigned long interval = 3600UL;

//Iridium send interval in seconds
unsigned long irid_interval = 3600UL;

// time setting interval
unsigned long time_set_interval = 86400UL;

// Time to log data next
unsigned long log_time;

// Time to next set the time
unsigned long set_time;

// Time to send satellite data
unsigned long send_sat_time;

//Iridium Base Time
unsigned long irid_tbase;

// Number of seconds to delay before retrying on failed iridium attempt
unsigned long retry_delay = irid_interval/10;

// counter for Iridium attempts
byte retries = 0;

byte last_retries = 0;

// temp variable for buffering intermediate data
int temp = 0;   

// Iridium attempts in one session
byte tries = 0;

// Array for maxbotix depth sensor {raw reading, corrected reading}
int maxdepth[2] = {
  0,0};

//Tipping Bucket Value
//volatile int tipBucket = 0;
//volatile long lastDebounceTime = 0;   // the last time the TB was triggered

// User entered command
char command;       // This is the command char, in ascii form, sent from the serial port     

// Data packet to be stored and also sent via iridium
String datapacket = "";  //Iridium Data packet

// Variable for incoming serial data bytes
int inByte = 0;         // Universial incoming serial byte

///i2C variables
unsigned long i2caddress;    //Current i2c memory address

// Packet Length
byte packet_l;

//Watchdog to prevent transmission from ocurring too often or not often enough
unsigned int watchdog = 0;

// Flag for new data available to send
boolean newdata;

// Flag to send modem status
boolean SEND_MODEM_STATUS = false;

// Flag to send data
boolean SEND_MODEM_DATA = false;

// Flag to auto set the time from iridium, defaults to false (-1)
boolean TIME_AUTO_SET = EEPROM.read(27);

//EEPROM Variables
//1,2 Depth Sensor Height
//5,6 Logging Interval
//7,8 Data packet size 
//9   Flag to send idata
//10,11,12,13 i2caddress
//14  Packet Length
//16  Unused
//17,18,19,20 Iridium Transmission Interval
//21,22 Tipping Bucket Value
//23,24,25,26 Iridium base time value
//27 iridium auto time set flag
//28 board version
//100 Mobile Originated Message Sent
//101-430 Mobile Originated Message
//500 Mobile Terminate Message Read
//501-770 Mobile Terminated Message



// Setup a oneWire instance to c\ommunicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
OneWire OW(SENSOR_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature panel_temp(&oneWire);
DallasTemperature tac_string(&OW);

// set up a new serial port
SoftwareSerial Ir_serial(RXPIN,TXPIN);
SoftwareSerial max_serial(MAX_SERIAL,TXPIN);



void loop()
{
  if(interval>8) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  //Sleep for 8 seconds
    watchdog = watchdog + 8;
  }
  else{
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  //Sleep for 1 seconds
    watchdog = watchdog + 1;
  }

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(retries <= 1){
    blinkLED(LEDPIN,1,30);
  }
  else {
    blinkLED(LEDPIN,2,50);
  }
  if(Serial.available()){
    blinkLED(LEDPIN,5,50);
    while(Serial.available()){
      Serial.read();
    }

    interactive_mode(60);

  }


  ////////////////////////Log Data ///////////////////
  //    setEEPROM(tipBucket,21)
  if ((now() > log_time) && (interval > 0)) { 
    setpins();
    readsensors(1);
    Serial.println('.');
    delay(50);
    i2caddress=logdata(packet_l,i2caddress);
    if(i2caddress > (EEPROM_size - (2*packet_l))) i2caddress = 0;
    EEPROM_writelong(10,i2caddress);
    delay(30);
    Serial.flush();
    log_time = ((now()/interval)+1UL)*interval;
  }

  ////////Send the data via the irridium modem///////////////////
  if(now() > send_sat_time) SEND_MODEM_DATA = true;
  if(watchdog > irid_interval*2) {
    SEND_MODEM_DATA = true;
  }

  //Don't send data unless watchdog is greater than 300 (approx 5 minutes) and irid_interval is > 0
  if(watchdog < 300) SEND_MODEM_DATA = false;
  if(irid_interval < 1) SEND_MODEM_DATA = false;

  delay(10);
  if(SEND_MODEM_DATA)  {
    setpins();
    SEND_MODEM_DATA = false;
    watchdog = 0;
    if(senddata()) 
    {
      delay(10);
      last_retries = retries + tries; 
      retries = 0; 
      set_times();     //Set both the iridium time and logging time
      retry_delay = irid_interval/10;       
    }
    else 
    {
      retries += tries; 
      send_sat_time = ((now()/retry_delay)+1)*retry_delay;
      if(retries > 120) retry_delay = irid_interval*2; 
    }

    if(SEND_MODEM_STATUS){
      SEND_MODEM_STATUS = sendstatus();
    }
  }

  ////////Check iridium time                ///////////////////

  if ((TIME_AUTO_SET == true) && (now() > set_time)) {
    getItime(1,30);
    set_time = ((now()/time_set_interval)+1)*time_set_interval;
  }

}


