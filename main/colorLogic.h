#ifndef COLORLOGIC_H
#define COLORLOGIC_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h" 
#include "esp_random.h"
#include <time.h>
#include <math.h>


#define RED1_PIN 9
#define GREEN1_PIN 10
#define BLUE1_PIN 11

#define RED2_PIN 12
#define GREEN2_PIN 13
#define BLUE2_PIN 14 

#define RED3_PIN 3
#define GREEN3_PIN 1
#define BLUE3_PIN 2

#define RED4_PIN 35 
#define GREEN4_PIN 36
#define BLUE4_PIN 37

// #define BUTTON_PIN 47

#define REDBICOLORED1_PIN 4
#define GREENBICOLORED1_PIN 5

#define REDBICOLORED2_PIN 6
#define GREENBICOLORED2_PIN 7 

#define REDBICOLORED3_PIN 15
#define GREENBICOLORED3_PIN 16

#define REDBICOLORED4_PIN 17
#define GREENBICOLORED4_PIN 18 

typedef struct {
    float voltage;
    int colorIndex;
} VoltageColorMap;


extern const uint8_t colors[6][3];
extern const int pinGroupBiLeds[4][2];
extern const VoltageColorMap voltageMap[];
extern const int pinGroups[4][3];


int getColorFromVoltage(float voltage);
void logReceivedColors(uint8_t buf[]);
void processReceivedColors(float receivedVoltages[4]); 
void configure_pins(); 
void set_color(int redPin, int greenPin, int bluePin, uint8_t red, uint8_t green, uint8_t blue);
void generate4Colors();
void gameWon(); 
void gameOVer(); 

#endif // COLORLOGIC_H