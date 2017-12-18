#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <math.h>
#include <util/delay.h>
#include <util/twi.h>
#include "main.h"

#define DD_MOSI 5
#define DD_SCK	7
#define	DDR_SPI	DDRB

#define A_LED_ON	PORTC|=(1<<1)
#define A_LED_OFF	PORTC&=~(1<<1)

#define B_LED_ON	PORTD|=(1<<4)
#define B_LED_OFF	PORTD&=~(1<<4)

#define T_RST_H	PORTA|=(1<<3)
#define T_RST_L	PORTA&=~(1<<3)
#define T_CK_H	PORTC|=1
#define T_CK_L	PORTC&=~1
#define T_DV	0x04
#define T_SDO	0x02
#define T_KEY_A	0x08
#define T_KEY_B	0x10
#define T_KEY_AB 0x18
unsigned char history[3] = {0};
void PortInit(void) 
{
	DDRB =  0B00000110;
	PORTB = 0B00000000;
	PINB =	0x00;

	DDRC =  0B00000011;		// PC0(K1),PC1(LED1),PC2(DV),PC3(K1_IN),PC4(K0_IN)
	PORTC = 0B00000000;
	PINC =	0x00;
	
	DDRD =  0B00010000;		// PD4(KEY1)
	PORTD = 0B00000000;
	PIND =	0x00;
}

void EI0Init(void)
{
    MCUCR = 0;
    EIMSK = 1<<INT1;
}


void SendData(unsigned char *buf,unsigned char len)
{
    A7139_WriteFIFO(buf,len);
    StrobeCmd(CMD_TX);
    _delay_us(10);
    while(GIO2);
}

unsigned char RecvData(unsigned char* buf)
{
    StrobeCmd(CMD_RX);
    _delay_us(10);
    while(GIO2);
    return A7139_ReadFIFO(buf);
}

unsigned char TouchFilter(void)
{
	_delay_ms(40);
	if(PINC&0x18)
		return (PINC&0x18);
	else
		return 0;
}

void BLink(unsigned char times)
{
	int i = 0;
	for(i=0;i<times;i++)
	{
		A_LED_ON;
		_delay_ms(400);
		A_LED_OFF;
		_delay_ms(400);
	}
}

volatile unsigned char len = 0;
volatile unsigned char flag = 0;
unsigned char rBuf[16] = {0};

ISR(INT1_vect)		// GIO1 中断
{
    len = RecvData(rBuf);
    flag = 1;
}

int main(void)
{
	unsigned char value = 0;
	unsigned char tmp = 0;
	cli();
	PortInit();
	EI0Init();
    do
    {
        tmp = A7139_Init(470.001f); 		// A7139初始化
        _delay_ms(100);
    }while(tmp);
    A7139_SetPowerLevel(6);		// 功率设置函数，参数范围0-8，0最小功率，8功率最大
    A7139_SetPackLen(16);
    A7139_WOR_ByPreamble();
    sei();
    StrobeCmd(CMD_RX);
	while(1)
	{
		if(flag)
        {
            if(len==5)
            {
				;
			}
			A_LED_ON;
			flag = 0;
            A7139_WOR_ByPreamble();
            StrobeCmd(CMD_RX);
		}
	
		if(PINC&T_DV)
			;
		else
		{
			if((value = TouchFilter()) > 0)
			{
				switch(value)
				{
					case T_KEY_A:
						if(history[0])
						{
							A_LED_ON;
							history[0] = 0;
						}
						else
						{
							A_LED_OFF;
							history[0] = 1;
						}
						break;
					case T_KEY_B:
						if(history[0])
						{
							B_LED_ON;
							history[0] = 0;
						}
						else
						{
							B_LED_OFF;
							history[0] = 1;
						}
						break;
					case T_KEY_AB:
						if(history[0])
						{
							A_LED_ON;
							history[0] = 0;
						}
						else
						{
							A_LED_OFF;
							history[0] = 1;
						}

						if(history[0])
						{
							B_LED_ON;
							history[0] = 0;
						}
						else
						{
							B_LED_OFF;
							history[0] = 1;
						}
						break;
					default:
						break;
				}
				while(1)
				{
					if((PINC&0x18) == 0)		// hand off
						break;
				}
			}
		}
	}
}
