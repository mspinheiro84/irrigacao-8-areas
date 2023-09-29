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
#include "esp_netif_sntp.h"
#include "esp_sleep.h"
#include "mqtt_client.h"

#include "esp_http_client.h"

#include "wifi.h"
#include "http_client.h"
#include "mqtt.h"
#include "sntp.h"

/*define*/
#define LED 2 //LED do kit
#define BOMBA 19 //Pino da bomba
#define BUTTON 21 //Pino do sensor

/*topicos do mqtt*/
#define TOPIC_RESUMO "resumo"
#define TOPIC_SENSOR "sensor/vazao"
#define TOPIC_ATUADORES "atuadores"
#define TOPIC_ATUADOR_BOMBA "atuador/bomba"
#define TOPIC_ATUADOR_SOLENOIDES "atuador/solenoides"

typedef struct {
    int ano;
    int mes;
    int dia;
    int hora;
    int minuto;
    int segundo;
    int fuso;

} DataHora;

typedef struct{
    uint8_t sensor_vazao;
    char atuador_bomba;
    char atuador_solenoides[8];
}Dados;

static Dados estado, dados = {
    .sensor_vazao = 0,
    .atuador_bomba = '0',
    .atuador_solenoides = {'0','0','0','0','0','0','0','0'},
};

static const char *TAG = "IRRIGACAO";

static struct tm timeinfo;

uint8_t cont = 0;

TaskHandle_t xHandleAcionamento;
TaskHandle_t xHandleContador;
TaskHandle_t xHandleAtualizaSensor;
TaskHandle_t xHandleSntp;
TaskHandle_t xHandleHttpRequest;
TaskHandle_t xHandleMqttPublish;

SemaphoreHandle_t xHandleSemaphore = NULL;
SemaphoreHandle_t xHandleSemaphoreAcionamento = NULL;

void vTaskAcionamento (void *pvParameters);
void vTaskContador (void *pvParameters);
void vTaskAtualizaSensor (void *pvParameters);
void vTaskSntp (void *pvParameters);
void vTaskHttpRequest (void *pvParameters);
void vTaskMqttPublish (void *pvParameters);

