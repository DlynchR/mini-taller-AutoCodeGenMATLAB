/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: pot_led.h
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

#ifndef pot_led_h_
#define pot_led_h_
#ifndef pot_led_COMMON_INCLUDES_
#define pot_led_COMMON_INCLUDES_
#include "rtwtypes.h"
#include "rtw_continuous.h"
#include "rtw_solver.h"
#endif                                 /* pot_led_COMMON_INCLUDES_ */

#include "pot_led_types.h"
#include "rt_nonfinite.h"
#include <stddef.h>
#include "MW_target_hardware_resources.h"

/* Macros for accessing real-time model data structure */
#ifndef rtmGetErrorStatus
#define rtmGetErrorStatus(rtm)         ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
#define rtmSetErrorStatus(rtm, val)    ((rtm)->errorStatus = (val))
#endif

/* Block signals and states (default storage) for system '<Root>' */
typedef struct {
  codertarget_arduinobase_int_o_T obj_p;/* '<Root>/Lectura_A0' */
  codertarget_arduinobase_inter_T obj; /* '<Root>/LED_D3' */
} DW_pot_led_T;

/* Parameters (default storage) */
struct P_pot_led_T_ {
  real32_T Escala_PWM_Gain;            /* Expression: single(255/1023)
                                        * Referenced by: '<Root>/Escala_PWM'
                                        */
  real32_T Limites_PWM_UpperSat;     /* Computed Parameter: Limites_PWM_UpperSat
                                      * Referenced by: '<Root>/Limites_PWM'
                                      */
  real32_T Limites_PWM_LowerSat;     /* Computed Parameter: Limites_PWM_LowerSat
                                      * Referenced by: '<Root>/Limites_PWM'
                                      */
};

/* Real-time Model Data Structure */
struct tag_RTM_pot_led_T {
  const char_T * volatile errorStatus;
};

/* Block parameters (default storage) */
extern P_pot_led_T pot_led_P;

/* Block signals and states (default storage) */
extern DW_pot_led_T pot_led_DW;

/* Model entry point functions */
extern void pot_led_initialize(void);
extern void pot_led_step(void);
extern void pot_led_terminate(void);

/* Real-time Model object */
extern RT_MODEL_pot_led_T *const pot_led_M;
extern volatile boolean_T stopRequested;
extern volatile boolean_T runModel;

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'pot_led'
 */
#endif                                 /* pot_led_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
