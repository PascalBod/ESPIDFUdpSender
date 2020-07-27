/**
 * This file is part of UdpSender.
 *
 * UdpSender is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UdpSender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UdpSender.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Pascal Bodin
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "connect_wifi.h"
#include "send_datagram.h"
#include "supervisor.h"

#define LOOP_PERIOD_MS 180000

static const char *VERSION = "0.7.0";

static const char *TAG = "US";

void app_main(void)
{

	ESP_LOGI(TAG, "Version: %s", VERSION);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP stack.
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop.
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Task depths have been chosen after the use of uxTaskGetStackHighWaterMark(),
    // with some margin.
    xTaskCreate(supervisor_task, "supervisor", 2000, NULL, 5, NULL);
    xTaskCreate(connect_wifi_task, "connect_wifi", 3000, NULL, 5, NULL);
    xTaskCreate(send_datagram_task, "send_datagram", 2000, NULL, 5, NULL);

    // Do not exit from app_main().
    while (true) {
    	vTaskDelay(pdMS_TO_TICKS(LOOP_PERIOD_MS));
    	ESP_LOGI(TAG, "End of wait");
    }
}