static void IRAM_ATTR gpio_isr_handle(void *arg){
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xHandleSemaphore, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

void ligaAgua(uint8_t bomba){
    gpio_set_level(LED, pdTRUE);
    ESP_LOGI(TAG,"Registro ligado");
    gpio_set_level(BOMBA, bomba);
    ESP_LOGI(TAG,"Bomba %d", bomba);
    vTaskResume(xHandleAtualizaSensor);
    gpio_intr_enable(BUTTON);
}

void desligaAgua(){
    gpio_set_level(BOMBA, pdFALSE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(LED, pdFALSE);
    gpio_intr_disable(BUTTON);
    vTaskSuspend(xHandleAtualizaSensor);
    ESP_LOGI(TAG,"Registro desligado");
}

void acionamentos(){
    if (estado.atuador_solenoides[0] == '1'){
        if (estado.atuador_bomba == '1'){
            ligaAgua(1);
        } else {
            ligaAgua(0);
        }
    } else {
        desligaAgua();
    }

}

void mqtt_app_data(void *pvParameters){
    esp_mqtt_event_handle_t data = pvParameters;
    // printf("TOPIC=%.*s\n\n", data->topic_len, data->topic);
    // printf("DATA=%.*s\n\n", data->data_len, data->data);
    char topic[20];
    sprintf(topic, "%.*s", data->topic_len, data->topic);
    if(strcmp(topic, TOPIC_RESUMO) == 0) {
        /*
        modelo do JSON para o tópico resumo
        {"atuadores":{"bomba":"0","solenoides":"00000000"},"sensor":{"vazao":"000"}}
        */
        //sensor
        // char sensor_vazao[5];
        // sprintf(sensor_vazao, "%.*s", 3, data->data+70);//posição 70 dados do sensor
        // printf("\nDATA=%.*s\n", data->data_len, data->data);
        // printf("Sensor recebido: %s\n", sensor_vazao);
        // printf("Sensor: %.3d\n", dados.sensor_vazao);
        //atuadores
        // dados.atuador_bomba = data->data[23]; //posição 23 dados da bomba
        // for (int i = 0, j = 40; i < 8; i++, j++){ //posição 40 dados dos solenoides
        //     dados.atuador_solenoides[i] = data->data[j];
        // }
        // printf("Bomba - %c\n", dados.atuador_bomba);
        // printf("Solenoides - %s\n\n", dados.atuador_solenoides);
        
    } else if (strcmp(topic, TOPIC_ATUADORES) == 0) {
        /*
        modelo do JSON para o tópico atuadores
        {"bomba":"0","solenoides":"00000000"}
        */
        dados.atuador_bomba = data->data[10]; //posição 10 dados da bomba
        for (int i = 0, j = 27; i < 8; i++, j++){ //posição 27 dados dos solenoides
            dados.atuador_solenoides[i] = data->data[j];
        }
        // printf("\nDATA=%.*s\n", data->data_len, data->data);
        // printf("Bomba - %c\n", dados.atuador_bomba);
        // printf("Solenoides - %s\n\n", dados.atuador_solenoides); 
    } else if (strcmp(topic, TOPIC_ATUADOR_SOLENOIDES) == 0) { //Solenoides
        sprintf(dados.atuador_solenoides, "%.*s", data->data_len-2, data->data+1);
        // printf("Solenoides - %s\n\n", dados.atuador_solenoides);
    } else if (strcmp(topic, TOPIC_ATUADOR_BOMBA) == 0) { //Bomba
        dados.atuador_bomba = data->data[0];
        // printf("Bomba - %c\n\n", dados.atuador_bomba);
        
    }
    xSemaphoreGive(xHandleSemaphoreAcionamento);
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
    gpio_config_led.pin_bit_mask = 1ULL<<LED | 1ULL<<BOMBA;
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
    gpio_reset_pin(BOMBA);
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

    xHandleSemaphore = xSemaphoreCreateCounting(10,0);
    xHandleSemaphoreAcionamento = xSemaphoreCreateBinary();
    if (xHandleSemaphore != pdFALSE){
        configPinos();

        if ((xTaskCreate(vTaskAcionamento, "Acionamento", configMINIMAL_STACK_SIZE+1024, NULL, 1, &xHandleAcionamento) != pdFAIL) &&
            (xTaskCreate(vTaskContador, "CONTADOR", configMINIMAL_STACK_SIZE+1024, NULL, 1, &xHandleContador) != pdFAIL) &&
            (xTaskCreate(vTaskAtualizaSensor, "ATUALIZA-SENSOR", configMINIMAL_STACK_SIZE+1024, NULL, 1, &xHandleAtualizaSensor) != pdFAIL) &&
            (xTaskCreate(vTaskSntp, "SNTP", configMINIMAL_STACK_SIZE+1024, NULL, 1, &xHandleSntp) != pdFAIL) &&
            (xTaskCreate(vTaskHttpRequest, "HTTP-REQUEST", configMINIMAL_STACK_SIZE+1024, NULL, 3, &xHandleHttpRequest) != pdFAIL) &&
            (xTaskCreate(vTaskMqttPublish, "MQTT-PUBLISH", configMINIMAL_STACK_SIZE+1024, NULL, 3, &xHandleMqttPublish) != pdFAIL)){

            gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
            gpio_isr_handler_add(BUTTON, gpio_isr_handle, NULL);

            vTaskSuspend(xHandleAtualizaSensor);
            gpio_intr_disable(BUTTON);

            estado = dados;

            while (1)
            {
                ESP_LOGI(TAG,"====> Aplicação principal <====\n\n");
                printf("{\"atuadores\":{\"bomba\":\"%c\",\"solenoides\":\"%.*s\"},\"sensor\":{\"vazao\":\"%.3d\"}}\n\n", estado.atuador_bomba, 8, estado.atuador_solenoides, estado.sensor_vazao);
                vTaskDelay(pdMS_TO_TICKS(15000));
            }
            
        } else {
            ESP_LOGE(TAG, "Erro ao criar task");
        }
    } else {
        ESP_LOGE(TAG, "Erro ao criar o Semaphore - memória heap insuficiente");
    }
}


void vTaskMqttPublish (void *pvParameters){    
    mqtt_app_start();
    vTaskDelay(pdMS_TO_TICKS(2000));
    int flagDisc = 1;
    char payload[78];
    while (1)
    {
        if (reconectaWifi()==ESP_OK){
            if(flagDisc) {
                mqtt_app_reconnect();
                vTaskDelay(pdMS_TO_TICKS(2000));
                mqtt_app_subscribe(TOPIC_RESUMO, 2);
                mqtt_app_subscribe(TOPIC_ATUADORES, 2);
                mqtt_app_subscribe(TOPIC_ATUADOR_BOMBA, 2);
                mqtt_app_subscribe(TOPIC_ATUADOR_SOLENOIDES, 2);
                flagDisc = 0;
            }
            /*
            Resumo: modelo do JSON para o tópico atuadores
            {"atuadores":{"bomba":"0","solenoides":"00000000"},"sensor":{"vazao":"000"}}
            */
            sprintf(payload, "{\"atuadores\":{\"bomba\":\"%c\",\"solenoides\":\"%.*s\"},\"sensor\":{\"vazao\":\"%.3d\"}}", estado.atuador_bomba, 8, estado.atuador_solenoides, estado.sensor_vazao);
            mqtt_app_publish(TOPIC_RESUMO, payload);
            vTaskDelay(pdMS_TO_TICKS(2000));
            // //sensor
            // sprintf(payload, "%d", estado.sensor_vazao);
            // mqtt_app_publish(TOPIC_SENSOR, payload);
            // vTaskDelay(pdMS_TO_TICKS(2000));
            // /*
            // Atuadores: modelo do JSON para o tópico atuadores
            // {"bomba":"0","solenoides":"00000000"}
            // */
            // sprintf(payload, "{\"bomba\":\"%c\",\"solenoides\":\"%.*s\"}", estado.atuador_bomba, 8, estado.atuador_solenoides);
            // mqtt_app_publish(TOPIC_ATUADORES, payload);
        } else {
            flagDisc = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
    
}


void vTaskHttpRequest (void *pvParameters){
    char *dado;
    static DataHora horario;
    while (1)
    {        
        if (reconectaWifi()==ESP_OK){
            dado = http_client_request("http://worldtimeapi.org/api/timezone/America/Recife");
            if ((strlen(dado) > 340) && (strlen(dado) < 360)){
                //printf("\n\nAPI World Time:\n%s\n", dado);
                carregaHorario(&horario, dado);
                // printf("\nExtração da data e hora:\nData: %d-%d-%d\nHora: %d:%d:%d\nFuso: %d\n\n", horario.dia, horario.mes, horario.ano, horario.hora, horario.minuto, horario.segundo, horario.fuso);
                free(dado);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
    
}

void vTaskSntp (void *pvParameters){
    static time_t now;
    char data[15], hora[9];
    while (1)
    {
        time(&now);
        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2022 - 1900)) {
            ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
            obtain_time();
            // update 'now' variable with current time
            time(&now);
        }
        //Set timezone to Brazil Standard Time
        setenv("TZ", "UTC+3", 1);
        tzset();
        localtime_r(&now, &timeinfo);        
        strftime(data, sizeof(data), "%a %d/%m/%Y", &timeinfo);
        strftime(hora, sizeof(hora), "%X", &timeinfo);
        printf("\nData: %s\n", data);
        printf("\nHorário no Brasil: %s\n\n", hora);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    
}

void vTaskContador (void *pvParameters){
    while (1)
    {
        xSemaphoreTake(xHandleSemaphore, portMAX_DELAY);
        cont++;
        //ESP_LOGI(TAG, "Contador: %d\n", cont);
    }
    
}

void vTaskAtualizaSensor (void *pvParameters){
    while (1)
    {
        estado.sensor_vazao = cont;
        cont = 0;
        ESP_LOGI(TAG, "Sensor atualizado %d", estado.sensor_vazao);
        vTaskDelay(pdMS_TO_TICKS(15000)); //zera a cada 60 segundos

    }
    
}

void vTaskAcionamento (void *pvParameters){
    uint8_t contAgua = 0;

    while (1)
    {
        xSemaphoreTake(xHandleSemaphoreAcionamento, portMAX_DELAY);
        if (dados.atuador_solenoides[0] == '1'){
            estado.atuador_solenoides[0] = '1';
            estado.atuador_bomba = dados.atuador_bomba;
        } else {
            estado.atuador_solenoides[0] = '0';
            estado.atuador_bomba = '0';
            dados.atuador_bomba = '0';
        }
        acionamentos();
        // vTaskDelay(pdMS_TO_TICKS(15000));
    }    
}