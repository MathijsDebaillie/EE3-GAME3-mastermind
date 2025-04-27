// BLE.c
#include "BLE.h"

char *TAG = "BLE-Server";
uint8_t ble_addr_type;
uint16_t characteristic_handle = 0;
uint16_t conn_handle; // Store connection handle globally

static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char * data = (char *)ctxt->om->om_data;
    printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    return 0;
}

static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(ctxt->om, "Data from the server", strlen("Data from the server"));
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180),
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          .access_cb = device_read,
          .val_handle = &characteristic_handle},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status == 0)
        {
            conn_handle = event->connect.conn_handle;
        }
        else
        {
            ble_app_advertise();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT DISCONNECTED");
        conn_handle = 0;
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

void notifyBLE(const char *message){
    if (conn_handle && characteristic_handle)
    {
        //const char *msg = "Hello from ESP32!";
        struct os_mbuf *om = ble_hs_mbuf_from_flat(message, strlen(message));
        
        if (om == NULL) {
            ESP_LOGE(TAG, "Failed to allocate mbuf");
            return;
        }
        
        int rc = ble_gatts_notify_custom(conn_handle, 
                                       characteristic_handle,
                                       om);
        
        if (rc == 0) {
            ESP_LOGI(TAG, "Notification sent successfully");
        } else {
            ESP_LOGE(TAG, "Notification failed: %d", rc);
            os_mbuf_free_chain(om);
        }
    }
    else {
        ESP_LOGW(TAG, "No BLE connection or characteristic handle is not ready.");
    }
}

void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    if (characteristic_handle == 0) {
        ESP_LOGE(TAG, "FATAL: Characteristic handle not set!");
        return;
    }
    
    ESP_LOGI(TAG, "Characteristic handle: %d", characteristic_handle);
    ble_app_advertise();
}

void host_task(void *param)
{
    nimble_port_run();
}

void BLE_initialize(void)
{
    nvs_flash_init();
    nimble_port_init();
    ble_svc_gap_device_name_set("BLE-server-IB3-game3");
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);
}