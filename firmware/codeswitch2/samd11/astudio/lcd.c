#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "samd11.h"
#include "hal_gpio.h"
#include "lcd.h"

#ifndef LCD
#  define LCD 0  // top (0=bottom)
#endif

int lcd_clk_state = 0;
int lcd_ser_count = 0;
int lcd_refresh_count = 0;
volatile int lcd_update_flag = 0;
volatile uint16_t lcd_clk_count = 0;

uint16_t lcd_value = 0;
uint16_t lcd_value_new[4] = {0,0,0,0};
uint8_t lcd_value_idx = 0;

const uint8_t segtable_dig_2[] = {
    0,1
};
const uint8_t segtable_dig_1[] = {
    // 0    1    2    3    4    5    6    7    8    9 blank =
#if (LCD==1) // top?
    0x7b,0x60,0x37,0x75,0x6c,0x5d,0x5f,0x70,0x7f,0x7d,0x00,0x11 //LCD on topside
#else
    0x6f,0x03,0x76,0x57,0x1b,0x5d,0x7d,0x07,0x7f,0x5f,0x00,0x44 //LCD on bottomside
#endif
};

const uint8_t segtable_dig_0[] = {
    // 0    1    2    3    4    5    6    7    8    9 blank ]
#if (LCD==1) // top?
    0x5f,0x06,0x3b,0x2f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x0f //LCD on topside
#else
    0x7d,0x30,0x6e,0x7a,0x33,0x5b,0x5f,0x70,0x7f,0x7b,0x00,0x78 //LCD on bottomside
#endif
};
#if (LCD==1) // top?
#define LCD_BLANK_VAL 0x0100  // common is on bit8 //LCD on topside
#else
#define LCD_BLANK_VAL 0x8000  // common is on bit15 //LCD on bottomside
#endif

uint8_t conv2bcd(uint8_t x);

void lcd_init(int8_t onoff)
{
    // reset all:
    HAL_GPIO_LCD_DATA_out();
    HAL_GPIO_LCD_DATA_clr();
    HAL_GPIO_LCD_CLK_out();
    HAL_GPIO_LCD_CLK_clr();
    HAL_GPIO_LCD_LATCH_out();
    HAL_GPIO_LCD_LATCH_clr();
    lcd_clk_state=0;
    lcd_ser_count=0;
    lcd_value=LCD_BLANK_VAL; 
    lcd_value_idx=0;
    lcd_value_new[0]=lcd_value;
    lcd_value_new[1]=lcd_value;
    lcd_value_new[2]=lcd_value;
    lcd_value_new[3]=lcd_value;
    lcd_update_flag=0;
    lcd_refresh_count=0;
    lcd_clk_count=0;
    if(onoff==0) {
        HAL_GPIO_LCD_DATA_disable();
        HAL_GPIO_LCD_CLK_disable();
        HAL_GPIO_LCD_LATCH_disable();
    }    
}

/*
 * Every time this function is called, the shift resister makes 1 operation (shift)
 * Assuming entering this function 4000x per second. CLK (toggling) will be 2kHz
 */
void lcd_clk(void)
{
    // runs in interrupt service routine: keep it short!
    
    lcd_clk_count++;
    lcd_clk_count&=0x1fff; // mask with 8191 to automatic wrap 0-8191
    
    // go through the 4 phases:
    if(lcd_clk_count==0) {
        lcd_value_idx=0;
        lcd_update_flag=1;
    } else if(lcd_clk_count==0x800) { // 2048
        lcd_value_idx=1;
        lcd_update_flag=1;
    } else if(lcd_clk_count==0x1000) { // 4096
        lcd_value_idx=2;
        lcd_update_flag=1;
    } else if(lcd_clk_count==0x1800) { // 6144
        lcd_value_idx=3;
        lcd_update_flag=1;
    }
    
    // toggle the clock:
    if(lcd_clk_state==0) {
        lcd_clk_state=1;
        // negative clk edge: make new SER available
        HAL_GPIO_LCD_CLK_clr();
            
        if(((lcd_value>>lcd_ser_count)&1) == 0) {
            HAL_GPIO_LCD_DATA_clr();
        } else {
            HAL_GPIO_LCD_DATA_set();
        }
        lcd_ser_count++;
        if(lcd_ser_count >= 17) {
            lcd_clk_state=2; // pause clk cycles
            HAL_GPIO_LCD_LATCH_set(); // latch in complete 16bit value to output
            lcd_ser_count=0;
            lcd_refresh_count++;
            
            if(lcd_update_flag) {
                lcd_update_flag=0;                
                lcd_value=lcd_value_new[lcd_value_idx];
            } else if(lcd_refresh_count >= 2) { // refresh f=30Hz
                lcd_refresh_count=0;
                lcd_value=~lcd_value; // invert all
            }
        }
    } else if(lcd_clk_state==1){
        lcd_clk_state=0;
        // positive clk edge: clocking in the SER value        
        HAL_GPIO_LCD_CLK_set();            
    } else if(lcd_clk_state==2) {
        lcd_clk_state=3;
        HAL_GPIO_LCD_LATCH_clr();
    } else {
        lcd_clk_state=0;
    }
}

