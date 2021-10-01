/*
 * Kamworks Codeswitch2 firmware
 *
 * Author: Leon Hiemstra <l.hiemstra@gmail.com>
 *
 * Compiler tool: Atmel Studio 7.0.1417
 *
 * Microcontroller: ATSAMD11D14AM
 * Required fuse settings: none (keep default)
 *
 * (Arduino IR decoder derived from: ...)
 */

//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "samd11.h"
#include "hal_gpio.h"
#include "utils.h"
#include "rtc.h"
#include "tone.h"
#include "flash.h"
#include "keypad.h"
#include "lcd.h"
#include "wdt.h"


#define VERSION     2
#define SUBVERSION  15


#ifndef KEYPAD_INTEGRATED
#  define KEYPAD_INTEGRATED 1 // enabled
#endif

#ifndef TESTING
#  define TESTING 0 // disabled
#endif

#ifndef LCD
#  define LCD 0  // top (0=bottom)
#endif

volatile bool alarmTriggered = false;

volatile bool usart_rx_ready=false;
volatile bool usart_tx_ready=true;
volatile bool print_info=false;
volatile uint8_t bod33_tripped,wdt_wakeup;
uint8_t rcause;
#define RXBUF_SIZE 100
char rxbuf[RXBUF_SIZE];
int data_receive_idx=0;

volatile unsigned long milliseconds=0UL;
int millis_div20=0;
int millis_div4=0;
static Eeprom eeprom;


void all_init(int8_t onoff);

static void uart_putc(char c)
{
    while (!(SERCOM1->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE));
    SERCOM1->USART.DATA.reg = c;
}
static void uart_puts(char *s)
{   while (*s) uart_putc(*s++); }

static void uart_puts_info(char *s)
{   if(print_info) { while (*s) uart_putc(*s++); } }

//void SYSCTRL_Handler(void)
void irq_handler_sysctrl(void)
{
    if(SYSCTRL->INTFLAG.bit.BOD33DET) {
        bod33_tripped=1;
        SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33DET; // clear interrupt  
    }    
}

// Errata ref 15513: disable BOD12 before sleep and enable after sleep
void set_bod12(uint8_t onoff)
{
    volatile uint32_t *ptr = (volatile uint32_t *)0x40000838;
    if(onoff) *ptr = 4; else *ptr = 6;
}

static void sys_init(uint8_t onoff)
{
    if(onoff==0) {
        SYSCTRL->OSC8M.bit.PRESC = 0;
       PM->APBAMASK.reg &= ~PM_APBAMASK_GCLK; 
    } else {
    PM->APBAMASK.reg |= PM_APBAMASK_GCLK;
    // Switch to 8MHz clock (disable prescaler)
    SYSCTRL->OSC8M.bit.PRESC = 0;
    SYSCTRL->BOD33.reg = 0; // disable to avoid spurious interrupts
    SYSCTRL->BOD33.reg = SYSCTRL_BOD33_LEVEL(48) | SYSCTRL_BOD33_CEN | SYSCTRL_BOD33_MODE |
                         SYSCTRL_BOD33_RUNSTDBY | SYSCTRL_BOD33_ACTION(2) | SYSCTRL_BOD33_HYST;
                         
    
    
    bod33_tripped=0;
    wdt_wakeup=0;
    SYSCTRL->BOD33.reg |= SYSCTRL_BOD33_ENABLE;
    while((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_B33SRDY) == 0);
    SYSCTRL->INTENSET.reg = SYSCTRL_INTENSET_BOD33DET;
    NVIC_EnableIRQ(SYSCTRL_IRQn);
    
    // Enable interrupts
    asm volatile ("cpsie i");
    }    
}


