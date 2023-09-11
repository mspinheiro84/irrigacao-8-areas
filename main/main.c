#include <stdio.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    int cont = 0;
    printf("Hello Word!\n");

    while (1)
    {
        if (cont++ >= 99) cont = 0;
        printf("%02d\n", cont);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}