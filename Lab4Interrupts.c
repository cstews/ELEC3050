/*====================================================================*/
/* Courtney Stewart                                                   */
/* ELEC3050 Lab 4                                                     */
/* Two buttons on PA1 and PA2 control the state of counters on 			  */
/* PB3 and PB4.																											  */
/* Pressing button 1 toggles the counters from on to off or vice versa*/
/* Pressing button 2 changes the direction counter 1 goes (up to down */
/* or down to up). Counter 2 is not affected.                         */
/*====================================================================*/

#include "stm32l4xx.h" 		/*microcontroller info */
static unsigned char run; /*keeps track of if the counters are running or not */
static unsigned char up = 1;  /*keeps track of which direction counter a is running */
static unsigned char PB3; /* toggled by PA1 interrupt */
static unsigned char PB4; /* toggled by PA2 interrupt */

static int counterA = 0;	/*counter on PA[8:5] */
static int counterB = 0; 	/*coutner on PA[12:9] */

/* Sets up the GPIO pins */
void GPIOPinSetup(){
	/* Configure PA[2:1] as inputs by setting to 00 */
	RCC->AHB2ENR |= 0x01;										 /* Enable GPIOA clock (bit 0) */
	GPIOA->MODER = GPIOA->MODER & 0xFFFFFFC3; /*keeping 15-3 and 0, erasing 1 & 2*/
	

	/* Configure PA[12:5] as outputs by setting to 00 */
	GPIOA->MODER &= ~(0x03FFFC00); 					/* Clear PA12-5 mode bits */
	GPIOA->MODER |=	(0x01555400); 					/* General purpose output mode*/

	
	/* Configure PB[4:3] as outputs to show counter value on LEDs */
	/* Meaning the values for PB[4:3] need to be set to 01 */
	RCC->AHB2ENR |= 0x02; 									/* Enable GPIOB clock (bit 1) */
	GPIOB->MODER &= ~(0x000003C0); 					/* Clear PB4-PB3 mode bits */
	GPIOB->MODER |=	(0x00000140); 					/* General purpose output mode*/
}
/* Sets up the interrupt on PA1 */
void interruptSetupPA1(){
	/* enable SYSCFG clock – only necessary for change of SYSCFG */
	RCC->APB2ENR |= 0x01; /* Set bit 0 of APB2ENR to turn on clock for SYSCFG */
	
	/* select PA1 as EXTI1 */
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI1; //clear EXTI1 bit in config reg
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PA; //select PA1 as interrupt source
	
	/* configure and enable EXTI1 as rising-edge triggered */
	EXTI->RTSR1 |= EXTI_RTSR1_RT1; //EXTI1 = rising-edge triggered
	EXTI->PR1 = EXTI_PR1_PIF1; //Clear EXTI1 pending bit
	EXTI->IMR1 |= EXTI_IMR1_IM1; //Enable EXTI1
	
	/* Program NVIC to clear pending bit and enable EXTI1 -- use CMSIS functions*/
	NVIC_ClearPendingIRQ(EXTI1_IRQn); // Clear NVIC pending bit
	NVIC_EnableIRQ(EXTI1_IRQn); //Enable IRQ
}
/* Sets up the interrupt on PA2 */
void interruptSetupPA2(){
	/* enable SYSCFG clock – only necessary for change of SYSCFG */
	RCC->APB2ENR |= 0x01; /* Set bit 0 of APB2ENR to turn on clock for SYSCFG */
	
	/* select PA1 as EXTI1 */
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI2; //clear EXTI1 bit in config reg
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PA; //select PA2 as interrupt source
	
	/* configure and enable EXTI1 as rising-edge triggered */
	EXTI->RTSR1 |= EXTI_RTSR1_RT2; //EXTI2 = rising-edge triggered
	EXTI->PR1 = EXTI_PR1_PIF2; //Clear EXTI1 pending bit
	EXTI->IMR1 |= EXTI_IMR1_IM2; //Enable EXTI1
	
	/* Program NVIC to clear pending bit and enable EXTI1 -- use CMSIS functions*/
	NVIC_ClearPendingIRQ(EXTI2_IRQn); // Clear NVIC pending bit
	NVIC_EnableIRQ(EXTI2_IRQn); //Enable IRQ
}
/* toggles run and PB3*/
void EXTI1_IRQHandler(){
		__disable_irq();		/*disable interrupts from coming in */
	
	//toggle PB3
	if(PB3==0){					/* check if PB3 is 0 */
		PB3 = 1;					/* if it is, change to 1 */
	}else{							/* if here it's 1, set to 0 */
		PB3 = 0;
	}
	
		GPIOB->ODR = (PB4 << 4) + (PB3 << 3); /* reflect PB3 in DIO */
	
	//toggle run
	if(run==0){					/*check if run is 0 */
		run = 1;					/*if it is, change to 1 */
	}else{							/*if here it's 1, set to 0 */
		run = 0;
	}
	
	EXTI->PR1 = EXTI_PR1_PIF1;				/*clear interrupt flag */
	NVIC_ClearPendingIRQ(EXTI1_IRQn);	/*clear pending status */
	__enable_irq();										/* enable interrupts */
}
/* toggles up */
void EXTI2_IRQHandler(){
	__disable_irq();		/*disable interrupts from coming in */
	
	//toggle PB4
	if(PB4==0){					/* check if PB4 is 0 */
		PB4 = 1;					/* if it is, change to 1 */
	}else{							/* if here it's 1, set to 0 */
		PB4 = 0;
	}
	
	GPIOB->ODR = (PB4 << 4) + (PB3 << 3); /* reflect PB4 in DIO */
	
	//toggle up
	if(up==0){		/*check if up is 0 */
		up = 1;			/*if it is, change to 1 */
	}else{				/*if here it's 1, set to 0 */
		up = 0;
	}
	EXTI->PR1 = EXTI_PR1_PIF2;				/*clear interrupt flag */
	NVIC_ClearPendingIRQ(EXTI2_IRQn);	/*clear pending status */
	__enable_irq();	
}
/* When run = 1, counters should count up */
/* when up = 1, counterA should increment */
void count(){
	if(run){						/* if run = 1, counters are going */
		if(counterB==9){		/* check if counterB is 9 */
			counterB = 0;			/* if so, reset it */
		}else{					  	/* if not, increment counterB */
			counterB++;
		}
		
		if(up){									/* checking the direction for counterA */
			if(counterA==9){ 			/*check if counterA is 9 */
				counterA = 0;				/* if it is reset it */
			}else{								/* if not, just increment A */
				counterA++;
			}
		}else{									/* if up = 0 we need to decrement */
			if(counterA==0) {			/*check if counterA is zero */
				counterA = 9;				/* if it is reset to 9 */
			}else{								/* if not, just decrement */
				counterA--;
			}
		}
		GPIOA->ODR = (counterA << 5) + (counterB << 9); /* reflect counters in DIO */
	}
}
/* Delays for about 0.5 seconds */
void delay(){
	int volatile i, j, n;				/*dummy variables*/
	for(i=0; i<125; i++){				/*outer loop*/
		for(j=0; j<1000; j++){		/*inner loop*/
			n=j;										/*dummy operation */
		}
	}
}
/*=======================*/
/* Main function         */
/*=======================*/
int main(void){
	/* set up the GPIO and interrupts for the board */
	GPIOPinSetup();
	interruptSetupPA1();
	interruptSetupPA2();
	__enable_irq();
	
	while(1){		/*infinite loop */
		delay();
		count();
	}
}
