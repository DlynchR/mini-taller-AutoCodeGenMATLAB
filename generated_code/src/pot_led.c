/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: pot_led.c
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

#include "pot_led.h"
#include "MW_AnalogIn.h"
#include "MW_PWM.h"
#include "rtwtypes.h"
#include "rt_nonfinite.h"

/* Block signals and states (default storage) */
DW_pot_led_T pot_led_DW;

/* Real-time model */
static RT_MODEL_pot_led_T pot_led_M_;
RT_MODEL_pot_led_T *const pot_led_M = &pot_led_M_;

/* Model step function */
void pot_led_step(void)
{
  real32_T u0;
  uint16_T b_varargout_1;
  uint8_T tmp;

  /* MATLABSystem: '<Root>/Lectura_A0' */
  pot_led_DW.obj_p.AnalogInDriverObj.MW_ANALOGIN_HANDLE = MW_AnalogIn_GetHandle
    (14UL);
  MW_AnalogInSingle_ReadResult
    (pot_led_DW.obj_p.AnalogInDriverObj.MW_ANALOGIN_HANDLE, &b_varargout_1,
     MW_ANALOGIN_UINT16);

  /* MATLABSystem: '<Root>/LED_D3' */
  pot_led_DW.obj.PWMDriverObj.MW_PWM_HANDLE = MW_PWM_GetHandle(3UL);

  /* Gain: '<Root>/Escala_PWM' incorporates:
   *  DataTypeConversion: '<Root>/ADC_a_single'
   *  MATLABSystem: '<Root>/Lectura_A0'
   * */
  u0 = pot_led_P.Escala_PWM_Gain * (real32_T)b_varargout_1;

  /* Saturate: '<Root>/Limites_PWM' */
  if (u0 > pot_led_P.Limites_PWM_UpperSat) {
    /* DataTypeConversion: '<Root>/PWM_a_uint8' */
    u0 = pot_led_P.Limites_PWM_UpperSat;
  } else if (u0 < pot_led_P.Limites_PWM_LowerSat) {
    /* DataTypeConversion: '<Root>/PWM_a_uint8' */
    u0 = pot_led_P.Limites_PWM_LowerSat;
  }

  /* End of Saturate: '<Root>/Limites_PWM' */

  /* DataTypeConversion: '<Root>/PWM_a_uint8' */
  if (u0 < 256.0F) {
    if (u0 >= 0.0F) {
      tmp = (uint8_T)u0;
    } else {
      tmp = 0U;
    }
  } else {
    tmp = MAX_uint8_T;
  }

  /* MATLABSystem: '<Root>/LED_D3' incorporates:
   *  DataTypeConversion: '<Root>/PWM_a_uint8'
   */
  MW_PWM_SetDutyCycle(pot_led_DW.obj.PWMDriverObj.MW_PWM_HANDLE, (real_T)tmp);
}

/* Model initialize function */
void pot_led_initialize(void)
{
  /* Registration code */

  /* initialize non-finites */
  rt_InitInfAndNaN(sizeof(real_T));

  /* Start for MATLABSystem: '<Root>/Lectura_A0' */
  pot_led_DW.obj_p.matlabCodegenIsDeleted = false;
  pot_led_DW.obj_p.isInitialized = 1L;
  pot_led_DW.obj_p.AnalogInDriverObj.MW_ANALOGIN_HANDLE = MW_AnalogInSingle_Open
    (14UL);
  pot_led_DW.obj_p.isSetupComplete = true;

  /* Start for MATLABSystem: '<Root>/LED_D3' */
  pot_led_DW.obj.matlabCodegenIsDeleted = false;
  pot_led_DW.obj.isInitialized = 1L;
  pot_led_DW.obj.PWMDriverObj.MW_PWM_HANDLE = MW_PWM_Open(3UL, 0.0, 0.0);
  pot_led_DW.obj.isSetupComplete = true;
}

/* Model terminate function */
void pot_led_terminate(void)
{
  /* Terminate for MATLABSystem: '<Root>/Lectura_A0' */
  if (!pot_led_DW.obj_p.matlabCodegenIsDeleted) {
    pot_led_DW.obj_p.matlabCodegenIsDeleted = true;
    if ((pot_led_DW.obj_p.isInitialized == 1L) &&
        pot_led_DW.obj_p.isSetupComplete) {
      pot_led_DW.obj_p.AnalogInDriverObj.MW_ANALOGIN_HANDLE =
        MW_AnalogIn_GetHandle(14UL);
      MW_AnalogIn_Close(pot_led_DW.obj_p.AnalogInDriverObj.MW_ANALOGIN_HANDLE);
    }
  }

  /* End of Terminate for MATLABSystem: '<Root>/Lectura_A0' */

  /* Terminate for MATLABSystem: '<Root>/LED_D3' */
  if (!pot_led_DW.obj.matlabCodegenIsDeleted) {
    pot_led_DW.obj.matlabCodegenIsDeleted = true;
    if ((pot_led_DW.obj.isInitialized == 1L) && pot_led_DW.obj.isSetupComplete)
    {
      pot_led_DW.obj.PWMDriverObj.MW_PWM_HANDLE = MW_PWM_GetHandle(3UL);
      MW_PWM_SetDutyCycle(pot_led_DW.obj.PWMDriverObj.MW_PWM_HANDLE, 0.0);
      pot_led_DW.obj.PWMDriverObj.MW_PWM_HANDLE = MW_PWM_GetHandle(3UL);
      MW_PWM_Close(pot_led_DW.obj.PWMDriverObj.MW_PWM_HANDLE);
    }
  }

  /* End of Terminate for MATLABSystem: '<Root>/LED_D3' */
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
