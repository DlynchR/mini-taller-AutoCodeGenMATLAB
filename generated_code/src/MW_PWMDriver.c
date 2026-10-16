/*
 * Code to configure Timer and set duty cycle
 * This file is modified version of wiring_analog.c file
 * It separates analogWrite function into two parts:
 * 1. MW_setAnalogFrequency: This function initializes PWM peripheral
 * 2. MW_analogWrite: This function sets the duty cycle
 * /* Copyright 2019-2025 The MathWorks, Inc. */

#include "MW_PWMDriver.h"

#if !( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )

#if defined(ARDUINO_ARCH_SAMD)                                                          /* SAMD Targets */
#include "Arduino.h"
#include "wiring_private.h"
#ifdef __cplusplus
extern "C" {
#endif

static int _writeResolution = 8;

// Wait for synchronization of registers between the clock domains
static __inline__ void syncDAC() __attribute__((always_inline, unused));
static void syncDAC() {
  while (DAC->STATUS.bit.SYNCBUSY == 1)
    ;
}

// Wait for synchronization of registers between the clock domains
static __inline__ void syncTC_8(Tc* TCx) __attribute__((always_inline, unused));
static void syncTC_8(Tc* TCx) {
  while (TCx->COUNT8.STATUS.bit.SYNCBUSY);
}

// Wait for synchronization of registers between the clock domains
static __inline__ void syncTCC(Tcc* TCCx) __attribute__((always_inline, unused));
static void syncTCC(Tcc* TCCx) {
  while (TCCx->SYNCBUSY.reg & TCC_SYNCBUSY_MASK);
}

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to)
{
  if (from == to) {
    return value;
  }
  if (from > to) {
    return value >> (from-to);
  }
  return value << (to-from);
}

void MW_analogWrite(uint32_t pin, uint32_t value)
{
  PinDescription pinDesc = g_APinDescription[pin];
  uint32_t attr = pinDesc.ulPinAttribute;

  if ((attr & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG)
  {
    // DAC handling code

    if ((pinDesc.ulADCChannelNumber != ADC_Channel0) && (pinDesc.ulADCChannelNumber != DAC_Channel0)) { // Only 1 DAC on AIN0 / PA02
      return;
    }

    value = mapResolution(value, _writeResolution, 10);

    syncDAC();
    DAC->DATA.reg = value & 0x3FF;  // DAC on 10 bits.
    syncDAC();
    DAC->CTRLA.bit.ENABLE = 0x01;     // Enable DAC
    syncDAC();
    return;
  }

  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
  {
    //value = mapResolution(value, _writeResolution, 16);

    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);

    if (attr & PIN_ATTR_TIMER) {
      #if !(ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10603)
      // Compatibility for cores based on SAMD core <=1.6.2
      if (pinDesc.ulPinType == PIO_TIMER_ALT) {
        pinPeripheral(pin, PIO_TIMER_ALT);
      } else
      #endif
      {
        pinPeripheral(pin, PIO_TIMER);
      }
    } else {
      // We suppose that attr has PIN_ATTR_TIMER_ALT bit set...
      pinPeripheral(pin, PIO_TIMER_ALT);
    }

    
    if (tcNum >= TCC_INST_NUM) {
        Tc* TCx = (Tc*) GetTC(pinDesc.ulPWMChannel);
        TCx->COUNT8.CC[tcChannel].reg = (uint32_t) value;
        syncTC_8(TCx);
    } else {
        Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
        //Disabling the double buffer lock for now.
        //If this is enabled, in double slope mode,
        //Counter wraparound occurs which results in 
        //unexpected duty cycles. This needs to be fixed/
        //Not behaving as per datasheet
        //TCCx->CTRLBSET.bit.LUPD = 1;
        //syncTCC(TCCx);
        TCCx->CCB[tcChannel].reg = (uint32_t) value;
        syncTCC(TCCx);
        //TCCx->CTRLBCLR.bit.LUPD = 1;
        //syncTCC(TCCx);
    }
    return;
  }

  // -- Defaults to digital write
  pinMode(pin, OUTPUT);
  value = mapResolution(value, _writeResolution, 8);
  if (value < 128) {
    digitalWrite(pin, LOW);
  } else {
    digitalWrite(pin, HIGH);
  }
}

void MW_setAnalogFrequency(uint16_t pin,uint16_t prescaler,uint16_t period)
{
  uint32_t value = 0;                 
  PinDescription pinDesc = g_APinDescription[pin];
  uint32_t attr = pinDesc.ulPinAttribute;

  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
  {
    //value = mapResolution(value, _writeResolution, 16);
    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
    static bool tcEnabled[TCC_INST_NUM+TC_INST_NUM];

    if (attr & PIN_ATTR_TIMER) {
      #if !(ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10603)
      // Compatibility for cores based on SAMD core <=1.6.2
      if (pinDesc.ulPinType == PIO_TIMER_ALT) {
        pinPeripheral(pin, PIO_TIMER_ALT);
      } else
      #endif
      {
        pinPeripheral(pin, PIO_TIMER);
      }
    } else {
      // We suppose that attr has PIN_ATTR_TIMER_ALT bit set...
      pinPeripheral(pin, PIO_TIMER_ALT);
    }

    if (!tcEnabled[tcNum]) {
      tcEnabled[tcNum] = true;

      uint16_t GCLK_CLKCTRL_IDs[] = {
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC0
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC1
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TCC2
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TC3
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC4
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC5
        GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC6
        GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC7
      };
      GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_IDs[tcNum]);
      while (GCLK->STATUS.bit.SYNCBUSY == 1);

      // Set PORT
      if (tcNum >= TCC_INST_NUM) {
        // -- Configure TC
        Tc* TCx = (Tc*) GetTC(pinDesc.ulPWMChannel);
        // Disable TCx
        TCx->COUNT8.CTRLA.bit.ENABLE = 0;
        syncTC_8(TCx);
        // Set Timer counter Mode to 16 bits, normal PWM
        TCx->COUNT8.CTRLA.reg |= TC_CTRLA_MODE_COUNT8 | TC_CTRLA_WAVEGEN_NPWM | (prescaler<<8);
        syncTC_8(TCx);
        // Set the initial value
        TCx->COUNT8.CC[tcChannel].reg = (uint32_t) value;
        syncTC_8(TCx);
        //freq is fTC/(N*(TOP+1))
        TCx->COUNT8.PER.reg = period - 1;
        syncTC_8(TCx);
        // Enable TCx
        TCx->COUNT8.CTRLA.bit.ENABLE = 1;
        syncTC_8(TCx);
      } else {
        // -- Configure TCC
        Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
        // Disable TCCx
        TCCx->CTRLA.bit.ENABLE = 0;
        syncTCC(TCCx);
        // Prescaler
        TCCx->CTRLA.reg |= (prescaler<<8);
        syncTCC(TCCx);
        // Set TCCx as dual slope PWM
        TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_DSTOP;
        //Invert the polarity because by default, ouput is high when counter > period
        TCCx->WAVE.reg |= TCC_WAVE_POL_Msk; 
        syncTCC(TCCx);
        // Set the initial value
        TCCx->CC[tcChannel].reg = (uint32_t) value;
        syncTCC(TCCx);
        //No 'period-1' required in dual slope because freq = fTCC/(2N*PER) . Refer RM page 716 section 31.6.2.5.6
        TCCx->PER.reg = period;
        syncTCC(TCCx);
        // Enable TCCx
        TCCx->CTRLA.bit.ENABLE = 1;
        syncTCC(TCCx);
      }
    }
    return;
  }
    // -- Defaults to digital write
  pinMode(pin, OUTPUT);
  value = mapResolution(value, _writeResolution, 8);
  if (value < 128) {
    digitalWrite(pin, LOW);
  } else {
    digitalWrite(pin, HIGH);
  }
}