void check_bod(void)
{
    uint8_t bod33_trip;
    //uart_puts_info("chk bod\n\r");
    __disable_irq();
    bod33_trip=bod33_tripped;
    bod33_tripped=0;   
    __enable_irq();
    
    // sleep when BOD or wakeup pin = 0
    if(bod33_trip || HAL_GPIO_WAKEUP_PIN_read()==0) {
        uart_puts_info(">>> bod33_trip SLEEP! <<<\n\r");
        all_init(0);
        set_bod12(0);
        
        while(1) {
            wdt_wakeup=0;            
            wdt_sleep(1000);
            
            // in case the wakeup pin is activated:       
            if( ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_BOD33DET) == 0) &&
                (HAL_GPIO_WAKEUP_PIN_read() != 0) ) {
                                        
                break;
            }
            
            // in case the alarm is just triggered:
            if( ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_BOD33DET) == 0) && alarmTriggered) {
                
                break;
            }
        }       
        set_bod12(1);
        all_init(1);
        uart_puts_info(">>> bod33 ok wakeup <<<\n\r");
    }      
}

//-----------------------------------------------------------------------------
void irq_handler_tc1(void)
{  
  if (TC1->COUNT16.INTFLAG.reg & TC_INTFLAG_MC(1)) {
            
      millis_div20++; 
      if(millis_div20 >= 20) {
            milliseconds++;
            millis_div20=0;
      }      
       millis_div4++;
       if(millis_div4 >= 5) {
           millis_div4=0;           
           lcd_clk(); // 20kHz / 4 = 5kHz
       }
  }
  TC1->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1); 
}

//-----------------------------------------------------------------------------
static void timer1_init(int8_t onoff)
{
  if(onoff==0) {
      TC1->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
      PM->APBCMASK.reg &= ~PM_APBCMASK_TC1;
  } else {
      PM->APBCMASK.reg |= PM_APBCMASK_TC1;
      GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(TC1_GCLK_ID) |
                          GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0);
    
      TC1->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ |
                               TC_CTRLA_PRESCALER_DIV8 | TC_CTRLA_PRESCSYNC_RESYNC;
    
      TC1->COUNT16.COUNT.reg = 0;
      
        TC1->COUNT16.CC[0].reg = 48; // 20kHz
        TC1->COUNT16.COUNT.reg = 0;
        
      TC1->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
      TC1->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);
      NVIC_EnableIRQ(TC1_IRQn);
  }
}


volatile uint8_t serial_rxbuf;
volatile bool serial_rxflag=false;

void irq_handler_sercom1(void)
{
    uint8_t c;
    if (SERCOM1->USART.INTFLAG.bit.RXC) {
        c = SERCOM1->USART.DATA.bit.DATA;
        serial_rxbuf = c;
        serial_rxflag = true;
    }
}



//-----------------------------------------------------------------------------
static void uart_init(int8_t onoff)
{
  uint32_t baud = 115200;
  uint64_t br = (uint64_t)65536 * (F_CPU - 16 * baud) / F_CPU;

  
  HAL_GPIO_UART_TX_pmuxen(PORT_PMUX_PMUXE_C_Val);
  HAL_GPIO_UART_RX_in();
  HAL_GPIO_UART_RX_pullup();
  HAL_GPIO_UART_RX_pmuxen(PORT_PMUX_PMUXE_C_Val);

  if(onoff==0) {
      SERCOM1->USART.CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
      PM->APBCMASK.reg &= ~PM_APBCMASK_SERCOM1;
      HAL_GPIO_UART_TX_disable();
  } else {
      HAL_GPIO_UART_TX_out();
      PM->APBCMASK.reg |= PM_APBCMASK_SERCOM1;
    
      GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM1_GCLK_ID_CORE) |
                          GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0);
    
      SERCOM1->USART.CTRLA.reg =
          SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE_USART_INT_CLK |
          SERCOM_USART_CTRLA_RXPO(1/*PAD1*/) | SERCOM_USART_CTRLA_TXPO(0/*PAD0*/);
    
      SERCOM1->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN |
          SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);
    
      SERCOM1->USART.BAUD.reg = (uint16_t)br+1;
      
      // set RX interrupt:
      SERCOM1->USART.INTENSET.reg = SERCOM_USART_INTENSET_RXC;
      
      SERCOM1->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
      NVIC_EnableIRQ(SERCOM1_IRQn);
  }
}

//-----------------------------------------------------------------------------

void print_prompt(void)
{
//    uart_puts("\n\rCMD> ");    
}

