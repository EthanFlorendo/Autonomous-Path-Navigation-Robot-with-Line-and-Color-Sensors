#ifndef SENSORS_H
#define SENSORS_H

//color codes returned by get_Color / get_End
#define COLOR_BLUE   1
#define COLOR_YELLOW 2
#define COLOR_GREEN  3
#define COLOR_RED    4
#define COLOR_WHITE  5
#define COLOR_BLACK  6

//color sensor (TCS34725 over I2C0, PTC8/PTC9)
void init_Color_Sensor(void); //init I2C, power on sensor, enable RGBC
int get_Color(void);          //returns read color val from sensor
int get_End(int color);       //returns destination color given one color

//line sensors (ADC0, PTE16 left / PTE17 right)
void init_Line_Sensor(void);  //init ADC + calibration
int read_Line_Left(void);
int read_Line_Right(void);

#endif
