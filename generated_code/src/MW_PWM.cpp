/* Copyright 2015-2026 The MathWorks, Inc. */

#include "MW_PWM.h"
#include "MW_PWMDriver.h"

#if (defined(ESP_PLATFORM))
#include "soc/soc_caps.h" // This file is present in new core v3.2.0 of esp32.
#endif

#include "NanoESP32PinRemap.h"

#if defined(ARDUINO_ARCH_RENESAS_UNO)
#include "pwm.h"

// Define the number of PWM pins
#define NUM_PWM_PINS 6

// Create an array of PwmOut objects
static PwmOut* pwmObjects[NUM_PWM_PINS];
static uint8_T pwmPinIndex[NUM_PWM_PINS];
static uint8_T pwmPinCounter = 0;


// Create a PwmOut object for a specified pin and add it to the array
void createPwmObject(int pin) {
    pwmObjects[pwmPinCounter] = new PwmOut(pin);
    pwmPinIndex[pwmPinCounter] = (uint8_T)pin;
    pwmPinCounter++;
    if (pwmPinCounter>= NUM_PWM_PINS) {
        // A safeguard to avoid accidental out-of-bound access
        pwmPinCounter = NUM_PWM_PINS-1;
    }
}


int getIndexForPWMPin(int pin) {
    int i=0;
    int idx = 0;
    for (i=0;i<NUM_PWM_PINS;i++) {
        if (pin==pwmPinIndex[i]) {
            idx = i;
            break;
        }
    }
    return idx;
}

#endif

