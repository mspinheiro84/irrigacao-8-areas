#include <stdio.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*include do ESP32*/
#include "driver/gpio.h"

/*define dos pinos*/
#define LED 2 /*LED do kit*/

void app_main(void)
{
    gpio_config_t gpio_config_led;
    gpio_config_led.pin_bit_mask = 1ULL<<LED;
    gpio_config_led.mode = GPIO_MODE_OUTPUT;
    gpio_config_led.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config_led.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config_led.intr_type = GPIO_INTR_DISABLE;

    gpio_reset_pin(LED);
    gpio_config(&gpio_config_led);

    printf("Hello Word!\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        gpio_set_level(LED, pdTRUE);
        printf("LED acesso\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED, pdFALSE);
        printf("LED apagado\n\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}