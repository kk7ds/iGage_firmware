unsigned long logdata(int length,unsigned  long eeadd){
  for (int i =0;i<length;i++){
    EEPROM1024.write(eeadd,datapacket.charAt(i)); 
    eeadd++;
  }
  return eeadd;
}
  

//Function to load new int data into the datastring
// NOTE datapacket is global string.
void loadint(int newdata)
{
  datapacket += char(lowByte(newdata));
  datapacket += char(highByte(newdata));
} 

//Function to load new byte data into the datastring
// NOTE datapacket is global string.
void loadbyte(byte newdata)
{
  datapacket += char(newdata);
}



// this function blinks the an LED light as many times as requested
void blinkLED(int targetPin, int numBlinks, int blinkRate) {
  for (int i=0; i<numBlinks; i++) {
    digitalWrite(targetPin, HIGH);   // sets the LED on
    delay(blinkRate);			   // waits for a blinkRate milliseconds
    digitalWrite(targetPin, LOW);    // sets the LED off
    delay(blinkRate);
  }
}

void setpins()
{
  pinMode(LEDPIN, OUTPUT);           // sets the digital pin as output
  digitalWrite(LEDPIN, LOW);
  pinMode(RXPIN, INPUT);
  //pinMode(TB, INPUT);
  pinMode(TXPIN, OUTPUT);
  pinMode(IRRIDIUMENABLE, OUTPUT);
  digitalWrite(IRRIDIUMENABLE, LOW); 
  pinMode(MAXENABLE,OUTPUT);
  digitalWrite(MAXENABLE,LOW);
  pinMode(LDOENABLE,OUTPUT);
  digitalWrite(LDOENABLE,LOW);
}

void setpinslow()
{
  pinMode(1,INPUT);  
  pinMode(2,INPUT);  
  pinMode(3,INPUT);  
  pinMode(4,INPUT);  
  pinMode(5,INPUT);  
  pinMode(6,INPUT);  
  pinMode(7,INPUT);  
  pinMode(8,INPUT);  
  pinMode(9,INPUT);  
  pinMode(10,INPUT);  
  pinMode(11,INPUT);  
  pinMode(12,INPUT);  
  pinMode(13,INPUT);  
}



////Need to pull String method out of setEEPROM function
void setEEPROM(int input, int index)
//Set a int value in the EEPROM Memory
{
  EEPROM.write(index,(highByte(input)));
  EEPROM.write(index+1,(lowByte(input)));
}

int getEEPROMint(int index) {
  //Get integer from memory
  byte lowByte = EEPROM.read(index + 1);
  byte highByte = EEPROM.read(index);
  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}



void dotdelay(int delayseconds) {
  for(int i=0; i<delayseconds; i++)
  {
    Serial.print(".");
    Serial.flush();
    delay(1000);
  }
}  


void clearIbuffer(){
  while(Ir_serial.available())Ir_serial.read();
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


void EEPROM_writelong(int address, unsigned long value) {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
}


unsigned long EEPROM_readlong(int address)    {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}



