/**********************************************************************************************************************
 * \file DrvDio.h
 * \brief Digital I/O driver - LED and general-purpose GPIO control
 *********************************************************************************************************************/
#ifndef DRVDIO_H
#define DRVDIO_H

#include "Ifx_Types.h"

void DrvDio_Init(void);
void DrvDio_ToggleLed0(void);
void DrvDio_SetLed0On(void);
void DrvDio_SetLed0Off(void);

#endif /* DRVDIO_H */
