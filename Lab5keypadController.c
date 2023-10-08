/*====================================================================*/
/* Courtney Stewart                                                   */
/* ELEC3050 Lab 5                                                     */
/* The infamous keypad lab																					  */
/* Using the microcontroller to control the peripheral device 				*/
/* (the keypad) and using an interrupt-driven algorithm.							*/
/* Main displays a 4-bit number that is either the output of the 		  */
/* counter or the value of the key in hexadecimal.                    */
/*====================================================================*/

#include "stm32l4xx.h" /*microcontroller info */
#include <math.h>
static unsigned char counter = 0; /*keeps track of decade counter value */
static unsigned char key = 0; /* keeps track of the value of the key button pressed */
static unsigned char displayTheKey = 0; /* if =1 key value should be shown, else counter should be shown */

void delay(void);
void smallDelay(void);



/* sets up GPIO pins for proper input or output */
void GPIOPinSetup(){
	RCC->AHB2ENR |= 0x03;										 /* Enable GPIOA clock (bit 0) */
	
	/* PA[5:2] input pull up */
	GPIOA->MODER &= ~(0xFF << 4);
	GPIOA->PUPDR &= ~(0xFF << 4);
	GPIOA->PUPDR |= (0x55 << 4);
	
	/* PA[11:8] outputs default low */
	GPIOA->MODER &= ~(0xFF << 16);
	GPIOA->MODER |= (0x55 << 16);
	GPIOA->BRR |= 0xF << 8;							/* set PA[11:8] to 0 */
	
	/* PB[6:3] outputs for display */
	GPIOB->MODER &= ~(0xFF << 6);
	GPIOB->MODER |= (0x55 << 6);

}
/* sets up interrupt for PB0 */
void interruptSetupPB0(){
	/* enable SYSCFG clock � only necessary for change of SYSCFG */
	RCC->APB2ENR |= 0x01; /* Set bit 0 of APB2ENR to turn on clock for SYSCFG */
	
		
	/* Configure PB0 as input for the output of the AND gate */
	GPIOB->MODER &= ~(0x00000003); 					/* Clear PB0 mode bits */
	
	/* select PB0 as EXTI0 */
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0; //clear EXTI0 bit in config reg
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB; //select PB0 as interrupt source
	
	/* configure and enable EXTI0 as falling-edge triggered */
	EXTI->FTSR1 |= EXTI_FTSR1_FT0; //EXTI0 = falling-edge triggered
	EXTI->PR1 = EXTI_PR1_PIF0; //Clear EXTI0 pending bit
	EXTI->IMR1 |= EXTI_IMR1_IM0; //Enable EXTI0
	
	/* Program NVIC to clear pending bit and enable EXTI0 -- use CMSIS functions*/
	NVIC_ClearPendingIRQ(EXTI0_IRQn); // Clear NVIC pending bit
	NVIC_EnableIRQ(EXTI0_IRQn); //Enable IRQ	
}
/* Interrupt service routine when PB0 goes low (falling edge triggered) */
void EXTI0_IRQHandler(){
	__disable_irq();		/*disable interrupts from coming in */

	/* instantiate variables */
	int volatile row, column, rowNumber, columnNumber, i;
	
	unsigned static char keypadArray[4][4] = {
		{0xD, 0xF, 0x0, 0xE},				/* Have to make it a little backwards from */
		{0xC, 0x9, 0x8, 0x7},				/* the way the keypad actually looks so    */
		{0xB, 0x6, 0x5, 0x4},				/* 0x1 is in [0][0] spot. 								 */
		{0xA, 0x3, 0x2, 0x1}				/* also # is 0xF and * is 0xE							 */
	};
		
	row = ((GPIOA->IDR >> 2) & 0xF) ^ 0xF;			/* pull in row values, masking the ones we don't need, flipping */
	
	for(i=8; i>0; i>>=1){												/* loop through from left to right */
		GPIOA->BSRR |= (i << 8);									/* shifting the BSR to shift the value */
		smallDelay();															/* give the processor a lil time */
		if(GPIOA->IDR & (row << 2)){							/* check if output and row value = 1 */
			column = i;															/* if so we've found the value */
		}
		GPIOA->BRR |= (i << 8);										/* shifting the value */
	}
		
		rowNumber = round(log2(row));								/* get the row # */
		columnNumber = round(log2(column));					/* get the column # */
		key = keypadArray[rowNumber][columnNumber];	/* set key value to right value */
	
		displayTheKey = 1;													/* we have the value so now we need to display it */
		
		GPIOA->BRR |= 0xF << 8;								/* reset columns back to 0 */
	
		smallDelay();													/* debouncing delay */
		EXTI->PR1 = EXTI_PR1_PIF0;				/*clear interrupt flag */
		NVIC_ClearPendingIRQ(EXTI0_IRQn);	/*clear pending status */
		__enable_irq();										/* enable interrupts */
	}
/* determines whether to display value of counter or keypad button */
void display(){
	if (displayTheKey==1){
		GPIOB->ODR = (key << 3); 	/* write the value to PB[6:3] */
		delay();									/* show the value for 3 seconds */
		delay();
		delay();
		displayTheKey = 0;				/* clear the value */
	}
GPIOB->ODR = (counter << 3);	/* if not supposed to display the key value, */
															/* or the key is done being shown, show counter */
}
/*updates the value of the counter variable */
void count(){
	if(counter==9){			/* check if counter is 9 */
		counter=0;				/* if so reset to 0 */
	}else{							/* if not, increment */
		counter++;
	}
}
/* delays for about 1 second */
void delay(){
	int volatile i, j, n;				/*dummy variables*/
	for(i=0; i<125; i++){			/*outer loop*/
		for(j=0; j<2000; j++){	/*inner loop*/
			n=j;										/*dummy operation */
		}
	}
}
/* delay for small pause after writing to output before reading input */
void smallDelay(){
	int volatile i, j;				/* dummy variable */
	for (i=0; i<4; i++){			/* useless function */
		j = i;
	}	
}
/*=======================*/
/* Main function         */
/*=======================*/
int main(void){
	GPIOPinSetup();						/* set up GPIO */
	interruptSetupPB0();			/* set up interrupts */
		__enable_irq();					/* enable interrupts */

	while(1){				/*infinite loop*/
		display();
		count();
		delay();
	}
}