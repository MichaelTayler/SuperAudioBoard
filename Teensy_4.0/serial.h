// Sine output test MCVE for SuperAudioBoard and Teensy 4.0.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 I2C and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler.  Updated 19/03/2021.

// To start the SuperAudioBoard output/acquisition loop, send character "A" over serial port.

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
  else if(serialData[0] != 0x41){ print_error(0); }  // starts with "A"
  
  // Read rest of data
  while (Serial.available() > 0){ Serial.read(); }
 
  if(serialData[0] == 0x41){
    state=State_Acquire;
    dataLength = 10000;
  }
}
