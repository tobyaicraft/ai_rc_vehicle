/**********************************************************************************************************************
 * \file DrvUltrasonic.c
 * \brief HC-SR04 Ultrasonic sensor driver (GTM TIM0_7 PWM capture)
 *
 * TRIG: P02.6 — GPIO push-pull output, 10us HIGH pulse
 * ECHO: P02.7 — TIM0_7 (TIN7) PWM measurement mode
 *
 * Distance(cm) = echo_pulse(us) / 58
 * Valid range: 2 ~ 400 cm
 *
 * Note: GTM module must be enabled before calling DrvUltrasonic_Init().
 *       DrvGtmTimer_Init() handles GTM enable and CMU_CLK0 setup.
 *********************************************************************************************************************/
#include "DrvUltrasonic.h"
#include "Port/Std/IfxPort.h"
#include "Gtm/Std/IfxGtm_Tim.h"
#include "Gtm/Std/IfxGtm_Cmu.h"
#include "_PinMap/IfxGtm_PinMap.h"

/******************************************************************************/
/*                           Defines                                          */
/******************************************************************************/
/* Trig: P02.6 (GPIO output) */
#define TRIG_PORT   &MODULE_P02
#define TRIG_PIN    6

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
/* Echo: P02.7 (TIM0_7 input capture) */
static Ifx_GTM_TIM_CH *s_echoTimCh;
static float32 s_echoClkFreq;

/* Last measured distance */
static volatile float32 s_distanceCm = 0.0f;

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvUltrasonic_Init(void)
{
    Ifx_GTM *gtm = &MODULE_GTM;

    /* Trig pin: P02.6 as push-pull output, initial LOW */
    IfxPort_setPinModeOutput(TRIG_PORT, TRIG_PIN, IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    IfxPort_setPinLow(TRIG_PORT, TRIG_PIN);

    /* Echo pin: TIM0_7 on P02.7 — PWM measurement mode */
    s_echoTimCh = IfxGtm_Tim_getChannel(&gtm->TIM[0], IfxGtm_Tim_Ch_7);

    IfxGtm_Tim_Ch_resetChannel(&gtm->TIM[0], IfxGtm_Tim_Ch_7);

    s_echoTimCh->CTRL.U = 0;
    s_echoTimCh->CTRL.B.TIM_MODE = IfxGtm_Tim_Mode_pwmMeasurement;
    s_echoTimCh->CTRL.B.DSL      = 1;   /* Duty = HIGH level */
    s_echoTimCh->CTRL.B.CNTS_SEL = IfxGtm_Tim_CntsSel_cntReg;
    s_echoTimCh->CTRL.B.GPR0_SEL = IfxGtm_Tim_GprSel_cnts;
    s_echoTimCh->CTRL.B.GPR1_SEL = IfxGtm_Tim_GprSel_cnts;

    IfxGtm_Tim_Ch_setClockSource(s_echoTimCh, IfxGtm_Cmu_Clk_0);

    /* Input source: TIM0_7 <- TIN7 <- P02.7 */
    gtm->TIM[0].IN_SRC.B.VAL_7  = 1;
    gtm->TIM[0].IN_SRC.B.MODE_7 = 1;
    s_echoTimCh->CTRL.B.CICTRL = IfxGtm_Tim_Input_currentChannel;

    IfxGtm_PinMap_setTimTin(&IfxGtm_TIM0_7_TIN7_P02_7_IN,
                             IfxPort_InputMode_noPullDevice);

    /* CMU_CLK0 must be enabled for TIM capture clock */
    IfxGtm_Cmu_setClkFrequency(gtm, IfxGtm_Cmu_Clk_0, IfxGtm_Cmu_getModuleFrequency(gtm));
    IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_CLK0);

    s_echoClkFreq = IfxGtm_Tim_Ch_getCaptureClockFrequency(gtm, s_echoTimCh);

    s_echoTimCh->CTRL.B.TIM_EN = 1;
}

void DrvUltrasonic_Trigger(void)
{
    /* Send 10us HIGH pulse on Trig pin */
    IfxPort_setPinHigh(TRIG_PORT, TRIG_PIN);

    /* ~10us delay at 200MHz: 200MHz * 10us = 2000 cycles */
    {
        volatile uint32 i;
        for (i = 0; i < 2000; i++)
        {
            /* wait */
        }
    }

    IfxPort_setPinLow(TRIG_PORT, TRIG_PIN);

    /* Read previous echo capture result */
    if (IfxGtm_Tim_Ch_isNewValueEvent(s_echoTimCh))
    {
        uint32 dutyTicks = s_echoTimCh->GPR0.B.GPR0;

        if (dutyTicks > 0 && s_echoClkFreq > 0)
        {
            /* Convert ticks to microseconds */
            float32 pulseUs = (float32)dutyTicks / s_echoClkFreq * 1000000.0f;

            /* Distance(cm) = pulse(us) / 58 */
            float32 dist = pulseUs / 58.0f;

            /* Clamp to valid range: 2-400cm */
            if (dist < 2.0f) dist = 2.0f;
            if (dist > 400.0f) dist = 400.0f;

            s_distanceCm = dist;
        }

        IfxGtm_Tim_Ch_clearNewValueEvent(s_echoTimCh);
    }
}

float32 DrvUltrasonic_GetDistanceCm(void)
{
    return s_distanceCm;
}
