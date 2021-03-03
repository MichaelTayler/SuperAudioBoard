// Sine output test MCVE for SuperAudioBoard and Teensy 4.0.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 I2C and I2S code TODO
// Written by Michael Tayler.  Updated 03/03/2021.

// Missing from Teensyduino avr/cores/teensy4/imxrt.h
#define I2S_TCR1_TFW(x)        (uint8_t)(((x) & 0x1F) << 0)
#define I2S_TCSR_FRIE 0x100

void i2s_init() {
  /* OLD Teensy 3.2 clock setup
  SIM_SCGC6 |= SIM_SCGC6_I2S;
  I2S1_MCR = I2S_MCR_MICS(0);   // Using external MCLK, so just effectively shut this off
  I2S1_MDR = 0;                 // Clock divide ratio register = 0
  */
  // New Teensy 4.0 clock setup
  // Cribbed from https://github.com/PaulStoffregen/Audio/blob/master/output_i2s.cpp#L393
  // Major difference from Teensy audio library setup is that MCLK is an input because the CS4272
  // generates it's own master clock from the crystal on board
  CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON); // Turn clock on to SAI1 (I2S1)
  IOMUXC_GPR_GPR1 = IOMUXC_GPR_GPR1 & ~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR; // Set SAI1 (I2S1) MCLK direction to input

  ///////////////////////
  // Setup TX side
  ///////////////////////

  // NOTE: Teensy 4 uses I2S1 instead of I2S0

  I2S1_TMR = 0; // Don't mask any words
  I2S1_TCR1 = I2S_TCR1_TFW(2); // Set watermark to one word
  // TODO: need to research watermark positioning a bit better
  // Setup for 24 bit left justified.  Set sync off (TX is master),
  // Bit clock selected for active low (drive on falling edge, sample on rising edge)
  I2S1_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP;  // Bit clock generated externally (slave mode)
  I2S1_TCR3 = I2S_TCR3_TCE; // Only enable ch 0

  // Frame size is 2 (L + R), sync width is 32 (LR clock is active for first word),
  // MSB first, LR clock asserted with first bit
  // LR clock is active low, and LR clock generated externally.
  I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF | I2S_TCR4_FSP;

  // Set up all word widths to 31, then right shift after reading
  // to get 24 bit data.
  // Could probably set the first word width to 31, remaining widths
  // to 23, and set the first bit to 23, but not sure if that would work
  I2S1_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

  //////////////////////////////
  // Setup Rx side
  // (pretty much the same as tx
  // but set sync to tx clocks)
  //////////////////////////////
  
  I2S1_RMR = 0; // Don't mask any words
  I2S1_RCR1 = I2S_RCR1_RFW(2); // Set watermark to one word
  I2S1_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP;  // Same settings as tx, but sync to tx
  I2S1_RCR3 = I2S_RCR3_RCE; // Enable ch 0
  // two words per frame, sync width 16 bit clocks, MSB first, sync early,
  I2S1_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF | I2S_RCR4_FSP;  // LR clock(sync) active low, sync generated internally
  I2S1_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);  // See TX word width discussion above

  // Setup pins
  /* Teensy 3.x pin setup
  PORTC_PCR1 = PORT_PCR_MUX(6); // TX
  PORTC_PCR2 = PORT_PCR_MUX(6); // LRCLK
  PORTC_PCR3 = PORT_PCR_MUX(6); // Bit clock
  PORTC_PCR5 = PORT_PCR_MUX(4); // RX
  PORTC_PCR6 = PORT_PCR_MUX(6); // MCLK
  */
  // Teensy 4 pin setup
  CORE_PIN8_CONFIG  = 3;  // RX
  CORE_PIN7_CONFIG  = 3;  // RX
  CORE_PIN23_CONFIG  = 3;  // MCLK
  CORE_PIN20_CONFIG  = 3;  // LRCLK
  CORE_PIN21_CONFIG  = 3;  // BCLK

//  attachInterruptVector(IRQ_SAI1,i2s1_tx_isr);
}

void i2s_start() {
  // Enable RX first as per Ref manual (I2S chapter, section 4.3.1)
  __disable_irq();
  
  // Updated 5/22/14 to disable receiver interrupts and do
  // all processing in i2s tx interrupt
  // -- RFWH
  
  // RX: enable, reset fifo, and interrupt on fifo request
  //I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_FR | I2S_RCSR_FRIE;
  I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_FR;

  // TX: enable, bit clock enable, reset fifo, and interrupt on fifo req
  //
  // Not sure if need bit clock enable for slave setup
  I2S1_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FR | I2S_TCSR_FRIE;

  // Load up TX FIFO so that the ISR isn't called immediately (with no
  // data available in RX fifo).  Four writes required to get data past
  // watermark value (2) and still use correct channel (L/R).
  I2S1_TDR0 = 0;
  I2S1_TDR0 = 0;
  I2S1_TDR0 = 0;
  I2S1_TDR0 = 0;

  // enable IRQs
  //NVIC_ENABLE_IRQ(IRQ_I2S1_RX);
  NVIC_ENABLE_IRQ(IRQ_SAI1);
  __enable_irq();
}

void i2s_stop() {
  __disable_irq();
  NVIC_DISABLE_IRQ(IRQ_SAI1);
  //NVIC_DISABLE_IRQ(IRQ_I2S1_RX);
  I2S1_TCSR &= ~I2S_TCSR_TE;
  I2S1_RCSR &= ~I2S_RCSR_RE;
  __enable_irq();
}
