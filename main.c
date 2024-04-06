/*
 * File:   main.c
 * Author: Brett
 *
 * Created on January 4, 2024, 2:07 PM
 */



#include <pic18f2221.h>

#include "config.h"

void system_init(void);
void delay(unsigned long milliseconds);

void keypad_init(void);
char keypress(char current_row);

void lcd_init(void);
void i2c_start_and_addr(unsigned char address);
void lcd_clear(void);
void i2c_data_tx(char data);
void i2c_stop(void);
void lcd_message(char *message);
void lcd_char(char letter);
void lcd_command(char data, char lt, char rw, char rs);
void lcd_move_cursor(char line, char position);
void lcd_backspace(void);

void send_message(void);
void packet_pre_post_amble(void);
void packet_interim_amble(void);
void send(char data);

char message_length = 0;
unsigned char key_pressed = 0;
unsigned char keys[MAX_MESSAGE + 1];

const unsigned char keypad_array[4][4] = {'1', '2', '3', 'A',
                                          '4', '5', '6', 'B',
                                          '7', '8', '9', 'C',
                                          '*', '0', '#', 'D'};


void main(void) 
{
    
    system_init();
    
    lcd_init();
    keypad_init();
    
    
    while(1)
    {
       
        if(key_pressed && keys[message_length-1] != '*' && keys[message_length-1] != 'D' && keys[message_length-1] != '#' && keys[message_length-1] != 'A')
        {
            lcd_message(keys + message_length-1);
            
            key_pressed = 0;
        }
        if(key_pressed && keys[message_length-1] == '*' && message_length > 1)
        {
            lcd_backspace();
            keys[message_length - 1] = '\0';
            keys[message_length - 2] = '\0';
            message_length -= 2;
            key_pressed = 0;
        }
        if(key_pressed && keys[message_length-1] == 'D')
        {
            lcd_clear();
            
            for(int i = 0; i < message_length; i++)
            {
                keys[i] = '\0';
            }
            
            message_length = 0;
            key_pressed = 0;
            
        }
        if(key_pressed && keys[message_length-1] == 'A')
        {
            //lcd_clear();
            
            for(int i = 0; i < 6; i++)
            {
                keys[i] = '0' + i;
            }
            lcd_message(keys + message_length-1);
            
            message_length = 6;
            key_pressed = 0;
            
        }

        if(key_pressed && keys[message_length-1] == '#')
        {
            lcd_move_cursor(1,0);
            lcd_message("Sending");
            keys[message_length - 1] = '\0';
            send_message();
            lcd_clear();
            for(int i = 0; i < message_length; i++)
            {
                keys[i] = '\0';
            }
            key_pressed = 0;
            message_length = 0;
        }
        Sleep();
        
        
         
    }

    return;
}

void __interrupt() ISR()
{
    if(INTCONbits.TMR0IF == 1)
    {
        //timer and variable init
        INTCONbits.TMR0IF = 0;
        TMR0H = 0x0B;
        TMR0L = 0xDD;
        char is_key_pressed = 0;
        char current_row = 0;
        
        //continues till pressed key is found or runs out of rows to scan
        while(!is_key_pressed && current_row < 4)
        {
            is_key_pressed = keypress(current_row);
            current_row++;
        }
        if(is_key_pressed && message_length < (MAX_MESSAGE) || (is_key_pressed && current_row == 4 && (is_key_pressed == 1 || is_key_pressed == 3 || is_key_pressed == 4)))
        {
            keys[message_length] = keypad_array[--current_row][--is_key_pressed];
            key_pressed = 1;
            message_length++;
        }
        return;
    
    }
    return;
}

void delay(unsigned long milliseconds)
{
    T1CON = 0b10110000;
    PIE1bits.TMR1IE = 0;
    unsigned long long timer_value = (milliseconds) * 1000 ;
    while(timer_value > 65535)
    {
        timer_value -= 65535;
        TMR1H = 0;
        TMR1L = 0;
        T1CONbits.TMR1ON = 1;
        while(!PIR1bits.TMR1IF);
        T1CONbits.TMR1ON = 0;
        PIR1bits.TMR1IF = 0;
    }
    timer_value = 65535 - timer_value + 1;
    TMR1H = (unsigned char)(0xFF00 & timer_value);
    TMR1L = (unsigned char)(0xFF & timer_value);
    T1CONbits.TMR1ON = 1;
    while(!PIR1bits.TMR1IF);
    T1CONbits.TMR1ON = 0;
    PIR1bits.TMR1IF = 0;
}

void send_message()
{
    int i = 0;
    int send_cnt = 0;
    packet_pre_post_amble();
    
    for(int j = 0; j < 7; j++)
    {
        while(keys[i] != '\0')
        {
            send(keys[i] & 0x0F);
            send((keys[i] & 0xF0)>> 4);
            i++;
        }
        packet_interim_amble();
        i = 0;
    }
    packet_pre_post_amble();
}

void packet_pre_post_amble()
{
    for(int i = 0; i < 3; i++)
    {
        send(0xA);
        send(0xA);
    }
}
void packet_interim_amble()
{
    for(int i = 0; i < 3; i++)
    {
        send(0xB);
        send(0xB);
    }
}

void send(char data)
{
    LATB = (LATB & 0xF0) | data;
    LATCbits.LC7 = 0;
    delay(70);
    LATCbits.LC7 = 1;
    delay(70);
}