/*Configure Output Polarity*/
void MW_ConfigureOutputPolarity(uint8_T pin,boolean_T invertedPolarity){
    PinDescription pinDesc = g_APinDescription[pin];
    uint32_t attr = pinDesc.ulPinAttribute;
    if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
    {
        uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
        uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
        if (tcNum < TCC_INST_NUM) {
            // -- Configure TCC
            Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
            // Disable TCCx
            TCCx->CTRLA.bit.ENABLE = 0;
            syncTCC(TCCx);
            // Invert the corresponding POL bit of WAVE register
            TCCx->WAVE.reg ^= ((1<<(TCC_WAVE_POL0_Pos + tcChannel)) & TCC_WAVE_POL_Msk); 
            syncTCC(TCCx);
            // Enable TCCx
            TCCx->CTRLA.bit.ENABLE = 1;
            syncTCC(TCCx);
        }
    }
}

/*Enable PWM Match Interrupt*/
void MW_EnablePWMMatchInterrupt(uint8_T pin){
    PinDescription pinDesc = g_APinDescription[pin];
    uint32_t attr = pinDesc.ulPinAttribute;
    if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
    {
        uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
        uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
        if (tcNum < TCC_INST_NUM) {
            // -- Configure TCC
            Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
            // Disable TCCx
            TCCx->CTRLA.bit.ENABLE = 0;
            syncTCC(TCCx);
            //Set the corresponsding MC bit in interrupt set register
            TCCx->INTENSET.reg |= ((1<<(TCC_INTENSET_MC0_Pos + tcChannel)) & TCC_INTENSET_MC_Msk);
            syncTCC(TCCx);
            // Enable TCCx
            TCCx->CTRLA.bit.ENABLE = 1;
            syncTCC(TCCx);
        }
    }
}

/*Enable PWM Overflow Interrupt*/
void MW_EnablePWMOverflowInterrupt(uint8_T pin){
    PinDescription pinDesc = g_APinDescription[pin];
    uint32_t attr = pinDesc.ulPinAttribute;
    if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
    {
        uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
        if (tcNum < TCC_INST_NUM) {
            // -- Configure TCC
            Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
            // Disable TCCx
            TCCx->CTRLA.bit.ENABLE = 0;
            syncTCC(TCCx);
            //Enable Overflow Interrupt
            TCC0->INTENSET.bit.OVF = 1;
            syncTCC(TCCx);
            // Enable TCCx
            TCCx->CTRLA.bit.ENABLE = 1;
            syncTCC(TCCx);
        }
    }
}

/*Set wave generation mode*/
void MW_SetWavegenMode(uint8_T pin,uint8_T wavegenMode){
    PinDescription pinDesc = g_APinDescription[pin];
    uint32_t attr = pinDesc.ulPinAttribute;
    if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
    {
        uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
        if (tcNum < TCC_INST_NUM) {
            // -- Configure TCC
            Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
            // Disable TCCx
            TCCx->CTRLA.bit.ENABLE = 0;
            syncTCC(TCCx);
            // Set TCCx wave mode
            TCCx->WAVE.bit.WAVEGEN = wavegenMode;
            syncTCC(TCCx);
            // Enable TCCx
            TCCx->CTRLA.bit.ENABLE = 1;
            syncTCC(TCCx);
        }
    }
}
   
#ifdef __cplusplus
}
#endif
#elif defined(ARDUINO_ARDUINO_NANO33BLE)
#include "Arduino.h"
#include "pins_arduino.h"
#ifdef __cplusplus
extern "C" {
#endif

static int _writeResolution = 8;

extern uint32_t nano33BLE_dutycycle[21];
extern uint16_t nano33BLE_period[21];

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to)
{
  if (from == to) {
    return value;
  }
  if (from > to) {
    return value >> (from-to);
  }
  return value << (to-from);
}

void MW_analogWrite(uint32_t pin, uint32_t value)
{
  nano33BLE_dutycycle[pin] = value;
}

void MW_setAnalogFrequency(uint16_t pin,uint16_t prescaler,uint16_t period)
{
    nano33BLE_period[pin] = period;
}
   
#ifdef __cplusplus
}
#endif

#elif (defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO)) // RP2040 targets
#include "hardware/pwm.h"
#include "hardware/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ARDUINO_NANO_RP2040_CONNECT)
// Inclusion of required files to get digitalPinToPinName is causing compilation errors saying cstddef is not defined. 
// It is most likely to do with our main being a c file.
// Force defining it here as a workaround
uint8_t digitalPinToPinName[] = {1,0,25,15,16,17,18,19,20,21,5,7,4,6,26,27,28,29,12,13};// D0 to D19
#elif (defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_W))
uint8_t digitalPinToPinName[]  = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,26,27,28};  // D0 to D28   
#endif

