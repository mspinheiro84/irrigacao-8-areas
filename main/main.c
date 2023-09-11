#include <stdio.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*include do ESP32*/
#include "driver/gpio.h"
#include "esp_log.h"

/*define dos pinos*/
#define LED 2 /*LED do kit*/
#define BUTTON 21 /*LED do kit*/

void vTaskLed (void *pvParameters);
void vTaskButton (void *pvParameters);

static const char *TAG = "IRRIGACAO";
int flagBotao = 1;

void app_main(void)
{
    gpio_config_t gpio_config_led;
    gpio_config_led.pin_bit_mask = 1ULL<<LED;
    gpio_config_led.mode = GPIO_MODE_OUTPUT;
    gpio_config_led.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config_led.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config_led.intr_type = GPIO_INTR_DISABLE;

    gpio_config_t gpio_config_button;
    gpio_config_button.pin_bit_mask = 1ULL<<BUTTON;
    gpio_config_button.mode = GPIO_MODE_INPUT;
    gpio_config_button.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config_button.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config_button.intr_type = GPIO_INTR_DISABLE;

    gpio_reset_pin(LED);
    gpio_reset_pin(BUTTON);

    gpio_config(&gpio_config_led);
    gpio_config(&gpio_config_button);
    
    TaskHandle_t xHandleLed;
    if (xTaskCreate(vTaskLed, "LED", configMINIMAL_STACK_SIZE, NULL, 1, &xHandleLed) == pdFAIL){
        ESP_LOGE(TAG, "Erro ao criar a task LED");
    }

    TaskHandle_t xHandleButton;
    if (xTaskCreate(vTaskButton, "BUTTON", configMINIMAL_STACK_SIZE, NULL, 1, &xHandleButton) == pdFAIL){
        ESP_LOGE(TAG, "Erro ao criar a task BUTTON");
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
        if(flagBotao == 0){
            gpio_set_level(LED, pdTRUE);
            ESP_LOGI(TAG,"LED acesso");
            vTaskDelay(pdMS_TO_TICKS(250));
            gpio_set_level(LED, pdFALSE);
            ESP_LOGI(TAG,"LED apagado\n");
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }    
}

void vTaskButton (void *pvParameters){
    
    while (1)
    {
        flagBotao = gpio_get_level(BUTTON);
        vTaskDelay(pdMS_TO_TICKS(250));
    }    
}