/**********************************************************************************************************************
 * \file DrvAdc.h
 * \brief VADC driver for IR distance sensors
 *
 * Left IR  : AN1  (P40.1) — VADC Group 0, Channel 1
 * Right IR : AN12 (P41.0) — VADC Group 1, Channel 0
 *********************************************************************************************************************/
#ifndef DRVADC_H
#define DRVADC_H

#include "Ifx_Types.h"

void DrvAdc_Init(void);

uint16 DrvAdc_GetIrLeft(void);    /* AN1  — Group 0, Channel 1 */
uint16 DrvAdc_GetIrRight(void);   /* AN12 — Group 1, Channel 0 */

#endif /* DRVADC_H */