void MW_analogWrite(uint32_t pin, uint32_t value)
{
  pwm_set_gpio_level(digitalPinToPinName[pin], value);
}

void MW_setAnalogFrequency(uint16_t pin, uint16_t clkDiv, uint16_t countWrap)
{
    uint8_t slice_num = pwm_gpio_to_slice_num(digitalPinToPinName[pin]);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config cfg = pwm_get_default_config();
    
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&cfg, clkDiv);
    pwm_config_set_wrap(&cfg, countWrap);
    
    // Load the configuration into our PWM slice, and set it running.
    #if defined(ARDUINO_NANO_RP2040_CONNECT)
    pwm_init(slice_num, pwm_gpio_to_channel(digitalPinToPinName[pin]), &cfg, true);
    #elif (defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_W))
    pwm_init(slice_num, &cfg, true);
    #endif
    gpio_set_function(digitalPinToPinName[pin], GPIO_FUNC_PWM);
    // Set zero dutycycle at setup call
    pwm_set_gpio_level(digitalPinToPinName[pin], 0*(countWrap+1));

}
   
#ifdef __cplusplus
}
#endif
#elif defined(ARDUINO_ARCH_RENESAS_UNO)
#ifdef __cplusplus
extern "C" {
#endif

void MW_analogWrite(uint32_t pin, uint32_t value)
{
  analogWrite(pin,value);
}

void MW_setAnalogFrequency(uint16_t pin,uint16_t prescaler,uint16_t period)
{
   //analogWriteFrequency(pin,1/period);
}
   
#ifdef __cplusplus
}
#endif


#elif defined(ARDUINO_ARCH_AVR)                                                         /* AVR Targets */
#include "wiring_private.h"
#include "pins_arduino.h"
void MW_analogWrite(uint32_t pin, uint32_t value)
{
    analogWrite(pin, value);
}