char addchar(char c)
{
    char buf[2]={0,0};
    buf[0]=c;
    
    if(buf[0]=='*') {
        data_receive_idx=0;
    } else if(buf[0]=='#' && rxbuf[0]=='*') {
        usart_rx_ready=true;
    } else if(!is_digit(buf[0])) {
        play_song_error();
        //data_receive_idx=0;
        return ' ';
    }
    
    if(data_receive_idx>80) {
        data_receive_idx=0;
        return ' ';
    }
    
//#if (KEYPAD_INTEGRATED==0)
    // possibly lit an LED during a keypress:
    // LED on here
    HAL_GPIO_TP1_set();
 //   HAL_GPIO_TP4_set();
//#endif
    play_toneC();
//#if (KEYPAD_INTEGRATED==0)
    // LED off here
    HAL_GPIO_TP1_clr();
 //   HAL_GPIO_TP4_clr();
//#endif   
    
    strcpy(&rxbuf[data_receive_idx],buf);
    data_receive_idx++;
    return c;
}


void mosfet_switch(bool sw)
{  
  if(sw) { // on
    HAL_GPIO_MOSFET_ON_set();
  } else { // off
    HAL_GPIO_MOSFET_ON_clr();  
  }
}


// returns -1 if passed alarm time already
// returns  >=0 if have still days credit left
int16_t read_print_rtc(bool do_print)
{
    char buf[50];
    Time now,alarm;
    int16_t credit;
    uint8_t show_hours_left=0; // default show days left
    
    getTimeNow(&now);
    getAlarmTimeNow(&alarm);
    
    credit = Days_left(&alarm,&now,show_hours_left,0);
    if(credit<=0) {
        show_hours_left=1;
        credit = Days_left(&alarm,&now,show_hours_left,0);
    }
    if(do_print) {
      sprintf(buf,"\n\r      YY-MM-DD hh:mm\n\r");
      uart_puts_info(buf);
      sprintf(buf," time:%02d-%02d-%02d %02d:%02d\n\r",now.year,now.month,now.day,now.hour,now.minute);
      uart_puts_info(buf);
      sprintf(buf,"alarm:%02d-%02d-%02d %02d:%02d\n\r",alarm.year,alarm.month,alarm.day,alarm.hour,alarm.minute);
      uart_puts_info(buf);
      sprintf(buf,"credit=%d %s left\n\r",credit,(show_hours_left ? "hours" : "days"));
      uart_puts_info(buf);
        
      lcd_write_number_int(credit,show_hours_left);
    }
    if(credit<=0) {
        credit = Days_left(&alarm,&now,0,1); // get credit for minutes left
    }
    return credit;
}
void generate_newalarm(Time *newalarm, Time *now, Time *credit)
{
    Time alarm;
    int16_t oldcredit;
    getAlarmTimeNow(&alarm);
    oldcredit = Days_left(&alarm,now,0,0); // are there days left?    
    if(oldcredit<=0) {
        oldcredit = Days_left(&alarm,now,0,1); // maybe some minutes left?
    }
    
    if(oldcredit < 0) {
        // credit is expired; add credit to now    
        uart_puts_info("add to now\n\r");
        Time_add(newalarm, now, credit);
    } else {
        // credit not yet expired; add credit; move alarm forward
        uart_puts_info("add to alarm\n\r");        
        Time_add(newalarm, &alarm, credit);
    }
}

