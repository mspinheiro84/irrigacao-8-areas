#include <stdio.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/*include do ESP32*/
#include "driver/gpio.h"
#include "esp_log.h"

/*define dos pinos*/
#define LED 2 /*LED do kit*/
#define BUTTON 21 /*LED do kit*/

void vTaskLed (void *pvParameters);

static const char *TAG = "IRRIGACAO";

SemaphoreHandle_t xHandleSemaphore = NULL;

static void IRAM_ATTR gpio_isr_handle(void *arg){
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xHandleSemaphore, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

void configPinos(){
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
    gpio_config_button.intr_type = GPIO_INTR_POSEDGE;

    gpio_reset_pin(LED);
    gpio_reset_pin(BUTTON);

    gpio_config(&gpio_config_led);
    gpio_config(&gpio_config_button);
}

void app_main(void)
{
    xHandleSemaphore = xSemaphoreCreateCounting(5,0);
    if (xHandleSemaphore != pdFALSE){
        configPinos();
        
        TaskHandle_t xHandleLed;

        if (xTaskCreate(vTaskLed, "LED", configMINIMAL_STACK_SIZE, NULL, 1, &xHandleLed) != pdFAIL){
            gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
            gpio_isr_handler_add(BUTTON, gpio_isr_handle, NULL);

            while (1)
            {
                ESP_LOGI(TAG,"====> Aplicação principal <====\n\n");
                vTaskDelay(pdMS_TO_TICKS(7500));
            }    
        } else {
            ESP_LOGE(TAG, "Erro ao criar a task LED");
        }
    } else {
        ESP_LOGE(TAG, "Erro ao criar o Semaphore - memória heap insuficiente");
    }
}

void vTaskLed (void *pvParameters){

    while (1)
    {
        xSemaphoreTake(xHandleSemaphore, portMAX_DELAY);
        gpio_set_level(LED, pdTRUE);
        ESP_LOGI(TAG,"LED acesso");
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(LED, pdFALSE);
        ESP_LOGI(TAG,"LED apagado\n");
        vTaskDelay(pdMS_TO_TICKS(250));
    }    
}