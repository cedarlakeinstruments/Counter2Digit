/*
 * CounterRGB.c
 *
 * Created: 4/9/2020 9:57:57 AM
 *  Author: lyndon
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
/* set the correct clock frequency based on fuse settings or external clock/crystal
 * has to be done before including delay.h */
#include <util/delay.h>
#include <avr/sleep.h>

#define PERIOD (100 / (F_CPU / 1000000UL))

// Segments

#ifdef BREADBOARD
	#define SEG_A (1 << PC0)
	#define SEG_B (1 << PC1)
	#define SEG_C (1 << PC2)
	#define SEG_D (1 << PC3)
	#define SEG_E (1 << PC4)
	#define SEG_F (1 << PC5)
	#define SEG_G (1 << PB0)
	// Cathodes
	#define UNITS_RED (1 << PD6)
	#define TENS_RED (1 << PD7)
	#define TENS_GRN (1 << PD5)
	#define UNITS_GRN (1 << PD4)
#else
	#define SEG_A (1 << PB1)
	#define SEG_B (1 << PC0)
	#define SEG_C (1 << PC1)
	#define SEG_D (1 << PC4)
	#define SEG_E (1 << PC5)
	#define SEG_F (1 << PC2)
	#define SEG_G (1 << PB0)
	#define DP	  (1 << PC3)
	// Cathodes
	#define UNITS_RED (1 << PD7)
	#define TENS_RED (1 << PD6)
	#define TENS_GRN (1 << PD5)
	#define UNITS_GRN (1 << PD4)
#endif

// Inputs
#define COUNT_UP (1 << PD3)
#define COUNT_DN (1 << PD2)
#define DEBOUNCE_COUNT 200
// Cathodes
#define COLOR_MASK 0x0F
#define COUNT_MASK  (COUNT_UP | COUNT_DN)

// Activity timeout
#define DIM_TIMEOUT 100000L
#define SHUTOFF_TIMEOUT 300000L

#define DIM_OFF_TIME 200

static const int QTR_SEC = 3906;
volatile char _qtrSecondTimeout=0;

void enableDigit(int d);
void showValue(char val, char color, char intensity);
void test(void);
void displayOff(void);
int count(char);
uint32_t _activityCount = 0;

ISR(INT0_vect)
{
	// No processing in ISR
}

ISR(INT1_vect)
{
	// No processing in ISR
}

int main(void)
{
	DDRC = 0xFF;
	DDRB = 3;	
	DDRD = UNITS_RED | TENS_RED | UNITS_GRN | TENS_GRN;
	
	// Pullups on input pins
	PORTD = COUNT_MASK;
	
	PORTC = 0;	
	PORTB = 0;
	
	// Low level on INT0, 1
	MCUCR |= MCUCR & 0xFC;
	GICR |= (1 << INT0) | (1 << INT1);
	
 	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	char color;
	int countVal = 0;
	
    while(1)
    {
		// Read switch inputs
		countVal = count(PIND & COUNT_MASK);
		color = (countVal == 1) ? 1 : 0;		
		_activityCount++;

		if (_activityCount >= SHUTOFF_TIMEOUT)
		{
			// Off
			displayOff();
		}
		else if (_activityCount >= DIM_TIMEOUT)
		{
			// Dim
			showValue(countVal, color, 0);
		}
		else
		{
			// Normal
			showValue(countVal, color, 1);
		}
	}
}