void code_init(int8_t onoff)
{
  char buf[100];
  
  if(onoff==0) return;
  
  // If this is the first run the "valid" value should be "false"
  if (eeprom.valid == false) {
      mosfet_switch(false);
      uart_puts_info("eeprom empty: mosfet off\n\r");
      setTime(0,0,0);// set time (hour 0-23, minute 0-59, second 0-59)
      setDate(1,1,1);// set date (day 1-31, month 1-12, year 0-99)
      eeprom.seq=99;
      read_print_rtc(true);
      play_song_expired();
 } else {
      int16_t creditleft=-1;
      uart_puts_info("eeprom ok: read values");
      
      // restore time, date, alarm:
      /*
      setTime(eeprom.now.hour,eeprom.now.minute,0);
      setDate(eeprom.now.day,eeprom.now.month,eeprom.now.year);
      setAlarmTime(eeprom.alarm.hour,eeprom.alarm.minute,0);
      setAlarmDate(eeprom.alarm.day,eeprom.alarm.month,eeprom.alarm.year);
      */
      sprintf(buf,"(days=%d) (seq=%d)\n\r",eeprom.days,eeprom.seq);
      uart_puts_info(buf);  
      
      if(eeprom.days==0 && eeprom.minutes==0) {
          uart_puts_info("free mode, no alarm: mosfet on\n\r");
          read_print_rtc(true);
          play_song_success();
          mosfet_switch(true);
      } else {
          creditleft=read_print_rtc(true);
          if(creditleft <= 0) { // passed the alarm time
              uart_puts_info("already expired, mosfet off\n\r");
              play_song_expired(); 
              mosfet_switch(false);                
          } else { // not yet passed the alarm time
              uart_puts_info("enable alarm, mosfet on\n\r");
              enableAlarm(RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val);
              play_song_success();
              mosfet_switch(true);
          }
      }
  }
}

uint32_t get_myid(void)
{
    // reserved 30 bits for id.
    // 
    // truncate 128bit id.
        
    // ATSAMD21 replies: 0x67718634504a5230352e3120ff03171c
    // ATSAMD11 replies: 0xe1c74e77514d4b5331202020ff0b4631
        
    // most significant side seems to be more unique.
        
    volatile uint32_t *chip_ptr = (volatile uint32_t *)0x0080A00C; // word0 address
    return (*chip_ptr & 0x3fffffff); // mask 30bits
}
void print_myid(void)
{
    char buf[50];
    uint32_t chipval = get_myid();    
    sprintf(buf, "\n\r%09ld\n\r",chipval);
    uart_puts(buf);
}

void io_init(void)
{
    HAL_GPIO_MOSFET_ON_out();
    HAL_GPIO_MOSFET_ON_clr();
    
    HAL_GPIO_WAKEUP_PIN_in();
    HAL_GPIO_WAKEUP_PIN_pullup();
    
    //HAL_GPIO_LED_RED_out();
    //HAL_GPIO_LED_RED_clr();
#if (KEYPAD_INTEGRATED==0) 
    HAL_GPIO_TP4_disable();      
#endif  
    HAL_GPIO_TP3_disable();    
    HAL_GPIO_TP2_disable();
   
    HAL_GPIO_TP1_out();
    HAL_GPIO_TP1_clr();
}

void show_intro_text(void)
{
  char buf[100];
  sprintf(buf, "Kamworks Codeswitch\n\rVersion=%d.%02d\n\r",VERSION,SUBVERSION);
  uart_puts(buf);
  sprintf(buf, "RCAUSE=0x%x\n\r",rcause);
  uart_puts(buf);
#if (TESTING==1)
  sprintf(buf, "TESTING\n\r"); uart_puts(buf);
  print_info=true;
#endif

  sprintf(buf, "\n\rChipID:"); uart_puts(buf);
  print_myid();
  lcd_write_digits(1, 8, 8, 0); // set test value
  lcd_write_digits(1, 8, 8, 1); // set test value
  lcd_write_digits(1, 8, 8, 2); // set test value
  lcd_write_digits(1, 8, 8, 3); // set test value
  delay_ms(300);
  lcd_write_digits(0, 0xa, 0xa, 0); // reset test value
  lcd_write_digits(0, 0xa, 0xa, 1); // reset test value
  lcd_write_digits(0, 0xa, 0xa, 2); // reset test value
  lcd_write_digits(0, 0xa, 0xa, 3); // reset test value
/*  
  sprintf(buf, "\nHelp: *code#\n\rcode=1: read id\n\rcode=2: remaining days\n\rcode=5: read RTC\n\r");
  uart_puts(buf);
  sprintf(buf, "code=10: info OFF, code=11: info ON\n\r");
  uart_puts(buf);
  sprintf(buf, "code=12: display subversion\n\r");
  uart_puts(buf);
#if (TESTING==1) 
  sprintf(buf, "code=3: mosfet ON\n\rcode=4: mosfet OFF\n\r");
  uart_puts(buf);
  sprintf(buf, "code=5yy: set Year\n\rcode=6mm: set Month\n\rcode=7dd: set Day\n\r");
  uart_puts(buf);
  sprintf(buf, "code=8hh: set Hour\n\rcode=9mm: set Minute\n\r");
  uart_puts(buf);
#endif
  sprintf(buf, "else code is credit (9 or 14 digits)\n\n\r");
  uart_puts(buf);
*/
}

