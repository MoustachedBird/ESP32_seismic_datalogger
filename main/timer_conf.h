
/* TIMER_BASE_CLK = 80 MHZ */ 
#define SAMPLE_RATE  100 //sample rate in HZ

//0.0025 = 400 HZ      0.004 = 250 HZ   <--- maximum recommended
#define TIMER_SENSOR TIMER_0 //Timer for counting 
#define TIMER_DIVIDER         800  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

void conf_timer();


/*
EDITED by MoustachedBird


***** IMPORTANT: Task notify is better than a semaphore?*****
Each RTOS task has a 32-bit notification value. An RTOS task notification is an event sent 
directly to a task that can unblock the receiving task, and optionally update the receiving 
task’s notification value.

Task notifications can update the receiving task’s notification value in the following ways:

    Set the receiving task’s notification value without overwriting a previous value
    Overwrite the receiving task’s notification value
    Set one or more bits in the receiving task’s notification value
    Increment the receiving task’s notification value

That flexibility allows task notifications to be used where previously it 
would have been necessary to create a separate queue, binary semaphore, counting semaphore or 
event group. Unblocking an RTOS task with a direct notification is 45% faster * and uses less 
RAM than unblocking a task with a binary semaphore.

More information about task notifications: https://www.freertos.org/RTOS-task-notifications.html

The ESP32 has two timer groups, each one with two general purpose hardware timers. 
All the timers are based on 64 bits counters and 16 bit prescalers.


**HARDWARE TIMER INFORMATION**

The prescaler is used to divide the frequency of the base signal (usually 80 MHz = TIMER_BASE_CLK), 
which is then used to increment / decrement the timer counter. Since the prescaler has 16 bits, 
it can divide the clock signal frequency by a factor from 2 to 65536, giving a lot 
of configuration freedom.

The timer counters can be configured to count up or down and support automatic reload 
and software reload. They can also generate alarms when they reach a specific value, 
defined by the software. The value of the counter can be read by the software program.

For more information about hardware timers check: 
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/timer.html?highlight=hw_timer

For sotfware timers (not this example) check: 
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html
*/
