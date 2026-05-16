/**********************************************************************************************************************
 * \file DrvSensorFusion.c
 * \brief 센서 노이즈 필터링 (이동 평균 + 이상치 제거)
 *
 * IR 센서: ADC raw → 이동 평균 필터
 * 초음파:  거리(cm) → 이동 평균 필터
 *
 * 100ms 주기로 DrvSensorFusion_Update() 호출
 *********************************************************************************************************************/
#include "DrvSensorFusion.h"
#include "DrvAdc.h"
#include "DrvUltrasonic.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static MovingAvg_t s_filterIrLeft;
static MovingAvg_t s_filterIrRight;
static MovingAvg_t s_filterUs;

static uint16 s_irLeftFiltered  = 0u;
static uint16 s_irRightFiltered = 0u;
static uint16 s_usFiltered      = 0u;

/******************************************************************************/
/*                           Moving Average Filter                            */
/******************************************************************************/
void MovingAvg_Init(MovingAvg_t *f)
{
    uint8 i;
    for (i = 0u; i < SENSOR_FILTER_SIZE; i++)
    {
        f->buffer[i] = 0u;
    }
    f->index  = 0u;
    f->filled = FALSE;
}

uint16 MovingAvg_Update(MovingAvg_t *f, uint16 newValue)
{
    f->buffer[f->index] = newValue;
    f->index = (f->index + 1u) % SENSOR_FILTER_SIZE;

    if (f->index == 0u)
    {
        f->filled = TRUE;
    }

    uint8  count = f->filled ? SENSOR_FILTER_SIZE : f->index;
    uint32 sum   = 0u;
    uint8  i;

    if (count == 0u) return 0u;

    /* 이상치(outlier) 제거: 최댓값, 최솟값 각 1개 제외 */
    if (count >= 3u)
    {
        uint16 minVal = 0xFFFFu;
        uint16 maxVal = 0u;

        for (i = 0u; i < count; i++)
        {
            if (f->buffer[i] < minVal) minVal = f->buffer[i];
            if (f->buffer[i] > maxVal) maxVal = f->buffer[i];
            sum += f->buffer[i];
        }

        /* 최대/최소 1개씩 제외 후 평균 */
        sum = sum - minVal - maxVal;
        return (uint16)(sum / (count - 2u));
    }
    else
    {
        /* 샘플 부족: 단순 평균 */
        for (i = 0u; i < count; i++)
        {
            sum += f->buffer[i];
        }
        return (uint16)(sum / count);
    }
}

/******************************************************************************/
/*                           Public API                                       */
/******************************************************************************/
void DrvSensorFusion_Update(void)
{
    s_irLeftFiltered  = MovingAvg_Update(&s_filterIrLeft,  DrvAdc_GetIrLeft());
    s_irRightFiltered = MovingAvg_Update(&s_filterIrRight, DrvAdc_GetIrRight());
    s_usFiltered      = MovingAvg_Update(&s_filterUs, (uint16)DrvUltrasonic_GetDistanceCm());
}

uint16 DrvSensorFusion_GetIrLeft(void)     { return s_irLeftFiltered; }
uint16 DrvSensorFusion_GetIrRight(void)    { return s_irRightFiltered; }
uint16 DrvSensorFusion_GetUltrasonic(void) { return s_usFiltered; }
