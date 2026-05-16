/**********************************************************************************************************************
 * \file DrvAdc.h
 * \brief VADC driver for IR distance sensors and battery voltage
 *
 * Left IR  : AN1  (P40.1) — VADC Group 0, Channel 1
 * Right IR : AN12 (P41.0) — VADC Group 1, Channel 0
 * Battery  : AN0  (P40.0) — VADC Group 0, Channel 0 (voltage divider x0.5)
 *********************************************************************************************************************/
#ifndef DRVADC_H
#define DRVADC_H

#include "Ifx_Types.h"

void DrvAdc_Init(void);

uint16 DrvAdc_GetIrLeft(void);    /* AN1  — Group 0, Channel 1 */
uint16 DrvAdc_GetIrRight(void);   /* AN12 — Group 1, Channel 0 */
uint16 DrvAdc_GetBatteryMv(void); /* AN0  — Group 0, Channel 0, returns mV */

#endif /* DRVADC_H */
