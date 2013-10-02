
#define F_CPU 8000000

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TinyWireS.h"
//#include "usiTwiSlave.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define I2C_SLAVE_ADDR  0x2F
#define PIN_2__PB_3     3   // ATtiny Pin 2 // used by adc3
#define PIN_3__PB_4     4   // ATtiny Pin 3 // used as pwm
#define PIN_6__PB_1     1   // ATtiny Pin 6

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool          bTrigger[] = {false, false};
unsigned int  uiPwm[]    = {0, 0};
uint8_t       u8Received = 0;
uint8_t       u8Command  = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void initADC() {
  
  ADCSRA =  (1 << ADPS2) |    // set prescaler to 64, bit 2 ~~ for 8MHz below is valid
            (1 << ADPS1) |    // set prescaler to 64, bit 1 ~~ 1 1 0 = ps64 = 125kHz
            (0 << ADPS0);     // set prescaler to 64, bit 0 ~~ 1 1 1 = ps128 = 62.5kHz  

  ADCSRA |= (1 << ADATE);     // set auto trigger enable
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
  
  ADCSRA |= (1 << ADSC);      // start ADC measurement
  ADCL;ADCH;                  // The first ADC conversion result after switching voltage reference source may be inaccurate, and the user is advised to discard this result.
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void initPWM() {
  
  //DDRB |= (1 << PB4) | (1 << PB1); // same as pinMode(PIN_3__PB_4, OUTPUT); ??
  
  // PWM pins are 3 (OC1B), 5 (OC0A), and 6 (OC0B)
  
  TCCR0A =
      (0 << COM0A1) |   // set OC0A on compare match, clear at BOTTOM
      (0 << COM0A0) |   // set OC0A on compare match, clear at BOTTOM
      (1 << COM0B1) |   // set OC0B on compare match, clear at BOTTOM
      (0 << COM0B0) |   // set OC0B on compare match, clear at BOTTOM
      //(-)
      //(-)
      (1 << WGM01)  |   // 0 1 1 = Fast PWM mode (1/3)
      (1 << WGM00);     // 0 1 1 = Fast PWM mode (2/3)
  
  // 1001 0001
  
  TCCR0B =
      (0 << FOC0A) |
      (0 << FOC0B) |
      //(-)
      //(-)
      (0 << WGM02)  |   // 0 1 1 = Fast PWM mode (3/3)
      (1 << CS02)   |
      (0 << CS01)   |
      (0 << CS00);
      // 0 0 1 -- No prescaling)
      // 0 1 0 -- 8 (From prescaler)
      // 0 1 1 -- 64 (From prescaler)
      // 1 0 0 -- 256 (From prescaler)
      // 1 0 1 -- 1024 (From prescaler)
  
  TCCR1 = 0<<PWM1A | 0<<COM1A0 | 1<<CS10;
  GTCCR = 1<<PWM1B | 2<<COM1B0;
  /*
  TIMSK = 
      //(-)
      (0 << OCIE1A) | 
      (0 << OCIE1B) | 
      (0 << OCIE0A) |
      (1 << OCIE0B) | 
      (0 << TOIE1);
      (1 << TOIE0);
      //(-)
  */
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup(){
  initPWM();
  initADC();
  pinMode(PIN_3__PB_4, OUTPUT); // yellow led, debugging (near ground), pwm
  pinMode(PIN_6__PB_1, OUTPUT); // green led, programming, pwm value set
  TinyWireS.begin(I2C_SLAVE_ADDR);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void loop() {
  
  if (TinyWireS.available()) {
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    u8Command = TinyWireS.receive();
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (u8Command == 0x01) {
      //digitalWrite(PIN_6__PB_1,HIGH);
      u8Received = TinyWireS.receive();
      uiPwm[0] = (TinyWireS.receive() << 8) + u8Received;
      OCR0B = lowByte(255 - uiPwm[0]);
      //digitalWrite(PIN_6__PB_1,LOW);
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x02) {
      //digitalWrite(PIN_6__PB_1,HIGH);
      u8Received = TinyWireS.receive();
      uiPwm[1] = (TinyWireS.receive() << 8) + u8Received;
      OCR1B = lowByte(uiPwm[1]);
      //digitalWrite(PIN_6__PB_1,LOW);
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x03) {
      // single conversion on ADC 2 --->
      /* //<--- Free Running Conversion
      ADMUX =   (0 << MUX3)  |    // MUX bit 3 ~~ 0 0 0 0 = ADC0 --> PB4 | Pin3, needs RESET fuse disabled, mcu can then not be reprogrammed.
                (0 << MUX2)  |    // MUX bit 2 ~~ 0 0 0 1 = ADC1 --> PB2 | Pin7
                (1 << MUX1)  |    // MUX bit 1 ~~ 0 0 1 0 = ADC2 --> PB4 | Pin3
                (1 << MUX0);      // MUX bit 0 ~~ 0 0 1 1 = ADC3 --> PB3 | Pin2
      ADCSRA |= (1 << ADSC); // start ADC measurement, needed when auto trigger enable is 0 (= single conversion)
      */ //<--- Free Running Conversion
      // <--- Single Conversion on ADC 2
      
      /*
      TinyWireS.send(ADCL);  // read lower byte
      TinyWireS.send(ADCH);  // read upper byte
      */
      
        TinyWireS.send(lowByte(uiPwm[0]));
        TinyWireS.send(highByte(uiPwm[0]));
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (u8Command == 0x04) {
      /*
        // single conversion on ADC 0 --->
        / * //<--- Free Running Conversion
        ADMUX =   (0 << MUX3)  |    // MUX bit 3 ~~ 0 0 0 0 = ADC0 --> PB4 | Pin3, needs RESET fuse disabled, mcu can then not be reprogrammed.
                  (0 << MUX2)  |    // MUX bit 2 ~~ 0 0 0 1 = ADC1 --> PB2 | Pin7
                  (0 << MUX1)  |    // MUX bit 1 ~~ 0 0 1 0 = ADC2 --> PB4 | Pin3
                  (0 << MUX0);      // MUX bit 0 ~~ 0 0 1 1 = ADC3 --> PB3 | Pin2
        ADCSRA |= (1 << ADSC); // start ADC measurement, needed when auto trigger enable is 0 (= single conversion)
        * / //<--- Free Running Conversion
        // <--- single conversion on ADC 0
        
        TinyWireS.send(ADCL);  // read lower byte
        TinyWireS.send(ADCH);  // read upper byte
      */
      
      TinyWireS.send(lowByte(uiPwm[1]));
      TinyWireS.send(highByte(uiPwm[1]));
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
  }
}
