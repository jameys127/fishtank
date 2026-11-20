#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/rand.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_ADDRESS 0x3C
#define PAGE_LENGTH 8
#define TOTAL_DISPLAY_BYTES (1 + (DISPLAY_HEIGHT * DISPLAY_WIDTH) / 8)
#define FISH_WIDTH 16
#define FISH_HEIGHT 8



// <'(((><

const uint16_t fish[8] = {
    0b0000000000000000,
    0b0000001010100001,
    0b0011010101010010,
    0b1101010101001100,
    0b1100010101001100,
    0b0011010101010010,
    0b0000001010100001,
    0b0000000000000000,
};
uint8_t display_array[TOTAL_DISPLAY_BYTES];

void set_pixel(int x, int y){
    if(x > DISPLAY_WIDTH - 1 || x < 0 || y > DISPLAY_HEIGHT - 1 || y < 0){
        return;
    }
    int index = 1 + (y/PAGE_LENGTH) * DISPLAY_WIDTH + x;
    int bit = y % PAGE_LENGTH;
    uint8_t mask = 1 << bit;
    display_array[index] |= mask;
}

void draw_sprite(int x, int y, bool right){
    for(int i = 0; i < FISH_HEIGHT; i++){
        for(int j = 0; j < FISH_WIDTH; j++){
            uint16_t mask;
            if(right){
                mask = 1 << j;    
            }else{
                mask = 1 << ((FISH_WIDTH - 1) - j);
            }
            if(fish[i] & mask){
                set_pixel(x + j, y + i);
            }
        }
    }
}

void send_command_to_display(uint8_t command_byte){
    uint8_t byte_array[2] = {0x00, command_byte};
    i2c_write_blocking(i2c0, DISPLAY_ADDRESS, byte_array, 2, false);
}

void display_init(){
    sleep_ms(200);
    const uint GP4 = 4;
    const uint GP5 = 5;

    i2c_init(i2c0, 400*1000); // 400 kHz is the maximum supported for my display
    gpio_set_function(GP4, GPIO_FUNC_I2C);
    gpio_set_function(GP5, GPIO_FUNC_I2C);

    gpio_pull_up(GP4);
    gpio_pull_up(GP5);

    send_command_to_display(0x8D); // the charge pump to power the display "the next byte is the charge pump setting"
    send_command_to_display(0x14); // the setting. not sure what this means though: bit 2 = 1 -> ON  || it specifically means that that specific bit means the charge pump is ON
    send_command_to_display(0xAF); // wakes up the display from sleep mode
    send_command_to_display(0xA4); // starts screen in normal mode and shows on screen whats in memory
                        // start of the pixel addressing
    send_command_to_display(0x20); // settings for writing mode: "the next abyte is the addressing mode"
    send_command_to_display(0x00); // mode 0 - horrizontal. Other modes are 0x01 = mode 1 - vertical moving down one page and wrapping back to the next column. 0x02 = mode 2 - page wrapping back to the same page instead of down one.

    send_command_to_display(0x21); // column range "the next two bytes are the column range"
    send_command_to_display(0); // start column
    send_command_to_display(127); //end column                the screen has 128 columns as the display resolution is 128x64

    send_command_to_display(0x22); // page range "the next two bytes are the page range" pages are 8 rows long so if the height of the screen is 64 pixels that means there are 8 pages total
    send_command_to_display(0); //start page
    send_command_to_display(7); //end page

    display_array[0] = 0x40;
}

void display_show(){
    i2c_write_blocking(i2c0, DISPLAY_ADDRESS, display_array, TOTAL_DISPLAY_BYTES, false);
}

void display_clear(){
    for(int i = 1; i < TOTAL_DISPLAY_BYTES; i++){
        display_array[i] = 0x00;
    }
}


int random_number(int min, int max){
    uint32_t temp = get_rand_32();
    int num = min + (int)(temp % (max - min + 1));
    return num;
}

int main()
{
    display_init();

    int fish_x = 0;
    int fish_y = 25;
    bool fish_facing_right = true;
    int destination_x = random_number(0, DISPLAY_WIDTH - FISH_WIDTH);
    int destination_y = random_number(0, DISPLAY_HEIGHT - FISH_HEIGHT);
    while (true) {

        if(destination_x > fish_x){
            fish_facing_right = true;
            fish_x = fish_x + 1;
        }else if(destination_x < fish_x){
            fish_facing_right = false;
            fish_x = fish_x - 1;
        }else{

        }

        if(destination_y > fish_y){
            fish_y = fish_y + 1;
        }else if(destination_y < fish_y){
            fish_y = fish_y - 1;
        }else{

        }

        if(fish_x == destination_x && fish_y == destination_y){
            destination_x = random_number(0, DISPLAY_WIDTH - FISH_WIDTH);
            destination_y = random_number(0, DISPLAY_HEIGHT - FISH_HEIGHT);
        }

        display_clear();
        draw_sprite(fish_x, fish_y, fish_facing_right);
        display_show();
        sleep_ms(50);
    }
}
