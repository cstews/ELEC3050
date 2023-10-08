/*===================================================================*/
/* Courtney Stewart                                                  */
/* ELEC3050 Lab 3                                                    */
/* 3-bit counter counts from 0-5 and rolls over approx. every second */
/* Counter only runs when special code is given (20 base-10)         */
/* Any other value pauses the counter                                */
/*===================================================================*/

#include "stm32l4xx.h" /*microcontroller info */
#include <stdbool.h>   /*might need this since C is so ooga booga and doesn't recognize bool */


/* global variables*/
static int counter; /*keeps track of value of counter*/

	
/* pinSetup() so what we do actually works */	
void pinSetup() {
	/* Configure PA[7:0] as inputs for special code entry*/
	/* Meaning the values for PA[7:0] need to be set to 00 */
	RCC->AHB2ENR |= 0x01;										 /* Enable GPIOA clock (bit 0) */
	GPIOA->MODER = GPIOA->MODER & 0xFFFF0000; /*keeping 15-8, erasing the rest */
	
	/* Configure PB[5:3] as outputs to show counter value on LEDs */
	/* Meaning the values for PB[5:3] need to be set to 01 */
	RCC->AHB2ENR |= 0x02; 									/* Enable GPIOB clock (bit 1) */
	GPIOB->MODER &= ~(0x00000FC0); 					/* Clear PB5-PB3 mode bits */
	GPIOB->MODER |=	(0x00000540); 					/* General purpose output mode*/
}

	
/*--------------------------------*/
/* Delay function                 */
/* do nothing for around 1 second */
/*--------------------------------*/
void delay() {
	int volatile i, j, n;				/*dummy variables*/
	for(i=0; i<125; i++){			/*outer loop*/
		for(j=0; j<2000; j++){	/*inner loop*/
			n=j;										/*dummy operation */
		}
	}
}
/*------------------------------------*/
/* Count function                     */
/* increments counter on pins PB[5:3] */
/*------------------------------------*/
void count() {
	if(counter==5){ 				/*check if counter is 5 */
		counter=0;						/*if it is needs to reset to 0 */
	}else{								/*if not we need to increment */
		counter++;						/* increments value */
	}
	GPIOB->ODR = (counter << 3); 	/*write value to PB[5:3] */
}
/*-----------------------------------------*/
/* Secret function                         */
/* checks values on PA[7:0] and returns    */
/* non-zero value if they contain the value*/
/* "the value" being 0001 0100 in binary   */
/* or 0x14                                 */
/*-----------------------------------------*/
bool secret() {
	uint16_t temp; 					/* declare temp variable that matches IDR size */
	temp = GPIOA->IDR;			/*read in all 16 pins which are PA[15:0] */
	temp &= 0xFF;						/* mask all bits except PA[7:0] */

	if(temp == 0x0014){		/*check if temp value is the super secret code */
		return 1;													/* if it is, return true */
	}
}

/*===================================*/
/* Main program
/*===================================*/
int main(void){
	pinSetup();											/* call pinSetup() function so the board works*/
	while(1) {											/* start endless loop */
		delay();											/* wait around 1 second */
		if (secret() == 1) {					/* check if values on PA[7:0] are the super secret value*/
			count();										/* if true we can increment the counter */
		}															/* if it's false then it'll exit and start the while loop again*/
	}
}