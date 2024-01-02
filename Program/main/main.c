/*
 * dcmotor.c
 *
 * Created: 11/22/2023 11:07:44 AM
 * Author : boybr
 */ 


#define F_CPU 8000000UL // change to 8Mhz
//#define F_CPU	16000000UL
#define sw (PINB&(1<<PB1))

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <math.h>

// LCD_I2C
#include "i2c.h"
#include "LCD_I2C.h"


// Some const
const unsigned char encoeder_res = 20;
const unsigned char sampling_time = 25; // 25ms

char dc[4] = {64,128,191,255}; // 25%, 50%, 75%, 100%
unsigned char i = 0;

volatile long int Pulses = 0, pre_Pulses = 0;
volatile unsigned char samples_count = 0;

char speed = 0;

int main(void)
{
	// Port Config
	DDRB &= ~(1<<PB1);				// PB1 input for direction control 
	PORTB |= (1<<1);				// Activate Pull up -> keep it high
	DDRA = 0xFF;
	DDRC = 0xff;					// Port C as output
	//PORTA = 0xff;
	
	// Phase correct PWM mode programming of Timer0
	DDRB |= (1<<PB3);				// Make PB3 output pin for pulse
	OCR0 = dc[i];					// for 25% duty cycle in non inverted phase correct PWM
	TCCR0 = 0x61;					// Phase correct PWM, Non inverted 15.686 khz, 8Mhz crystal, so N = 1
	
	// External interrupt INT0 for speed control 
	DDRD &= ~(1<<PD2);				// Make PD2 an input pin (INT0 pin)
	PORTD |= (1<<PD2);				// Activate pull up -> keep it high
	MCUCSR = 0x02;					// Make interrupt0 negative edge triggered
	GICR = (1<<INT0);				// Enable external interrupt0
	sei();							// enable interrupts
	
	// External interrupt INT2 for counting Pulses
	MCUCSR &= ~(0<<ISC2);			// falling-edge triggered
	GICR |= (1<<INT2);				// Enable INT2

	// Using Timer2 to create sampling time
	TCCR2 = 0x07;					// Prescaler = 1024
	TCNT2 = 60;						// count for 25ms
	TIMSK |= (1<<TOIE2);			// Enable overflow interrupt of Timer2 
	
	// Initializing LCD
	i2c_init();
	i2c_start();
	i2c_write(0x70);
	lcd_init();
    while (1) 
    {
		// Direction
		if(sw == 0)					// if switch is pushed -> change direction
		    PORTA = 0x01; 
		else                        // switch not pushed
			PORTA = 0x02;			
		OCR0 = dc[i];
		
		/* LCD example 
		lcd_cmd(0x80);
		lcd_msg("   CodE BlacK");
		_delay_ms(1000);
		lcd_cmd(0xC0);
		_delay_ms(1000);
		lcd_msg("We are Using ");
		lcd_msg("I2");
		lcd_msg("C");
		_delay_ms(1000);
		lcd_cmd(0x01);
		_delay_ms(1000);
		*/
		
		/*
		
		if(samples_count >= 10) // display every 250ms
		{
			// Compute the Motor's speed
			speed = (Pulses/encoeder_res)/(sampling_time*samples_count)/1000;
			// Reset samples_count and Pulses count
			samples_count = 0;
			Pulses = 0;
		}
		*/ 
		
		lcd_cmd(0x80); // force cursor to the beginning of the 1st line
		if(i==0)
		{
			lcd_msg("D.C: 25%");
		}
		else if (i==1)
		{
			lcd_msg("D.C: 50%");
		}
		else if (i == 2)
		{
			lcd_msg("D.C: 75%");
		}
		else
		{
			lcd_msg("D.C: 100%");
		}
		//lcd_msg(speed); // display input dc
		lcd_cmd(0xC0); // force cursor to the beginning of the 2nd line 
		lcd_msg("Speed: ");
	}
	return 0;
}


// Interrupt Service Routine
ISR(INT0_vect)
{
	i++;
	if(i>3) i = 0;
}

// Samples count
ISR(TIMER2_OVF_vect)
{
	TCNT2 = 60; 
	samples_count++;
}

// Pulses count
ISR(INT2_vect)
{
	  Pulses++;
}