void MW_setAnalogFrequency(uint16_t pin,uint16_t prescaler,uint16_t period)
{
    switch(digitalPinToTimer(pin))
    {
        // XXX fix needed for atmega8
        #if defined(TCCR0) && defined(COM00) && !defined(__AVR_ATmega8__)
        case TIMER0A:
            // connect pwm to pin on timer 0
            sbi(TCCR0, COM00);
            break;
        #endif

        #if defined(TCCR0A) && defined(COM0A1)
        case TIMER0A:
            // connect pwm to pin on timer 0, channel A
            sbi(TCCR0A, COM0A1);
            break;
        #endif

        #if defined(TCCR0A) && defined(COM0B1)
        case TIMER0B:
            // connect pwm to pin on timer 0, channel B
            sbi(TCCR0A, COM0B1);
            break;
        #endif

        #if defined(TCCR1A) && defined(TCCR1B) && defined(ICR1) && defined(COM1A1)
        case TIMER1A:
            TCCR1A = (TCCR1A & 0xFCU) | 2U;                               //WGM11 for PWM, Phase Correct. Mode 10
            TCCR1B = (TCCR1B & 0xE0U) | 0x10U | prescaler;                //WGM13 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR1   = period;                                              //Period
            sbi(TCCR1A, COM1A1);
            break;
        #endif

        #if defined(TCCR1A) && defined(TCCR1B) && defined(ICR1) && defined(COM1B1)
        case TIMER1B:
            TCCR1A = (TCCR1A & 0xFCU) | 2U;                               //WGM11 for PWM, Phase Correct. Mode 10
            TCCR1B = (TCCR1B & 0xE0U) | 0x10U | prescaler;                //WGM13 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR1   = period;                                              //Period
            sbi(TCCR1A, COM1B1);
            break;
        #endif

        #if defined(TCCR1A) && defined(TCCR1B) && defined(ICR1) && defined(COM1C1)
        case TIMER1C:
            // connect pwm to pin on timer 1, channel B
            TCCR1A = (TCCR1A & 0xFCU) | 2U;                               //WGM11 for PWM, Phase Correct. Mode 10
            TCCR1B = (TCCR1B & 0xE0U) | 0x10U | prescaler;                //WGM13 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR1   = period;                                              //Period
            sbi(TCCR1A, COM1C1);
            break;
        #endif

        #if defined(TCCR2) && defined(COM21)
        case TIMER2:
            // connect pwm to pin on timer 2
            sbi(TCCR2, COM21);
            break;
        #endif

        #if defined(TCCR2A) && defined(TCCR2B) && defined(COM2A1)
        case TIMER2A:
            // connect pwm to pin on timer 2, channel A
            sbi(TCCR2A, COM2A1);
            TCCR2B=TCCR2B & 0b11111000 | prescaler;                 //Prescaler
            break;
        #endif

        #if defined(TCCR2A) && defined(TCCR2B) && defined(COM2B1)
        case TIMER2B:
            // connect pwm to pin on timer 2, channel B
            sbi(TCCR2A, COM2B1);
            TCCR2B=TCCR2B & 0b11111000 | prescaler;                 //Prescaler
            break;
        #endif

        #if defined(TCCR3A) && defined(TCCR3B) && defined(ICR3) && defined(COM3A1)
        case TIMER3A:
            // connect pwm to pin on timer 3, channel A
            TCCR3A = (TCCR3A & 0xFCU) | 2U;                               //WGM31 for PWM, Phase Correct. Mode 10
            TCCR3B = (TCCR3B & 0xE0U) | 0x10U | prescaler;                //WGM33 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR3   = period;                                              //Period
            sbi(TCCR3A, COM3A1);
            break;
        #endif

        #if defined(TCCR3A) && defined(TCCR3B) && defined(ICR3) && defined(COM3B1)
        case TIMER3B:
            // connect pwm to pin on timer 3, channel B
            TCCR3A = (TCCR3A & 0xFCU) | 2U;                               //WGM31 for PWM, Phase Correct. Mode 10
            TCCR3B = (TCCR3B & 0xE0U) | 0x10U | prescaler;                //WGM33 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR3   = period;                                              //Period
            sbi(TCCR3A, COM3B1);
            break;
        #endif

        #if defined(TCCR3A) && defined(TCCR3B) && defined(ICR3) && defined(COM3C1)
        case TIMER3C:
            // connect pwm to pin on timer 3, channel C
            TCCR3A = (TCCR3A & 0xFCU) | 2U;                               //WGM31 for PWM, Phase Correct. Mode 10
            TCCR3B = (TCCR3B & 0xE0U) | 0x10U | prescaler;                //WGM33 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR3   = period;                                              //Period
            sbi(TCCR3A, COM3C1);
            break;
        #endif

        #if defined(TCCR4A) && defined(TCCR4B) && defined(TCCR4D)  && defined(COM4A1) && defined(COM4A0)/* beginning of timer4 block for 32U4 and similar */
        case TIMER4A:
            TCCR4B = (TCCR4B & 0xf0) | prescaler;
            OCR4C = period;
            sbi(TCCR4A, COM4A1);
            cbi(TCCR4A, COM4A0);
            break;
        #else /* beginning of timer4 block for ATMEGA1280 and ATMEGA2560 */
        #if defined(TCCR4A) && defined(TCCR4B) && defined(ICR4) && defined(COM4A1)
        case TIMER4A:
            //connect pwm to pin on timer 4, channel A
            TCCR4A = (TCCR4A & 0xFCU) | 2U;                               //WGM41 for PWM, Phase Correct. Mode 10
            TCCR4B = (TCCR4B & 0xE0U) | 0x10U | prescaler;                //WGM43 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR4   = period;                                              //Period
            sbi(TCCR4A, COM4A1);
            break;
        #endif
        #endif

        #if defined(TCCR4A) && defined(TCCR4B) && defined(TCCR4D)  && defined(COM4B1) /* beginning of timer4 block for 32U4 and similar */
        case TIMER4B:
            TCCR4B = (TCCR4B & 0xf0) | prescaler;
            OCR4C = period;
            sbi(TCCR4A, COM4B1);
            break;
        #else /* beginning of timer4 block for ATMEGA1280 and ATMEGA2560 */
        #if defined(TCCR4A) && defined(TCCR4B) && defined(ICR4) && defined(COM4B1)    
        case TIMER4B:
            // connect pwm to pin on timer 4, channel B
            TCCR4A = (TCCR4A & 0xFCU) | 2U;                               //WGM41 for PWM, Phase Correct. Mode 10
            TCCR4B = (TCCR4B & 0xE0U) | 0x10U | prescaler;                //WGM43 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR4   = period;                                              //Period
            sbi(TCCR4A, COM4B1);
            break;
        #endif
        #endif

        #if defined(TCCR4A) && defined(TCCR4B) && defined(TCCR4D)  && defined(COM4C1) /* beginning of timer4 block for 32U4 and similar */
        case TIMER4C:
            TCCR4B = (TCCR4B & 0xf0) | prescaler;
            OCR4C = period;
            sbi(TCCR4A, COM4C1);
            break;
        #else /* beginning of timer4 block for ATMEGA1280 and ATMEGA2560 */
        #if defined(TCCR4A) && defined(TCCR4B) && defined(ICR4) && defined(COM4C1)    
        case TIMER4C:
            // connect pwm to pin on timer 4, channel C
            TCCR4A = (TCCR4A & 0xFCU) | 2U;                               //WGM41 for PWM, Phase Correct. Mode 10
            TCCR4B = (TCCR4B & 0xE0U) | 0x10U | prescaler;                //WGM43 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR4   = period;                                              //Period
            sbi(TCCR4A, COM4C1);
            break;
        #endif
        #endif

        #if defined(TCCR4A) && defined(TCCR4B) && defined(TCCR4D)  && defined(COM4D1) && defined(COM4D0)	/* beginning of timer4 block for 32U4 and similar */
        case TIMER4D:
            TCCR4B = (TCCR4B & 0xf0) | prescaler;
            OCR4C = period;
            // connect pwm to pin on timer 4, channel D
            sbi(TCCR4C, COM4D1);
            cbi(TCCR4C, COM4D0);
        #else /* beginning of timer4 block for ATMEGA1280 and ATMEGA2560 */
        #if defined(TCCR4C) && defined(COM4D1)
        case TIMER4D:				
            // connect pwm to pin on timer 4, channel D
            sbi(TCCR4C, COM4D1);
            #if defined(COM4D0)		// only used on 32U4
            cbi(TCCR4C, COM4D0);
            #endif
            break;
        #endif
        #endif

        #if defined(TCCR5A) && defined(TCCR5B) && defined(ICR5) && defined(COM5A1)
        case TIMER5A:
            // connect pwm to pin on timer 5, channel A
            TCCR5A = (TCCR5A & 0xFCU) | 2U;                               //WGM51 for PWM, Phase Correct. Mode 10
            TCCR5B = (TCCR5B & 0xE0U) | 0x10U | prescaler;                //WGM53 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR5   = period;                                              //Period
            sbi(TCCR5A, COM5A1);
            break;
        #endif

        #if defined(TCCR5A) && defined(TCCR5B) && defined(ICR5) && defined(COM5B1)
        case TIMER5B:
            // connect pwm to pin on timer 5, channel B
            TCCR5A = (TCCR5A & 0xFCU) | 2U;                               //WGM51 for PWM, Phase Correct. Mode 10
            TCCR5B = (TCCR5B & 0xE0U) | 0x10U | prescaler;                //WGM53 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR5   = period;                                              //Period
            sbi(TCCR5A, COM5B1);
            break;
        #endif

        #if defined(TCCR5A) && defined(TCCR5B) && defined(ICR5) && defined(COM5C1)
        case TIMER5C:
            // connect pwm to pin on timer 5, channel C
            TCCR5A = (TCCR5A & 0xFCU) | 2U;                               //WGM51 for PWM, Phase Correct. Mode 10
            TCCR5B = (TCCR5B & 0xE0U) | 0x10U | prescaler;                //WGM53 for PWM, Phase Correct. Mode 10
                                                                          //and prescaler
            ICR5   = period;                                              //Period
            sbi(TCCR5A, COM5C1);
            break;
        #endif
    }
    digitalWrite(pin, LOW);
}

