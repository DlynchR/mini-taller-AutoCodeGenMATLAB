/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: pot_led_types.h
 *
 * Code generated for Simulink model 'pot_led'.
 *
 * Model version                  : 1.2
 * Simulink Coder version         : 26.2 (R2026b) 22-May-2026
 * C/C++ source code generated on : Thu Oct 15 20:27:40 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Atmel->AVR
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef pot_led_types_h_
#define pot_led_types_h_
#include "MW_SVD.h"
#include "rtwtypes.h"
#ifndef struct_tag_UvJx03atA3spPs6mvx8QqE
#define struct_tag_UvJx03atA3spPs6mvx8QqE

struct tag_UvJx03atA3spPs6mvx8QqE
{
  MW_Handle_Type MW_PWM_HANDLE;
};

#endif                                 /* struct_tag_UvJx03atA3spPs6mvx8QqE */

#ifndef typedef_e_matlabshared_ioclient_perip_T
#define typedef_e_matlabshared_ioclient_perip_T

typedef struct tag_UvJx03atA3spPs6mvx8QqE e_matlabshared_ioclient_perip_T;

#endif                             /* typedef_e_matlabshared_ioclient_perip_T */

#ifndef struct_tag_98oXr1FLAUvDVbfqcQVTfD
#define struct_tag_98oXr1FLAUvDVbfqcQVTfD

struct tag_98oXr1FLAUvDVbfqcQVTfD
{
  boolean_T matlabCodegenIsDeleted;
  int32_T isInitialized;
  boolean_T isSetupComplete;
  e_matlabshared_ioclient_perip_T PWMDriverObj;
};

#endif                                 /* struct_tag_98oXr1FLAUvDVbfqcQVTfD */

#ifndef typedef_codertarget_arduinobase_inter_T
#define typedef_codertarget_arduinobase_inter_T

typedef struct tag_98oXr1FLAUvDVbfqcQVTfD codertarget_arduinobase_inter_T;

#endif                             /* typedef_codertarget_arduinobase_inter_T */

#ifndef struct_tag_mVifzvLPn0StcnJENmjEHE
#define struct_tag_mVifzvLPn0StcnJENmjEHE

struct tag_mVifzvLPn0StcnJENmjEHE
{
  MW_Handle_Type MW_ANALOGIN_HANDLE;
};

#endif                                 /* struct_tag_mVifzvLPn0StcnJENmjEHE */

#ifndef typedef_e_arduinodriver_ArduinoAnalog_T
#define typedef_e_arduinodriver_ArduinoAnalog_T

typedef struct tag_mVifzvLPn0StcnJENmjEHE e_arduinodriver_ArduinoAnalog_T;

#endif                             /* typedef_e_arduinodriver_ArduinoAnalog_T */

#ifndef struct_tag_vVzUTixqlt3OpD5jelgrr
#define struct_tag_vVzUTixqlt3OpD5jelgrr

struct tag_vVzUTixqlt3OpD5jelgrr
{
  boolean_T matlabCodegenIsDeleted;
  int32_T isInitialized;
  boolean_T isSetupComplete;
  e_arduinodriver_ArduinoAnalog_T AnalogInDriverObj;
};

#endif                                 /* struct_tag_vVzUTixqlt3OpD5jelgrr */

#ifndef typedef_codertarget_arduinobase_int_o_T
#define typedef_codertarget_arduinobase_int_o_T

typedef struct tag_vVzUTixqlt3OpD5jelgrr codertarget_arduinobase_int_o_T;

#endif                             /* typedef_codertarget_arduinobase_int_o_T */

/* Parameters (default storage) */
typedef struct P_pot_led_T_ P_pot_led_T;

/* Forward declaration for rtModel */
typedef struct tag_RTM_pot_led_T RT_MODEL_pot_led_T;

#endif                                 /* pot_led_types_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
