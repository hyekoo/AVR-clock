#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#include "lcd.c"

#define CPU_CLOCK_KHZ	16000UL
#define PRESCALE	256UL

#define MIN_POS		3
#define BLINK_POS	(MIN_POS+2)
#define TEMP_POS		2

char bit_data[41];
char clock[20]; 
volatile int done=0, toggle=0, time=0;
volatile int check1=0, check2=0;
volatile int cnt=0;

static volatile unsigned short timer;
static unsigned char hour=0, min=0, sec=0;

ISR(TIMER1_OVF_vect)
{
	static unsigned char blink_on=0;

	
	TCNT1 = timer;

	if(blink_on)
	{
		sec++;
		if(sec>=60)
		{
			sec=0;
			min++;
		} 
		else if(min>=60) 
		{
			min=0;
			hour++;
		}
		else if(hour>=24)
		{
			hour=0;
			min=0;
		}
		sprintf(clock, "%2d:%2d.%2d", (unsigned int)hour, (unsigned int)min, (unsigned int)sec);
		LcdMove(0, MIN_POS);
		LcdPuts(clock);
	}
	else 
	{
		LcdMove(0, BLINK_POS);
		LcdPuts(" ");
	}
	blink_on = !blink_on;
	LcdMove(1, TEMP_POS);
}

void send() {
	DDRE = 0xFF;
	PORTE = 0xFF;
	_delay_ms(100);
	PORTE = 0x00;
	_delay_ms(18);
	PORTE = 0xFF;
	_delay_us(30);
	PORTE = 0x00;
	DDRE = 0x00;

	EICRB = 0b00000011;
	EIMSK = 0b00010000;
	cnt = 0;
}

ISR(TIMER3_OVF_vect) 
{
	ETIMSK = 0x04;
	EIMSK = 0b00000000;
	done = 1;
}

ISR(INT4_vect) {

	if(toggle==0) {
		ETIMSK = 0x04;
		TCNT3 = 0;
		EICRB = 0b00000010;
		check1 += 1;
		toggle = 1;
	} else if(toggle==1) {
		ETIMSK = 0x04;
		EICRB = 0b00000011;
		time = TCNT3;
		check2 += 1;
		if(time>=100*8) bit_data[cnt++] = '1';
		else if(time<100*8) bit_data[cnt++] = '0';
		toggle = 0;
	}
	
}

void Reset() {
	
	if( ~PIND & 0x02 )  sec = 0;

	_delay_ms(20);
	while(~PIND & 0x02);
	_delay_ms(20);

}

void SetHour() { 	
	
	if( ~PIND & 0x04 ) {
		if( hour >= 24 ) hour = 0; else hour++;
	}	
	
	_delay_ms(20);
	while(~PIND & 0x04);
	_delay_ms(20);	

}

void SetMin() {
	
	if( ~PIND & 0x08 ) { 
		if( min >= 60 ) { min = 0; hour++; } else min++;
	}

	_delay_ms(20);
	while( ~PIND & 0x08 );
	_delay_ms(20);
}

int main()
{
    
	unsigned short icount,sw;
	unsigned short ts=500;
	int i, temp=0, humi=0;
	char temphumi[13];

	icount = (unsigned short)(ts*CPU_CLOCK_KHZ/PRESCALE);
	timer = 0xFFFF - icount + 1;

	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TCNT1 = timer;

	TIMSK = (1<<TOIE1);

	TCCR1B |= (4<<CS10);

	LcdInit();

	TCCR3A = 0x00;
	TCCR3B = 0x01;

	DDRD = 0x00;
	PORTD = 0xFF;

	EICRA = 0b00100000;
	EIMSK = 0b00000100;

	SREG = 0x80;

	while(1) {

		_delay_ms(100);

		send();
		while(done==0);
		done = 0;

		humi = 0;
		temp = 0;

		for( i=0; i<8; i++ ) 
			if( bit_data[i+1] == '1' ) humi += 1<<(7-i);
		
		for( i=0; i<8; i++ )
			if( bit_data[i+16] == '1' ) temp += 1<<(7-i);

		sprintf(temphumi, "T:%3dC H:%3dp", temp, humi);
		LcdMove(1, TEMP_POS);
		LcdPuts(temphumi);

		Reset();
		SetHour();
		SetMin();

	}

}


