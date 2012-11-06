
#include <util/delay.h>
#include <avr/interrupt.h>
#include "globals.h"
#include "adc.h"
#include "current.h"
#include "serial.h"

#define LOOP_DELAY 100

uint16_t redadc, greenadc, blueadc;

void init_timer0(void);

/* Interrupt vectors */

// Timer0 overflow - Every 32us
ISR(SIG_OVERFLOW0)
{
	adc_loop();
	serial_tick();
}

// ADC reading ready
ISR(SIG_ADC)
{
	/* Note: it's important to read ADCL first and then ADCH */
	uint16_t adc = ADCL;
	adc |= ADCH << 8;

	process_adc_reading(adc);
}

// Serial buffer overflow (1 byte received)
ISR(SIG_USI_OVERFLOW)
{
	serial_rx_byte(USIDR);
	
	// Clear the interrupt
	USISR = (1 << USIOIF);
}

int main(void)
{
	/* Use the hardware (Timer1) to generate a fast (125kHz) pwm
	 * that will drive the buck converter on/off.
	 * 
	 * Initialize the PWM value *before*.
	 */
	redcpwm = 0x00;
	greencpwm = 0x00;
	bluecpwm= 0xFF; /* inverted */
	init_current_loop();
	
	/* Initializes the ADC */
	init_adc();
	
	/* Run a much slower (122Hz) PWM (based on Timer0 interrupts) */
	init_timer0();
	
	/* Initializes the serial port and prepare to receive data */
	init_serial();

	/* Enable interrupts and let the show begin! */
	sei();

	while(1) {
	}
}

/*
 * Timer0 is used to generate software interrupts that we use for different purpose.
 * 
 * This counter counts @8Mhz and will overflow every 256 ticks so the interrupt will be
 * called every: t = 1/8Mhz * 256 = 32uS
 */
void init_timer0()
{
	/* Enable counter with no prescaling (speed will be 8Mhz) */
	TCCR0B = (1 << CS00);
	
	/* Enable interrupt when an overflow occurs */
	TIMSK |= (1 << TOIE0);
}
