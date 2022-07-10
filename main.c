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


const uint OUT_TARGET_POWER = 0;
const uint IN_NRF_VDD = 0;

// const uint32_t PDND_TRIG = 18;
// const uint32_t PDND_GLITCH = 19;

const uint32_t TARGET_POWER = 18;

// void pdnd_initialize_glitcher() {
//     gpio_init(PDND_TRIG);
//     gpio_set_dir(PDND_TRIG, GPIO_IN);
//     gpio_pull_up(PDND_TRIG);
//     gpio_init(PDND_GLITCH);
//     gpio_put(PDND_GLITCH, 0);
//     gpio_set_dir(PDND_GLITCH, GPIO_OUT);
// }

void initialize_board() {
    gpio_init(TARGET_POWER);
    gpio_put(TARGET_POWER, 0);
    gpio_set_dir(TARGET_POWER, GPIO_OUT);
    gpio_init(PDND_GLITCH);
    gpio_put(PDND_GLITCH, 0);
    gpio_set_dir(PDND_GLITCH, GPIO_OUT);
}

static inline power_cycle_target() {
    gpio_put(TARGET_POWER, 0);
    sleep_ms(50);
    gpio_put(TARGET_POWER, 1);
}


const uint8_t CMD_DELAY = 0x41;
const uint8_t CMD_PULSE = 0x42;
const uint8_t CMD_GLITCH = 0x43;

void dv(uint32_t delay, uint32_t pulse, uint32_t counter) {
    cls(false);
    printf("Try Number %d: Delay = %d , Pulse = %d \r", counter, delay, pulse);
}

int main() {
    stdio_init_all();
    // Sleep X MS to Wait for USB Serial to initialize
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
    
    printf("Boot-Sequence finished - starting Glitch Loop. \r\n\n");
    uint32_t delay = 0;
    uint32_t pulse = 0;
    uint32_t counter = 0;
    
    while(1) {
        counter = counter + 1;
        if((counter % 100) == 0){
            dv(delay, pulse, counter);
        }
        uint8_t cmd = getchar();
        switch(cmd) {
            case CMD_DELAY:
                fread(&delay, 1, 4, stdin);
                //dv(delay, pulse, counter);
                break;
            case CMD_PULSE:
                fread(&pulse, 1, 4, stdin);
                //dv(delay, pulse, counter);
                break;
            case CMD_GLITCH:
                power_cycle_target();

                while(gpio_get(1 + IN_NRF_VDD));
                for(uint32_t i=0; i < delay; i++) {
                    asm("NOP");
                }
                gpio_put(PDND_GLITCH, 1);
                for(uint32_t i=0; i < pulse; i++) {
                    asm("NOP");
                }
                gpio_put(PDND_GLITCH, 0);

        }
    }

    return 0;
}

