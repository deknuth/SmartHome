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

#define LED_ON	PORTD|=(1<<5)
#define LED_OFF	PORTD&=~(1<<5)
#define LED_BLINK	PORTD^=(1<<5)

unsigned char history[3] = {0};
void PortInit(void) 
{
	DDRB =  0B00000110;
	PORTB = 0B00000000;
	PINB =	0x00;

	DDRC =  0B00100001;		// PC0(K1),PC1(LED1),PC2(DV),PC3(K1_IN),PC4(K0_IN)
	PORTC = 0B00000000;
	PINC =	0x00;
	
	DDRD =  0B00100000;
	PORTD = 0B00000000;
	PIND =	0x00;
}

void A7139Send(unsigned char *buf,unsigned char len)
{
    A7139_WriteFIFO(buf,len);
    StrobeCmd(CMD_TX);
    _delay_us(10);
    while(GIO2);
}

volatile unsigned char sFlag = 0;

unsigned char RecvData(unsigned char *buf)
{
    StrobeCmd(CMD_RX);
    _delay_us(10);
    while(GIO2)
	{
		if(sFlag==0)
			return 0;
	}
    return A7139_ReadFIFO(buf);
}

unsigned char TouchFilter(void)
{
	_delay_ms(40);
	if(PINC!=0x1E)
		return (PINC);
	else
		return 0;
}

void BLink(unsigned char times)
{
	int i = 0;
	for(i=0; i<(times<<1); i++)
	{
		LED_BLINK;
		_delay_ms(200);
	}
	LED_OFF;
}

int main(void)
{
	unsigned char value = 0;
	unsigned char tmp = 0;
	unsigned char tBuf[5] = {0xfe,0x00,0x01,0x02,0xFF};
	PortInit();
	do
    {
        tmp = A7139_Init(470.001f); 		// A7139初始化
        _delay_ms(100);
    }while(tmp);
    A7139_SetPowerLevel(8);		// 功率设置函数，参数范围0-8，0最小功率，8功率最大
    A7139_SetPackLen(16);
    StrobeCmd(CMD_STBY);
	BLink(3);
	while(1)
	{
		if((value = TouchFilter()) > 0)
		{
			switch(value)
			{
				case 0x1C:
					BLink(1);
					A7139Send(tBuf,5);
					break;
				case 0x1A:
					A7139Send(tBuf,5);
					BLink(2);
					break;
				case 0x16:
					BLink(3);
					break;
				case 0x0E:
					BLink(4);
					break;
				default:
					break;
			}
			while(1)
			{
				if(PINC == 0x1E)		// hand off
					break;
			}
		}
	}
}
