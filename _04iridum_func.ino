void modem_on(){
  digitalWrite(LEDPIN, HIGH);                    
  digitalWrite(LDOENABLE, HIGH);
  if(board_version >= 3){
    delay(1000);
    digitalWrite(IRRIDIUMENABLE, HIGH);
  } 
}
  
void modem_off(){
  Ir_serial.println("AT*F");
  delay(1000);
  digitalWrite(LEDPIN, LOW);                    
  digitalWrite(LDOENABLE, LOW);
  if(board_version >= 3){
    digitalWrite(IRRIDIUMENABLE, LOW);
  }  
}



void passthrough()
{
  unsigned long intime = millis();
  modem_on();
  delay(100);
  clearIbuffer();
  Serial.println(F("! to escape"));
  while(1){
  if ((unsigned long)(millis()-intime)>360000){
    modem_off();
    return;
  }
  if(Ir_serial.available())
  {
    Serial.write(Ir_serial.read());
    delay(10);
  }
  if(Serial.available())
  {
    if(Serial.peek() == 33){
     modem_off();
     return;
    }

    Ir_serial.write(Serial.read());
    delay(10);
  }
  }
}


boolean sendstatus() {
  //////Sends the modem status
  /////Store the current datapacket
  datapacket = "";
  datapacket += "V";
  datapacket += version[0];
  datapacket += version[1];
  datapacket += version[2];
  datapacket += version[3];
  datapacket += "H";
  datapacket += getEEPROMint(1);
  datapacket += "I";
  datapacket += interval;
  datapacket += "T";
  datapacket += irid_interval;
  datapacket += "G";
  datapacket += i2caddress;
  datapacket += "PS";
  datapacket += packet_l;
  datapacket += "B";
  datapacket += irid_tbase;
  if(TIME_AUTO_SET) datapacket += "A";
  boolean resend = !senddata();
  Serial.println();
  Serial.println(datapacket);
  Serial.flush();
  delay(10);
  datapacket = "";
  return resend;
}
  

boolean senddata() {
  ///////Iridium Setup//////////////////////////////

  unsigned int checksum = 0; /*Unsigned 16 bit integer*/
  char irstatus[] = "AT+CIER=1,0,1,0";
  char at_sbdwb[] = "AT+SBDWB=";
  char at_sbdix[] = "AT+SBDIX";
  char at_sbdd0[] = "AT+SBDD0";
  String input = "";
  char response[40];
  boolean network = false;
  boolean success = false;
  boolean dataloaded = false;
  unsigned long time = 0;
  char buffer;
  byte i,y;
  tries = 0;
  if(datapacket.length() == 0) readsensors(1);   //Make sure there is data if we are going to send a packet

  for (i=0;i<datapacket.length();i++) {
    checksum += byte(datapacket.charAt(i));
  }
  modem_on();
  delay(1000);
  clearIbuffer();
  y=0;
  while (dataloaded == false) {
    Ir_serial.print(at_sbdwb);
    Ir_serial.println(datapacket.length());
    delay(200);
    clearIbuffer();
    ////////////Send Data to Irridium Modem/////////
    for (i=0;i<datapacket.length();i++) {
      Ir_serial.print(char(datapacket.charAt(i)));
      delay(30);
    }    
    Ir_serial.print(char(highByte(checksum)));
    delay(30);   
    Ir_serial.print(char(lowByte(checksum)));
    delay(300);
    while(Ir_serial.available()>0){
      if (Ir_serial.read() == 48) {
        dataloaded = true;
      }
    } 
    if (y > 0) {
      Serial.println("Load_F");
      tries = tries + 3;
      Serial.flush();
      delay(100);
      modem_off();
      return success;
    }
    y++;  
  }
  ///Data loaded continue and try to send message
  while ((success == false) && (tries < 4)){
    delay(50);
    Ir_serial.println(irstatus);
    byte availableBytes = 0;
    int count = 0;
    input = "";
    network = false;
    time = millis();

    while (network == false)
    {
      if ((unsigned long)(millis() - time) > 1000*count){
        Serial.print(".");
        count++;
      }
      availableBytes = 0;     
      if(Ir_serial.available() > 0) {
        // read the incoming data as a char:
        buffer = Ir_serial.read();
        input.concat(buffer);
        if (input.indexOf("+CIEV:1,1")>0){
          Serial.println("Con!");
          network = true;
        }
      }
      /////////Break out of loop eventually if network not available
      if ((unsigned long)(millis() - time) > 15000){
        network = true;   //Try anyway
        break;
      }
    }  

    ///////////////////Network is Connected /////////////////////////////
    clearIbuffer();
    delay(50);
    Ir_serial.println(at_sbdix);
    Ir_serial.flush();    
    Serial.println("Send");


    time = millis();
    while ((Ir_serial.available() < 30) && ((unsigned long)(millis()-time)<30000)) {
      delay (100);
    }
    delay(100);
    while(Ir_serial.available())
    {
      if(Ir_serial.read() == ':') {
        break;
      }
    }
    y = 0;
    delay(100);
    while(Ir_serial.available())
    {
      buffer = Ir_serial.read();
      delay(50);
      response[y] = buffer;
      y++;
    }
    response[y]= '\0';
    Serial.println(response);

    
    ///  Check for incoming message and process
    if (atoi(subStr(response,",",3)) == 1){
      Serial.println("Msg");
      get_message();
      set_times();
    }

    if ((atoi(subStr(response,",",1)) < 3) && (y > 10))   {
      Serial.println("Sent!");
      Serial.flush();
      digitalWrite(LEDPIN, LOW);                    
      delay(50);
      blinkLED(LEDPIN,3,50);
      success = true;
    }
    else {
      Serial.println("F");
    }   
    tries++;
    Serial.print("T=");
    Serial.println(tries);
    Serial.flush();
    delay(50);
  
  }
  modem_off();
  return success;
}

