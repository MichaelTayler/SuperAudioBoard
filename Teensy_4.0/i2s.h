// Sine output test MCVE for SuperAudioBoard and Teensy 4.0.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 I2C and I2S code TODO
// Written by Michael Tayler and RF William Hollender.  Updated 19/03/2021.

// Missing from Teensyduino avr/cores/teensy4/imxrt.h
#define I2S_TCR1_TFW(x)        (uint8_t)(((x) & 0x1F) << 0)
#define I2S_TCSR_FRIE 0x100
#define I2S_RCSR_FRIE 0x100

void i2s_stop() {
  __disable_irq();
  NVIC_DISABLE_IRQ(IRQ_SAI1);
  I2S1_TCSR &= ~I2S_TCSR_TE;
  I2S1_RCSR &= ~I2S_RCSR_RE;
  __enable_irq();
}

void sai1_isr(void){
  I2S1_RDR0;                                                // IN_B
  rawa = ((int32_t)(I2S1_RDR0));                            // IN_A
  I2S1_TDR0 = 0;                                            // OUT_B
  I2S1_TDR0 = sine32_ref[sinecounter]&0xFFFFFF00;           // OUT_A, 1.5 kHz output
  
  ina[datapts]=(biquad2/4096)&0xFFFFFF00; 
  sinecounter=(sinecounter+1)%32; // counts 0,1,2, ..., n-1

  // Update counters  
  counter++;  if(counter % 8 == 0){datapts++;}      // Downsampling at subharmonic of fmod.
  if(datapts >= dataLength){i2s_stop();state=State_Transfer;}  // if max points reached, stop
}

void i2s_init() {
  CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON); // Turn clock on to SAI1 (I2S1)
  IOMUXC_GPR_GPR1 = IOMUXC_GPR_GPR1 & ~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR; // Set SAI1 (I2S1) MCLK direction to input

  // Teensy 4 pin setup and MUX
  CORE_PIN7_CONFIG  = 3;   // SAI_TX_DATA00         B1_00
  CORE_PIN8_CONFIG  = 3;   // SAI_RX_DATA00         B1_01
  CORE_PIN23_CONFIG  = 3;  // SAI1_MCLK             AD_B1_09
  CORE_PIN20_CONFIG  = 3;  // SAI1_RX_SYNC = LRCLK  AD_B1_10
  CORE_PIN21_CONFIG  = 3;  // SAI1_RX_BCLK = BCLK   AD_B1_11
  IOMUXC_SAI1_MCLK2_SELECT_INPUT = 1; // 1=GPIO_AD_B1_09_ALT3, page 871
  IOMUXC_SAI1_RX_BCLK_SELECT_INPUT = 1; // 1=GPIO_AD_B1_11_ALT3, page 868
  IOMUXC_SAI1_RX_SYNC_SELECT_INPUT = 1; // 1=GPIO_AD_B1_10_ALT3, page 872
  IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // 2 = GPIO_B1_00, page 873
  
  // Setup TX
  I2S1_TMR = 0; // Don't mask any words
  I2S1_TCR1 = I2S_TCR1_TFW(2); // Watermark = 1 word
  I2S1_TCR2 = I2S_TCR2_SYNC(1) | I2S_TCR2_BCP;  // Bit clock generated externally (slave mode)   // Bit clock selected for active low (drive on falling edge, sample on rising edge)
  I2S1_TCR3 = I2S_TCR3_TCE; // Only enable ch 0
  // Frame size is 2 (L + R), sync width is 32 (LR clock is active for first word),
  I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF | I2S_TCR4_FSP;   // LR clock is active low, and LR clock generated externally.  // MSB first, LR clock asserted with first bit
  I2S1_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);  // Word widths = 31

  // Setup RX
  I2S1_RMR = 0; // Don't mask any words
  I2S1_RCR1 = I2S_RCR1_RFW(2); // Set watermark to one word
  I2S1_RCR2 = I2S_RCR2_SYNC(0) | I2S_TCR2_BCP;  // Same settings as tx, but sync to tx
  I2S1_RCR3 = I2S_RCR3_RCE; // Enable ch 0,
  I2S1_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF | I2S_RCR4_FSP;  // LR clock(sync) active low, sync generated internally   // two words per frame, sync width 16 bit clocks, MSB first, sync early
  I2S1_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);  // See TX word width discussion above
}

void i2s_start() {
  // Reset counters and filter buffer
  sinecounter=0;counter=0;datapts=0;
    
  __disable_irq();
  I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_FR ;// | I2S_RCSR_FRIE; // RX: enable, reset fifo, and interrupt on fifo request
  I2S1_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FR | I2S_TCSR_FRIE;  // TX: enable, bit clock enable, reset fifo, interrupt on fifo req
  I2S1_TDR0 = 0;  // Load TX FIFO so that the ISR isn't called with empty RX fifo.  Four writes required to get data past watermark value (2) and still use correct channel (L/R).
  I2S1_TDR0 = 0;
  I2S1_TDR0 = 0;
  I2S1_TDR0 = 0;
  attachInterruptVector(IRQ_SAI1, sai1_isr);
  NVIC_ENABLE_IRQ(IRQ_SAI1);
  __enable_irq();
}