// Counts value at input
// Value must remain stable for DEBOUNCE #
// of calls before count is valid.
// Input is the bitmasked value of the switch
// input pins
int count(char input)
{
	static int debounce = DEBOUNCE_COUNT;
	static char lastReading = 0xFF;
	static int countVal = 0;
	static char validCount = 0;
	
	if (input == COUNT_MASK)
	{
		// Counted down to zero and released button
		if (countVal == 0 && validCount)
		{
			displayOff();
		}
		// Both idle, nothing to do
		debounce = DEBOUNCE_COUNT;
		validCount = 0;
	}
	else
	{
		// Must be a stable input reading and not waiting for switch release after last count
		if (lastReading == input && !validCount)
		{
			// Stable input
			if (--debounce == 0)
			{
				// Indicate that it was activated
				_activityCount = 0;
				// Reset counter
				debounce = DEBOUNCE_COUNT;
				// Input was stable long enough, let's see what it is
				switch (input)
				{
					// Count down pin low
					case COUNT_UP:
						validCount = 1;
						if (countVal >= 1)
						{
							countVal--;
						}			
						break;
					// Count up pin low
					case COUNT_DN:
						if (countVal < 99)
						{
							countVal++;
						}							
						validCount = 1;
						break;
					default:
						break;
				}
			}
		}	
		else
		{
			// Reset debounce
			debounce = DEBOUNCE_COUNT;
		}	
	}
	lastReading = input;
	return countVal;
}

// Draw a single digit
void enableDigit(int d)
{
	char portc = PORTC & 0x40;
	char portb = PORTB & 0xFC;
	
	switch(d)
	{
		case 0:
			portc |= (SEG_B|SEG_C|SEG_D|SEG_E|SEG_F);
			portb |= SEG_A;
			break;
		case 1:
			portc |= (SEG_B | SEG_C);
			break;
		case 2:
			portc |= (SEG_B | SEG_D | SEG_E);
			portb |= (SEG_A | SEG_G);
			break;
		case 3:
			portc |= SEG_B | SEG_C | SEG_D;
			portb |= SEG_A | SEG_G;
			break;		
		case 4:
			portc |= (SEG_B | SEG_C | SEG_F);
			portb |= SEG_G;
			break;
		case 5:
			portc |= (SEG_C | SEG_D | SEG_F);
			portb |= (SEG_A | SEG_G);
			break;
		case 6:
			portc |= (SEG_F | SEG_C | SEG_D | SEG_E);
			portb |= (SEG_A | SEG_G);	
			break;
		case 7:
			portc |= (SEG_B | SEG_C);
			portb |= SEG_A;
			break;
		case 8:
			portc |= (SEG_B|SEG_C|SEG_D|SEG_E|SEG_F);
			portb |= (SEG_A | SEG_G);
			break;
		case 9:
			portc |= (SEG_B|SEG_C|SEG_D|SEG_F);
			portb |= (SEG_A | SEG_G);
			break;				
		default:
			break;
	}	
	// Set high values
	PORTC = portc;
	PORTB = portb;
}

// Display two digit value
// Color:0 - red, 1 - green
void showValue(char val, char color, char intensity)
{
	val = val < 99 ? val : 99;
	char units = val % 10;
	char tens = val/10;
	
	// Overall PWM period is 60Hz
	const int onTime = 50;
	
	// Units:
	enableDigit(units);
	PORTD = (PORTD & COLOR_MASK) | (color ? UNITS_GRN : UNITS_RED);
	_delay_us(onTime);
	// Clear
	PORTD = PORTD & COLOR_MASK;
	if (intensity == 0)
	{
		_delay_us(DIM_OFF_TIME);
	}
	if (val < 10)
	{
		// Compensate for other digit off
		_delay_us(onTime);
		if (intensity == 0)
		{
			_delay_us(DIM_OFF_TIME);
		}			
		return;
	}
	
	// Tens
	enableDigit(tens);
	PORTD = (PORTD & COLOR_MASK) | (color ? TENS_GRN : TENS_RED);
	_delay_us(onTime);

	// Clear
	PORTD = PORTD & COLOR_MASK;	
	if (intensity == 0)
	{
		_delay_us(DIM_OFF_TIME);
	}
}

// Blank display and power down
void displayOff(void)
{
	_activityCount = 0;
	PORTD = (PORTD & COLOR_MASK);
	sei();
	// Put processor to sleep. Will wake up when switch pressed
	sleep_enable();
	sleep_cpu();
	// Shut off interrupts after waking up
	cli();
	_delay_ms(500);
}

// Cycle through some digits
void test(void)
{
	char values[] = {0, 1,2,3,4,5,6,7,8,9};
	for (int i=0; i < 10; i++)
	{
		showValue(values[i], 0, 200);
		_delay_ms(1000);
	}
}