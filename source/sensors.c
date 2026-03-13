#include "MKL46Z4.h"
#include "sensors.h"
#include "delay.h"

//7-bit address of the TCS34725 color sensor
#define TCS34725_ADDR 0x29

/* ---------- I2C (color sensor bus) ---------- */

//initialize I2C for color sensor
static void initI2C(){
	//enable clock gating
    SIM->SCGC4 |= (1<<6);//enable clock gating for I2C0
    SIM->SCGC5 |= (1<<11); // enable clock gating for PortC (color sensor)

    //Setup Pins for I2C
    PORTC->PCR[8] &= ~0x700; //clear mux
    PORTC->PCR[8] |= (2<<8); //Setup PTC8 mux as ALT2(I2C0_SCL)
    PORTC->PCR[9] &= ~0x700; //clear mux
    PORTC->PCR[9] |= (2<<8); //Setup PTC9 mux as ALT2(I2C0_SDA)

    I2C0->C1 = 0; //disable and reset I2C config
    I2C0->FLT = 0x50; //clear all bits of FLT register
    I2C0->S |= (1<<4)|(1<<1); //clear status flags, w1c for arbitration and interrupt
    I2C0->F = 0x23; //corresponding to 256 in table, 24Mhz / 256 ~ 94 kHz

    I2C0->C1 |= (1<<7); //I2C set enable bit
}

//collection of basic I2C functions
static void clearStatusFlags(){
	I2C0->FLT |= (1<<6); //Clear StopF in filter register (w1c)
    I2C0->FLT |= (1<<4); //Undocumented StartF bit 4 -look over
    I2C0->S |= (1<<4)|(1<<1); //Clear ARBL and IICIF in status reg (w1c)
}

static void TCFWait(){
    while(!(I2C0->S & (1<<7))); //wait for TCF bit set in status reg
}

static void IICIFWait(){
    while(!(I2C0->S & (1<<1))); //wait for IICIF bit set in status reg (w1c)
}

static void SendStart(){
    I2C0->C1 |= (1<<5)|(1<<4); //Set MST and TX in C1 reg
}

static void RepeatStart(){
	I2C0->C1 |= (1<<5)|(1<<4); //Set MST and TX in C1 reg
    I2C0->C1 |= (1<<2);		   //Set RSTA in C1 reg
    for(int i=0; i<6; i++); 	   //wait 6 cycles check on this part
}

static void SendStop(){
    I2C0->C1 &= ~((1<<5)|(1<<4)|(1<<3)); //Clear MST, TX, and TXAK in C1 reg
    while(I2C0->S & (1<<5)); //wait for busy but to go low in status reg
}

static void clearIICIF(){
    I2C0->S |= (1<<1); //clear IICIF in status reg (w1c)
}

static int RxAK(){
    return !(I2C0->S & (1<<0)); //return 1 if RXAK == 0 , check RXAK in status reg
}

static void I2C_WriteByte(unsigned char reg, unsigned char data){
    clearStatusFlags();
    TCFWait();
    SendStart();

    I2C0->D = (TCS34725_ADDR << 1) | (0<<0); //write device address,  TCS34725 -> 0x29, r/w = write = 0

    IICIFWait();
    if(!RxAK()){
    	SendStop();
    	return;
    }
    clearIICIF();

    I2C0->D = (1<<7) | reg; // Set command bit, Write register address to data address

    IICIFWait();
    if(!RxAK()){
    	SendStop();
    	return;
    }

    TCFWait();
    clearIICIF();

    I2C0->D = data;//write data byte to data register

    IICIFWait();

    if (!RxAK()){}// do not need to print anything, stop or return here

    clearIICIF();
    SendStop();
}

static void ReadBlock(unsigned char reg, unsigned char *data, int length){
    unsigned char dummy = 0;
    clearStatusFlags();
    TCFWait();
    SendStart();

    dummy++;//Suppress warning

    I2C0->D = (TCS34725_ADDR << 1) | (0<<0); //write device address,  TCS34725 -> 0x29, r/w = write = 0

    IICIFWait();

    if(!RxAK()){
		SendStop();
		return;
	}

    clearIICIF();

    I2C0->D = (1<<7) | reg; //set command bit, write register address to data address

    IICIFWait();
    if(!RxAK()){
		SendStop();
		return;
	}
    clearIICIF();
    RepeatStart();

    I2C0->D = (TCS34725_ADDR << 1) | (1<<0); //write device address,  TCS34725 -> 0x29, r/w = read = 1

    IICIFWait();

    if(!RxAK()){
		SendStop();
		return;
	}

    TCFWait();
    clearIICIF();

    I2C0->C1 &= ~((1<<4)|(1<<3));//switch to rx, clear tx and TxAK

    if(length==1){
    	I2C0->C1 |= (1<<3); // Set TXAK to NACK in C1, no more data
    }

    dummy = I2C0->D;

    for(int i=0;i<length;i++){
        IICIFWait();
        clearIICIF();

        if(i == length-2){
            I2C0->C1 |= (1<<3); // Set TXAK to NACK in C1, no more data
        }

        if(i == length-1){
            SendStop();
        }

        data[i] = I2C0->D; //put data byte into array
    }
}

