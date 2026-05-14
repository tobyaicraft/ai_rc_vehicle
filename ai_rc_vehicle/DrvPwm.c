/**********************************************************************************************************************
 * \file DrvPwm.c
 * \brief GTM TOM0 4채널 모터 PWM 드라이버 (1kHz)
 *
 * Clock chain:
 *   GTM GCLK  = 100 MHz
 *   FXCLK1    = GCLK / 16 = 6,250,000 Hz
 *   period    = 6250 ticks → 1 kHz PWM
 *
 * FL: TOM0_Ch0  -> P33.4   FR: TOM0_Ch1  -> P33.5
 * RL: TOM0_Ch6  -> P02.4   RR: TOM0_Ch13  -> P02.5
 *
 * 주의: GTM Enable 및 FXCLK 활성화는 DrvGtmTimer_Init()에서 수행됨
 *       반드시 DrvGtmTimer_Init() 이후에 호출할 것
 *********************************************************************************************************************/
#include "DrvPwm.h"
#include "Gtm/Tom/Timer/IfxGtm_Tom_Timer.h"
#include "_PinMap/IfxGtm_PinMap.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static IfxGtm_Tom_Timer s_tomFL;
static IfxGtm_Tom_Timer s_tomFR;
static IfxGtm_Tom_Timer s_tomRL;
static IfxGtm_Tom_Timer s_tomRR;

/******************************************************************************/
/*                           Static Functions                                 */
/******************************************************************************/
static void initTomPwm(IfxGtm_Tom_Timer *timer, Ifx_GTM *gtm,
                        IfxGtm_Tom tom, IfxGtm_Tom_Ch ch,
                        IfxGtm_Tom_ToutMap *pin)
{
    IfxGtm_Tom_Timer_Config cfg;
    IfxGtm_Tom_Timer_initConfig(&cfg, gtm);

    cfg.base.frequency       = 1000;         /* 1 kHz PWM */
    cfg.base.isrPriority     = 0;            /* ISR 불필요 */
    cfg.base.isrProvider     = IfxSrc_Tos_cpu0;
    cfg.base.minResolution   = (1.0 / cfg.base.frequency) / 100;
    cfg.tom                  = tom;
    cfg.timerChannel         = ch;
    cfg.clock                = IfxGtm_Tom_Ch_ClkSrc_cmuFxclk1;  /* GCLK/16 = 6.25MHz */

    cfg.triggerOut                      = pin;
    cfg.base.trigger.outputEnabled      = TRUE;
    cfg.base.trigger.enabled            = TRUE;
    cfg.base.trigger.triggerPoint       = 0;    /* 초기 duty = 0% */
    cfg.base.trigger.risingEdgeAtPeriod = TRUE;

    IfxGtm_Tom_Timer_init(timer, &cfg);
    IfxGtm_Tom_Timer_run(timer);
}

static void setDuty(IfxGtm_Tom_Timer *timer, float32 dutyPercent)
{
    if (dutyPercent < 0.0f) dutyPercent = 0.0f;
    if (dutyPercent > 100.0f) dutyPercent = 100.0f;

    Ifx_TimerValue period = IfxGtm_Tom_Timer_getPeriod(timer);
    Ifx_TimerValue triggerPoint = (Ifx_TimerValue)(period * dutyPercent / 100.0f);

    IfxGtm_Tom_Timer_disableUpdate(timer);
    IfxGtm_Tom_Timer_setTrigger(timer, triggerPoint);
    IfxGtm_Tom_Timer_applyUpdate(timer);
}

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvPwm_Init(void)
{
    Ifx_GTM *gtm = &MODULE_GTM;

    /* FL: TOM0_Ch0 -> P33.4 (X102 pin 14) */
    initTomPwm(&s_tomFL, gtm, IfxGtm_Tom_0, IfxGtm_Tom_Ch_0,
               &IfxGtm_TOM0_0_TOUT26_P33_4_OUT);

    /* FR: TOM0_Ch1 -> P33.5 (X102 pin 13) */
    initTomPwm(&s_tomFR, gtm, IfxGtm_Tom_0, IfxGtm_Tom_Ch_1,
               &IfxGtm_TOM0_1_TOUT27_P33_5_OUT);

    /* RL: TOM0_Ch6 -> P02.4 (X103 pin 17) */
    initTomPwm(&s_tomRL, gtm, IfxGtm_Tom_0, IfxGtm_Tom_Ch_6,
               &IfxGtm_TOM0_6_TOUT4_P02_4_OUT);

    /* RR: TOM0_Ch13 -> P02.5 (X103 pin 18) */
    initTomPwm(&s_tomRR, gtm, IfxGtm_Tom_0, IfxGtm_Tom_Ch_13,
               &IfxGtm_TOM0_13_TOUT5_P02_5_OUT);

    /* 모든 TOM 채널 설정 완료 후 FXCLK 활성화 */
    IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_FXCLK);
}

void DrvPwm_SetDutyFL(float32 dutyPercent) { setDuty(&s_tomFL, dutyPercent); }
void DrvPwm_SetDutyFR(float32 dutyPercent) { setDuty(&s_tomFR, dutyPercent); }
void DrvPwm_SetDutyRL(float32 dutyPercent) { setDuty(&s_tomRL, dutyPercent); }
void DrvPwm_SetDutyRR(float32 dutyPercent) { setDuty(&s_tomRR, dutyPercent); }
