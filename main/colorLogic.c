#include "colorLogic.h"
#include "servo.h"
#include "BLE.h"
#include "main.c"


int generatedColors[4] = {-1, -1, -1, -1}; // tracks colors for each position
int tries = 0; // counter for the tries (max is 8 tries, so if its seven)
// if we do it with difficulties then tis number will change according to the difficulty

const uint8_t colors[6][3] = {
    {1, 0, 0}, // Red
    {0, 1, 0}, // Blue (sometimes switched with purple?)
    {0, 0, 1}, // Green
    {1, 1, 0}, // Purple (Red + Blue) 
    {1, 0, 1}, // Yellow (Red + Green) 
    {1, 1, 1}  // White 
};

const int pinGroupBiLeds[4][2] = {
    {REDBICOLORED1_PIN, GREENBICOLORED1_PIN},
    {REDBICOLORED2_PIN, GREENBICOLORED2_PIN},
    {REDBICOLORED3_PIN, GREENBICOLORED3_PIN},
    {REDBICOLORED4_PIN, GREENBICOLORED4_PIN}
};

const VoltageColorMap voltageMap[] = {
    {0.65, 0}, // Red
    {1.24, 1}, // Purple 
    {1.79, 2}, // Green
    {2.5, 3},  // Blue
    {3.21, 4}, // Yellow
    {4.36, 5}  // White
};  

const int pinGroups[4][3] = {
    {RED1_PIN, GREEN1_PIN, BLUE1_PIN},
    {RED2_PIN, GREEN2_PIN, BLUE2_PIN},
    {RED3_PIN, GREEN3_PIN, BLUE3_PIN},
    {RED4_PIN, GREEN4_PIN, BLUE4_PIN}
};

int getColorFromVoltage(float voltage) {
    for (int i = 0; i < 6; i++) {
        if (fabs(voltage - voltageMap[i].voltage) < 0.1) { // Allow small error margin
            //printf("Voltage: %.2f -> Color: %d\n", voltage, voltageMap[i].colorIndex);
            return voltageMap[i].colorIndex; // returns 0/1/2/... corresponds to the color
        }
    }
    return -1; // Invalid voltage
}

void gameWon(){
    printf("GAME WON!\n"); 
    notifyBLE("WON");
    sendNRF((const uint8_t*) "WON"); 
    open_cover(); // opens cover to show the pins
    printf("Restart the game by pressing the button\n"); 
}

void gameOver(){
    printf("GAME OVER\n");
    notifyBLE("OVER");
    sendNRF((const uint8_t*) "OVER");
    open_cover(); 
    printf("Restart the game by pressing the button\n"); 
}

void logReceivedColors(uint8_t buf[]){
    printf("===== checking received voltages =====\n");
    
    uint16_t adcValues[4];
    float receivedVoltages[4];
    for (int i = 0; i < 4; i++) {
        adcValues[i] = buf[i * 2] | (buf[i * 2 + 1] << 8);
        receivedVoltages[i] = (adcValues[i] / 1023.0) * 5.0; // Assuming 5V reference on PIC -> i think this is actually 3.3V
        printf("Voltage %d: %.2f V \n", i + 1, receivedVoltages[i]);
        // put every biColoredLED on 00
        gpio_set_level(pinGroupBiLeds[i][0], 0); // red
        gpio_set_level(pinGroupBiLeds[i][1], 0);
    }   
}