void all_init(int8_t onoff)
{
    bod33_tripped=0;    
    rtc_init(onoff);
    uart_init(onoff);  
    timer1_init(onoff); 
    tone_init(onoff);   
    lcd_init(onoff);
    if(onoff) {
        show_intro_text();
    } 
    code_init(onoff);    
    keypad_init(onoff);
    
    
    /*
    // these ones below seem not differ the low power
    AC->CTRLA.bit.ENABLE = 0; // disable Analog Comperator
    ADC->CTRLA.bit.ENABLE = 0; // disable ADC
    PM->APBCMASK.reg &= ~PM_APBCMASK_AC;
    PM->APBCMASK.reg &= ~PM_APBCMASK_ADC;    
    DAC->CTRLA.bit.ENABLE = 0; // disable DAC
    PM->AHBMASK.bit.USB_ = 0;
    PM->AHBMASK.bit.DMAC_ = 0;
    PM->APBAMASK.bit.EIC_ = 0;
    PM->APBBMASK.bit.USB_ = 0;
    PM->APBBMASK.bit.PORT_ = 0;
    */

    // achieved:
    // 32uA @ 1.8V
    // 68uA @ 3.3V
}

//-----------------------------------------------------------------------------
int main(void)
{
  char buf[100];
  uint16_t cnt=0;
  char ch='.';
  uint8_t rtc_hour_old=0;
  uint8_t rtc_minutes_old=0;
  
  rcause=PM->RCAUSE.reg;
  
  delay_ms(10);
  
  // fixed pheripherals
 
  sys_init(1); 
  rtc_initialize();
  wdt_init();
  io_init();
  flash_config();
  flash_read((void *)&eeprom);
/*
  HAL_GPIO_TP4_set();
  
  setTime(0,0,0);// set time (hour 0-23, minute 0-59, second 0-59)
  setDate(1,1,1);// set date (day 1-31, month 1-12, year 0-99)
  setAlarmTime(0,0,20);
  setAlarmDate(1,1,1);
  enableAlarm(RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val);
  HAL_GPIO_TP4_clr();

  // dynamic pheripherals
  all_init(1);
   
  read_print_rtc(true);
  print_prompt();
  all_init(0);
     
   set_bod12(0);
   sys_init(0); 
   SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // Deepest sleep   
   __DSB(); // (Data Synchronisation Barrier)  -> finish all ongoing memory accesses
   __WFI(); // (Wait For Interrupt)
   sys_init(1);
   set_bod12(1);
  HAL_GPIO_TP4_set();
 */ 
  all_init(1);
  //read_print_rtc(true);
  
  while (1) {
      cnt++;
      check_bod();
      delay_ms(100);
      check_bod();
      __disable_irq();
      if(serial_rxflag) {
          serial_rxflag=false;    
          ch=addchar(serial_rxbuf);
          __enable_irq();
          uart_putc(ch);
      } else { __enable_irq(); }

      if ((ch=keypad_getKey()) != NO_KEY) {
          addchar(ch);
          uart_putc(ch);
      }

      if(usart_rx_ready) {
          int cmdstat=0;
          int16_t creditleft;    
          uart_puts_info("\n\r");        
          if(strcmp(rxbuf,"*1#")==0) {
              print_myid();        
          } else if(strcmp(rxbuf,"*2#")==0) {
              creditleft=read_print_rtc(true);
              if(creditleft<=0) {
                  play_song_expired();
              } else {
                  play_nof_tones(creditleft);
              }
      } else if(strcmp(rxbuf,"*5#")==0) {
              if(!print_info) {
                  print_info=true;
                  read_print_rtc(true);
                  print_info=false;
              } else {
                  read_print_rtc(true);
              }
          } else if(strcmp(rxbuf,"*10#")==0) {
              print_info = false;
          } else if(strcmp(rxbuf,"*11#")==0) {
              print_info = true;
          } else if(strcmp(rxbuf,"*12#")==0) {    
              lcd_write_number_int(SUBVERSION, 0);        
#if (TESTING==1)
          } else if(strcmp(rxbuf,"*3#")==0) {
              mosfet_switch(true); // on
          } else if(strcmp(rxbuf,"*4#")==0) {
              mosfet_switch(false); // off
          } else if(strlen(rxbuf) == 5) { // *123#  (set date, time) // disable this command after debug!
              unsigned long givencode;
              char opcode = rxbuf[1];
              rxbuf[0]=' '; // strip '*'
              rxbuf[4]=0;   // strip '#'
              rxbuf[1]=' '; // strip opcode
              givencode = strtoul(rxbuf,NULL,10);        
              if(opcode == '5') { // set year            
                  setYear((uint8_t)givencode);
              } else if(opcode == '6') { // set month
                  setMonth((uint8_t)givencode);
              } else if(opcode == '7') { // set day
                  setDay((uint8_t)givencode);
              } else if(opcode == '8') { // set hour
                  setHours((uint8_t)givencode);
              } else if(opcode == '9') { // set minute
                  setMinutes((uint8_t)givencode);
              } 
              read_print_rtc(true);
              sprintf(buf, "Enter new credit to write flash!\n\r");
              uart_puts_info(buf);
#endif
          } else {
#if (TESTING==1)
              if(strlen(rxbuf) == 11) { // *123456789# or *987654321# for demo
                  unsigned long givencode;
                  uint8_t givenminutes=0;
                  uint8_t givendays=0;
                  uint8_t seconds_now;
                  
                  rxbuf[0]=' '; // strip '*'
                  rxbuf[10]=0;  // strip '#'
                  if(strncasecmp(&rxbuf[1],"123456789",9)==0) {
                      givenminutes=3;
                  } else if(strncasecmp(&rxbuf[1],"987654321",9)==0) {
                      givendays=5;
                  } else {                                    
                      play_song_error();                      
                  } 
                  
                  if(givenminutes>0 || givendays>0) {
                      givencode = strtoul(rxbuf,NULL,10);
                      sprintf(buf," code=%lu (0x%lx):",givencode,givencode);
                      uart_puts_info(buf);
                      // done decoding the code                
                      Time now, credit, newalarm;
                      uart_puts_info("[setting alarm]");
                                
                      credit.year=0;
                      credit.month=0;
                      credit.day=givendays;
                      credit.hour=0;
                      credit.minute=givenminutes; 
                                
                      getTimeNow(&now); 
                      seconds_now=getSeconds();
                      generate_newalarm(&newalarm,&now,&credit);
                      setAlarmTime(newalarm.hour,newalarm.minute,seconds_now);
                      setAlarmDate(newalarm.day,newalarm.month,newalarm.year);
                      enableAlarm(RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val);
                                
                      eeprom.alarm.hour   = newalarm.hour;
                      eeprom.alarm.minute = newalarm.minute;
                      eeprom.alarm.day    = newalarm.day;
                      eeprom.alarm.month  = newalarm.month;
                      eeprom.alarm.year   = newalarm.year;
                      eeprom.now.hour   = now.hour;
                      eeprom.now.minute = now.minute;
                      eeprom.now.day    = now.day;
                      eeprom.now.month  = now.month;
                      eeprom.now.year   = now.year;
                                                  
                      eeprom.valid = true;
                      eeprom.seq   = 1;
                      eeprom.days  = givendays;
                      eeprom.minutes = givenminutes;
                      uart_puts_info("[write flash]");
                      flash_write((void *)&eeprom);
                      read_print_rtc(true);
                      play_song_success();
                      mosfet_switch(true);
                  }                                        
              } else 
#endif
              if(strlen(rxbuf) == 16) { // *12345678901234#
                  unsigned long givenid;
                  uint8_t givenseq;
                  uint16_t givendays,givendays_i;
                  int8_t givendays_int8;                  
                  char rxbuf_cpy[32];                
                  rxbuf[16]=0;  // strip '#'
                  strcpy(rxbuf_cpy,&rxbuf[1]); // skip '*'            
                          
                  // decode the code:
                  if(decode_rcode(rxbuf_cpy,&givenseq,&givenid,&givendays) > 0) {
                      sprintf(buf," [seq=%d days=%d id=%ld]",givenseq,givendays,givenid);
                      uart_puts_info(buf);            
                      if(givenid == get_myid()) {
                          uart_puts_info("[id match]");
                          if(givenseq > eeprom.seq || (givenseq==0 && eeprom.seq>95)) {
                              if(givendays > 0 && givendays < 1000) {
                                  Time now, credit, newalarm;
                                  givendays_i=givendays;
                                  while(givendays_i > 0) {
                                      getTimeNow(&now);        
                                      uart_puts_info("[setting alarm]");                            
                                      credit.year=0;
                                      credit.month=0;
                                      // days is an int8_t, so max 127.  127-31=96 is the max increment
                                      if(givendays_i > 96) {givendays_int8=96;} 
                                      else {givendays_int8=(int8_t)givendays_i;}
                                      givendays_i -= givendays_int8;                                
                                      credit.day=givendays_int8;                            
                                      credit.hour=0;                            
                                      credit.minute=0;                            
                                      generate_newalarm(&newalarm,&now,&credit);
                                      setAlarmTime(newalarm.hour,newalarm.minute,0);
                                      setAlarmDate(newalarm.day,newalarm.month,newalarm.year);
                                      enableAlarm(RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val);
                                      read_print_rtc(true);
                                  }        
                                  
                                  eeprom.alarm.hour   = newalarm.hour;
                                  eeprom.alarm.minute = newalarm.minute;
                                  eeprom.alarm.day    = newalarm.day;
                                  eeprom.alarm.month  = newalarm.month;
                                  eeprom.alarm.year   = newalarm.year;                            
                                  eeprom.now.hour   = now.hour;
                                  eeprom.now.minute = now.minute;
                                  eeprom.now.day    = now.day;
                                  eeprom.now.month  = now.month;
                                  eeprom.now.year   = now.year;                        
                              } else {  // if givendays==0 stay on forever.
                                  uart_puts_info("[disable alarm]");
                                  disableAlarm();
                              }
                              eeprom.valid = true;
                              eeprom.seq   = givenseq;
                              eeprom.days  = givendays;
                              eeprom.minutes = 0;
                              uart_puts_info("[write flash]");
                              flash_write((void *)&eeprom);
                              read_print_rtc(true);
                              play_song_success();
                              mosfet_switch(true);
                          } else {
                              uart_puts_info("[already used]");
                              cmdstat=-1;
                          }
                      } else {
                          uart_puts_info("[id not match]");
                          cmdstat=-1;
                      }
                  } else {
                      uart_puts_info("[decode error]");
                      cmdstat=-1;
                  }
              } else cmdstat=-1;
          }
          if(cmdstat < 0) {
              play_song_error();
          }
          memset(rxbuf,0,RXBUF_SIZE);
          usart_rx_ready=false;        
          print_prompt();
      }
      if (alarmTriggered) {   // If the alarm has been triggered
          alarmTriggered=false;
          uart_puts_info("Alarm! mosfet OFF!\n\r");
          play_song_expired(); 
          mosfet_switch(false);
          getTimeNow(&eeprom.now);
          uart_puts_info("[write flash]\r\n");
          flash_write((void *)&eeprom);
      }


      if((cnt%10)==0) {        
          uint8_t rtc_hour_new = getHours();
          
          uint8_t rtc_minute_new = getMinutes();        
          if(rtc_minutes_old != rtc_minute_new) {
              rtc_minutes_old = rtc_minute_new;
              if(read_print_rtc(true) == 0) { // 0 credit left? (last day)
              }
          }
          
          if(((rtc_hour_new%8)==0) && (rtc_hour_old != rtc_hour_new)) { // every 8 hours
              rtc_hour_old = rtc_hour_new;
              getTimeNow(&eeprom.now);
              uart_puts_info("[write flash]\r\n");
              flash_write((void *)&eeprom);
          } 
      }
  }  
  return 0;
}

// get rid of linker error when using linker flag --specs=nano.specs
void _sbrk(void) {}