/* ---------- Color sensor (TCS34725) ---------- */

//initialize I2C color sensor and turn on
void init_Color_Sensor(void){
    initI2C();
    I2C_WriteByte(0x00, 0x01); //Power On color Sensor
    delay_ms(100); //"A minimum interval of 2.4 ms must pass after PON is
    			   //asserted before an RGBC can be initiated", decided to give more time
	I2C_WriteByte(0x00, 0x03); //RGBC enable
}

//returns destination color given one color
int get_End(int color){
	if (color == COLOR_BLUE) return COLOR_RED;//blue to red
	else if (color == COLOR_YELLOW) return COLOR_RED;//yellow to red
	else if (color == COLOR_GREEN) return COLOR_BLUE;//green to blue
	else if (color == COLOR_RED) return COLOR_GREEN;//red to green
	else return COLOR_WHITE;//should not start with another color
}

//returns read color val from sensor
int get_Color(void){
	int clear, red, green, blue; //initialize 4 vals
	unsigned char data[8]; //unsigned char as matches length 8 bytes total, 16 bits per color

	//fastest to read all 8 bytes together instead of individually
	//this starts with low byte of clear to high byte of blue (0x1B)
	ReadBlock(0x14, data, 8);

	//separate by color, combine bytes to get a single integer value
	clear = (data[1]<<8)|data[0];
	red = (data[3]<<8)|data[2];
	green = (data[5]<<8)|data[4];
	blue = (data[7]<<8)|data[6];

	//used to check sensor values
	//PRINTF("Clear:%d  Red:%d  Green:%d  Blue:%d\r\n",clear, red, green, blue);

	//check if color is white
	if (clear > 25000) {
		if(red - blue < 3000){//make sure not yellow
			return COLOR_WHITE; //white
		}
	}

	//check if color is black
	if (clear < 3000)  return COLOR_BLACK; //black

	//check other colors
	if (green > blue && green > red) return COLOR_GREEN; //green
	else if (blue > green && blue > red) return COLOR_BLUE; //blue
	else if (red > blue && red > green) { //red or yellow
		if (clear > 14000) return COLOR_YELLOW; // yellow
		else return COLOR_RED; //red
	}
	return COLOR_BLACK;

}

/* ---------- Line sensors (ADC0) ---------- */

//Initialize ADC + calibration for both sensors
void init_Line_Sensor(void){
	//calibration value variable
	unsigned short cal_val = 0;

	SIM->SCGC5 |= (1<<13); //clock gating for Port E (Line Sensors)
	SIM->SCGC6 |= (1<<27); //clock gating for ADC0

	//Setup ADC clock with defaults
	ADC0->CFG1 = 0;

	//ADC calibrate
	ADC0->SC3 = 0x07; //Enable Max hardware averaging
	ADC0->SC3 |= 0x80; //Set calibration bit
	while((ADC0->SC3 & 0x80)){} //wait for calibration to complete (bit to clear)

	//after calibration is complete, write calibration registers.
	cal_val = ADC0->CLP0 + ADC0->CLP1 + ADC0->CLP2 + ADC0->CLP3 + ADC0->CLP4 + ADC0->CLPS;
	cal_val = cal_val >> 1 | 0x8000;
	ADC0->PG = cal_val;

	cal_val = 0;
	cal_val = ADC0->CLM0 + ADC0->CLM1 + ADC0->CLM2 + ADC0->CLM3 + ADC0->CLM4 + ADC0->CLMS;
	cal_val = cal_val >> 1 | 0x8000;
	ADC0->MG = cal_val;

	//Disable Max hardware averaging
	ADC0->SC3 = 0;
}

//return left line sensor reading
int read_Line_Left(void){
	ADC0->SC1[0] = 0x01; //set channel and start conversion for PTE16 (ADC0_SE1) - Left
	while(!(ADC0->SC1[0] & 0x80)){} //wait for calibration to complete
	return ADC0->R[0]; //return conversion result, reset COCO
}

//return right line sensor reading
int read_Line_Right(void){
	ADC0->SC1[0] = 0x05; //set channel and start conversion for PTE17 (ADC0_SE5) - Right
	while(!(ADC0->SC1[0] & 0x80)){} //wait for calibration to complete
	return ADC0->R[0]; //return conversion result, reset COCO
}
