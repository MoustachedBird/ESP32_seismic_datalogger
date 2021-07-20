#include "driver/periph_ctrl.h"
#include "driver/timer.h"

/*custom headers*/
#include "timer_conf.h"
#include "task_list.h"


/*============================================================================================
 * Timer ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 ============================================================================================*/
void IRAM_ATTR timer_isr_handler(){
    /*Start the interrupt and lock timer_group 0 (i guess)*/
    timer_spinlock_take(TIMER_GROUP_0);

    /* Clear the interrupt (with reload)*/
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    
   /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_SENSOR);
    
    /* Notify get_data_from_sensor_task that the buffer is full which means that get_data_from_sensor_task gets activated every
    time the isr vector is called
    */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    //Turn on the 24 bit ADC task
    vTaskNotifyGiveFromISR(get_data_adc_mcp3561_taskID, &xHigherPriorityTaskWoken);
    
    //Turn on the 20 bit accelerometer task
    vTaskNotifyGiveFromISR(get_data_adxl355_taskID, &xHigherPriorityTaskWoken);

    //Turn on the 14 bit accelerometer task
    vTaskNotifyGiveFromISR(get_data_mma8451q_taskID, &xHigherPriorityTaskWoken);
    

    if (xHigherPriorityTaskWoken) {
      portYIELD_FROM_ISR();
    }

    /*Finish the interrupt and unlock timer_group 0 (i guess)*/
    timer_spinlock_give(TIMER_GROUP_0);
}





/*============================================================================
 * Initialize selected timer of the timer group 0
 *
 * TIMER_SENSOR - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * SAMPLE_RATE - the interval of alarm clock in HZ (period = 1/SAMPLE_RATE)
 ============================================================================*/
void conf_timer(){
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TEST_WITH_RELOAD,
    }; // default clock source is APB
    timer_init(TIMER_GROUP_0, TIMER_SENSOR, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, TIMER_SENSOR, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_SENSOR, TIMER_SCALE/SAMPLE_RATE);
    timer_enable_intr(TIMER_GROUP_0, TIMER_SENSOR);
    //Setting timer interrupt ESP_INTR_FLAG_IRAM = priority of timer interrupt
    //ESP_INTR_FLAG_NMI = highest priority
    timer_isr_register(TIMER_GROUP_0, TIMER_SENSOR, timer_isr_handler, NULL, ESP_INTR_FLAG_LEVEL3, NULL);
    timer_start(TIMER_GROUP_0, TIMER_SENSOR);
}