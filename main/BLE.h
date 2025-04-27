// BLE.h
#ifndef BLE_H
#define BLE_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "sdkconfig.h"
#include "esp_mac.h"

// Global variables
extern char *TAG;
extern uint8_t ble_addr_type;
extern uint16_t characteristic_handle;
extern uint16_t conn_handle;

// Function declarations
void ble_app_advertise(void);
void ble_app_on_sync(void);
void host_task(void *param);
void notifyBLE(const char *message);
void BLE_initialize(void);

#endif // BLE_H