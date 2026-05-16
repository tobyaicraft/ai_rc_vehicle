/**********************************************************************************************************************
 * \file DrvAdc.c
 * \brief VADC driver for IR distance sensors and battery voltage
 *
 * Left IR  : AN1  (P40.1) — VADC Group 0, Channel 1, Auto Scan
 * Right IR : AN12 (P41.0) — VADC Group 1, Channel 0, Auto Scan
 * Battery  : AN0  (P40.0) — VADC Group 0, Channel 0, Auto Scan (voltage divider x0.5)
 *
 * 12-bit resolution, continuous scan mode
 *********************************************************************************************************************/
#include "DrvAdc.h"
#include "DrvFlash.h"
#include "Vadc/Adc/IfxVadc_Adc.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static IfxVadc_Adc vadc;

/* Group 0: AN1 (Channel 1) — Left IR sensor */
/*          AN0 (Channel 0) — Battery voltage  */
static IfxVadc_Adc_Group   s_adcGroup0;
static IfxVadc_Adc_Channel s_adcChIrLeft;
static IfxVadc_Adc_Channel s_adcChBattery;

/* Group 1: AN12 (Channel 0) — Right IR sensor */
static IfxVadc_Adc_Group   s_adcGroup1;
static IfxVadc_Adc_Channel s_adcChIrRight;

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvAdc_Init(void)
{
    /* ---- Module init ---- */
    IfxVadc_Adc_Config adcConfig;
    IfxVadc_Adc_initModuleConfig(&adcConfig, &MODULE_VADC);
    IfxVadc_Adc_initModule(&vadc, &adcConfig);

    /* ---- Group 0: Left IR (AN1 = Group0, Ch1) ---- */
    {
        IfxVadc_Adc_GroupConfig groupConfig;
        IfxVadc_Adc_initGroupConfig(&groupConfig, &vadc);

        groupConfig.groupId = IfxVadc_GroupId_0;
        groupConfig.master  = IfxVadc_GroupId_0;

        groupConfig.arbiter.requestSlotScanEnabled = TRUE;
        groupConfig.scanRequest.autoscanEnabled     = TRUE;
        groupConfig.scanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

        IfxVadc_Adc_initGroup(&s_adcGroup0, &groupConfig);

        /* Channel 1: Left IR (AN1, P40.1) */
        {
            IfxVadc_Adc_ChannelConfig chConfig;
            IfxVadc_Adc_initChannelConfig(&chConfig, &s_adcGroup0);
            chConfig.channelId      = IfxVadc_ChannelId_1;
            chConfig.resultRegister = IfxVadc_ChannelResult_1;
            IfxVadc_Adc_initChannel(&s_adcChIrLeft, &chConfig);
        }

        /* Channel 0: Battery voltage (AN0, P40.0) */
        {
            IfxVadc_Adc_ChannelConfig chConfig;
            IfxVadc_Adc_initChannelConfig(&chConfig, &s_adcGroup0);
            chConfig.channelId      = IfxVadc_ChannelId_0;
            chConfig.resultRegister = IfxVadc_ChannelResult_0;
            IfxVadc_Adc_initChannel(&s_adcChBattery, &chConfig);
        }

        uint32 channels = (1 << 1) | (1 << 0);   /* Ch1 + Ch0 */
        uint32 mask     = (1 << 1) | (1 << 0);
        IfxVadc_Adc_setScan(&s_adcGroup0, channels, mask);
        IfxVadc_Adc_startScan(&s_adcGroup0);
    }

    /* ---- Group 1: Right IR (AN12 = Group1, Ch0) ---- */
    {
        IfxVadc_Adc_GroupConfig groupConfig;
        IfxVadc_Adc_initGroupConfig(&groupConfig, &vadc);

        groupConfig.groupId = IfxVadc_GroupId_1;
        groupConfig.master  = IfxVadc_GroupId_1;

        groupConfig.arbiter.requestSlotScanEnabled = TRUE;
        groupConfig.scanRequest.autoscanEnabled     = TRUE;
        groupConfig.scanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

        IfxVadc_Adc_initGroup(&s_adcGroup1, &groupConfig);

        /* Channel 0 (AN12 = Group1, Ch0, P41.0) */
        IfxVadc_Adc_ChannelConfig chConfig;
        IfxVadc_Adc_initChannelConfig(&chConfig, &s_adcGroup1);

        chConfig.channelId      = IfxVadc_ChannelId_0;
        chConfig.resultRegister = IfxVadc_ChannelResult_0;

        IfxVadc_Adc_initChannel(&s_adcChIrRight, &chConfig);

        uint32 channels = (1 << 0);
        uint32 mask     = (1 << 0);
        IfxVadc_Adc_setScan(&s_adcGroup1, channels, mask);
        IfxVadc_Adc_startScan(&s_adcGroup1);
    }
}

uint16 DrvAdc_GetIrLeft(void)
{
    Ifx_VADC_RES result;
    result = IfxVadc_Adc_getResult(&s_adcChIrLeft);

    if (result.B.VF)
    {
        return (uint16)result.B.RESULT;
    }
    return 0;
}

uint16 DrvAdc_GetIrRight(void)
{
    Ifx_VADC_RES result;
    result = IfxVadc_Adc_getResult(&s_adcChIrRight);

    if (result.B.VF)
    {
        return (uint16)result.B.RESULT;
    }
    return 0;
}

uint16 DrvAdc_GetBatteryMv(void)
{
    Ifx_VADC_RES result;
    result = IfxVadc_Adc_getResult(&s_adcChBattery);

    if (result.B.VF)
    {
        /* 12-bit ADC (0~4095), Vref = 5V
         * V_adc = raw * 5000 / 4095  (mV)
         * V_battery = V_adc * g_batMultiplier / 1000
         * g_batMultiplier: 2000 = x2.000 (기본), 캘리브레이션으로 보정 가능
         * => V_battery_mV = raw * 5000 / 4095 * g_batMultiplier / 1000
         * => V_battery_mV = raw * (5 * g_batMultiplier) / 4095            */
        return (uint16)((uint32)result.B.RESULT * (5u * g_batMultiplier) / 4095u);
    }
    return 0;
}
