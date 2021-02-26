// Sine output test MCVE for SuperAudioBoard and Teensy 3.2.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 I2C and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler.  Updated 26/02/2021.

// To start the SuperAudioBoard output/acquisition loop, send character "A" over serial port.

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
  else if(serialData[0] != 0x41){ print_error(0); }  // starts with "A"
  
  // Read rest of data
  while (Serial.available() > 0){ Serial.read(); }
 
  if(serialData[0] == 0x41){
  state=State_ISR;
      counter=0;
      datapts=0;
      dataLength = 256;
      i2s_start();    // start sampling
  }
}
