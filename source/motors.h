#ifndef MOTORS_H
#define MOTORS_H

void init_pwmIO(void);     //init motors (TPM2 PWM + direction GPIO) and sw1
void motors_forward(void);
void motors_stop(void);
void motors_right(void);
void motors_left(void);

#endif
