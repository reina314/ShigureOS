#include <kernel/timer.h>
#include <kernel/irq.h>  // for irq register
#include <kernel/proc.h> // for switch_proc

/// @brief How many times timer_handler is called since booted
unsigned int timer_ticks = 0;

/// @brief Time passed since booted in seconds
int seconds_passed = 0;

/// @brief The timer fires 18.222 times per second
/// @param r Not used
void timer_handler(struct regs *r)
{
    (void)r; // Avoid unused parameter warning
    timer_ticks++;

    // Switch proc
    switch_proc(r);

    if ((double)timer_ticks / 18.222 > seconds_passed)
        seconds_passed++;
}

/// @brief To be called in kernel main
/// @param
void timer_install(void)
{
    register_request_handler(0, timer_handler);
}

/// @brief Loop until given time passes; be ware that accepted only in ticks and not in seconds
/// @param ticks
void timer_wait(int ticks)
{
    unsigned long eticks;
    eticks = timer_ticks + ticks;
    while (timer_ticks < eticks)
        asm volatile("pause"); // Avoid CPU from spininng too fast
}