/*Enable PWM Match Interrupt*/
/* This function covers these MCUs:
* atmega32u4 -> TIMSK0,1,3,4
* atmega2560 -> TIMSK0,1,2,3,4,5
* atmega328p -> TIMSK0,1,2
* Source: Register summary in datasheet
If support is added for a board which uses some other MCU, then this function needs to be updated
*/
void MW_EnablePWMMatchInterrupt(uint8_T pin){
    switch(digitalPinToTimer(pin))
    {
        #if defined(TIMSK0) && defined(OCIE0A)
        case TIMER0A: TIMSK0 |= (1 << OCIE0A); break;
        #endif

        #if defined(TIMSK0) && defined(OCIE0B)
        case TIMER0B: TIMSK0 |= (1 << OCIE0B); break;
        #endif

        #if defined(TIMSK1) && defined(OCIE1A)
        case TIMER1A: TIMSK1 |= (1 << OCIE1A); break;
        #endif

        #if defined(TIMSK1) && defined(OCIE1B)
        case TIMER1B: TIMSK1 |= (1 << OCIE1B); break;
        #endif

        #if defined(TIMSK1) && defined(OCIE1C)
        case TIMER1C: TIMSK1 |= (1 << OCIE1C); break;
        #endif

        #if defined(TIMSK2) && defined(OCIE2A)
        case TIMER2A: TIMSK2 |= (1 << OCIE2A); break;
        #endif

        #if defined(TIMSK2) && defined(OCIE2B)
        case TIMER2B: TIMSK2 |= (1 << OCIE2B); break;
        #endif
		
        #if defined(TIMSK3) && defined(OCIE3A)
        case TIMER3A: TIMSK3 |= (1 << OCIE3A); break;
        #endif

        #if defined(TIMSK3) && defined(OCIE3B)
        case TIMER3B: TIMSK3 |= (1 << OCIE3B); break;
        #endif

        #if defined(TIMSK3) && defined(OCIE3C)
        case TIMER3C: TIMSK3 |= (1 << OCIE3C); break;
        #endif
		
        #if defined(TIMSK4) && defined(OCIE4A)
        case TIMER4A: TIMSK4 |= (1 << OCIE4A); break;
        #endif

        #if defined(TIMSK4) && defined(OCIE4B)
        case TIMER4B: TIMSK4 |= (1 << OCIE4B); break;
        #endif

        #if defined(TIMSK4) && defined(OCIE4C)
        case TIMER4C: TIMSK4 |= (1 << OCIE4C); break;
        #endif

        #if defined(TIMSK4) && defined(OCIE4D)
        case TIMER4D: TIMSK4 |= (1 << OCIE4D); break;
        #endif
		
        #if defined(TIMSK5) && defined(OCIE5A)
        case TIMER5A: TIMSK5 |= (1 << OCIE5A); break;
        #endif

        #if defined(TIMSK5) && defined(OCIE5B)
        case TIMER5B: TIMSK5 |= (1 << OCIE5B); break;
        #endif

        #if defined(TIMSK5) && defined(OCIE5C)
        case TIMER5C: TIMSK5 |= (1 << OCIE5C); break;
        #endif
    }
}

/*Enable PWM Overflow Interrupt*/
void MW_EnablePWMOverflowInterrupt(uint8_T pin){
    switch(digitalPinToTimer(pin))
    {
        #if defined(TIMSK0) && defined(TOIE0)
        case TIMER0A: TIMSK0 |= (1 << TOIE0); break;
        #endif

        #if defined(TIMSK0) && defined(TOIE0)
        case TIMER0B: TIMSK0 |= (1 << TOIE0); break;
        #endif

        #if defined(TIMSK1) && defined(TOIE1)
        case TIMER1A: TIMSK1 |= (1 << TOIE1); break;
        #endif

        #if defined(TIMSK1) && defined(TOIE1)
        case TIMER1B: TIMSK1 |= (1 << TOIE1); break;
        #endif

        #if defined(TIMSK1) && defined(TOIE1)
        case TIMER1C: TIMSK1 |= (1 << TOIE1); break;
        #endif

        #if defined(TIMSK2) && defined(TOIE2)
        case TIMER2A: TIMSK2 |= (1 << TOIE2); break;
        #endif

        #if defined(TIMSK2) && defined(TOIE2)
        case TIMER2B: TIMSK2 |= (1 << TOIE2); break;
        #endif
		
        #if defined(TIMSK3) && defined(TOIE3)
        case TIMER3A: TIMSK3 |= (1 << TOIE3); break;
        #endif

        #if defined(TIMSK3) && defined(TOIE3)
        case TIMER3B: TIMSK3 |= (1 << TOIE3); break;
        #endif

        #if defined(TIMSK3) && defined(TOIE3)
        case TIMER3C: TIMSK3 |= (1 << TOIE3); break;
        #endif
		
        #if defined(TIMSK4) && defined(TOIE4)
        case TIMER4A: TIMSK4 |= (1 << TOIE4); break;
        #endif

        #if defined(TIMSK4) && defined(TOIE4)
        case TIMER4B: TIMSK4 |= (1 << TOIE4); break;
        #endif

        #if defined(TIMSK4) && defined(TOIE4)
        case TIMER4C: TIMSK4 |= (1 << TOIE4); break;
        #endif

        #if defined(TIMSK4) && defined(TOIE4)
        case TIMER4D: TIMSK4 |= (1 << TOIE4); break;
        #endif
		
        #if defined(TIMSK5) && defined(TOIE5)
        case TIMER5A: TIMSK5 |= (1 << TOIE5); break;
        #endif

        #if defined(TIMSK5) && defined(TOIE5)
        case TIMER5B: TIMSK5 |= (1 << TOIE5); break;
        #endif

        #if defined(TIMSK5) && defined(TOIE5)
        case TIMER5C: TIMSK5 |= (1 << TOIE5); break;
        #endif
    }
}

#elif defined(ARDUINO_ARCH_SAM)                                                         /* SAM Targets */
#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif
    
