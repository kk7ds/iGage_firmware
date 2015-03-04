void setup()
{
  
  //////////////////SETUP ARDUINO////////////////////////////////
  setpins(); 
  panel_temp.begin();                   // Start up the One Wire library
  tac_string.begin();
  Ir_serial.begin(19200);
  max_serial.begin(9600);
  Ir_serial.listen();
 
  Serial.begin(19200);
  delay(500);
  /////////////////GET TIME FROM RTC///////////////////////////
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  setSyncInterval(3600);   // the function to get the time from the RTC
  
  if(year()< 2014 | year() > 2030){
    Serial.println(year());
    process_command("D000000010110",13);     //If time not set then set time to Jan 1, 2010 to get things running
    if (irid_interval >= 1){
      if (TIME_AUTO_SET) getItime(1,30);                //Try to set with iridium time as well.
    }
  }  
  /////////////////GET LOGING INTERVAL FROM EEPPROM////////////
  interval = getEEPROMint(5);
  if (interval > 86400){
    setEEPROM(3600,5);
    interval = 3600;
  }  
  ////////////////Get Tipping Bucket Value////////////////////
//  tipBucket = getEEPROMint(21);
//  if (tipBucket <0){
//      tipBucket = 0;
//  }    
  
  //attachInterrupt(TB, precip, RISING);

  
  /////////////////Get current memory address//////////////////
  i2caddress = EEPROM_readlong(10);
  if ((i2caddress < 0 )|(i2caddress > 131072)) i2caddress = 0; 
    
  irid_interval = EEPROM_readlong(17);
  if (irid_interval > 86400){
    setEEPROM(3600,17);
    irid_interval = 3600;
  }  
  retry_delay = irid_interval/10;
  
  ////////////////Get Iridium Base Time from EEPROM////////////
  irid_tbase = EEPROM_readlong(23);
  if(irid_tbase > 2099818235) {
    
    irid_tbase = 1173325821;                 //Chnage this value to update the default iridium base epock time.
                                             //The second iridium time epoch May 11, 2014, at 14:23:55 (1399818235)
                                             // Epoch 1 was 8 march 2007 3:50:21 UTC (1173325821)
    EEPROM_writelong(23,irid_tbase);
  }
  
  board_version = EEPROM.read(28);
  if(board_version < 0){
    board_version = 4;
    EEPROM.write(28,4);
  }  

  //////////////////Determine number of TACS and add 12 character header////////////////////
  packet_l = tac_string.getDeviceCount()*2+10;

  /////////////////////GO INTO INTERACTIVE MODE/////////////
  set_times();
  set_time = ((now()/time_set_interval)+1)*time_set_interval;
  
  setEEPROM(0,1);
  interactive_mode(60);
  
  
 }

