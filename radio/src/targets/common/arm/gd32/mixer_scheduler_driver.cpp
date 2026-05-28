#include "mixer_scheduler.h"

#include "hal.h"

#if defined(MIXER_SCHEDULER_TIMER)

// TODO: Implement GD32 mixer scheduler using MIXER_SCHEDULER_TIMER,
//       similar to STM32 counterpart (stm32/mixer_scheduler_driver.cpp).
//       Requires: MIXER_SCHEDULER_TIMER, MIXER_SCHEDULER_TIMER_FREQ,
//                 MIXER_SCHEDULER_TIMER_IRQn, MIXER_SCHEDULER_TIMER_IRQHandler

#else // no MIXER_SCHEDULER_TIMER defined, stub functions

// TODO: Assign a real timer in hal.h and implement driver above

void mixerSchedulerStart()
{
}

void mixerSchedulerStop()
{
}

void mixerSchedulerEnableTrigger()
{
}

void mixerSchedulerDisableTrigger()
{
}

void mixerSchedulerSoftTrigger()
{
}

#endif
