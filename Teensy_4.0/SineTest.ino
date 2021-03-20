// Sine output test for SuperAudioBoard and Teensy 4.0.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 I2C and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler and RF William Hollender.  Updated 19/03/2021.

//TO DO: 
// - Document I2S pinout differences between T3.2 and T4.0
// - New KiCAD file for SuperAudioBoard + T4.0

enum State {State_Idle, State_Acquire, State_ISR, State_Transfer, State_Flush};
State state = State_Idle;
byte debug=0;
volatile unsigned int counter;
volatile int32_t rawa, ina[100000];

#include "Wire.h"
#include "i2s.h"
#include "cs4272.h"
#include "sine.h"
#include "serial.h"

void setup(){
  Serial_init();
  codec_init(); delay(10);    // Initialize CS4272
  i2s_init();   delay(10);    // Initialize I2S subsystem 
  if(debug==1){// Print out CS4272 registers
  Serial.println("CS4272 registers after init");
  for(unsigned int i = 1; i < 9; i++)
  {
    Wire.beginTransmission(CS4272_ADDR);
    Wire.write(i);
    int ii = Wire.endTransmission();
    if(ii != 0)
    {
      Serial.println("Error in end transmission:");  Serial.println(ii);
      break;
    }
    if(Wire.requestFrom(CS4272_ADDR,1) < 1)
    {
      Serial.println("Error in request from");
      break;
    }  
    Serial.print(i);
    Serial.print(" ");
    Serial.println(Wire.read());
  }
}  
}

void loop(){
  switch (state){
    case State_Idle:
    readSerialData();  
    break;
      
    case State_Acquire:
    state=State_ISR;
    i2s_start();
    break;
 
    case State_Transfer:
    transferData();
    break;

    case State_Flush:
    delay(10);
    while (Serial.available() > 0){Serial.read();}          
    state = State_Idle;
    break;

    default:
    break;
  }
}

void transferData(){
//  for(unsigned int i=0; i<dataLength; i++){  // transfer data via USB to PC
//    Serial.write(data1[i]);  
//    Serial.write(data2[i]);  
//    Serial.write(data3[i]);  
//  }           

  if(debug==1){Serial.println("Done!");} //  For now, send a message that let's us know we've finished
  Serial.flush();
  state = State_Idle;
}