#if !(( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) ))
#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
    #endif

    #define PWM_MODULES_MAX 70  /* For Mega and Due. Other boards have less number of Modules */
    static uint8_T PinMapTable[MW_NUM_PINS]={0};
    #if (defined(ARDUINO_ARDUINO_NANO33BLE))
    uint32_t nano33BLE_dutycycle[21];
    uint16_t nano33BLE_period[21];
    #endif

    typedef uint8_T PeripheralHandleMapType;
    extern PeripheralHandleMapType PWMPinHandleMap[];     /* The array is defined in ArduinoPinHandleMap.cpp */

    /* For PWM SVd provides no function to get the handle */
    MW_Handle_Type MW_PWM_GetHandle(uint32_T Pin);
    void setPWMHandle(uint32_T Pin, MW_Handle_Type handle);

    #if (defined(ESP_PLATFORM))
    #define ESP32_PWM_RESOLUTION            8
    #if (defined(_RTT_NUMSERVOS_) && (_RTT_NUMSERVOS_ != 0))
    #define LAST_CHAN (8)
    #else
    #define LAST_CHAN (16)
    #endif
    static int8_t pin_to_channel[SOC_GPIO_PIN_COUNT] = { 0 };
    static int channelCounter = LAST_CHAN;
    #endif

    /* PWM Initialisation selected by the pinNumber (PWM Channel) */
    MW_Handle_Type MW_PWM_Open(uint32_T pin, real_T prescaler, real_T period)
    {
        #if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )
        /* This will be run in Rapid Accelerator Mode */
        return NULL;
        #else

        #if (defined(ESP_PLATFORM))
        int8_t gpioPin = convertPinToGPIO(pin);
        if (gpioPin >= 0 && gpioPin < SOC_GPIO_PIN_COUNT) {
            if (pin_to_channel[gpioPin] == 0) {
                if (!channelCounter) {
                    log_e("No more analogWrite channels available! You can have maximum %u", LAST_CHAN);
                    return 0;
                }
                pinMode(gpioPin, OUTPUT);

                // Update based on ESP32 Core v3.2.0
                // ledcAttach returns the channel number
                uint8_t channel;
                if(prescaler == 0) {
                    // Default frequency
                    channel = ledcAttach(gpioPin, ARDUINO_DEFAULT_PWM_FREQUENCY, ESP32_PWM_RESOLUTION);
                }
                else {
                    channel = ledcAttach(gpioPin, prescaler, ESP32_PWM_RESOLUTION);
                }

                // Check if channel assignment was successful
                if(channel == 255) {
                    log_e("ledcAttach failed for pin %u", gpioPin);
                    return 0;
                }

                // Store the channel mapping
                pin_to_channel[gpioPin] = channel + 1;  // Store as channel+1 (0 means unused)

                if(period > 0) {
                    channelCounter = channelCounter - 2;  // Use only one channel from channel pair
                }
                else {
                    channelCounter = channelCounter - 1;
                }

                // Set initial duty cycle to 0 - NEW API uses pin directly
                ledcWrite(gpioPin, 0);
            }
        } else {
            return 0;
        }
        setPWMHandle(pin, (MW_Handle_Type)(pin + 1));
        return (MW_Handle_Type)(pin + 1);

        #elif defined(__IMXRT1062__)
        #include "core_pins.h"
        if(prescaler > 0)                    // Specify Frequency
        {
            PinMapTable[pin]=1;
            analogWriteResolution(ARDUINO_TEENSY_RESOLUTION);
            MW_setAnalogFrequency(pin, prescaler,period);
        }
        setPWMHandle(pin, (MW_Handle_Type)(pin+1));
        return (MW_Handle_Type)(pin+1);

        #else
        pinMode(pin,OUTPUT);

        #if (defined(ARDUINO_ARDUINO_NANO33BLE))
        nano33BLE_dutycycle[pin] = 0;
        nano33BLE_period[pin] = 1000000/500;

        #elif (defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO))
        // RP2040 uses a hardware PWM, hence we need to use the prescaler,period values based on the default frequency configset parameter
        if(prescaler == 0)                    // Default frequency
        {
            MW_setAnalogFrequency(pin, ARDUINO_RP2040_DEFAULT_PWM_PRESCALER, ARDUINO_RP2040_DEFAULT_PWM_PERIOD);
        }
        #endif
        /* handle is stored as Pin+1 because, to reset the handle of a pin, 0 is passed as handle. Otherwise it will create confusion for Pin number 0. */
        setPWMHandle(pin, (MW_Handle_Type)(pin+1));

        if(period>0) /*Initialization needed for specify frequnecy*/
        {
            PinMapTable[pin]=1;
            #if defined(ARDUINO_ARCH_RENESAS_UNO)
            createPwmObject(pin);
            pwmObjects[pwmPinCounter-1]->begin(prescaler, 0, false, TIMER_SOURCE_DIV_1);
            #else
            MW_setAnalogFrequency(pin, prescaler,period);
            #endif
        }
        return (MW_Handle_Type)(pin+1);
        #endif
        #endif
    }

    /* Start PWM */
    void MW_PWM_Start(MW_Handle_Type PWMPinHandle)
    {
        (void)PWMPinHandle;
    }

    /* Set the duty cycle or pulse width for the PWM signal */
    void MW_PWM_SetDutyCycle(MW_Handle_Type PWMPinHandle, real_T dutyCycle)
    {
        #if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )
        /* This will be run in Rapid Accelerator Mode */
        (void)PWMPinHandle;
        (void)dutyCycle;
        return;
        #else
        #if (defined(ESP_PLATFORM))
        uint8_T pin;
        if(*((uint8_T*)(&PWMPinHandle)))
        {
            /* The handle of a Pin is stored as (Pin+1) */
            pin = *((uint8_T*)(&PWMPinHandle)) - 1;
            int gpioPin = convertPinToGPIO(pin);
            if(dutyCycle < 0) {  /* Changed frequency */
                dutyCycle = dutyCycle * -1;
            }
            // NEW API: ledcWrite uses pin directly, not channel
            ledcWrite(gpioPin, (uint32_t)dutyCycle);
        }
        #elif defined(__IMXRT1062__)
        uint8_T pin;
        if(*((uint8_T*)(&PWMPinHandle)))
        {
            /* The handle of a Pin is stored as (Pin+1) */
            pin =  *((uint8_T*)(&PWMPinHandle)) - 1;
            if(PinMapTable[pin]==1)       /* Changed frequency */
            {
                dutyCycle = dutyCycle * -1;
            }

            MW_analogWrite(pin, dutyCycle);
        }

        #elif defined(ARDUINO_ARCH_RENESAS_UNO)
        uint8_T pin;
        if(*((uint8_T*)(&PWMPinHandle)))
        {
            /* The handle of a Pin is stored as (Pin+1) */
            pin =  *((uint8_T*)(&PWMPinHandle)) - 1;
            if(PinMapTable[pin]==1)       /* Specify frequency */
            {
                dutyCycle = dutyCycle * -1;
                int locPinIndex = getIndexForPWMPin(pin);
                pwmObjects[locPinIndex]->pulse_perc((float)(dutyCycle * 100) / 255);
            }
            else
            {
                analogWrite(pin, dutyCycle);  /* Default frequency*/
            }
        }

        #else
        uint8_T pin;
        /* If the PWM handle is 0 then do nothing. */
        if(*((uint8_T*)(&PWMPinHandle)))
        {
            /* The handle of a Pin is stored as (Pin+1) */
            pin =  *((uint8_T*)(&PWMPinHandle)) - 1;
            if( PinMapTable[pin]==1)       /* make sure MW_analogWrite gets executed at Specify Frequency*/
            {
                dutyCycle = dutyCycle * -1;
                MW_analogWrite(pin, dutyCycle);
            }
            else                    /* Default frequency*/
            {
                #if defined(ARDUINO_ARDUINO_NANO33BLE)
                nano33BLE_period[pin] = 1000000/500; //Default frequency of 500Hz
                nano33BLE_dutycycle[pin] = (dutyCycle* nano33BLE_period[pin]/255);
                #elif (defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO))
                MW_analogWrite(pin, ((dutyCycle/255) * ARDUINO_RP2040_DEFAULT_PWM_PERIOD));
                #else
                analogWrite(pin, dutyCycle);
                #endif
            }
        }
        #endif
        #endif
    }

    /* Set the PWM signal frequency */
    void MW_PWM_SetFrequency(MW_Handle_Type PWMPinHandle, real_T frequency)
    {
        (void)PWMPinHandle;
        (void)frequency;
    }

    /* Disable notifications on the channel */
    void MW_PWM_DisableNotification(MW_Handle_Type PWMPinHandle)
    {
        (void)PWMPinHandle;
    }

    /* Enable notifications on the channel */
    void MW_PWM_EnableNotification(MW_Handle_Type PWMPinHandle, MW_PWM_EdgeNotification_Type Notification)
    {
        (void)PWMPinHandle;
        (void)Notification;
    }

    /* Set PWM output to idle state */
    void MW_PWM_SetOutputToIdle(MW_Handle_Type PWMPinHandle)
    {
        (void)PWMPinHandle;
    }

    /* Get the PWM output status */
    boolean_T MW_PWM_GetOutputState(MW_Handle_Type PWMPinHandle)
    {
        (void)PWMPinHandle;
        return 0;
    }

    /* Stop PWM */
    void MW_PWM_Stop(MW_Handle_Type PWMPinHandle)
    {
        (void)PWMPinHandle;
    }

    /* Close PWM */
    void MW_PWM_Close(MW_Handle_Type PWMPinHandle)
    {
        #if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )
        return;
        #else
        /* If the PWM handle is 0 then do nothing. */
        if(*((uint8_T*)(&PWMPinHandle)))
        {
            uint8_T pin = 0;
            /* The handle of a Pin is stored as (Pin+1) */
            pin =  *((uint8_T*)(&PWMPinHandle)) - 1;
            setPWMHandle(pin, (MW_Handle_Type)0);
        }
        (void)PWMPinHandle;
        #endif
    }

    MW_Handle_Type MW_PWM_GetHandle(uint32_T Pin)
    {
        #if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )
        return NULL;
        #else
        uint16_T ByteNo=0, handleValue=0;
        uint8_T BitPositionNo=0;

        ByteNo = Pin/8;
        BitPositionNo = Pin%8;

        handleValue = PWMPinHandleMap[ByteNo] & (PeripheralHandleMapType)(1<<BitPositionNo);

        if(handleValue)
        {
            /*
            Returns the value of the module if the pin is opened earlier
            */
            handleValue = Pin+1;
            /*
            This allows to differentiate between pin zero and a Null condition

            Another option - return true, and then remove the decrement in the SVD of all functions in Arduino
            */
        }
        else
        {
            handleValue = 0;
        }
        return (MW_Handle_Type)handleValue;
        #endif
    }

    void setPWMHandle(uint32_T module, MW_Handle_Type handle)
    {
        #if ( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER) )
        return;
        #else
        /*
        Assumes pin number from count zero
        */
        uint16_T ByteNo=0;
        uint8_T BitPositionNo=0;
        PeripheralHandleMapType tempVar=0;
        /*
        Integer division rounds off to value providing the abs byte no
        */
        ByteNo = module/8;
        BitPositionNo = module%8;

        tempVar = (1<<BitPositionNo);
        /*
        The assumption is module number is the handle,
        but when handle is Null, reset the handle
        */
        if(!handle)
        {
            /*reset the handle*/
            tempVar = ~tempVar;
            PWMPinHandleMap[ByteNo] = PWMPinHandleMap[ByteNo] & tempVar;
        }
        else
        {
            /*set the handle*/
            PWMPinHandleMap[ByteNo] = PWMPinHandleMap[ByteNo] | tempVar;
        }
        #endif
    }

    #ifdef __cplusplus
}
#endif
#else
MW_Handle_Type MW_PWM_Open(uint32_T pin, real_T frequency, real_T dutyCycle){return 0;}
void MW_PWM_Start(MW_Handle_Type PWMPinHandle){}
void MW_PWM_SetDutyCycle(MW_Handle_Type PWMPinHandle, real_T dutyCycle){}
void MW_PWM_SetFrequency(MW_Handle_Type PWMPinHandle, real_T frequency){}
void MW_PWM_DisableNotification(MW_Handle_Type PWMPinHandle){}
void MW_PWM_EnableNotification(MW_Handle_Type PWMPinHandle, MW_PWM_EdgeNotification_Type Notification){}
void MW_PWM_SetOutputToIdle(MW_Handle_Type PWMPinHandle){}
boolean_T MW_PWM_GetOutputState(MW_Handle_Type PWMPinHandle){return 0;}
void MW_PWM_Stop(MW_Handle_Type PWMPinHandle){}
void MW_PWM_Close(MW_Handle_Type PWMPinHandle){}
MW_Handle_Type MW_PWM_GetHandle(uint32_T Pin){return 0;}
#endif
