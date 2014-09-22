void set_times(){
    log_time = ((now()/interval)+1UL)*interval;
    send_sat_time = ((now()/irid_interval)+1UL)*irid_interval;
}  

void print_time(byte timebyte, int timeint){
     byte temp,temp2;
     int tempint;
     temp = B1111000 & timebyte;
     tempint = (temp >> 4)+2010;
     Serial.print(tempint);
     Serial.print("/");
     temp = B00001111 & timebyte;
     Serial.print(temp);
     Serial.print("/");
     temp = B11111000 & highByte(timeint);
     temp = temp >>3;
     Serial.print(temp);
     Serial.print(" ");
     temp = B00000111 & highByte(timeint);
     temp = temp << 2;
     temp2 = B11000000 & lowByte(timeint);
     temp2 = temp2 >> 6;
     temp = temp | temp2;
     Serial.print(temp);
     Serial.print(":");
     tempint = B00111111 & lowByte(timeint);
     if(tempint < 10) Serial.print("0");
     Serial.print(tempint);
}


void setDateDs1307(char inp[15])                
{
  int seconds, minutes, hours, days, months, years;
  seconds = (int) ((inp[1] - 48) * 10 + (inp[2] - 48)); // Use of (byte) type casting and ascii math to achieve 
  minutes = (int) ((inp[3] - 48) *10 +  (inp[4] - 48));
  hours  = (int) ((inp[5] - 48) *10 +  (inp[6] - 48));
  days  = (int) ((inp[7] - 48) *10 +  (inp[8] - 48));
  months = (int) ((inp[9] - 48) *10 +  (inp[10] - 48));
  years= (int) ((inp[11] - 48) *10 +  (inp[12] - 48));
  setTime(hours,minutes,seconds,days,months,years); // another way to set the time
  RTC.set(now());
}


void digitalClockDisplay(long time){
  // digital clock display of the time
  Serial.print(hour(time));
  printDigits(minute(time));
  printDigits(second(time));
  Serial.print(" ");
  Serial.print(dayStr(weekday(time)));
  Serial.print(" ");
  Serial.print(day(time));
  Serial.print(" ");
  Serial.print(monthShortStr(month(time)));
  Serial.print(" ");
  Serial.print(year(time)); 
  Serial.println(); 
  Serial.flush();
}

 

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
  Serial.flush();
}



