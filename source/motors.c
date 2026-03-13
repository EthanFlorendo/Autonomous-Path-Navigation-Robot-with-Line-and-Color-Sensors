#include "MKL46Z4.h"
#include "motors.h"

//initialize motors and sw1, mostly unchanged from previous projects
void init_pwmIO(void) {
	//clock gating for ports B and C
    SIM->SCGC5 |= (1 << 10) | (1 << 11);

    //PWM clock gating, enable TP2 and clock to 8MHz oscer
	SIM->SCGC6 |= (1 << 26);
	SIM->SOPT2 |= (1 << 24);

    //Left Motor Setup
	PORTB->PCR[0] &= ~0x700; //clear
	PORTB->PCR[0] |= 0x700 & (1 << 8); // Set MUX to ALT1 GPIO
	PORTB->PCR[1] &= ~0x700; //clear
	PORTB->PCR[1] |= 0x700 & (1 << 8); // Set MUX to ALT1 GPIO
	GPIOB->PDDR |= (1 << 0) | (1 << 1); // Set pins as outputs

	//Right Motor Setup
	PORTC->PCR[1] &= ~0x700;
	PORTC->PCR[1] |= 0x700 & (1 << 8);
	PORTC->PCR[2] &= ~0x700;
	PORTC->PCR[2] |= 0x700 & (1 << 8);
	GPIOC->PDDR |= (1 << 1) | (1 << 2);

	//Switch Setup
	PORTC->PCR[3] &= ~0x703; //clear
	PORTC->PCR[3] |= (1 << 8) | 0x03;  // Set MUX to ALT1 GPIO and enable pull up res
	GPIOC->PDDR &= ~(1 << 3); // Clear  as input

	//Motor PWM Setup
	PORTB->PCR[2] &= ~(0x700);   // clear
	PORTB->PCR[2] |= 0x300;      // Set MUX to ALT3 TPM2ch0
	PORTB->PCR[3] &= ~(0x700);   // clear
	PORTB->PCR[3] |= 0x300;      // Set MUX to ALT3 TPM2ch1


	//Start motor configuration
	TPM2->SC = 0; //clear
	TPM2->MOD = 999;//chosen MOD val

	//Set TPM2 channel 0 and 1 as Edge aligned PWM, High true pulses
	TPM2->CONTROLS[0].CnSC = (1 << 5) | (1 << 3);
	TPM2->CONTROLS[1].CnSC = (1 << 5) | (1 << 3);

	TPM2->CONTROLS[0].CnV = 999; // set CnV to non zero
	TPM2->CONTROLS[1].CnV = 999; // set CnV to non zero

	TPM2->SC = (1 << 3) | 0x03; //enable TPM2,set prescaler to 2^(3) = 8

	//initialize motors as stopped
	GPIOB->PDOR &= ~(1 << 0);
	GPIOB->PDOR &= ~(1 << 1);
	GPIOC->PDOR &= ~(1 << 1);
	GPIOC->PDOR &= ~(1 << 2);
}

//basic functions for movement
void motors_forward(void) {
    GPIOB->PDOR &= ~(1 << 0);
    GPIOB->PDOR |=  (1 << 1);
    GPIOC->PDOR &= ~(1 << 1);
    GPIOC->PDOR |=  (1 << 2);
}
void motors_stop(void) {
	GPIOB->PDOR &= ~(1 << 0);
	GPIOB->PDOR &= ~(1 << 1);
	GPIOC->PDOR &= ~(1 << 1);
	GPIOC->PDOR &= ~(1 << 2);
}
void motors_right(void) {
    GPIOB->PDOR |=  (1 << 0);
    GPIOB->PDOR &= ~(1 << 1);
    GPIOC->PDOR &= ~(1 << 1);
    GPIOC->PDOR |=  (1 << 2);
}
void motors_left(void) {
    GPIOB->PDOR &= ~(1 << 0);
    GPIOB->PDOR |=  (1 << 1);
    GPIOC->PDOR |=  (1 << 1);
    GPIOC->PDOR &= ~(1 << 2);
}