void processReceivedColors(float receivedVoltages[4]){
    int feedbackCounter = 0; // counts how many feedback leds will be used
    int rightPositionCounter = 0;
    int receivedColors[4]; 
    char colorString[12]; // this is needed for the msg for BLE sending
    // this is the code for the leds
    for (int i = 0; i < 4; i++){
        int receivedColor = getColorFromVoltage(receivedVoltages[i]);
        receivedColors[i] = receivedColor;  // store received color indexes to send to java application
        for (int j = 0; j < 4; j++){
            if (receivedColor == generatedColors[j] && i == j){ // recievedColor is at same position as in generatedColors
                gpio_set_level(pinGroupBiLeds[feedbackCounter][0], 1); // red
                gpio_set_level(pinGroupBiLeds[feedbackCounter][1], 0); // green
                printf("Color %d is at the right position!\n", i + 1); 
                feedbackCounter++;
                rightPositionCounter++;
            }
            else if(receivedColor == generatedColors[j]){ // recievedColor is just among the generatedColors
                gpio_set_level(pinGroupBiLeds[feedbackCounter][0], 0);
                gpio_set_level(pinGroupBiLeds[feedbackCounter][1], 1);
                printf("Color %d is among the generated colors, but at the wrong position\n", i + 1);
                feedbackCounter++; 
            }
        }
    }

    // Convert receivedColors array to a string
    snprintf(colorString, sizeof(colorString), "%d,%d,%d,%d", 
            receivedColors[0], receivedColors[1], receivedColors[2], receivedColors[3]);
    notifyBLE(colorString); 
    tries++; 
    if (rightPositionCounter == 4){gameWon();}; 
    if (tries == 7){gameOver();}
}

void configure_pins() {
    int colorPins[] = {RED1_PIN, GREEN1_PIN, BLUE1_PIN, RED2_PIN, GREEN2_PIN, BLUE2_PIN, RED3_PIN, GREEN3_PIN, BLUE3_PIN, RED4_PIN, GREEN4_PIN, BLUE4_PIN};
        
    for (int i = 0; i < sizeof(colorPins) / sizeof(colorPins[0]); i++) {
        gpio_reset_pin(colorPins[i]);
        esp_rom_gpio_pad_select_gpio(colorPins[i]);
        gpio_set_direction(colorPins[i], GPIO_MODE_OUTPUT);
    }    

    //bi-colored pins
    for (int i = 0; i < 4; i++){
        gpio_reset_pin(pinGroupBiLeds[i][0]);
        esp_rom_gpio_pad_select_gpio(pinGroupBiLeds[i][0]);
        gpio_set_direction(pinGroupBiLeds[i][0], GPIO_MODE_OUTPUT);

        gpio_reset_pin(pinGroupBiLeds[i][1]);
        esp_rom_gpio_pad_select_gpio(pinGroupBiLeds[i][1]);
        gpio_set_direction(pinGroupBiLeds[i][1], GPIO_MODE_OUTPUT);
    }
}

void set_color(int redPin, int greenPin, int bluePin, uint8_t red, uint8_t green, uint8_t blue) {
    gpio_set_level(redPin, red);
    gpio_set_level(greenPin, green);
    gpio_set_level(bluePin, blue);
}


void generate4Colors(){
    srand(esp_random()); 
    int availableColors = 6;
    int colorUsedFlags[6] = {0}; // To prevent duplicate colors

    printf("\n \n ====== NEW GAME STARTED ======\n");
    close_cover(); 
    vTaskDelay(pdMS_TO_TICKS(2000)); // wait 2 seconds so the cover is closed!  
    printf("=== generating 4 random colors ===\n");
    for (int i = 0; i < 4; i++) {
        int randomIndex;
        do {
            randomIndex = rand() % availableColors;
        } while (colorUsedFlags[randomIndex]); // Ensure no duplicate colors

        printf("generated color index: %d \n", randomIndex );

        colorUsedFlags[randomIndex] = 1; // Mark as used
        generatedColors[i] = randomIndex; // Store the assigned color for this position

        uint8_t redState = colors[randomIndex][0];
        uint8_t greenState = colors[randomIndex][1];
        uint8_t blueState = colors[randomIndex][2];
        set_color(pinGroups[i][0], pinGroups[i][1], pinGroups[i][2], redState, greenState, blueState);

        // start with feedback leds off
        gpio_set_level(pinGroupBiLeds[i][0], 0);
        gpio_set_level(pinGroupBiLeds[i][1], 0);
    }

    vTaskDelay(pdMS_TO_TICKS(10)); 
}



