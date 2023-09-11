#include <stdio.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*include do ESP32*/
#include "driver/gpio.h"
#include "esp_log.h"

/*define dos pinos*/
#define LED 2 /*LED do kit*/

void vTaskLed (void *pvParameters);

static const char *TAG = "IRRIGACAO";

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

    
    TaskHandle_t xHandleLed;
    if (xTaskCreate(vTaskLed, "LED", configMINIMAL_STACK_SIZE, NULL, 1, &xHandleLed) == pdFAIL){
        ESP_LOGE(TAG, "Erro ao criar a task LED");
    }

    while (1)
    {
        ESP_LOGI(TAG,"====> Aplicação principal <====\n\n");
        vTaskDelay(pdMS_TO_TICKS(7500));
    }
    
}

void vTaskLed (void *pvParameters){

    while (1)
    {
        gpio_set_level(LED, pdTRUE);
        ESP_LOGI(TAG,"LED acesso");
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED, pdFALSE);
        ESP_LOGI(TAG,"LED apagado\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }    
}