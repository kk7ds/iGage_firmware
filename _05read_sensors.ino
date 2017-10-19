//void precip()
//{
//    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) //if current time minus the last trigger time is greater than
//    {                                                  //the delay (debounce) time, button is completley closed.
//      lastDebounceTime = millis();
//      tipBucket++;
//  }
//
//} 


void readsensors(int record){
  //Clear global datapacket 
  datapacket = "";  
  int panelTemp,batt,inches,airTemp;
  byte first = (byte(year()-2010) << 4) | month();
  int second = byte(day()) << 11;
  int shift = byte(hour()) << 6;
  int hasAT = 0;
  second = second | shift;
  shift = minute();
  second = second | shift;
  batt =analogRead(BATPIN)*2*5/10.23; // reading * 5.0(ref)/1023 * 2 (voltage divider) / 100 (scale reading)
  delay(100); 
  ///////read Panel Temp sensor////////////////////////////////
  panel_temp.requestTemperatures(); // Send the command to get temperatures
  panelTemp = (panel_temp.getTempCByIndex(0))*10;
  
  delay(50);
 
  ///////read AirTemp sensor////////////////////////////////
  if(tac_string.getDeviceCount() > 0){
    tac_string.requestTemperatures(); // Send the command to get temperatures
    airTemp = (tac_string.getTempCByIndex(0))*10;
    if(airTemp < -1000) airTemp = -9999;  //Should cover when no sensor responds
    if(airTemp > 800) airTemp = -9999;  //Should cover when sensor responds with 85
  }  
  else{  
     airTemp = -9999;
  }    
  ///////read max sensor////////////////////////////////
  readmaxttl(maxdepth,airTemp);
  if(record) {
    loadbyte(first);
    loadint(second);
    loadbyte(last_retries);
    loadint(batt);
    if(airTemp != -9999){
      loadint(airTemp);
    }
    else{
      loadint(panelTemp);
    }    
    loadint(maxdepth[1]);      //Temperature Compensated Depth
  }
  else {
  Serial.println();  
  Serial.print("V:");
  Serial.println(float(batt/100.0),2);
  Serial.print("PT:");
  Serial.println(float(panelTemp/10.0),1);
  Serial.print("AT:");
  Serial.println(float(airTemp/10.0),1); 
  Serial.print("RawDepth:");
  Serial.println(float(maxdepth[0]/10.0),1);
  Serial.print("D:");
  Serial.println(float(maxdepth[1]/10.0),1);
  Serial.print("1W:");
  Serial.println(tac_string.getDeviceCount(),DEC);
  }
  
  //////read TAC cable////////
//  for (int i=0; i<tac_string.getDeviceCount(); i++) {
//    tac_string.requestTemperatures(); // Send the command to get temperatures
//    ATtemp = (tac_string.getTempCByIndex(i))*10;
//    if(record) {
//    loadint(ATtemp);
//    
//    }
//    else {
//      Serial.print("T");
//      Serial.print(i);
//      Serial.print(":");
//      Serial.println(float(ATtemp/10.0),1);
//    }
//  }
  Serial.println();
 
}


int tempcorrect(float reading, float temp)  {

  if(temp == -999){
    return reading;
  }
  float at_twentyfive = 672;    //Speed of sound at 25c ft/s
  float z = 643.855;
  float zeroK = 273.15;
  float actual_speed = z*sqrt(((temp+zeroK)/zeroK));
  int corrected = reading*(actual_speed/at_twentyfive);

  return corrected;
}

////Function to sort in descending order
void isort(int *a, int n)
{
  for (int i = 1; i < n; ++i)
  {
    int j = a[i];
    int k;
    for (k = i - 1; (k >= 0) && (j > a[k]); k--)
    {
      a[k + 1] = a[k];
    }
    a[k + 1] = j;
  }
}


int readmaxttl(int depths[],int temperature){
  depths[1] = -4000;
  max_serial.listen(); 
  byte index = 0;
  int reading[20];
  int  value = 0;
  byte read_max = 1;
  unsigned long time;
  boolean range = false; 
  char in;
  byte skipvals=0;
  
  //Turn on LDO and Moxbotix
  digitalWrite(LDOENABLE, HIGH);
  delay(500);
  digitalWrite(MAXENABLE,HIGH);
  if(board_version == 2){
    digitalWrite(MAXPOWER, HIGH);   
  }
  
  time = millis();
  
  
  
  while(read_max){
    //delay(10);
    if (((millis()-time)>20000) || index > 19) {   ////Time out after 10 seconds or after 10th reading from maxbotix
        break;
    }    
    else if (max_serial.available()>0) {
        depths[1] = -3936;
        in = max_serial.read();
        Serial.print(in);
        if (range & (in != 13)) {
          value = (int)value*10 + (in - 48);
        }  
        if (in == 82) range = true;
        if (in == 13 & range == true){
          skipvals++;
          if(value > 500 & value < 9999 & skipvals > 10){
            Serial.println();
            reading[index] = value;
            index++;
          }
          else{
            Serial.println('S');
         }
         value = 0;
         range = false;
        }  
    } 
  }   //End of while read max


  //Turn off maxbotix and ldo
  if(board_version == 2){
    digitalWrite(MAXPOWER,LOW);    
  }
  digitalWrite(MAXENABLE,LOW); 
  digitalWrite(LDOENABLE,LOW);
  
  

  if(index > 0){
    isort(reading,(index));
    depths[0] = getEEPROMint(1) - (reading[index/2])/2.54; 
  }

  
  depths[1] = tempcorrect(depths[0],temperature/10);     //No raw depth

  Ir_serial.listen();

}


// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

int HexByte(byte a,byte b)
{
  a = HexDigit(a);
  b = HexDigit(b);
  if (a<0 || b<0) {
    return -1;  // an invalid hex character was encountered
  } 
  else {
    return (a*16) + b;
  }
}

int HexDigit(byte c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  } 
  else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } 
  else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } 
  else {
    return -1;   // getting here is bad: it means the character was invalid
  }
}


// Function to return a substring defined by a delimiter at an index
char* subStr (char* str, char *delim, int index) {
  char *act, *sub, *ptr;
  static char copy[MAX_STRING_LEN];
  int i;
  // Since strtok consumes the first arg, make a copy
  strcpy(copy, str);
  for (i = 1, act = copy; i <= index; i++, act = NULL) {
    //Serial.print(".");
    sub = strtok_r(act, delim, &ptr);
    if (sub == NULL) break;
  }
  return sub;
}



