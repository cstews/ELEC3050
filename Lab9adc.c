/*====================================================================*/
/* Courtney Stewart                                                    */
/* ELEC3050 Lab 9                                                      */
/* Analog-to-Digital Converter					       */
/* Used in conjunction with rectifier and filter circuit to	       */
/* convert ac signal from motor to dc signal, which is processed       */
/* by this program to change duty cycles/speed of a motor.	       */
/* By using each of the key pads 0-A the motor speed will increase     */
/* by a factor of 10% (0%, 10%,...,100%) 			       */
/*====================================================================*/

#include "stm32l4xx.h" /*microcontroller info */
#include <math.h>

static unsigned char key = 0; /* keeps track of the value of the key button pressed */
static unsigned int counter = 0;
static int dutyCycle;
static int adc_in;

void delay(int);
void smallDelay(void);

/* sets up GPIO pins for proper input or output */
void GPIOPinSetup(){
	RCC->AHB2ENR |= 0x03;				/* Enable GPIOA clock (bit 0) */
	
	/* PA[5:2] input pull up */
	GPIOA->MODER &= ~(0xFF << 4);
	GPIOA->PUPDR &= ~(0xFF << 4);
	GPIOA->PUPDR |= (0x55 << 4);
	
	/* PA[11:8] outputs default low */
	GPIOA->MODER &= ~(0xFF << 16);
	GPIOA->MODER |= (0x55 << 16);
	GPIOA->BRR |= 0xF << 8;				/* set PA[11:8] to 0 */
	
	/* PB[6:3] outputs for display */
	GPIOB->MODER &= ~(0xFF << 6);
	GPIOB->MODER |= (0x55 << 6);

}
/* sets up interrupt for PB0 */
void interruptSetupPB0(){
	/* enable SYSCFG clock only necessary for change of SYSCFG */
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
/* set up TIM6 */
void timer6Setup(){
	RCC->APB1ENR1 |= 0x10; 			/* turn on clock for timer 6 */
	TIM6->ARR = 3999;			/* PWM period = timer clock freq/desired PWM */
						/* = 4MHz/1kHz = 4000 */
	TIM6->PSC = 99;				/* 1kHz */
	TIM6->CR1 |= 0x01;			/* 399 and 9? */
	TIM6->DIER |= 0x01;
	
	RCC->APB1ENR1 |= 0x01; 		/* turn on clock for timer 2 */
	TIM2->ARR = 99;
	TIM2->PSC = 399;
	TIM2->CR1 |= 0x01;
	TIM2->DIER |= 0x01;
	
	NVIC_ClearPendingIRQ(TIM6_IRQn); // Clear NVIC pending bit
	NVIC_EnableIRQ(TIM6_IRQn); //Enable IRQ	
	
	NVIC_ClearPendingIRQ(TIM2_IRQn); // Clear NVIC pending bit
	NVIC_EnableIRQ(TIM2_IRQn); //Enable IRQ	
	
}
/* Pulse Width Modulation Setup */
/* Using PA0 as output for PWM */
void PWMSetup(){
	RCC->AHB2ENR |= 0x01;			/* set bit for GPIOA EN */
	
	GPIOA->MODER &= ~0x00000003;		/* clear MODE0[1:0] */
	GPIOA->MODER |= 0x00000002;			/* set MODE0 to 10 */
	
	GPIOA->AFR[0] &= ~0x0000000F;		/* clear AFRL0 */
	GPIOA->AFR[0] |= 0x00000001;		/* PA0 = AF1 */
						//TIM2_CH1 is for PA0
}
/* Analog-to-digital converter Setup */
/* Using PA1 as output */
void ADCSetup(){
	RCC->AHB2ENR |= 0x01;						/* set bit 0 for GPIOA EN */
	
	GPIOA->MODER &= ~0x0C;					/* clear MODE1[1:0] */			
	GPIOA->MODER |= 0x0C;						/* set MODE1 to 11 */
	
	RCC->AHB2ENR |= (0x01 << 13);		/* set ADCEN */
	
	RCC->CCIPR |= (0x03 << 28);			/* configure ADCSEL[1:0] */
	
	ADC1->CR &= ~(0x01 << 29);			/* clear deep power down mode */
	ADC1->CR|= (0x01 << 28);				/* set ADVREGEN */
	delay(1);												/* delay for startup */
	
	ADC1->CFGR |= (0x3 << 12);			/* set CONT and OVRMOD */
	
	ADC1->SQR1 &= ~0x0F;						/* clear L[3:0] for sequence length */
	ADC1->SQR1 |= 0x06 << 6;				
	
	ADC1->SQR1 &= ~(0x1F << 6);			/* clear SQ1 (5 bits) */
	ADC1->SQR1 |= 0x06 << 6;				/* configure to 0x6 */
	
	ADC1->CR |= 0x1;								/* set ADC enable bit */
	
	while (!(ADC1->ISR & 0x00000001));	/* check ADC ready bit) */
	
	ADC1->CR |= 0x01 << 2;					/* set ADSTART */
}
/* set up TIM2 */
void timer2Setup(){
	RCC->APB1ENR1 |= 0x01;					/* turn on TIM2 */
	
	TIM2->CR1 |= 0x01;							/* enable timer counter */
	
	TIM2->CCMR1 &= 0x00000003;		/* clear CC1S bits to put into output mode */
	
	TIM2->CCMR1 &= ~0x00000070;		/* clear bits [6:4] */
	TIM2->CCMR1 |= 0x00000060;		/* set bits [6:4] to 110 */ 
	
	TIM2->CCER &= ~0x00000003;			/* clear CC1 E & P */
	TIM2->CCER |= 0x00000001;				/* set CC1 E */
	
	TIM2->CCR1 = 0x00;							/* set duty cycle to 0 as default */
	
	/* for 1kHz */
	TIM2->ARR = 0x18F;							/* 399 in decimal */
	TIM2->PSC = 0x9;								/* just 9 nothing special */
	
	/* for 100 Hz */
	//TIM2->ARR = 0x18F;						/* 399 */
	//TIM2->PSC = 0x63;							/* 99 */
	
	/* for 10kHz */
	//TIM2->ARR = 0x18f;						/* 399 */
	//TIM2->PSC = 0x00;							/* just set her to 0 */
}
/* Interrupt service routine when PB0 goes low (falling edge triggered) */
void EXTI0_IRQHandler(){
	__disable_irq();		/*disable interrupts from coming in */

	/* instantiate variables */
	int volatile row, column, rowNumber, columnNumber, i;
	
	unsigned static char keypadArray[4][4] = {
		{0xD, 0xF, 0x0, 0xE},				/* Have to make it a little backwards from */
		{0xC, 0x9, 0x8, 0x7},				/* the way the keypad actually looks so    */
		{0xB, 0x6, 0x5, 0x4},				/* 0x1 is in [0][0] spot. 	 */
		{0xA, 0x3, 0x2, 0x1}				/* also # is 0xF and * is 0xE	 */
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
		
		smallDelay();											/* debouncing delay */
		EXTI->PR1 = EXTI_PR1_PIF0;				/*clear interrupt flag */
		NVIC_ClearPendingIRQ(EXTI0_IRQn);	/*clear pending status */
		__enable_irq();										/* enable interrupts */
	}
	/* timer handler */
void TIM6_IRQHandler(){ 
	TIM6->SR &= ~0x01;
	if(counter>=9){										/* check if counter is 9 */
		counter=0;											/* if so reset to 0 */
		GPIOB->BRR |= 0xF <<3;
	}else{														/* if not, increment */
		counter++;
		GPIOB->BRR |= 0xF << 3;
		GPIOB->BSRR |= counter << 3;
	}
		NVIC_ClearPendingIRQ(TIM6_IRQn); // Clear NVIC pending bit
}
/* delays for about 0.008 second */
void delay(int k){
	int volatile i, j, n;						/*dummy variables*/
	for(i=0; i<k; i++){							/*outer loop*/
		for(j=0; j<2000; j++){				/*inner loop*/
			n=j;												/*dummy operation */
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
	PWMSetup();								/* set up PWM */
	timer2Setup();						/* set up timers */
	ADCSetup();								/* set up ADC */
		__enable_irq();					/* enable interrupts */
	
	while(1){									/*infinite loop*/
		if(key < 11){
			dutyCycle = key * (TIM2->ARR + 1)/10;			/* calculate duty cycle associated with key press */
			TIM2->CCR1 = dutyCycle;										/* set CCR1 to associated duty cycle value */
			
			GPIOB->ODR &= 0x78;												/* clear PB[6:3] */
			GPIOB->ODR |= (key << 3);									/* shift in value of key */
		}
	}
}