static int _writeResolution = 8;
static uint8_t PWMEnabled = 0;
static uint8_t TCChanEnabled[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to) {
	if (from == to)
		return value;
	if (from > to)
		return value >> (from-to);
	else
		return value << (to-from);
}

static void TC_SetCMR_ChannelA(Tc *tc, uint32_t chan, uint32_t v)
{
	tc->TC_CHANNEL[chan].TC_CMR = (tc->TC_CHANNEL[chan].TC_CMR & 0xFFF0FFFF) | v;
}

static void TC_SetCMR_ChannelB(Tc *tc, uint32_t chan, uint32_t v)
{
	tc->TC_CHANNEL[chan].TC_CMR = (tc->TC_CHANNEL[chan].TC_CMR & 0xF0FFFFFF) | v;
}

void MW_analogWrite(uint32_t ulPin, uint32_t ulValue)
{
	uint32_t attr = g_APinDescription[ulPin].ulPinAttribute;

	if ((attr & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG) {
		EAnalogChannel channel = g_APinDescription[ulPin].ulADCChannelNumber;
		if (channel == DA0 || channel == DA1) {
			uint32_t chDACC = ((channel == DA0) ? 0 : 1);
			if (dacc_get_channel_status(DACC_INTERFACE) == 0) {
				/* Enable clock for DACC_INTERFACE */
				pmc_enable_periph_clk(DACC_INTERFACE_ID);

				/* Reset DACC registers */
				dacc_reset(DACC_INTERFACE);

				/* Half word transfer mode */
				dacc_set_transfer_mode(DACC_INTERFACE, 0);

				/* Power save:
				 * sleep mode  - 0 (disabled)
				 * fast wakeup - 0 (disabled)
				 */
				dacc_set_power_save(DACC_INTERFACE, 0, 0);
				/* Timing:
				 * refresh        - 0x08 (1024*8 dacc clocks)
				 * max speed mode -    0 (disabled)
				 * startup time   - 0x10 (1024 dacc clocks)
				 */
				dacc_set_timing(DACC_INTERFACE, 0x08, 0, 0x10);

				/* Set up analog current */
				dacc_set_analog_control(DACC_INTERFACE, DACC_ACR_IBCTLCH0(0x02) |
											DACC_ACR_IBCTLCH1(0x02) |
											DACC_ACR_IBCTLDACCORE(0x01));
			}

			/* Disable TAG and select output channel chDACC */
			dacc_set_channel_selection(DACC_INTERFACE, chDACC);

			if ((dacc_get_channel_status(DACC_INTERFACE) & (1 << chDACC)) == 0) {
				dacc_enable_channel(DACC_INTERFACE, chDACC);
			}

			// Write user value
			ulValue = mapResolution(ulValue, _writeResolution, DACC_RESOLUTION);
			dacc_write_conversion_data(DACC_INTERFACE, ulValue);
			while ((dacc_get_interrupt_status(DACC_INTERFACE) & DACC_ISR_EOC) == 0);
			return;
		}
	}
    
    if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM) {
        uint32_t chan = g_APinDescription[ulPin].ulPWMChannel;
		//ulValue = mapResolution(ulValue, _writeResolution, PWM_RESOLUTION);
		PWMC_SetDutyCycle(PWM_INTERFACE, chan, ulValue);
		return;
	}

	if ((attr & PIN_ATTR_TIMER) == PIN_ATTR_TIMER) {
		// Setup Timer for this pin
		ETCChannel channel = g_APinDescription[ulPin].ulTCChannel;
		static const uint32_t channelToChNo[] = { 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2 };
		static const uint32_t channelToAB[]   = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
		static Tc *channelToTC[] = {
			TC0, TC0, TC0, TC0, TC0, TC0,
			TC1, TC1, TC1, TC1, TC1, TC1,
			TC2, TC2, TC2, TC2, TC2, TC2 };
		static const uint32_t channelToId[] = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8 };
		uint32_t chNo = channelToChNo[channel];
		uint32_t chA  = channelToAB[channel];
		Tc *chTC = channelToTC[channel];
		uint32_t interfaceID = channelToId[channel];

		if (ulValue == 0) {
			if (chA)
				TC_SetCMR_ChannelA(chTC, chNo, TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR);
			else
				TC_SetCMR_ChannelB(chTC, chNo, TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR);
		} else {
			if (chA) {
				TC_SetRA(chTC, chNo, ulValue);
				TC_SetCMR_ChannelA(chTC, chNo, TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET);
			} else {
				TC_SetRB(chTC, chNo, ulValue);
				TC_SetCMR_ChannelB(chTC, chNo, TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_SET);
			}
		}
		if ((g_pinStatus[ulPin] & 0xF) != PIN_STATUS_PWM) {
			PIO_Configure(g_APinDescription[ulPin].pPort,
					g_APinDescription[ulPin].ulPinType,
					g_APinDescription[ulPin].ulPin,
					g_APinDescription[ulPin].ulPinConfiguration);
			g_pinStatus[ulPin] = (g_pinStatus[ulPin] & 0xF0) | PIN_STATUS_PWM;
		}
		return;
	}

	// Defaults to digital write
	pinMode(ulPin, OUTPUT);
	ulValue = mapResolution(ulValue, _writeResolution, 8);
	if (ulValue < 128)
		digitalWrite(ulPin, LOW);
	else
		digitalWrite(ulPin, HIGH);
}
void MW_setAnalogFrequency(uint16_t ulPin,uint16_t prescaler,uint16_t period)
{
    uint32_t attr = g_APinDescription[ulPin].ulPinAttribute;
    if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM) {
		if (!PWMEnabled) {
			// PWM Startup code
		    pmc_enable_periph_clk(PWM_INTERFACE_ID);
		    PWMC_ConfigureClocks(PWM_FREQUENCY * PWM_MAX_DUTY_CYCLE, 0, VARIANT_MCK);
			PWMEnabled = 1;
		}

		uint32_t chan = g_APinDescription[ulPin].ulPWMChannel;
		if ((g_pinStatus[ulPin] & 0xF) != PIN_STATUS_PWM) {
			// Setup PWM for this pin
			PIO_Configure(g_APinDescription[ulPin].pPort,
					g_APinDescription[ulPin].ulPinType,
					g_APinDescription[ulPin].ulPin,
					g_APinDescription[ulPin].ulPinConfiguration);
			PWMC_ConfigureChannel(PWM_INTERFACE, chan, prescaler, 0, 0);
			PWMC_SetPeriod(PWM_INTERFACE, chan, period);
			PWMC_SetDutyCycle(PWM_INTERFACE, chan, 0);
			PWMC_EnableChannel(PWM_INTERFACE, chan);
			g_pinStatus[ulPin] = (g_pinStatus[ulPin] & 0xF0) | PIN_STATUS_PWM;
		}
		return;
	}

	if ((attr & PIN_ATTR_TIMER) == PIN_ATTR_TIMER) {
		// We use MCLK/2 as clock.
		const uint32_t TC = period;

		// Setup Timer for this pin
		ETCChannel channel = g_APinDescription[ulPin].ulTCChannel;
		static const uint32_t channelToChNo[] = { 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2 };
		static const uint32_t channelToAB[]   = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
		static Tc *channelToTC[] = {
			TC0, TC0, TC0, TC0, TC0, TC0,
			TC1, TC1, TC1, TC1, TC1, TC1,
			TC2, TC2, TC2, TC2, TC2, TC2 };
		static const uint32_t channelToId[] = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8 };
		uint32_t chNo = channelToChNo[channel];
		uint32_t chA  = channelToAB[channel];
		Tc *chTC = channelToTC[channel];
		uint32_t interfaceID = channelToId[channel];

		if (!TCChanEnabled[interfaceID]) {
			pmc_enable_periph_clk(TC_INTERFACE_ID + interfaceID);
			TC_Configure(chTC, chNo,
				prescaler |
				TC_CMR_WAVE |         // Waveform mode
				TC_CMR_WAVSEL_UP_RC | // Counter running up and reset when equals to RC
				TC_CMR_EEVT_XC0 |     // Set external events from XC0 (this setup TIOB as output)
				TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR |
				TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR);
			TC_SetRC(chTC, chNo, TC);
		}
		
		if ((g_pinStatus[ulPin] & 0xF) != PIN_STATUS_PWM) {
			PIO_Configure(g_APinDescription[ulPin].pPort,
					g_APinDescription[ulPin].ulPinType,
					g_APinDescription[ulPin].ulPin,
					g_APinDescription[ulPin].ulPinConfiguration);
			g_pinStatus[ulPin] = (g_pinStatus[ulPin] & 0xF0) | PIN_STATUS_PWM;
		}
		if (!TCChanEnabled[interfaceID]) {
			TC_Start(chTC, chNo);
			TCChanEnabled[interfaceID] = 1;
		}
		return;
	}
}

