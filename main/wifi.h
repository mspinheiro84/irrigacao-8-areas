#ifndef WIFI_H
#define WIFI_H

void wifi_init_softap(void);
void wifi_init_sta(void);
esp_err_t reconectaWifi(void);

#endif //WIFI_H