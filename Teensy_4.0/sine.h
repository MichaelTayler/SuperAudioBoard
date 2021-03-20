// Sine output test MCVE for SuperAudioBoard and Teensy 4.0.  
// Frequency limited to 48 kHz subharmonics.
// Upload via Arduino IDE and Teensyduino
// CS4272 I2C and I2S code based on https://github.com/whollender/SuperAudioBoard
// Written by Michael Tayler.  Updated 19/03/2021.
// Reference sine wave, 32 points per period.  24-bit signed integer left-justified on 32 bits for I2S.
const int64_t sine32_ref[32] = {
0,
418949087,
821798195,
1193066060,
1518485065,
1785549541,
1983996349,
2106199290,
2147462173,
2106199290,
1983996349,
1785549541,
1518485065,
1193066060,
821798195,
418949087,
0,
-418949087,
-821798195,
-1193066060,
-1518485065,
-1785549541,
-1983996349,
-2106199290,
-2147462173,
-2106199290,
-1983996349,
-1785549541,
-1518485065,
-1193066060,
-821798195,
-418949087
};