#ifdef __cplusplus
}
#endif
 
/* Architecture selection endif*/                                                                             
#elif defined(__IMXRT1062__)                                                         /* TEENSY Targets */
#include "Arduino.h"
#include "core_pins.h"
#include "pins_arduino.h"
#include "HardwareSerial.h"
extern void customflexpwm1_isr(void) __attribute__ ((weak));
extern void customflexpwm2_isr(void) __attribute__ ((weak));
extern void customflexpwm3_isr(void) __attribute__ ((weak));
extern void customflexpwm4_isr(void) __attribute__ ((weak));

#ifdef __cplusplus
extern "C" {
#endif
struct pwm_pin_info_struct{
  uint8_t type;    // 0=not PWM, 1=FlexPWM, 2=QuadTimer
  uint8_t module;  // for FlexPWM: module = 0xmn (m = FlexPWM index, n = submodule)
  uint8_t channel; // channel A or B (0=A, 1=B)
  uint8_t muxval;
};
// Declare the external array (defined in core)
extern const struct pwm_pin_info_struct pwm_pin_info[];

void customflexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, uint16_t frequency)
{
    uint16_t mask = 1 << submodule;  
    //uint16_t oldinit  = p->SM[submodule].INIT; 
    //uint32_t olddiv   = (int32_t)p->SM[submodule].VAL1 - (int32_t)oldinit;
    uint32_t newdiv = (uint32_t)((float)F_BUS_ACTUAL / frequency + 0.5f);
    uint32_t prescale = 0;
    while (newdiv > 65535 && prescale < 7) {
        newdiv = newdiv >> 1;
        prescale = prescale + 1;
    }
    if (newdiv > 65535) {
        newdiv = 65535;
    } else if (newdiv < 2) {
        newdiv = 2;
    }
    p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
    p->SM[submodule].CTRL = FLEXPWM_SMCTRL_FULL | FLEXPWM_SMCTRL_PRSC(prescale);
    p->SM[submodule].INIT = -(newdiv/2);
		p->SM[submodule].VAL1 = +(newdiv/2);
		p->SM[submodule].VAL0 = 0;
		p->SM[submodule].VAL2 = 0;
		p->SM[submodule].VAL3 = 0;
		p->SM[submodule].VAL4 = 0;
		p->SM[submodule].VAL5 = 0;
	p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);

}

void customflexpwmWrite(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, uint16_t val)
{
    uint16_t mask = 1 << submodule;
    uint32_t modulo = p->SM[submodule].VAL1 * 2; // - p->SM[submodule].INIT;
    uint32_t cval = ((uint32_t)val * (modulo + 1)) >> ARDUINO_TEENSY_RESOLUTION; //8 is analog_write_res
    if (cval > modulo) cval = modulo; // TODO: is this check correct?

    p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
    switch (channel) {
        case 0: // X
            p->SM[submodule].VAL0 = modulo/2 - cval;
		        p->OUTEN |= FLEXPWM_OUTEN_PWMX_EN(mask);
            break;
        case 1: // A
            p->SM[submodule].VAL2 = -(cval/2);
		        p->SM[submodule].VAL3 = +(cval/2);
            p->OUTEN |= FLEXPWM_OUTEN_PWMA_EN(mask);
            break;
        case 2: // B
            p->SM[submodule].VAL4 = -(cval/2);
		        p->SM[submodule].VAL5 = +(cval/2);
            p->OUTEN |= FLEXPWM_OUTEN_PWMB_EN(mask);
    }
    p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
  
}

