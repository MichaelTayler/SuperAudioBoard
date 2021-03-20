// Sine output test for SuperAudioBoard and Teensy 3.2.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler.  Updated 02/03/2021.

enum State {State_Idle, State_ISR, State_Transfer, State_Flush};
State state = State_Idle;

volatile unsigned int counter;
// volatile byte data1[16384], data2[16384], data3[16384];
volatile int32_t ina[256];

#include "Wire.h"
#include "i2s.h"
#include "cs4272.h"
#include "sine.h"
#include "serial.h"

void setup(){
  Serial_init();
  codec_init(); delay(10);    // Initialize CS4272
  i2s_init();   delay(10);    // Initialize I2S subsystem 
}

void loop(){
  switch (state){
    case State_Idle:
    readSerialData();  
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

void i2s0_tx_isr(void){
// ----------------------------------------------------------------------
// -- Write 1.5 kHz sine wave to DAC (= 48 kHz / 32 points per period) --
// ----------------------------------------------------------------------
    I2S0_TDR0 = 0;                                  // OUT_B
    I2S0_TDR0 = (sine32_ref[(counter) & 0xFF])/2;   // OUT_A

//  Data acquisition: (TODO)
//  ina[counter] = (I2S0_RDR0);                     // IN_B
//  ina[counter] = (I2S0_RDR0);                     // IN_A

//  data1[datapts] = (((uint32_t)ina[counter])>>24)&0xFF;
//  data2[datapts] = (((uint32_t)ina[counter])>>16)&0xFF;
//  data3[datapts] = (((uint32_t)ina[counter])>>8)&0xFF; // channel A data, 24-bit signed integer left-justified on 32 bits
// ----------------------------------------------------------------------

  counter++;
  if(counter % 2 == 0){datapts++;}  // Downsampling at subharmonic of fmod.  Change modulo integer  
  if(datapts >= dataLength){i2s_stop(); state=State_Transfer;}  // if max points reached, stop
}

void transferData(){
//  for(unsigned int i=0; i<dataLength; i++){  //  TODO: transfer data via USB to PC
//    Serial.write(data1[i]);  
//    Serial.write(data2[i]);  
//    Serial.write(data3[i]);  
//  }           

  Serial.println("Done!"); //  For now, send a message that let's us know we've finished
  Serial.flush();
  state = State_Idle;
}
