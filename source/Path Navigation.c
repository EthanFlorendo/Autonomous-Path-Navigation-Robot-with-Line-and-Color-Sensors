#include "MKL46Z4.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"

#include "delay.h"
#include "sensors.h"
#include "motors.h"

int main(){
	//Initialize color vals
    int color, end_color, start_color;
    //double check if end color is correct
    int color_chk;
    //Init line sensor vals
    int left_val, right_val;
    //fsm state
    int state = 0;

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    //initialize I2C color sensor and turn on
    init_Color_Sensor();

    init_pwmIO();
    init_Line_Sensor();

    while(1){
    	//color = get_Color(); //use to check sensor works
    	switch (state){
			case 0: //will initialize with switch press, get starting and end color,
					//move forward and turn to position around ring
				if (!(GPIOC->PDIR & (1 << 3))) {
					//get start and end color
					start_color = get_Color();
					PRINTF("Start: %d\r\n",start_color);
					end_color = get_End(start_color);
					PRINTF("End: %d\r\n",end_color);

					//move to position
					delay_ms(1000);
					motors_forward();
					delay_ms(3500);
					motors_right();
					delay_ms(1500);
					motors_stop();
					state = 1;
				}
				break;
			case 1: //continuously move forward and check sensor vals
				motors_forward();
				left_val = read_Line_Left();
				right_val = read_Line_Right();
				color = get_Color();
				//PRINTF("Color: %d\r\n",color);

				//check left line sensor to turn
				if (left_val > 230){
					motors_stop();
					delay_ms(2000);
					state = 2;
				}
				//check right line sensor to turn
				if (right_val > 230){
					motors_stop();
					delay_ms(2000);
					state = 3;
				}
				//check color sensor for end
				if (color == end_color){
					delay_ms(700);
					color_chk = get_Color();//double check for correct reading
					if(color_chk != end_color){
						state = 1;
					}
					else{
						motors_stop();
						delay_ms(2000);
						state = 4;
					}
				}
				break;

			case 2://turn angle right
				motors_right();
				delay_ms(800);
				motors_stop();
				state = 1;
				break;

			case 3:	//turn angle left
				motors_left();
				delay_ms(800);
				motors_stop();
				state = 1;
				break;
			case 4: //stop state, get to correct position
				delay_ms(2000);
				motors_right();
				delay_ms(1500);
				motors_stop();
				delay_ms(2000);
				motors_forward();
				delay_ms(3000);
				motors_stop();
				delay_ms(2000);
				state = 0;
				break;

    	        }




    }
}