void lcd_init()
{
    //i2c module data format: P7-P0: 0b 7 6 5 4 lt E Rw Rs
    //                                  0 0 0 0  0 0  0  0
    
    
    //sets lcd into 4 bit mode
    lcd_command(0x02, 1, 0, 0);
    lcd_command(0x28, 1, 0, 0);
    
    lcd_clear();
    
    //turns cursor on
    lcd_command(0x0F, 1, 0, 0);
    
    
    
    //prints out capstone splash screen
    lcd_move_cursor(0, 5);
    lcd_message("Senior");
    lcd_move_cursor(1, 4);
    lcd_message("Capstone");
    //delay(500);
    lcd_clear();
    //lcd_message("By Aidan, Brett,");
    //lcd_move_cursor(1, 0);
    //lcd_message("Chris, and Gabe");
    //delay(500);
    //lcd_clear();
    
    
    
}

void lcd_backspace()
{
    lcd_command(0x10, 1, 0, 0);
    lcd_message(" ");
    lcd_command(0x10, 1, 0, 0);
}

void lcd_move_cursor(char line, char position)
{
    if(!line)
    {
        lcd_command(0x80 | position, 1, 0, 0);
    }
    else
    {
        lcd_command(0xC0 | position, 1, 0, 0);
    }
    
    
}

void lcd_command(char data, char lt, char rw, char rs)
{
    i2c_start_and_addr(LCD_ADDR);
    
    i2c_data_tx((data & 0xF0) | (lt << 3) | 4 | (rw <<1) | rs);
    delay(1);
    i2c_data_tx(lt<<3);
    
    
    i2c_data_tx(((data & 0x0F) <<4) | (lt << 3) | 4 | (rw <<1) | rs);
    delay(1);
    i2c_data_tx(lt<<3);
    
    i2c_stop();
}

void lcd_message(char* message)
{
    i2c_start_and_addr(LCD_ADDR);
    while(*message != 0)
    {
        lcd_char(*message);
        message++;
    }
    i2c_stop();
}

void lcd_char(char letter)
{
    i2c_data_tx((letter & 0xF0) | 0b1101);
    i2c_data_tx((letter & 0xF0) | 8);
    
    
    
    i2c_data_tx(((letter & 0x0F) << 4) | 0b1101);
    i2c_data_tx(((letter & 0x0F) << 4) | 8);
    delay(1);
}

void lcd_clear()
{
    i2c_start_and_addr(LCD_ADDR);
    
    //clears lcd
    i2c_data_tx(0b00001100);
    delay(1);
    i2c_data_tx(0b00001000);
    
    
    i2c_data_tx(0b00011100);
    delay(1);
    i2c_data_tx(0b00011000);
    delay(1);

    i2c_stop();
}

void i2c_data_tx(char data)
{
    do
    {
        SSPBUF = data;
        while(!PIR1bits.SSPIF);
        PIR1bits.SSPIF = 0;
        
    }while(SSPCON2bits.ACKSTAT);
}

void i2c_stop()
{
   //stops i2c transmission
    SSPCON2bits.PEN = 1;
    while(!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0; 
}

void i2c_start_and_addr(unsigned char address)
{
    //start condition and addressing the i2c
    SSPCON2bits.SEN = 1;
    while(!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;
    do
    {
        SSPBUF = address;
        while(!PIR1bits.SSPIF);
        PIR1bits.SSPIF = 0;
    }while(SSPCON2bits.ACKSTAT);
    
}

//returns column number pressed
char keypress(char current_row)
{
    //turns current row that's being scanned on
    LATA = (unsigned char)(1 << current_row) | LATA;
    char col_key_press = 0;
    char col_num = 0;
    
    while(col_num < 4 && !col_key_press)
    {
        if((128 >> col_num) & PORTB)
        {
            T3CON = 0b10010000;
            PIE2bits.TMR3IE = 0;
            TMR3H = 0x63;
            TMR3L = 0xC1;
            PIR2bits.TMR3IF = 0;
        
            T3CONbits.TMR3ON = 1;
        
            while(!PIR2bits.TMR3IF);
            T3CONbits.TMR3ON = 0;
            PIR2bits.TMR3IF = 0;
        
            if((128 >> col_num) & PORTB)
            {
                while((128 >> col_num) & PORTB);
                col_key_press = col_num + 1;
            }
        }
        col_num++;
    }
    
    //turns off current row that was scanned
    LATA = (unsigned char)(1 << current_row) ^ LATA;
    return col_key_press;
    
}


void system_init()
{
    //sets internal clock and pll to 32MHz
    OSCCONbits.SCS = 0;
    OSCCONbits.IDLEN = 1;
    OSCCONbits.IRCF = 7;
    OSCTUNEbits.PLLEN = 1;
    
    //enables interrupts and priorities
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    
    INTCON2bits.RBPU = 0;
    
    
    //change to one if we were to want priority levels
    RCONbits.IPEN = 0;
    
    //transmitter init
    LATCbits.LC7 = 1;
    TRISCbits.RC7 = 0;
    TRISB = TRISB & 0xF0;
    
    //i2c init
    SSPSTATbits.SMP = 1;
    SSPSTATbits.CKE = 0;
    SSPADD = 0x50;
    SSPCON1bits.SSPEN = 1;
    SSPCON1bits.SSPM = 0b1000;
    TRISCbits.RC3 = 1;
    TRISCbits.RC4 = 1;
    
    //puts null at end of key array for safety
    keys[MAX_MESSAGE] = '\0';
}

void keypad_init()
{
    ADCON1bits.PCFG = 15;
    TRISA = TRISA & 0xF0; 
    TRISB = (TRISB & 0xFF) | 0xF0;
    
    LATA = LATA & 0b11110000;
    PORTB = PORTB & 0b00001111;
    
    //Enables timer0 for interrupt every .25 seconds
    T0CON = 0b00000100;
    INTCONbits.TMR0IF = 0;    
    TMR0H = 0x0B;
    TMR0L = 0xDD;
    INTCONbits.TMR0IE = 1;
    T0CONbits.TMR0ON = 1;
}