/*====================================================*/
/* Victor P. Nelson */
/* Modified by: Joey Hines, Spring 2021 */
/* Edited by: Soham Roy, 13Jan2021 */
/* Fixed errors about PB3-PB4 bit pattern and comments */
/* ELEC 3040/3050 - Lab 1, Program 1 */
/* Toggle LED1 while button pressed, with short delay inserted */
/* Count toggles */
/* Program has one error that must be corrected. */
/*====================================================*/
#include "stm32l4xx.h" /* Microcontroller information */
/* Define global variables */
int toggles; /* number of times LED state toggled */
/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA11 = push button */
/* PB4 = LDR, PB5 = green LED */
/*---------------------------------------------------*/
void PinSetup () {
/* Configure PA0 as input pin to read push button */
RCC->AHB2ENR |= 0x01; /* Enable GPIOA clock (bit 0) */
GPIOA->MODER &= ~(0x00C00000); /* General purpose input mode */
/* Configure PB4,PB3 as output pins to drive LEDs */
RCC->AHB2ENR |= 0x02; /* Enable GPIOB clock (bit 1) */
GPIOB->MODER &= ~(0x000003C0); /* Clear PB4-PB3 mode bits */
GPIOB->MODER |= (0x00000140); /* General purpose output mode*/
}
/*----------------------------------------------------------*/
/* Delay function - do nothing for about 1 second */
/*----------------------------------------------------------*/
void delay () {
int volatile i,j,n;
for (i=0; i<20; i++) { //outer loop
for (j=0; j<20000; j++) { //inner loop
n = j; //dummy operation for single-step test
} //do nothing
}
}
/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
unsigned int sw1; //state of SW1
unsigned char led1; //state of LED1
	
unsigned char led2; //ADDED FOR BONUS, state of LED2
	
PinSetup(); //Configure GPIO pins
led1 = 0; //Initial LED state
	
led2 = 1; //ADDED FOR BONUS, initial state
	
toggles = 0; //#times LED state changed
/* Endless loop */
while (1) { //Can also use: for(;;) {
if (led1 == 0) { //LED off
GPIOB->BSRR = 0x0010 << 16; //Reset PB4=0 to turn OFF LED (in BSRRupper half)
}
else { //LED on
GPIOB->BSRR = 0x0010; //Set PB4=1 to turn ON LED (in BSRR lowhalf)
}
sw1 = GPIOA->IDR & 0x0800; //Read GPIOA and mask all but bit 11
/* Wait in loop until SW1 pressed */
while (sw1 == 0) { //Wait for SW1 = 1 on PA11
sw1 = GPIOA->IDR & 0x0800; //Read GPIOA and mask all but bit 11
}
delay(); //Time delay for button release
led1 = ~led1; //Complement LED1 state
toggles++; //Increment #times LED toggled
} /* repeat forever */
}