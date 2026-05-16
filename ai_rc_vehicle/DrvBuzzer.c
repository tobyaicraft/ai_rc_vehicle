/**********************************************************************************************************************
 * \file DrvBuzzer.c
 * \brief Passive buzzer driver (PWM, variable frequency)
 *
 * TOM0_Ch10 -> TOUT2 -> P02.2 (X103 pin 15)
 * FXCLK1 = 6,250,000 Hz
 * PlayNote(freq): period = 6,250,000 / freq, duty = 50%
 *********************************************************************************************************************/
#include "DrvBuzzer.h"
#include "Gtm/Tom/Timer/IfxGtm_Tom_Timer.h"
#include "_PinMap/IfxGtm_PinMap.h"

/******************************************************************************/
/*                           Configuration                                    */
/******************************************************************************/
#define BUZZER_FXCLK1_HZ    6250000u

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static IfxGtm_Tom_Timer s_tomBuzzer;

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvBuzzer_Init(void)
{
    Ifx_GTM *gtm = &MODULE_GTM;

    IfxGtm_Tom_Timer_Config cfg;
    IfxGtm_Tom_Timer_initConfig(&cfg, gtm);

    cfg.base.frequency       = 2000;
    cfg.base.isrPriority     = 0;
    cfg.base.isrProvider     = IfxSrc_Tos_cpu0;
    cfg.base.minResolution   = (1.0 / cfg.base.frequency) / 100;
    cfg.tom                  = IfxGtm_Tom_0;
    cfg.timerChannel         = IfxGtm_Tom_Ch_10;
    cfg.clock                = IfxGtm_Tom_Ch_ClkSrc_cmuFxclk1;

    cfg.triggerOut                      = &IfxGtm_TOM0_10_TOUT2_P02_2_OUT;
    cfg.base.trigger.outputEnabled      = TRUE;
    cfg.base.trigger.enabled            = TRUE;
    cfg.base.trigger.triggerPoint       = 0;
    cfg.base.trigger.risingEdgeAtPeriod = TRUE;

    IfxGtm_Tom_Timer_init(&s_tomBuzzer, &cfg);
    IfxGtm_Tom_Timer_run(&s_tomBuzzer);
}

void DrvBuzzer_PlayNote(uint16 freqHz)
{
    if (freqHz == 0u)
    {
        DrvBuzzer_Off();
        return;
    }

    Ifx_TimerValue newPeriod = BUZZER_FXCLK1_HZ / freqHz;
    Ifx_TimerValue trigger   = newPeriod / 2;   /* 50% duty */

    IfxGtm_Tom_Timer_disableUpdate(&s_tomBuzzer);
    IfxGtm_Tom_Timer_setPeriod(&s_tomBuzzer, newPeriod);
    IfxGtm_Tom_Timer_setTrigger(&s_tomBuzzer, trigger);
    IfxGtm_Tom_Timer_applyUpdate(&s_tomBuzzer);
}

void DrvBuzzer_Off(void)
{
    IfxGtm_Tom_Timer_disableUpdate(&s_tomBuzzer);
    IfxGtm_Tom_Timer_setTrigger(&s_tomBuzzer, 0);
    IfxGtm_Tom_Timer_applyUpdate(&s_tomBuzzer);
}
