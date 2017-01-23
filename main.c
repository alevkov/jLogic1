#include <msp430.h>

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

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	P1DIR &= ~ALL_BUTTONS;
	P2DIR |= ALL_LED;
	P1IE |= ALL_BUTTONS;
	P1IES |= ALL_BUTTONS;
	P1REN |= ALL_BUTTONS;
	P1IFG &= ~ALL_BUTTONS;
	P2OUT &= 0x0;
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
	for(;;) {  } // LPM3 with interrupt
	return 0;
}

// Port 1 interrupt service routine
// Compiler-independent
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
#else
#error Compiler not supported!
#endif
{
	P1IE &= ~ALL_BUTTONS;
	P2OUT &= 0x0;
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
	P1IE |= ALL_BUTTONS; // re-enable IR on port 1
}
