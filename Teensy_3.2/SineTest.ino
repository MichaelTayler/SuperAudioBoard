// Sine output test for SuperAudioBoard and Teensy 3.2.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler.  Updated 19/03/2021.

enum State {State_Idle, State_Aqcuire, State_ISR, State_Transfer, State_Flush};
State state = State_Idle;

volatile unsigned int counter;
volatile int32_t ina[10000];
byte debug=0;

#include "Wire.h"
#include "cs4272.h"
#include "sine.h"
#include "serial.h"
#include "i2s.h"

void setup(){
  delay(100);
  Serial_init();
  codec_init(); delay(10);    // Initialize CS4272
  i2s_init();   delay(10);    // Initialize I2S subsystem 
  // Read CS4272 registers  
if(debug==1){
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
    i2s_start();  break; // start sampling

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
    I2S0_TDR0 = sine32_ref[sinecounter];            // OUT_A

//  Data acquisition: (TODO)
//  (I2S0_RDR0);                                    // IN_B
//  ina[datapts] = (I2S0_RDR0);                     // IN_A
// ----------------------------------------------------------------------

  // Update counters
  counter++;
  sinecounter=(sinecounter+1)%32; // counts 0,1,2, ..., n-1
  if(counter % 2 == 0){datapts++;}  // Downsampling at subharmonic of fmod.  Change modulo integer  
  if(datapts >= dataLength){i2s_stop(); state=State_Transfer;}  // if max points reached, stop
}

void transferData(){ 
  delay(10);
//  for(unsigned int i=0; i<dataLength; i++){ // Serial output to PC
//    data1 = (((uint32_t)ina[i])>>24)&0xFF;
//    data2 = (((uint32_t)ina[i])>>16)&0xFF;
//    data3 = (((uint32_t)ina[i])>>8)&0xFF;
//    Serial.write(data1);  
//    Serial.write(data2);  
//    Serial.write(data3);
//  }

  if(debug==1){Serial.println("Done!");} //  For now, send a message that let's us know we've finished
  Serial.flush();
  state = State_Idle;
}