/*
 * Fill in each digit for phase 0..3
 * The variable 'phase' can be 0,1,2 or 3. LCD will swap in between phases every 0.5sec
 *
 * in case you want to blink the LCD: fill in your number in phase 0,1 and a blank value for phase 2,3
 * for example:
 *   lcd_write_digits(1, 2, 3, 0);
 *   lcd_write_digits(1, 2, 3, 1);
 *   lcd_write_digits(0, 0xa, 0xa, 2);
 *   lcd_write_digits(0, 0xa, 0xa, 3);
 * this example will blink the number 123 on the LCD (slow blink).
 * if you don't want blinking, redo example:
 *   lcd_write_digits(1, 2, 3, 0);
 *   lcd_write_digits(1, 2, 3, 1);
 *   lcd_write_digits(1, 2, 3, 2);
 *   lcd_write_digits(1, 2, 3, 3);
 */
void lcd_write_digits(uint8_t dig2, uint8_t dig1, uint8_t dig0, uint8_t phase)
{
#if (LCD==1) // top?
    uint16_t val = (segtable_dig_1[dig1]<<9)|(segtable_dig_2[dig2]<<7)|(segtable_dig_0[dig0]); // LCD on topside
#else
    uint16_t val = (segtable_dig_1[dig1]<<8)|(segtable_dig_0[dig0]<<1)|segtable_dig_2[dig2]; // LCD on bottomside
#endif
    lcd_value_new[(phase&3)] = val;
    lcd_update_flag=1;
}

/*
 * in case value <0  truncate to 0. LCD does not support negative values (yet)
 *
 * if you want the number to blink; make blinking=1 otherwise 0
 *
 * in case value is >188, for example 18899, the display will toggle between
 * 188 and 99. Slow toggle when blinking=0 otherwise fast toggle
 */
void lcd_write_number_int(int16_t value, uint8_t blinking)
{
    uint16_t val;
    uint8_t v;
    if(value > 18899) value=18899; // sorry
    if(value < 0) val=0; else val=(uint16_t)value;
    
    if(val > 188) { // value too large to display on 188 LCD, let it toggle
        v=(uint8_t)(val%100); // least significant part
        lcd_write_number(v,0,0);
        if(blinking) { lcd_write_number(v,2,0); } else { lcd_write_number(v,1,0); }
        
        v=(uint8_t)(val/100); // most significant part
        lcd_write_number(v,3,1);
        if(blinking) { lcd_write_number(v,1,1); } else { lcd_write_number(v,2,1); }
    } else {
        v=(uint8_t)val;    
        lcd_write_number(v, 0,1);
        lcd_write_number(v, 2,1);
        if(blinking) { 
            lcd_write_digits(0, 0xa, 0xa, 1); // blank
            lcd_write_digits(0, 0xa, 0xa, 3); // blank
        } else { 
            lcd_write_number(v,1,1); 
            lcd_write_number(v,3,1); 
        }
    }
}

void lcd_write_number(uint8_t val, uint8_t phase, uint8_t allow_blank_digit)
{
    uint8_t v;
    uint8_t bcddig_2; // can only show 1 or nothing
    uint8_t bcddig_1;
    uint8_t bcddig_0;

    v=conv2bcd(val/100) &0xf;
    if(v==0) { v=0; } else { v=1; }
    bcddig_2=v;

    v=(conv2bcd( (val/10)%10 ) &0xf);    
    if(v==0 && bcddig_2 ==0 && allow_blank_digit) { v=0xa; }
    bcddig_1=v;
    bcddig_0=conv2bcd(val%10);

    lcd_write_digits(bcddig_2, bcddig_1, bcddig_0, phase);
}

uint8_t conv2bcd(uint8_t x)
{
    uint8_t p=0;
    uint8_t c=0;
    uint8_t i;

    for(i=x;i!=0;i=i/10) {
        p |= (i%10)<<c;
        c += 4;
    }
    return p;
}



