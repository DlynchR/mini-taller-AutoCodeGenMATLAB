/* Arduino Bareboard scheduler
 * 
 * Copyright 2013-2024 The MathWorks, Inc.
 */

#ifndef _MW_ARDUINOHWINIT_H_
#define _MW_ARDUINOHWINIT_H_

#ifdef __cplusplus
extern "C" {
#endif
    
void MW_Arduino_Init(void);
void MW_Arduino_Terminate(void);
void MW_Modbus_Slave_Poll(void);
void onOverrunDetect(int id);

//Hardware specific declaration for OTA Poll function
#if (defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_RENESAS_UNO))
void MW_OTA_Poll(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // _MW_ARDUINOHWINIT_H_

/* LocalWords:  Bareboard
 */
