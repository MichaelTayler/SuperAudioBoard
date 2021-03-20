byte serialData[4];
unsigned int dataLength;
volatile unsigned int datapts;
volatile byte sinecounter;

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
  state=State_Acquire;
  dataLength = 10000;    
  }
  else{Serial.println("Error.  Try again.");}
}
