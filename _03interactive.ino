

void interactive_mode(unsigned int timeout){
   char instring[20];   
   unsigned long time;
   byte length = 0;
   boolean lf = false;
   int display_menu = 1;
   delay(50); 
   while (display_menu ){
        
    time = millis();
    length = 0;
    Serial.println(timeout);
    delay(1000);
    menu();
    lf = false;
    while (lf == false)  
    {      // Look for char in serial que and process if found
      blinkLED(LEDPIN,1,50);   
      
      if (Serial.available()>0) {
 
        if (Serial.peek() == 13){
          Serial.read();
          Serial.println("#");         
          lf = true;
          timeout = 60;                       //Menu entered on startup reset the timeout to 3 minutes
          while(Serial.read() >= 0){}        //Clear the input buffer of any data after newline
          
        }
        else {
          instring[length] = Serial.read();
          Serial.print(instring[length]);     // Echo command CHAR in ascii that was sent
          length++;
          Serial.flush();
          delay(30);
        }
      }
      else if ((millis()-time)>timeout*1000){     ////Time out after 5 minutes
        lf = true;
        display_menu = 0;
        Serial.println("TO.");
        delay(100);
        return;
      } 
    }
    lf = false;
    display_menu = process_command(instring,length);
    delay(100);
  }   //End of while
  
  
}

void mo_function(unsigned long address,unsigned long length)
  {
    boolean lf = false;
    for(i=address;i<=address+length;i++){
        EEPROM.write(i,255);
      }
      while (lf == false)  {      // Look for char in serial que and process if found
        if (Serial.available()>0) {
          if (Serial.peek() == 13){
            Serial.read();
            Serial.println("#");         
            lf = true;
          }
          else {
            command = Serial.read();
            EEPROM.write(address,command);
            Serial.print(command); 
            address++;
          }
        }
      }
  }

byte process_command(char instring[20],byte length)
  { 
    unsigned long  value = 0;
    unsigned int  mo_index = 101;
    char command;
    byte redisplay_menu = 1;
    command = instring[0];
    for(i=1;i<length && i<18;i++){
      value = (long) value *10 +  (instring[i] - 48);
      }

    switch(command) {
    case 65:
      //A OW Addresses
      DeviceAddress address;
      for (int i=0; i<tac_string.getDeviceCount(); i++) {
        tac_string.getAddress(address, i);
        printAddress(address);
        Serial.println();
      }
      break;   
    case 66:
      // B  set the iridium epoch base time
      irid_tbase=value;
      EEPROM_writelong(23,irid_tbase);
      break;  
  
    case 67:
      // C
      clear_epprom();
      i2caddress = 0;
      EEPROM_writelong(10,i2caddress);
      break;  
    case 68:
      // D Set the date manually
      if (length == 13){
        setDateDs1307(instring);
        set_times();
      }
      else {
        Serial.println("Inv.");
        Serial.println(length);
      } 
      break;
    case 69:
      // E  export data
      download(value,i2caddress,packet_l);
      break;  
    case 70:
      // F Toggle iridium auto time set
      TIME_AUTO_SET = !TIME_AUTO_SET;
      EEPROM.write(27,TIME_AUTO_SET);
      break;  
    case 71:
      //G set I2C MEMORY LOCATION
      i2caddress= value;
      EEPROM_writelong(10,i2caddress);
      break;
    case 72:
      //H set sensor height
      setEEPROM(value,1);
      break;
    case 73:
      // I Set logging Interval
      setEEPROM(value,5);
      interval = value;
      set_times();
      break;
    case 74:
      // J Mobile Originated Message
      mo_function(101,100);
      break;
    case 75:
      // K Mobile Originated Message
      mo_function(201,240);
      break;
    case 76:
      //L Print MO message to screen and send
      Serial.println();
      for(i=101;i<=200;i++) {
        if(EEPROM.read(i) != 255){
          Serial.write(EEPROM.read(i));
        }
      }
      Serial.println();
      for(i=201;i<=440;i++) {
        if(EEPROM.read(i) != 255){
          Serial.write(EEPROM.read(i));
        }
      }
      Serial.println();
      Serial.println();
      break;
    case 77:
      //M
      getItime(1,60);
      Serial.print(F("RTC:"));
      digitalClockDisplay(now());  
      break;
    case 80:
      // P  Iridium modem pass through
      passthrough();
      break;
    case 82:
      // R  read the sensors
      readsensors(0);
      break;
    case 83:
      //S  manually send a data packet
      Serial.println(F("Send"));
      readsensors(1);
      senddata();
      datapacket = "";
      break;
    case 84:
      //T Telemetry Interval (multiple of logging interval)
      EEPROM_writelong(17,value);
      irid_interval = value;
      retry_delay = irid_interval/10;
      set_times();
      break;
    case 88:
      // X
      redisplay_menu = 0;
      break;
    default:
      Serial.println("Unk!");
    }
    Serial.flush();
    return redisplay_menu;
}