byte get_message(){
  char sbdrb[] = "AT+SBDRB";
  byte mtbuffer = 0;
  byte mes_length = 0;
  int check = 0;
  int checksum = 0;
  int mt_value = 0;
  char mt_message[20];
  byte mt_command = 0;
 
  Ir_serial.print(sbdrb);
  delay(100);
  clearIbuffer();
  delay(100);
  Ir_serial.println();
  delay(100);
  while(Ir_serial.available())
    {
      if(Ir_serial.read() == 0) {
        delay(50);
        break;
      }
    }
  mes_length = Ir_serial.read();  //Second MO byte will be the packet length
  Serial.println(mes_length); 
  for(i=0;i<mes_length+2;i++){
    mt_message[i] = Ir_serial.read();
    Serial.print(mt_message[i]);
  }
    
  checksum = word(mt_message[mes_length],mt_message[mes_length+1]); 
  for(int i=0;i<mes_length;i++){
    check += byte(mt_message[i]);
  }
  
  if (check == checksum){
    Serial.println("OK");
    process_command(mt_message,mes_length);
    ///Change Global Flag
    SEND_MODEM_STATUS = TRUE; 
  }
  
  
  
}


byte getItime(int set,int timeout) {
  byte msg_len = 8;
  char time_buffer[20];
  char at_msstm[] = "AT-MSSTM";
  unsigned long irtime = 0;
  byte time_returned = 0;
 
  modem_on();
  dotdelay(timeout);
  clearIbuffer();
  Ir_serial.println(at_msstm);
  delay(1000);
  while(Ir_serial.available())
  {
    if(Ir_serial.read() == ':') {
      delay(50);
      break;
    }
  }
  int y = 0;
  while(Ir_serial.available())
  {
    time_buffer[y] = Ir_serial.read();
    Serial.print(time_buffer[y]);
    if(time_buffer[y] == 13) break;
    if(time_buffer[y] > 47) y++;
    delay(10);
  }  
  time_buffer[y] = '\0';
  delay(500);
  modem_off();

  if(time_buffer[0] == 110 || y == 0)
  {
    retries = retries + 200;
    Serial.println("F");
    set = 0;
  }
  else if(y == 8){
    for (i = 0; i < 8; i++) {
      irtime = (unsigned long)(irtime * 16) + HexDigit(time_buffer[i]);
    }
    irtime = (unsigned long)(irtime *.09);
    irtime = irtime + irid_tbase;
    Serial.println();
    Serial.print("ITime: ");  
    digitalClockDisplay(irtime);  
  }
  Serial.println();
  Serial.flush();
  Serial.print("RTC:");
  digitalClockDisplay(now());  
        
  if ((set == 1)&& (irtime > 0)) {
    retries = retries + 100;
    RTC.set(irtime);   // set the RTC and the system time to the received value
    setTime(irtime);  
    blinkLED(LEDPIN,5,500);
  }
  set_times();    //Set both the logging and transmit times just in case they were changed.
  return 1;
}    


