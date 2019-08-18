/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * 
 * SMTCAl003 BRWKAI001
 * 18/08/2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include <signal.h>//for keyboard interrupt
#include <math.h>
#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime =0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS;





void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
	
	//Set up the LEDS
	for(int i=0; i < sizeof(LEDS); i++){
	    pinMode(LEDS[i], OUTPUT);
	}
	
	//Set Up the Seconds LED for PWM
	pinMode(SECS,PWM_OUTPUT);
	//Write your logic here
	
	printf("LEDS done\n");
	
	//Set up the Buttons
	for(int j=0; j < sizeof(BTNS); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	
	//Attach interrupts to Buttons
	wiringPiISR(5, INT_EDGE_FALLING,  hourInc);//BTN0 for hours
	wiringPiISR(30, INT_EDGE_FALLING, minInc);//BTN1 for mins
	//Write your logic here
	
	printf("BTNS done\n");
	printf("Setup done\n");
}


/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	initGPIO();
	signal(SIGINT,keyboardInterrupt);
	//Set random time (3:04PM)
	//You can comment this file out later
	wiringPiI2CWriteReg8(RTC, HOUR, 0x10);//
	wiringPiI2CWriteReg8(RTC, MIN, 0x50);
	wiringPiI2CWriteReg8(RTC, SEC, 0x50+0b10000000);
	//toggleTime();
	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		secs =hexCompensation(wiringPiI2CReadReg8(RTC,SEC)-0b10000000);//-0b10000000 not neccesary with btns
		mins =hexCompensation(wiringPiI2CReadReg8(RTC,MIN));
		hours = hexCompensation(wiringPiI2CReadReg8(RTC,HOUR));
		lightHours(hFormat(hours));
		lightMins(mins);
		secPWM(secs);
		//wiringPiI2CReadReg8(RTC,SEC);
		//Write your logic here
		
		//Function calls to toggle LEDs
		//Write your logic here
		
		// Print out the time we have stored on our RTC
		printf("The current time is:%d:%d:%d \n",hFormat(hours), mins, secs);

		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */

void lightHours(int units){
	// Write your logic to light up the hour LEDs here	
	if((units&0b0001)==1){digitalWrite(LEDS[0],1);//if statement for LED 0
	}
	else{digitalWrite(LEDS[0],0);
	}

	if((units&0b0010)==0b10){digitalWrite(LEDS[1],1);
	}//if statement for LED 1
	else{digitalWrite(LEDS[1],0);
	}

	if((units&0b0100)==0b100){digitalWrite(LEDS[2],1);
	}
	else{digitalWrite(LEDS[2],0);
	}

	if((units&0b1000)==0b1000){digitalWrite(LEDS[3],1);
	}
	else{
	digitalWrite(LEDS[3],0);
	}

}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Write your logic to light up the minute LEDs here
	if((units&0b000001)==1){
	digitalWrite(LEDS[4],1);
	}else{
	digitalWrite(LEDS[4],0);
	}

	if((units&0b000010)==0b10){
	digitalWrite(LEDS[5],1);
	}else{
	digitalWrite(LEDS[5],0);
	}

	if((units&0b000100)==0b100){
	digitalWrite(LEDS[6],1);
	}else{
	digitalWrite(LEDS[6],0);
	}

	if((units&0b001000)==0b1000){
	digitalWrite(LEDS[7],1);
	}else{
	digitalWrite(LEDS[7],0);
	}

	if((units&0b010000)==0b10000){
	digitalWrite(LEDS[8],1);
	}else{
	digitalWrite(LEDS[8],0);
	}

	if((units&0b100000)==0b100000){
	digitalWrite(LEDS[9],1);
	}else{
	digitalWrite(LEDS[9],0);
	}
}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
	double ratio = 1024*units/59;
	int pwmratio = round(ratio);
	//printf("PWM: %f",ratio);
	pwmWrite(SECS,pwmratio);
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	long interruptTime = millis();
	if (interruptTime - lastInterruptTime>500){//debounce time at 500ms (makes sure no two interrupts are within 500ms of each other
		printf("Button 1 pressed: Inc. Hrs \n");
		hours = hexCompensation(wiringPiI2CReadReg8(RTC,HOUR));
		hours+=1;
		if(hours==24){hours=0;}
		//Increase hours by 1, ensuring not to overflow
		wiringPiI2CWriteReg8(RTC,HOUR,decCompensation(hours));
		printf("Value written to RTC: %x\n",decCompensation(hours));
		//Write hours back to the RTC
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>500){
		printf("Button 2 pressed: Inc. Mins\n");
		//Fetch RTC Time
		mins =hexCompensation(wiringPiI2CReadReg8(RTC,MIN));
		//Increase minutes by 1, ensuring not to overflow
		mins +=1;
		if(mins==60){mins=0;}
		//Write minutes back to the RTC
		wiringPiI2CWriteReg8(RTC,MIN,decCompensation(mins));
		printf("Value written to RTC: %d mins\n",mins); 
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}


void keyboardInterrupt(int signum){//GPIO cleanup function
printf("interrupt from keyboard\n");
for(int i=0;i<sizeof(LEDS);i++){
digitalWrite(LEDS[i],0);
}
pwmWrite(SECS,0);
printf("All outputs set to zero");
exit(0);
}

