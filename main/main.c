#include <stdio.h>
#include <string.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

/*include do ESP32*/
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_tls.h"

#include "esp_http_client.h"

#include "wifi.h"
#include "http_client.h"

/*define*/
#define LED 2 /*LED do kit*/
#define BUTTON 21 /*LED do kit*/

TaskHandle_t xHandleLed;
void vTaskContador (void *pvParameters);
void vTaskLed (void *pvParameters);
void vTaskHttpRequest (void *pvParameters);

static const char *TAG = "IRRIGACAO";

SemaphoreHandle_t xHandleSemaphore = NULL;

typedef struct {
    int ano;
    int mes;
    int dia;
    int hora;
    int minuto;
    int segundo;
    int fuso;

} DataHora;

static void IRAM_ATTR gpio_isr_handle(void *arg){
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xHandleSemaphore, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

void carregaHorario(DataHora *horario, char *dado){
    char *aux, *temp;
    aux = strstr(dado, "\"datetime")+12;
    temp = (char *) calloc(4, sizeof(char));
    memcpy(temp, aux, 4);
    horario->ano = atoi(temp);
    temp = (char *) calloc(3, sizeof(char));
    memcpy(temp, aux+26, 3);
    horario->fuso = atoi(temp);
    temp = (char *) calloc(2, sizeof(char));
    memcpy(temp, aux+17, 2);
    horario->segundo = atoi(temp);
    memcpy(temp, aux+14, 2);
    horario->minuto = atoi(temp);
    memcpy(temp, aux+11, 2);
    horario->hora = atoi(temp);
    memcpy(temp, aux+8, 2);
    horario->dia = atoi(temp);
    memcpy(temp, aux+5, 2);
    horario->mes = atoi(temp);
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
    /*Conexão com WIFI*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    vTaskDelay(pdMS_TO_TICKS(5000));

    xHandleSemaphore = xSemaphoreCreateCounting(5,0);
    if (xHandleSemaphore != pdFALSE){
        configPinos();
        
        TaskHandle_t xHandleContador;
        TaskHandle_t xHandleHttpRequest;

        if ((xTaskCreate(vTaskLed, "LED", configMINIMAL_STACK_SIZE+1024, NULL, 1, &xHandleLed) != pdFAIL) &&
            (xTaskCreate(vTaskContador, "CONTADOR", configMINIMAL_STACK_SIZE+1024, NULL, 1, &xHandleContador) != pdFAIL) &&
            (xTaskCreate(vTaskHttpRequest, "HTTP-REQUEST", configMINIMAL_STACK_SIZE+1024, NULL, 3, &xHandleHttpRequest) != pdFAIL)){

            gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
            gpio_isr_handler_add(BUTTON, gpio_isr_handle, NULL);

            while (1)
            {
                ESP_LOGI(TAG,"====> Aplicação principal <====\n\n");
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
            
            //example_disconnect();

        } else {
            ESP_LOGE(TAG, "Erro ao criar task");
        }
    } else {
        ESP_LOGE(TAG, "Erro ao criar o Semaphore - memória heap insuficiente");
    }
}


void vTaskHttpRequest (void *pvParameters){
    char *dado;
    static DataHora horario;
    while (1)
    {        
        dado = http_client_request("http://worldtimeapi.org/api/timezone/America/Recife");
        //printf("\n\nAPI World Time:\n%s\n", dado);
        carregaHorario(&horario, dado);
        printf("\nExtração da data e hora:\nData: %d-%d-%d\nHora: %d:%d:%d\nFuso: %d\n\n", horario.dia, horario.mes, horario.ano, horario.hora, horario.minuto, horario.segundo, horario.fuso);
        free(dado);
        vTaskDelay(pdMS_TO_TICKS(8000));
    }
    
}

void vTaskContador (void *pvParameters){
    uint8_t cont = 0;
    while (1)
    {
        xSemaphoreTake(xHandleSemaphore, portMAX_DELAY);
        if (++cont == 5){
            vTaskSuspend(xHandleLed);
        }
        if (cont == 8){
            vTaskResume(xHandleLed);
            cont = 0;
        }
        ESP_LOGI(TAG, "Contador: %d\n", cont);
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
        vTaskDelay(pdMS_TO_TICKS(500));
    }    
}