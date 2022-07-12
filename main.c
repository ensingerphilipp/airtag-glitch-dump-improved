#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include "pdnd/pdnd.h"
#include "pdnd/pdnd_display.h"
// #include "uart_rx.pio.h"
// #include "uart_tx.pio.h"
#include "hardware/pio.h"
// #include "spi.pio.h"
#include "pdnd/pio/pio_spi.h"
const uint SI_PIN = 3;
const uint SO_PIN = 4;
const uint CLK_PIN = 2;


//IMPORTANT!
//PINs used:
//
//Probe input to check if target has powered up: GPIO 1
//Pin to provide airtag with 3.3V Power: GPIO18
//Not Visible in this File but PDND_Glitch output to N-Channel Mosfet: GPIO19
//
// ignore leftover artifacts

const uint32_t OUT_TARGET_POWER = 0;
const uint32_t IN_NRF_VDD = 1;
const uint32_t TARGET_POWER = 18;

// const uint32_t PDND_TRIG = 18;
// const uint32_t PDND_GLITCH = 19;


void initialize_board() {
    gpio_init(TARGET_POWER);
    gpio_put(TARGET_POWER, 0);
    gpio_set_dir(TARGET_POWER, GPIO_OUT);
    gpio_init(PDND_GLITCH);
    gpio_put(PDND_GLITCH, 0);
    gpio_set_dir(PDND_GLITCH, GPIO_OUT);
}

//restart the Airtag
static inline power_cycle_target() {
    gpio_put(TARGET_POWER, 0);
    sleep_ms(50);
    gpio_put(TARGET_POWER, 1);
}

//Serial Connection Byte Commands to execute the corresponding actions in main()
const uint8_t CMD_DELAY = 0x41;
const uint8_t CMD_PULSE = 0x42;
const uint8_t CMD_GLITCH = 0x43;
const uint8_t CMD_POWERUP = 0x44;
const uint8_t CMD_POWERDOWN = 0x45;

//Print function to print current stats to serial output
void dv(uint32_t delay, uint32_t pulse, uint32_t counter) {
    cls(false);
    printf("Try Number %d: Delay = %d , Pulse = %d \r", counter, delay, pulse);
}


//Main Function
int main() {
    stdio_init_all();
    // Sleep X MS to Wait for USB Serial to initialize <-- This means you will need to connect via serial before pico fully starts up (This is important to not encounter no serial found bug)
    while (!tud_cdc_connected()) { sleep_ms(100);  }
    printf("USB-Serial connected!\r\n");
    stdio_set_translate_crlf(&stdio_usb, false);
    pdnd_initialize();
    pdnd_enable_buffers(0);
    pdnd_display_initialize();
    pdnd_enable_buffers(1);
    cls(false);
    printf("Initializing Raspberry Pi Pico Board ... \r\n");

    // Sets up trigger & glitch output
    initialize_board();
    
    printf("Boot-Sequence finished - ready for Serial Commands \r\n\n");
    
    //Delay to wait after receiving the signal that airtag NRF has power (Delay = 10000 is 10000 ASM NOP executions on the Pico)
    uint32_t delay = 0;
    
    //Length of the Glitch (how long power cut from cpu core) measured in ASM Nop executions
    uint32_t pulse = 0;
    
    //counter to count attempts
    uint32_t counter = 0;
    
    while(1) {
        counter = counter + 1;
        if((counter % 100) == 0){
            dv(delay, pulse, counter);
        }
        
        //Get Command from Serial and execute accordingly
        uint8_t cmd = getchar();
        switch(cmd) {
            //Set Delay
            case CMD_DELAY:
                fread(&delay, 1, 4, stdin);
                //dv(delay, pulse, counter);
                break;
            //Set Pulse
            case CMD_PULSE:
                fread(&pulse, 1, 4, stdin);
                //dv(delay, pulse, counter);
                break;
            //Command to Power up the Airtag 
            case CMD_POWERUP:
                gpio_put(TARGET_POWER, 1);
                break;
            //Command to Power down the Airtag 
            case CMD_POWERDOWN:
                gpio_put(TARGET_POWER, 0);
                break;
            //Command to execute glitch
            case CMD_GLITCH:
                power_cycle_target();

                //Wait until Pin IN_NRF_VDD has no Voltage applied (No voltage applied means the airtag has power because the level shifter logic inverts the signal)
                while(gpio_get(IN_NRF_VDD));
                for(uint32_t i=0; i < delay; i++) {
                    asm("NOP");
                }
                
                //Trigger N-Channel-Mosfet for Pulse Number of ASM NOPs (cut power of cpu core for Pulse number of asm NOPs)
                gpio_put(PDND_GLITCH, 1);
                for(uint32_t i=0; i < pulse; i++) {
                    asm("NOP");
                }
                gpio_put(PDND_GLITCH, 0);

        }
    }

    return 0;
}

