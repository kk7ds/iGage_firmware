int read_i2c_int(unsigned long eeaddress ) {
    byte high,low;
    low = EEPROM1024.read(eeaddress);
    high  = EEPROM1024.read(eeaddress+1);
    return ((low << 0) & 0xFF) + ((high << 8) & 0xFF00);
}
    
  // maybe let's not read more than 30 or 32 bytes at a time!
void i2c_eeprom_read_buffer( int deviceaddress, unsigned long eeaddress, byte *buffer, int length ) {
    Wire.beginTransmission((uint8_t)((deviceaddress | eeaddress) >> 16));
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(deviceaddress,length);
    int c = 0;
    for ( c = 0; c < length; c++ )
      if (Wire.available()) buffer[c] = Wire.read();
  }


void clear_epprom(){
    digitalWrite(LEDPIN, HIGH);   // sets the LED on
    unsigned long buf[4]={0,0,0,0};
    for (unsigned long addr=0;addr<EEPROM_size;addr+=16)
    {
      i2c_eeprom_write_page( 0x50, addr, (byte*) buf, 16);
      delay(5);
    }
    digitalWrite(LEDPIN, LOW);   // sets the LED on
    Serial.println("!!!");
    delay(500);
}

 // WARNING: address is a page address, 6-bit end will wrap around
  // also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
void i2c_eeprom_write_page( int deviceaddress, unsigned long eeaddresspage, byte* data, byte length ) {
    Wire.beginTransmission((uint8_t)((0x500000 | eeaddresspage) >> 16)); // B1010xxx
    Wire.write((uint8_t)((eeaddresspage & WORD_MASK) >> 8)); // MSB
    Wire.write((uint8_t)(eeaddresspage & 0xFF)); // LSB
    byte c;
    for ( c = 0; c < length; c++)
      Wire.write(data[c]);
    Wire.endTransmission();
  }