void MW_analogWrite(uint32_t ulPin, uint32_t ulValue)
{
    const struct pwm_pin_info_struct *info;
	info = pwm_pin_info + ulPin;
    if (info->type != 1) 
    {
        analogWrite(ulPin, ulValue);
    }
    else
    {
        IMXRT_FLEXPWM_t *flexpwm;
        switch ((info->module >> 4) & 3) {
            case 0: flexpwm = &IMXRT_FLEXPWM1; break;
            case 1: flexpwm = &IMXRT_FLEXPWM2; break;
            case 2: flexpwm = &IMXRT_FLEXPWM3; break;
            case 3: flexpwm = &IMXRT_FLEXPWM4; break;
            default: break;
        }
        
        customflexpwmWrite(flexpwm, info->module & 0x03, info->channel, ulValue);
        *(portConfigRegister(ulPin)) = info->muxval;
    }
}
void MW_setAnalogFrequency(uint16_t ulPin,uint16_t prescaler,uint16_t period)
{
    const struct pwm_pin_info_struct *info;
    info = pwm_pin_info + ulPin;
    
    if (info->type != 1) 
    {
        analogWriteFrequency(ulPin,prescaler);
    }
    else
    {
        IMXRT_FLEXPWM_t *flexpwm;
         switch ((info->module >> 4) & 3) {
            case 0: flexpwm = &IMXRT_FLEXPWM1; break;
            case 1: flexpwm = &IMXRT_FLEXPWM2; break;
            case 2: flexpwm = &IMXRT_FLEXPWM3; break;
            case 3: flexpwm = &IMXRT_FLEXPWM4; break;
            default: break;
         }
        customflexpwmFrequency(flexpwm, info->module & 0x03, info->channel, prescaler);

    }

    
}

    /*Enable PWM Overflow Interrupt*/
void MW_EnablePWMOverflowInterrupt(uint8_T pin){
    const struct pwm_pin_info_struct *info;
	info = pwm_pin_info + pin;
    typedef uint32_t IRQn_Type;

     IMXRT_FLEXPWM_t *flexpwm;

        switch ((info->module >> 4) & 3) {
            case 0: flexpwm = &IMXRT_FLEXPWM1; break;
            case 1: flexpwm = &IMXRT_FLEXPWM2; break;
            case 2: flexpwm = &IMXRT_FLEXPWM3; break;
            case 3: flexpwm = &IMXRT_FLEXPWM4; break;
            default: break;
        }
       
        flexpwm->SM[info->module & 0x03].INTEN |= FLEXPWM_SMINTEN_CMPIE(0x02);
        flexpwm->MCTRL |= FLEXPWM_MCTRL_LDOK(15); 

    }

    static const uint8_t cmp_mask_A[8] = {
        0x00, // 0: none enabled
        0x08, // 1: falling
        0x04, // 2: rising
        0x0C, // 3: rising + falling
        0x01, // 4: center
        0x09, // 5: center + falling
        0x05, // 6: center + rising
        0x0D  // 7: center + rising + falling
        };
    static const uint8_t cmp_mask_B[8] = {
        0x00, // 0: none enabled
        0x20, // 1: falling
        0x10, // 2: rising
        0x30, // 3: rising + falling
        0x01, // 4: center
        0x21, // 5: center + falling
        0x11, // 6: center + rising
        0x31  // 7: center + rising + falling
        };
    void MW_EnableTeensyPWMMatchInterrupt(uint8_T pin,uint8_T mask){
        const struct pwm_pin_info_struct *info;
        info = pwm_pin_info + pin;
        
        
        if (info->type == 1) {
            IMXRT_FLEXPWM_t *flexpwm;
            switch ((info->module >> 4) & 3) {
                case 0: flexpwm = &IMXRT_FLEXPWM1; break;
                case 1: flexpwm = &IMXRT_FLEXPWM2; break;
                case 2: flexpwm = &IMXRT_FLEXPWM3; break;
                case 3: flexpwm = &IMXRT_FLEXPWM4; break;
                default: break;
            }
            switch (info->channel) {
                case 0: // X
                    flexpwm->SM[info->module & 0x03].INTEN |= FLEXPWM_SMINTEN_CMPIE(0x00);
                    break;
                case 1: // A
                    flexpwm->SM[info->module & 0x03].INTEN |= FLEXPWM_SMINTEN_CMPIE(cmp_mask_A[mask]);
                    break;
                case 2: // B
                    flexpwm->SM[info->module & 0x03].INTEN |= FLEXPWM_SMINTEN_CMPIE(cmp_mask_B[mask]);
                    break;
            }
            flexpwm->MCTRL |= FLEXPWM_MCTRL_LDOK(15); 
        }
    }
	
	/*Configure Output Polarity*/
void MW_ConfigureOutputPolarity(uint8_T pin,boolean_T invertedPolarity){
    const struct pwm_pin_info_struct *info;
	info = pwm_pin_info + pin;
    typedef uint32_t IRQn_Type;

     IMXRT_FLEXPWM_t *flexpwm;

        switch ((info->module >> 4) & 3) {
            case 0: flexpwm = &IMXRT_FLEXPWM1; break;
            case 1: flexpwm = &IMXRT_FLEXPWM2; break;
            case 2: flexpwm = &IMXRT_FLEXPWM3; break;
            case 3: flexpwm = &IMXRT_FLEXPWM4; break;
            default: break;
        }
       switch (info->channel) {
                case 0: // X
                    flexpwm->SM[info->module & 0x03].OCTRL |= FLEXPWM_SMOCTRL_POLX;
                    break;
                case 1: // A
                    flexpwm->SM[info->module & 0x03].OCTRL |= FLEXPWM_SMOCTRL_POLA;
                    break;
                case 2: // B
                    flexpwm->SM[info->module & 0x03].OCTRL |= FLEXPWM_SMOCTRL_POLB;
                    break;
            }
        
        flexpwm->MCTRL |= FLEXPWM_MCTRL_LDOK(15); 
}

#ifdef __cplusplus
}
#endif
#endif               
#endif /*Rapid Accel condition check*/
/* [EOF] */