// Sine output test MCVE for SuperAudioBoard and Teensy 4.0.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler.  Updated 03/03/2021.

  // Rx data in upper 24 bits of 32-bit int.  Reading converts uint32_t to int32_t (keeping sign intact).  Then we need to
  // right shift by 8 bits to get the 24 bits we want, in the right place (assuming the compile will do an arithmetic shift.
  // i.e. new bits is same as previous MSB to sign extend). 

  // For CS4272 (uC i2s interface in slave mode) not sure about initialization sequence 
  // (i2s interface first, or setup cs4272 first). For now, start with codec (start interface clocks first)

// Section 8.1 Mode Control
#define CS4272_MODE_CONTROL							(uint8_t)0x01
#define CS4272_MC_FUNC_MODE(x)						(uint8_t)(((x) & 0x03) << 6)
#define CS4272_MC_RATIO_SEL(x)						(uint8_t)(((x) & 0x03) << 4)
#define CS4272_MC_MASTER_SLAVE						(uint8_t)0x08
#define CS4272_MC_SERIAL_FORMAT(x)				(uint8_t)(((x) & 0x07) << 0)

// Section 8.2 DAC Control
#define CS4272_DAC_CONTROL							  (uint8_t)0x02
#define CS4272_DAC_CTRL_AUTO_MUTE				(uint8_t)0x80
#define CS4272_DAC_CTRL_FILTER_SEL				(uint8_t)0x40
#define CS4272_DAC_CTRL_DE_EMPHASIS(x)		(uint8_t)(((x) & 0x03) << 4)
#define CS4272_DAC_CTRL_VOL_RAMP_UP			(uint8_t)0x08
#define CS4272_DAC_CTRL_VOL_RAMP_DN			(uint8_t)0x04
#define CS4272_DAC_CTRL_INV_POL(x)				(uint8_t)(((x) & 0x03) << 0)

// Section 8.3 DAC Volume and Mixing
#define CS4272_DAC_VOL								    (uint8_t)0x03
#define CS4272_DAC_VOL_CH_VOL_TRACKING		(uint8_t)0x40
#define CS4272_DAC_VOL_SOFT_RAMP(x)			(uint8_t)(((x) & 0x03) << 4)
#define CS4272_DAC_VOL_ATAPI(x)					(uint8_t)(((x) & 0x0F) << 0)

// Section 8.4 DAC Channel A volume
#define CS4272_DAC_CHA_VOL							  (uint8_t)0x04
#define CS4272_DAC_CHA_VOL_MUTE					(uint8_t)0x80
#define CS4272_DAC_CHA_VOL_VOLUME(x) 		(uint8_t)(((x) & 0x7F) << 0)

// Section 8.5 DAC Channel B volume
#define CS4272_DAC_CHB_VOL							  (uint8_t)0x05
#define CS4272_DAC_CHB_VOL_MUTE					(uint8_t)0x80
#define CS4272_DAC_CHB_VOL_VOLUME(x)			(uint8_t)(((x) & 0x7F) << 0)

// Section 8.6 ADC Control
#define CS4272_ADC_CTRL								  (uint8_t)0x06
#define CS4272_ADC_CTRL_DITHER						(uint8_t)0x20
#define CS4272_ADC_CTRL_SER_FORMAT				(uint8_t)0x10
#define CS4272_ADC_CTRL_MUTE(x)					(uint8_t)(((x) & 0x03) << 2)
#define CS4272_ADC_CTRL_HPF(x)						(uint8_t)(((x) & 0x03) << 0)

// Section 8.7 Mode Control 2
#define CS4272_MODE_CTRL2							  (uint8_t)0x07
#define CS4272_MODE_CTRL2_LOOP						(uint8_t)0x10
#define CS4272_MODE_CTRL2_MUTE_TRACK			(uint8_t)0x08
#define CS4272_MODE_CTRL2_CTRL_FREEZE		(uint8_t)0x04
#define CS4272_MODE_CTRL2_CTRL_PORT_EN		(uint8_t)0x02
#define CS4272_MODE_CTRL2_POWER_DOWN			(uint8_t)0x01

// Section 8.8 Chip ID
#define CS4272_CHIP_ID							    	(uint8_t)0x08
#define CS4272_CHIP_ID_PART(x)						(uint8_t)(((x) & 0x0F) << 4)
#define CS4272_CHIP_ID_REV(x)						(uint8_t)(((x) & 0x0F) << 0)

#define CS4272_RESET_PIN 2 
#define CS4272_ADDR 0x10 
uint8_t reglocal[8]; 

// For CS4272 all data is written between single start/stop sequence
void codec_write(uint8_t reg, uint8_t data){  
  Wire.beginTransmission(CS4272_ADDR);
  Wire.write(reg & 0xFF);
  Wire.write(data & 0xFF);
  Wire.endTransmission();
}

// Initialize registers to CS4272 reset status
void initlocalregisters()
{
  reglocal[CS4272_MODE_CONTROL] = 0x00;
  reglocal[CS4272_DAC_CONTROL]  = CS4272_DAC_CTRL_AUTO_MUTE;
  reglocal[CS4272_DAC_VOL]      = CS4272_DAC_VOL_SOFT_RAMP(2) | CS4272_DAC_VOL_ATAPI(9);
  reglocal[CS4272_DAC_CHA_VOL]  = 0x00;
  reglocal[CS4272_DAC_CHB_VOL]  = 0x00;
  reglocal[CS4272_ADC_CTRL]     = 0x00;  
  reglocal[CS4272_MODE_CTRL2]   = 0x00;
}

// Setup Initial Codec
void codec_init(void){
  Wire.begin();  delay(10);
  initlocalregisters();  
  delay(100);  // Initialize I2C
  pinMode(CS4272_RESET_PIN,OUTPUT);
  digitalWrite(CS4272_RESET_PIN,LOW);
  delay(1);
  digitalWrite(CS4272_RESET_PIN,HIGH);
  delay(2);  // Wait for ~2-5ms (1-10 ms time window spec'd in datasheet)
  codec_write(CS4272_MODE_CTRL2, CS4272_MODE_CTRL2_POWER_DOWN | CS4272_MODE_CTRL2_CTRL_PORT_EN);   // Set power down and control port enable as spec'd in the datasheet for control port mode
  delay(1);

  // Set ratio select for MCLK=512*LRCLK (BCLK = 64*LRCLK), and master mode
  codec_write(CS4272_MODE_CONTROL, CS4272_MC_RATIO_SEL(2) | CS4272_MC_MASTER_SLAVE); // Left-justified data
  //codec_write(CS4272_MODE_CONTROL, CS4272_MC_RATIO_SEL(2) | CS4272_MC_MASTER_SLAVE | CS4272_MC_SERIAL_FORMAT(3)); // Right-justified data
  delay(10);

  // ADC right justify 24-bit data 
  //  codec_write(CS4272_ADC_CTRL, CS4272_ADC_CTRL_SER_FORMAT); // Right-justified 24-bit data
  //  delay(10);
  
  codec_write(CS4272_MODE_CTRL2, CS4272_MODE_CTRL2_CTRL_PORT_EN);  // Release power down bit to start up codec
  delay(10);                                                     // Wait for everything to come up
}
