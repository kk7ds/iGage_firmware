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
  int ATtemp,batt,inches,temp_correct;
  byte first = (byte(year()-2010) << 4) | month();
  int second = byte(day()) << 11;
  int shift = byte(hour()) << 6;
  second = second | shift;
  shift = minute();
  second = second | shift;
  batt =analogRead(BATPIN)*2*.50;
  delay(100); 
  ///////read Panel Temp sensor////////////////////////////////
  panel_temp.requestTemperatures(); // Send the command to get temperatures
  ATtemp = (panel_temp.getTempCByIndex(0))*10;
  delay(50);
 
  ///////read AirTemp sensor////////////////////////////////
  tac_string.requestTemperatures(); // Send the command to get temperatures
  temp_correct = (tac_string.getTempCByIndex(0))*10;

  ///////read max sensor////////////////////////////////
  readmaxttl(maxdepth);
  if(record) {
    loadbyte(first);
    loadint(second);
    //loadbyte(status_byte);
    loadbyte(last_retries);
    loadint(batt);
    loadint(ATtemp);
    //loadint(maxdepth[0]);      //Raw Depth
    loadint(maxdepth[1]);        // Temperature Compensated Depth
  }
  else {
  Serial.println();  
  Serial.print("V:");
  Serial.println(float(batt/100.0),2);
  Serial.print("PT:");
  Serial.println(float(ATtemp/10.0),1);
  //Serial.print("RawDepth:");
  //Serial.println(float(maxdepth[0]/10.0),1);
  Serial.print("D:");
  Serial.println(float(maxdepth[1]/10.0),1);
  Serial.print("1W:");
  Serial.println(tac_string.getDeviceCount(),DEC);
  }
  //////read TAC cable////////
  for (int i=0; i<tac_string.getDeviceCount(); i++) {
    tac_string.requestTemperatures(); // Send the command to get temperatures
    ATtemp = (tac_string.getTempCByIndex(i))*10;
    if(record) {
    loadint(ATtemp);
    
    }
    else {
      Serial.print("T");
      Serial.print(i);
      Serial.print(":");
      Serial.println(float(ATtemp/10.0),1);
    }
  }
  Serial.println();
 
}


//int tempcorrect(float reading, float temp)  {
//  float zeroK = 273.15;
//  float a = 20.05;
//  float b = 2;
//  float tof;
//  float d = 0.0000147 ;
//  float conv = 39.37;
//  float dm = 0;
//  tof = reading*d;
//  dm = tof*((a*sqrt(temp+zeroK))/b)*conv;
//  
//  int corrected = dm * 10;
//  return corrected;
//}

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


int readmaxttl(int depths[]){
  max_serial.listen(); 
  byte index = 0;
  int reading[10];
  int  value = 0;
  byte read_max = 1;
  unsigned long time;
  boolean range = false; 
  char in;
  
  //Turn on LDO and Moxbotix
  digitalWrite(LDOENABLE, HIGH);
  delay(500);
  digitalWrite(MAXENABLE,HIGH);
  if(board_version == 2){
    digitalWrite(MAXPOWER, HIGH);   
  }
  
  time = millis();
  
  value = 0;
  range = false;
  while (read_max){
    //delay(10);
    if (((millis()-time)>10000) || index > 9) {   ////Time out after 10 seconds or after the 13th line returned from the MB sensor
        break;
    }    
    else if (max_serial.available()>0) {
        in = max_serial.read();
        Serial.print(in);
        if (range & in != 13) {
          value = (int)value*10 + (in - 48);
        }  
        if (in == 82) range = true;
        if (in == 13 & range == true){
          Serial.println(value);
          if(value > 500 & value < 9999){
            reading[index] = value;
            index++;
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
  
  isort(reading,(index));
  if(index == 0){
    depths[1] = -4000;
  }
  else{
    depths[1] = getEEPROMint(1) - (reading[index/2])/2.54; 
  }
  depths[0] = 0;     //No raw depth

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



