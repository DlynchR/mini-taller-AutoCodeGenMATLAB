#ifndef _MW_ARDUINOPWMDRIVER_H_
#define _MW_ARDUINOPWMDRIVER_H_
#include <inttypes.h>
#include "rtwtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) ||  defined(RSIM_WITH_SL_SOLVER))
 /* This will be run in Rapid Accelerator Mode */
 #define MW_analogWrite(a,b)                        (0)
 #define MW_setAnalogFrequency(a,b,c)               (0)
 #define MW_ConfigureOutputPolarity(a,b)            (0)
 #define MW_EnablePWMMatchInterrupt(a)              (0)
 #define MW_EnablePWMOverflowInterrupt(a)           (0)
 #define MW_EnableTeensyPWMMatchInterrupt(a,b)      (0)
 #define MW_SetWavegenMode(a,b)                     (0)
 
#else    
    void MW_analogWrite(uint32_t pin, uint32_t value);
    void MW_setAnalogFrequency(uint16_t pin,uint16_t prescalar,uint16_t period);
    void MW_ConfigureOutputPolarity(uint8_T pin,boolean_T invertedPolarity);
    void MW_EnablePWMMatchInterrupt(uint8_T pin);
    void MW_EnableTeensyPWMMatchInterrupt(uint8_T pin,uint8_T mask);
    void MW_EnablePWMOverflowInterrupt(uint8_T pin);
    void MW_SetWavegenMode(uint8_T pin,uint8_T wavegenMode);
#endif

#ifdef __cplusplus
}
#endif
#endif //_MW_ARDUINOPWMDRIVER_H_
