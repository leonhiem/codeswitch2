#ifndef LCD_H
#define LCD_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void lcd_init(int8_t onoff);
void lcd_clk(void);
void lcd_write_digits(uint8_t dig2, uint8_t dig1, uint8_t dig0, uint8_t phase);
void lcd_write_number_int(int16_t value, uint8_t blinking);
void lcd_write_number(uint8_t val, uint8_t phase, uint8_t allow_blank_digit);

#endif
