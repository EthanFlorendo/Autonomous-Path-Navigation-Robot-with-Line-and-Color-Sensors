#ifndef DELAY_H
#define DELAY_H

#include "MKL46Z4.h"

//simple delay
static inline void delay_ms(int ms) {
	//cycles at 48MHz
	//scale up to make inputs more precise
    for (int i=0; i < ms*800; i++) {
    	//One Cycle No operation
        __NOP();
    }
}

#endif
