/*	Mirf Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"

#include "mirf.h"
#include "colorLogic.h"
#include "servo.h"
#include "BLE.h"

#define SERVO_GPIO 38
#define RESET_BUTTON_GPIO 39

//reciever mode use: 
//#define CONFIG_RECEIVER 1
//#define CONFIG_SENDER 0

// sender mode use: 
#define CONFIG_SENDER 0
#define CONFIG_RECEIVER 1
#define CONFIG_RADIO_CHANNEL 98 // this needs to be 98 for GAME3
#define CONFIG_ADVANCED 1
#define CONFIG_RF_RATIO_1M 1
#define CONFIG_RETRANSMIT_DELAY 100

int gameState = 0; // gameState = 0 not playing, = 1 is playing
volatile bool buttonPress = false; // buttonPress (can be pressed and unpressed)
bool resetState = false; // this is to know the state
NRF24_t dev;

//function declarations
void startGame(void);
void sendNRF(const uint8_t* msg);
void initializeEverything(void);
void AdvancedSettings(NRF24_t *dev);
void receiveNRF(void *pvParameter);
void reset_handler_task(void *arg);

#if CONFIG_ADVANCED
void AdvancedSettings(NRF24_t * dev)
{
#if CONFIG_RF_RATIO_2M
	ESP_LOGW("MAIN", "Set RF Data Ratio to 2MBps");
	Nrf24_SetSpeedDataRates(dev, 1);
#endif // CONFIG_RF_RATIO_2M

#if CONFIG_RF_RATIO_1M
	ESP_LOGW("MAIN", "Set RF Data Ratio to 1MBps");
	Nrf24_SetSpeedDataRates(dev, 0);
#endif // CONFIG_RF_RATIO_2M

#if CONFIG_RF_RATIO_250K
	ESP_LOGW("MAIN", "Set RF Data Ratio to 250KBps");
	Nrf24_SetSpeedDataRates(dev, 2);
#endif // CONFIG_RF_RATIO_2M

	ESP_LOGW("MAIN", "CONFIG_RETRANSMIT_DELAY=%d", CONFIG_RETRANSMIT_DELAY);
	Nrf24_setRetransmitDelay(dev, CONFIG_RETRANSMIT_DELAY);
}
#endif // CONFIG_ADVANCED

static void IRAM_ATTR button_isr_handler(void* arg) {
    buttonPress = true;
}

void receiveNRF(void *pvParameter)
{
	ESP_LOGI("RECEIVER", "Started");

	// Set my own address using 5 characters
	esp_err_t ret = Nrf24_setRADDR(&dev, (uint8_t *)"FGHIJ");
	if (ret != ESP_OK) {
		ESP_LOGE("RECEIVER", "nrf24l01 not installed");
		while(1) { vTaskDelay(1); }
	}
	//Nrf24_printDetails(&dev);

	ESP_LOGI("RECEIVER", "Listening...");
	uint8_t buf[32];


	// Clear RX FiFo
	//while(1) {
	//	if (Nrf24_dataReady(&dev) == false) break;
	//	Nrf24_getData(&dev, buf);	
	//}

	int receiveCounter = 0; // test
	while(1) {
		// Wait for received data
		if (Nrf24_dataReady(&dev)) {
			Nrf24_getData(&dev, buf);
			//ESP_LOGI(pcTaskGetName(NULL), "Got data:%s", buf);  // commented out bcs checkColor already prints received voltages
			//ESP_LOG_BUFFER_HEXDUMP(pcTaskGetName(NULL), buf, payload, ESP_LOG_INFO);
			// here it should convert the received data (voltages) into colors and then check
			receiveCounter++;
			ESP_LOGI("RECEIVER", "\n New data received, counter = %d \n", receiveCounter);
			logReceivedColors(buf); 
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	}
}



void sendNRF(const uint8_t* msg){
	Nrf24_send(&dev, msg);
	ESP_LOGI("SENDER", "Sending: %s", msg);
	if (Nrf24_isSend(&dev, 1000)) {
		ESP_LOGI("SENDER", "Send success");
	} else {
		ESP_LOGW("SENDER", "Send failed");
	}  
}

void reset_handler_task(void *arg) {
    TickType_t lastDebounceTime = 0;
    const TickType_t debounceDelay = pdMS_TO_TICKS(500); // 500ms debounce

    while (1) {
        if (buttonPress) {
            buttonPress = false;

            TickType_t currentTime = xTaskGetTickCount();
            if ((currentTime - lastDebounceTime) > debounceDelay) {
                lastDebounceTime = currentTime;

                ESP_LOGW("RESET", "Soft reset triggered!");

                if (!resetState) {
                    resetState = true;
                    ESP_LOGW("RESET", "Entered reset mode. Remove pins and select difficulty.");
                    gameState = 0; 
                    open_cover();
                } else {
                    resetState = false;
                    ESP_LOGW("RESET", "Starting game again.");
                    gameState = 1; 
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}



void startGame(){
	close_cover(); 
	vTaskDelay(pdMS_TO_TICKS(2000)); // wait 2 seconds for the cover to close again
	//char *msg = "START"; 
	//notifyBLE(msg);
	//sendNRF((uint8_t*) msg); 
	gameState = 1; 
	generate4Colors(); //generates 4 new colors 
}

void initializeEverything(){
	// initialize the servo code
	setup_pwm(SERVO_GPIO);

	//initialize reset button
	gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_NEGEDGE,    // falling edge
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,  // optional, depends on your button wiring
		.pin_bit_mask = (1ULL << RESET_BUTTON_GPIO)
	};
	gpio_config(&io_conf);
	//gpio_install_isr_service(0);
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	gpio_isr_handler_add(RESET_BUTTON_GPIO, button_isr_handler, NULL);

	//initialize the colorLogic Code
	configure_pins(); // configure pins colorLogic code

	//initialize BLE server
	BLE_initialize(); 

	// initialize nrf
	Nrf24_init(&dev);
	uint8_t payload = 32;
	uint8_t channel = CONFIG_RADIO_CHANNEL;
	Nrf24_config(&dev, channel, payload);
	AdvancedSettings(&dev);

	// Set destination address using 5 characters (sending part)
	//esp_err_t ret = Nrf24_setTADDR(&dev, (uint8_t *)"FGHIJ");
	//if (ret != ESP_OK) {
	//	ESP_LOGE("SENDER", "nrf24l01 not installed");
	//	while(1) { vTaskDelay(1); }
	//}

	esp_err_t ret = Nrf24_setTADDR(&dev, (uint8_t *)"FGHIJ");
	if (ret != ESP_OK) {
		ESP_LOGE("SENDER", "nrf24l01 not installed");
		// should we try again here or? 
		// maybe try again after 5 seconds
	}
	
}

void app_main(void)
{	
	
	initializeEverything();
	startGame(); 
	
	// added some things to fix the printing
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    fflush(stdout); // Force log output
    vTaskDelay(pdMS_TO_TICKS(1000)); // Give time for log to appear

	// create 2 threads (tasks)
	//xTaskCreate(&sender, "SenderTask", 4096, NULL, 5, NULL);  // we use the send as a method now
	xTaskCreate(&receiveNRF, "ReceiverTask", 4096, NULL, 5, NULL);
	xTaskCreate(&reset_handler_task, "ResetHandler", 4096, NULL, 2, NULL);
}