void menu() {
  Serial.print("V");
  for (int i = 0; i < 4; i++){
     Serial.print(version[i]);
  }
  Serial.println();
  if(EEPROM.read(501) != 255){
      Serial.write("!!!");
  }    
  for(i=501;i<=770;i++) {
        if(EEPROM.read(i) != 255){
          Serial.write(EEPROM.read(i));
        }
  }
  if(EEPROM.read(501) != 255){
      Serial.println();
      Serial.println("!!!");
  }    
  delay(100); 
  Serial.print("1w:");
  Serial.print(tac_string.getDeviceCount(),DEC);
  if (tac_string.isParasitePowerMode()) Serial.println("p");
  else Serial.println("a");
  float batv = (analogRead(BATPIN)*2*.49
    )/100;    // read the input pin
  delay(100); 
  //Serial.print("FM:");
  //Serial.println(freeMemory());
  Serial.print(F("Bat:"));
  Serial.println(batv,2);  
  
  Serial.print(F("Time:"));
  digitalClockDisplay(now());  
  Serial.print("L:");
  digitalClockDisplay(log_time);  
  Serial.print("S:");
  digitalClockDisplay(send_sat_time);  
  
  Serial.print(F("(H)Off:"));
  Serial.println(float(getEEPROMint(1)/10.0),1);
  Serial.print(F("TS_Int:"));
  Serial.println(time_set_interval);

  if(TIME_AUTO_SET){
    Serial.println(F("F AutoTime On"));
  }
  else{
    Serial.println(F("F AutoTime Off"));
  }
  Serial.print(F("(I)nterval:"));
  Serial.println(interval);
  Serial.print(F("(T)ele Int:"));
  Serial.println(irid_interval);
  Serial.print(F("(B)ase Time:"));
  Serial.println(irid_tbase);
  Serial.print(F("Mem (G):"));
  Serial.println(i2caddress);
  Serial.print(F("P_Size:"));
  Serial.println(packet_l);
  Serial.println(F("D DSSMMHHDDMMYY"));
  Serial.println(F("R Read Sens"));
  Serial.println(F("A OW_Add"));
  Serial.println(F("P IpassThru"));
  Serial.println(F("E Export"));
  Serial.println(F("C Clr Mem"));
  Serial.println(F("M Set iTime"));
  Serial.println(F("S Send Imsg"));
  Serial.println(F("X log"));
  delay(100);
  Serial.flush();
} 

void download(unsigned long loc,unsigned long eeadd,int length){
  int y = 0;
  int i;
  int record_num = 1;
  byte buffer,time1,high,low;
  int time2,value;
  float dataval;
  Serial.println(length);
  while(loc<eeadd){
    Serial.print(record_num);
    Serial.print(",");
    time1 = EEPROM1024.read(loc);
    time2 = read_i2c_int(loc+1);
    print_time(time1,time2);
    Serial.print(",");
    //buffer = EEPROM1024.read(loc+3);
    //Serial.print(buffer);
    //Serial.print(",");
    buffer = EEPROM1024.read(loc+3);
    Serial.print(buffer);
    Serial.print(",");
    value = read_i2c_int(loc+4);
    dataval = float(value)/100;
    Serial.print(dataval,2);
    Serial.print(",");
    for(i=6;i<length;) {
      value = read_i2c_int(loc+i);
      dataval = float(value)/10;
      Serial.print(dataval,2);
      Serial.print(",");
      i=i+2;
    }
    Serial.println();
    loc=loc+length;
    record_num++;
  }
}  
      

