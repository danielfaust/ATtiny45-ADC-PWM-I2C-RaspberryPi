
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TinyWireS.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define I2C_SLAVE_ADDR  0x2F

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uint8_t u8Pwm0     = 0;
uint8_t u8Pwm1     = 0;
uint8_t u8Command  = 0;

const uint8_t adc_is_freerunning = 0; // don't use freerunning when using multiple adc's
                                      // changing ADMUX while freerunning may make things difficult
                                      // and single conversion seems to work just fine.

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void initADC() {
  
  ADCSRA =  (1 << ADPS2) |    // set prescaler to 64, bit 2 ~~ for 8MHz below is valid
            (1 << ADPS1) |    // set prescaler to 64, bit 1 ~~ 1 1 0 = ps64 = 125kHz
            (0 << ADPS0);     // set prescaler to 64, bit 0 ~~ 1 1 1 = ps128 = 62.5kHz  

  ADCSRA |= (adc_is_freerunning << ADATE); // set auto trigger enable
  ADCSRA |= (0 << ADTS2) |    // set free running mode, bit 2   
            (0 << ADTS1) |    // set free running mode, bit 1   
            (0 << ADTS0);     // set free running mode, bit 0   
  
  
  ADMUX =   (1 << REFS2) |    // Sets ref. voltage to VCC, bit 3 ~~ i2c makes use of ADREF pin, only internal vref can be used
            (1 << REFS1) |    // Sets ref. voltage to VCC, bit 1 ~~ 0 1 0 Internal 1.1V Voltage Reference.
            (0 << REFS0);     // Sets ref. voltage to VCC, bit 0 ~~ 1 1 0 Internal 2.56V Voltage Reference without external bypass capacitor, disconnected from PB0 (AREF)
  
  ADMUX =   (0 << MUX3)  |    // MUX bit 3 ~~ 0 0 0 0 = ADC0 --> PB4 | Pin3, needs RESET fuse disabled, mcu can then not be reprogrammed.
            (0 << MUX2)  |    // MUX bit 2 ~~ 0 0 0 1 = ADC1 --> PB2 | Pin7
            (1 << MUX1)  |    // MUX bit 1 ~~ 0 0 1 0 = ADC2 --> PB4 | Pin3
            (1 << MUX0);      // MUX bit 0 ~~ 0 0 1 1 = ADC3 --> PB3 | Pin2
  
  ADCSRA |= (1 << ADEN);      // Enable ADC
  
  ADCSRA |= (1 << ADSC);      // Start ADC measurement
  ADCL; ADCH;                 // "The first ADC conversion result after switching voltage reference source may be inaccurate, and the user is advised to discard this result."
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void initPWM() {
  
  DDRB  |= (1 << PB1) | (1 << PB4);
  
  // Buzzer is on Counter 0 ( OC0B ) PB1 = pin 6
  // LED is on Counter 1 ( OC1B compl. ) PB1 = pin 6
  // OC0A cannot be used, because PB0 = pin 5 is used by I2C (SDA), this frees up OCR0A which we will use as TOP for Counter 0 (via WGM0x) to get a variable frequency for the Buzzer
  // OC1A might have been used on PB1 = pin 6 for the Buzzer, but luckily OC0B did the job just fine so that this didn't get tested.
  // I guess that using OC1A wouldn't work, as we then couldn't use OCR1A as TOP for itself, since OCRA's are the only TOP that can be used in WGM0x. But it's just a guess.
  
  TCCR0A = // ------------ Timer/Counter Control Register A ------------
      (0 << COM0A1) |   // Normal port operation, OC0A disconnected. | OC0A Pin is used by I2C and we will use OC0A's register as the top value
      (0 << COM0A0) |   // ..                                        | for the Counter 0 to set the pitch of PWM OC0B
      (1 << COM0B1) |   // Clear OC0B on Compare Match when up-counting. Set OC0B on Compare Match when down-counting. <----
      (0 << COM0B0) |   // .. <----
      //(-)
      //(-)
      (0 << WGM01)  |   // (1) 0 1 = PWM, Phase Correct with -->OCR0A<-- as TOP (1/3) <----
      (1 << WGM00);     // (1) 0 1 = PWM, Phase Correct with -->OCR0A<-- as TOP (2/3) <----
  
  TCCR0B = // ------------ Timer/Counter Control Register B ------------
      (0 << FOC0A) |
      (0 << FOC0B) |
      //(-)
      //(-)
      (1 << WGM02)  |   // 1 (0 1) = PWM, Phase Correct with -->OCR0A<-- as TOP (3/3) <----
      (1 << CS02)   |   // Prescaler used to pick the base frequency <---
      (0 << CS01)   |   // of the OC0B.                              <---
      (0 << CS00);      //                                           <---
      // 2/1/0
      // 0 0 1       No prescaling   - ok
      // 0 1 0    8 (From prescaler) - not ok
      // 0 1 1   64 (From prescaler) - ok
      // 1 0 0  256 (From prescaler) - ok
      // 1 0 1 1024 (From prescaler) - ok
  
  TCCR1 = // ------------ Timer/Counter1 Control Register ------------
      (0 << CTC1)   |
      (0 << PWM1A)  |
      (0 << COM1A1) |
      (0 << COM1A0) |
      (1 << CS13)   | // Prescaler used to pick the fixed <----
      (0 << CS12)   | // frequency of the OCR1B PWM       <----
      (0 << CS11)   | //                                  <----
      (1 << CS10);    //                                  <----
      // 3/2/1/0
      // 0 0 0 0     T/C1 stopped T/C1 stopped
      // 0 0 0 1     P
      // 0 0 1 0     2
      // 0 0 1 1     4
      // 0 1 0 0     8s
      // 0 1 0 1    16
      // 0 1 1 0    32
      // 0 1 1 1    64 <---- more limited 'dynamic range'
      // 1 0 0 1   256 <---- limited 'dynamic range'
      // 1 0 0 1   256 <---- no flickering on LED, good 'dynamic range'
      // 1 0 1 0   512 <---- noticeable flickering
      // 1 0 1 1  1024 <---- more noticeable flickering
      // 1 1 0 0  2048
      // 1 1 0 1  4096
      // 1 1 1 0  8192
      // 1 1 1 1 16384
  
  GTCCR = // ------------ General Timer/Counter1 Control Register ------------
      (0 << TSM)    |
      (1 << PWM1B)  |
      (1 << COM1B1) | // 0 1 - PB4 (pin3) - COM1B 0=0V
      (0 << COM1B0) | // 1 0 - PB3 (pin2) - COM1B complementary 0=5V <----
      (0 << FOC1B)  |
      (0 << FOC1A)  |
      (0 << PSR1)   |
      (0 << PSR0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup(){
  initADC();
  initPWM();
  TinyWireS.begin(I2C_SLAVE_ADDR);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void loop() {
  if (TinyWireS.available()) {
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    u8Command = TinyWireS.receive();
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         if (u8Command == 0x00) {}  // Nothing done here, maybe implemet a reset ?
    else if (u8Command == 0x01) {}  // Nothing done here...
    else if (u8Command == 0x02) {   // Write to PWM0 = OCR1B, which is the PWM of the LED
      u8Pwm0 = TinyWireS.receive();
      OCR1B = u8Pwm0;
      return;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x03) {   // Read from PWM0 = OCR1B, which is the PWM of the LED
      TinyWireS.send(u8Pwm0);
      return;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x04) {   // Write to PWM1 = OCR0B and OCR0A, which is the PWM and looping point of the Buzzer
                                    // OCR0A sets looping point on the counter: frequency
                                    // OCR0B sets the pulse width, = OCR0A / 2 obviously.
      u8Pwm1 = TinyWireS.receive();
      if (!u8Pwm1) {
        OCR0B = 0;
        OCR0A = 1; // just in case, force this to 1, may not be neccesary
      }
      else {
        OCR0B = u8Pwm1 * 0.5;
        OCR0A = u8Pwm1;
      }
      return;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x05) {   // Read from PWM1 = OCR0B and OCR0A, which is the PWM and looping point of the Buzzer
      TinyWireS.send(u8Pwm1);
      return;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x06) {   // Read from ADC2 (PB4 | Pin3)
      if (!adc_is_freerunning) { // ADC 2, single conversion
        ADMUX =   (0 << MUX3)  |    // MUX bit 3 ~~ 0 0 0 0 = ADC0 --> PB4 | Pin3, needs RESET fuse disabled, mcu can then not be reprogrammed.
                  (0 << MUX2)  |    // MUX bit 2 ~~ 0 0 0 1 = ADC1 --> PB2 | Pin7
                  (1 << MUX1)  |    // MUX bit 1 ~~ 0 0 1 0 = ADC2 --> PB4 | Pin3
                  (1 << MUX0);      // MUX bit 0 ~~ 0 0 1 1 = ADC3 --> PB3 | Pin2
        ADCSRA |= (1 << ADSC); // start ADC measurement, needed when auto trigger enable is 0 (= single conversion)
        while (ADCSRA & (1 << ADSC)); // wait until conversion is done
      }
      TinyWireS.send(ADCL);
      TinyWireS.send(ADCH);
      return;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x07) {
      // Apparently the only free ADC is ADC0 | PB4 | Pin3, but in order to use it, the RESET fuse needs to get disabled.
      // This will most probably result in the inability to reprogram this chip, but if you know that you won't need to do
      // that again, you gain one ADC channel. You won't be able to change the I2C_SLAVE_ADDR then, so keep that in mind.
      // But I2C_SLAVE_ADDR could be written to EEPROM and be writable by the host...
      /*
        if (!adc_is_freerunning) { // ADC 0, single conversion
          ADMUX =   (0 << MUX3)  |    // MUX bit 3 ~~ 0 0 0 0 = ADC0 --> PB4 | Pin3, needs RESET fuse disabled, mcu can then not be reprogrammed.
                    (0 << MUX2)  |    // MUX bit 2 ~~ 0 0 0 1 = ADC1 --> PB2 | Pin7
                    (0 << MUX1)  |    // MUX bit 1 ~~ 0 0 1 0 = ADC2 --> PB4 | Pin3
                    (0 << MUX0);      // MUX bit 0 ~~ 0 0 1 1 = ADC3 --> PB3 | Pin2
          ADCSRA |= (1 << ADSC); // start ADC measurement, needed when auto trigger enable is 0 (= single conversion)
          while (ADCSRA & (1 << ADSC)); // wait until conversion is done
        }
        TinyWireS.send(ADCL);
        TinyWireS.send(ADCH);
      */
      TinyWireS.send(0); // Not implemented, return 0
      TinyWireS.send(0); // Not implemented, return 0
      return;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
  }
}
