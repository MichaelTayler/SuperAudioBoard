byte serialData[4];
unsigned int dataLength;
volatile unsigned int datapts;

void Serial_init(){Serial.begin(115200); delay(100);}
void print_error(byte err){while (Serial.available() > 0){ Serial.read(); } Serial.println(err);}
  
void readSerialData(){

  while (Serial.available() == 0){}
  delay(10);
  
  // Data valid?
  if (Serial.readBytes((char*)serialData, 3) != 3){ print_error(0); } 
//  else if(serialData[0] != 0x41){ print_error(0); }  // starts with "A"
  
  while (Serial.available() > 0){ Serial.read(); }    // Read rest of data
   
  if(serialData[0] == 0x41){
  state=State_ISR;
      counter=0;
      datapts=0;
      dataLength = 256;
      Serial.print("Ready...  ");
      delay(100);
      i2s_start();    // start sampling
  }
  else{Serial.println("Error.  Try again.");}
}
