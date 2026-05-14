#include "AppMotorTest.h"
#include "DrvMotor.h"
#include "DrvGtmTimer.h"

static void delay_ms(uint32 ms)
{
    uint32 start = g_1ms_counter;
    while ((g_1ms_counter - start) < ms) {}
}

void AppMotorTest_Sequence(void)
{
    /* 전진 2초 */
    DrvMotor_SetDuty(MOTOR_FL, +50);
    DrvMotor_SetDuty(MOTOR_FR, +50);
    DrvMotor_SetDuty(MOTOR_RL, +50);
    DrvMotor_SetDuty(MOTOR_RR, +50);
    delay_ms(2000);

    /* 역회전 2초 */
    DrvMotor_SetDuty(MOTOR_FL, -50);
    DrvMotor_SetDuty(MOTOR_FR, -50);
    DrvMotor_SetDuty(MOTOR_RL, -50);
    DrvMotor_SetDuty(MOTOR_RR, -50);
    delay_ms(2000);

    /* 제동 2초 */
    DrvMotor_Brake(MOTOR_FL);
    DrvMotor_Brake(MOTOR_FR);
    DrvMotor_Brake(MOTOR_RL);
    DrvMotor_Brake(MOTOR_RR);
    delay_ms(2000);

    /* 정지 */
    DrvMotor_Coast(MOTOR_FL);
    DrvMotor_Coast(MOTOR_FR);
    DrvMotor_Coast(MOTOR_RL);
    DrvMotor_Coast(MOTOR_RR);
}
