#include <msp430.h>

/* timer */

#define TRIGGER 1
#define DEBOUNCE_LIMIT 1500

/* buttons */
#define UP 0x80 // 1.7
#define BUTTON_A 0x04 // 1.5
#define BUTTON_B 0x08 // 1.4
#define BUTTON_C 0x10 // 1.3
#define BUTTON_D 0x20 // 1.2
#define DOWN 0x02 // 1.1
#define ALL_BUTTONS 0x00BE

/* LEDs */
#define ALL_LED 0x0F // 2.0-2.3

static volatile unsigned int display_num = 0;
static volatile unsigned int ir_interval = 0;

void initDebounceTimer()
{
  BCSCTL3 |= LFXT1S_2; // VLO
  TACCTL0 |= CCIE;
  TACCR0 = TRIGGER;
  TACTL |= TASSEL_1; // ACLK
  TACTL |= MC_1; // up mode
}

void initPins()
{
  P1DIR &= ~ALL_BUTTONS;
  P2DIR |= ALL_LED;
  P1IE |= ALL_BUTTONS;
  P1IES |= ALL_BUTTONS;
  P1REN |= ALL_BUTTONS;
  P1IFG &= ~ALL_BUTTONS;
  P2OUT &= 0x0;
}

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
  initPins();
  initDebounceTimer();
	// visual startup sequence
	volatile int i = 0;
	for (; i <= 0x0F; i++)
	{
		P2OUT &= 0x0;
		P2OUT |= i;
		__delay_cycles(0x16000);
	}
	P2OUT &= 0x0;
  _bis_SR_register(LPM3_bits + GIE);
	for(;;) { } // LPM3 with interrupt
	return 0;
}

/*
 *  Port 1 interrupt handler
 */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
#else
#error Compiler not supported!
#endif
{
	P2OUT &= 0x0;
  ir_interval = 0;
  switch (P1IFG)
  {
    case UP: // increment
      if (display_num == 0x0F) display_num = 0x0;
      else ++display_num;
      break;
    case DOWN: // decrement
      if (display_num == 0x0) display_num = 0x0F;
      else --display_num;
      break;
    default: // toggle a, b, c, d
      display_num ^= ((P1IFG >> 0x02) & 0x0F); // A-D starting from 1.2
      break;
  }
	P2OUT |= display_num;
	P1IFG &= ~ALL_BUTTONS; // clear IR flags on all pins
}

/*
 * Timer A Interrupt Handler
 * The timer will be used as a debouncing mechanism
 * It will measure the interval between interrupts on Port 1
 * And detect if they happen too frequently
 */
 #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
 #pragma vector=TIMER0_A0_VECTOR
 __interrupt void Timer_A0(void)
 #elif defined(__GNUC__)
 void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer_A0 (void)
 #else
 #error Compiler not supported!
 #endif
{
  ir_interval++;
  if (ir_interval < 0x0)
	  ir_interval = 0;
  // if interrupts are happening within a very short interval
  // prevent addition interrupts
  if (ir_interval < DEBOUNCE_LIMIT)
  {
	  P1IFG &= ~ALL_BUTTONS; // disable all interrupts until timer catches up
	  P1IE &= ~ALL_BUTTONS;
  }
  else
    P1IE |= ALL_BUTTONS;
}
