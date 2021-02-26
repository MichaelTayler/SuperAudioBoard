// Sine output test MCVE for SuperAudioBoard and Teensy 3.2.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 I2C and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler.  Updated 26/02/2021.

// Rx data in upper 24 bits of 32-bit int.  Reading converts uint32_t to int32_t (keeping sign intact).  Then we need to
// right shift by 8 bits to get the 24 bits we want, in the right place (assuming the compile will do an arithmetic shift.
// i.e. new bits is same as previous MSB to sign extend). 

// For CS4272 (uC i2s interface in slave mode) not sure about initialization sequence 
// (i2s interface first, or setup cs4272 first). For now, start with codec (start interface clocks first)

// Section 8.1 Mode Control
#define CODEC_MODE_CONTROL							(uint8_t)0x01
#define CODEC_MC_FUNC_MODE(x)						(uint8_t)(((x) & 0x03) << 6)
#define CODEC_MC_RATIO_SEL(x)						(uint8_t)(((x) & 0x03) << 4)
#define CODEC_MC_MASTER_SLAVE						(uint8_t)0x08
#define CODEC_MC_SERIAL_FORMAT(x)				(uint8_t)(((x) & 0x07) << 0)

// Section 8.2 DAC Control
#define CODEC_DAC_CONTROL							  (uint8_t)0x02
#define CODEC_DAC_CTRL_AUTO_MUTE				(uint8_t)0x80
#define CODEC_DAC_CTRL_FILTER_SEL				(uint8_t)0x40
#define CODEC_DAC_CTRL_DE_EMPHASIS			(uint8_t)(((x) & 0x03) << 4)
#define CODEC_DAC_CTRL_VOL_RAMP_UP			(uint8_t)0x08
#define CODEC_DAC_CTRL_VOL_RAMP_DN			(uint8_t)0x04
#define CODEC_DAC_CTRL_INV_POL					(uint8_t)(((x) & 0x03) << 0)

// Section 8.3 DAC Volume and Mixing
#define CODEC_DAC_VOL								    (uint8_t)0x03
#define CODEC_DAC_VOL_CH_VOL_TRACKING		(uint8_t)0x40
#define CODEC_DAC_VOL_SOFT_RAMP					(uint8_t)(((x) & 0x03) << 4)
#define CODEC_DAC_VOL_ATAPI							(uint8_t)(((x) & 0x0F) << 0)

// Section 8.4 DAC Channel A volume
#define CODEC_DAC_CHA_VOL							  (uint8_t)0x04
#define CODEC_DAC_CHA_VOL_MUTE					(uint8_t)0x80
#define CODEC_DAC_CHA_VOL_VOLUME				(uint8_t)(((x) & 0x7F) << 0)

// Section 8.5 DAC Channel B volume
#define CODEC_DAC_CHB_VOL							  (uint8_t)0x05
#define CODEC_DAC_CHB_VOL_MUTE					(uint8_t)0x80
#define CODEC_DAC_CHB_VOL_VOLUME				(uint8_t)(((x) & 0x7F) << 0)

// Section 8.6 ADC Control
#define CODEC_ADC_CTRL								  (uint8_t)0x06
#define CODEC_ADC_CTRL_DITHER						(uint8_t)0x20
#define CODEC_ADC_CTRL_SER_FORMAT				(uint8_t)0x10
#define CODEC_ADC_CTRL_MUTE							(uint8_t)(((x) & 0x03) << 2)
#define CODEC_ADC_CTRL_HPF							(uint8_t)(((x) & 0x03) << 0)

// Section 8.7 Mode Control 2
#define CODEC_MODE_CTRL2							  (uint8_t)0x07
#define CODEC_MODE_CTRL2_LOOP						(uint8_t)0x10
#define CODEC_MODE_CTRL2_MUTE_TRACK			(uint8_t)0x08
#define CODEC_MODE_CTRL2_CTRL_FREEZE		(uint8_t)0x04
#define CODEC_MODE_CTRL2_CTRL_PORT_EN		(uint8_t)0x02
#define CODEC_MODE_CTRL2_POWER_DOWN			(uint8_t)0x01

// Section 8.8 Chip ID
#define CODEC_CHIP_ID								    (uint8_t)0x08
#define CODEC_CHIP_ID_PART							(uint8_t)(((x) & 0x0F) << 4)
#define CODEC_CHIP_ID_REV						  	(uint8_t)(((x) & 0x0F) << 0)

// Setup I2C address for codec
#define CODEC_ADDR 0x10 

// For CS4272 all data is written between single start/stop sequence
void codec_write(uint8_t reg, uint8_t data){  
  uint8_t buf[2];
  buf[0] = reg;  buf[1] = data;
  i2c_write(CODEC_ADDR,2,buf);
}

// No waveform demo for read so assume first write MAP, then rep-start (or stop and start), then read
uint8_t codec_read(uint8_t reg){  
  i2c_write(CODEC_ADDR,1,&reg);
  uint8_t buf;
  if(i2c_read(CODEC_ADDR,1,&buf) != 1) {return 0;}
  return buf;
}

// Setup Initial Codec
void codec_init(){
  i2c_init();  delay(100);  // Initialize I2C
  pinMode(2,OUTPUT); // Setup Reset pin (GPIO) Teensy pin 2  
  digitalWrite(2,LOW);
  delay(1);
  digitalWrite(2,HIGH);
  delay(2);  // Wait for ~2-5ms (1-10 ms time window spec'd in datasheet)
  
  // Set power down and control port enable as spec'd in the datasheet for control port mode
  codec_write(CODEC_MODE_CTRL2, CODEC_MODE_CTRL2_POWER_DOWN | CODEC_MODE_CTRL2_CTRL_PORT_EN);
  delay(1);

  // Set ratio select for MCLK=512*LRCLK (BCLK = 64*LRCLK), and master mode
  codec_write(CODEC_MODE_CONTROL, CODEC_MC_RATIO_SEL(2) | CODEC_MC_MASTER_SLAVE); // Left-justified data
  //codec_write(CODEC_MODE_CONTROL, CODEC_MC_RATIO_SEL(2) | CODEC_MC_MASTER_SLAVE | CODEC_MC_SERIAL_FORMAT(3)); // Right-justified data
  delay(10);

  // ADC right justify 24-bit data 
  //  codec_write(CODEC_ADC_CTRL, CODEC_ADC_CTRL_SER_FORMAT); // Right-justified 24-bit data
  //  delay(10);
  
  codec_write(CODEC_MODE_CTRL2, CODEC_MODE_CTRL2_CTRL_PORT_EN);  // Release power down bit to start up codec
  delay(10);                                                     // Wait for everything to come up
}
