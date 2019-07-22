/*
 * test.c
 *
 * Created: 2019/7/6 18:06:56
 * Author : Coco
 */ 
#include <avr/io.h>
#include "scheduler.h"
#include "bit.h"
#include "io.c"
#include "io.h"
#include "keypad.h"
//#include "lcd_8bit_task.h"
//#include "queue.h"
//#include "seven_seg.h"
//#include "stack.h"
#include "timer.h"
//#include "usart.h"
unsigned char led0_output = 0x00;
unsigned char led1_output = 0x00;
unsigned char pause = 0;

unsigned char string[37]="CS120B is Legend... wait for it DARY!";
unsigned char string_length=37;
unsigned char pointer = 0;
unsigned char i = 0;
unsigned char display_string[16];
//Monitors button connected to PA0.
//When button is pressed, shared variable "pause" is toggled.
enum tickStates{start,Lessthan16, Greaterthan16};
unsigned char tick(int state)
{
	
	display_string[16]="";
	
	switch(state)
	{
		case start: state = Lessthan16;break;
		case Lessthan16: state = (pointer>15)? Greaterthan16: Lessthan16; break;
		case Greaterthan16: state = (pointer == 36)? state = start: Greaterthan16;break;
	}
	switch(state)
	{
		case start:
			i = 0;
			pointer = 0;
			break;
		case Lessthan16:
			for(i = 0; i < pointer; i++)
			{
				display_string[15-pointer+i] = string[i];
			}
			for(i = 0; i < 15- pointer;i++)
			{
				display_string[i] = " ";
			}
			pointer++;
			break;
		case Greaterthan16:
			for(i = 0; i < pointer; i++)
			{
				display_string[pointer-16+i] = string[i];
			}
			pointer++;
			break;
		default:
			break;
	}
}

enum displayStates{start,display};

int main(void) {
    /* Insert DDR and PORT initializations */
	/*unsigned char x;
	DDRB = 0xFF; PORTB = 0x00;*/
	DDRC = 0xFF; PORTC  = 0x00;
//	DDRA = 0x00; PORTA = 0xFF;
//	DDRB = 0xFF; PORTB = 0x00;
	LCD_init();
    /* Insert your solution below */
    static task task1, task2;
    task *tasks[] = {&task1, &task2};
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
    
    //Task 1 (pauseButtonToggleSM)
    
    task1.state = pauseButton_wait;//Task initial state
    task1.period = 1;//Task Period
    task1.elapsedTime = task1.period;//Task current elapsed time.
    task1.TickFct = &pauseButtonSMTick;//Function pointer for the tick
    //Set the timer and turn it on
	unsigned long int GCD = 1;
    TimerSet(1);
    TimerOn();
    
    unsigned short i;//Scheduler for-loop iterator
    while (1) {
		for(i = 0; i < numTasks; i++){//Scheduler code
			if(tasks[i]->elapsedTime == tasks[i]->period){//Task is ready to tick
				tasks[i]->state= tasks[i]->TickFct(tasks[i]->state);//set next state
				tasks[i]->elapsedTime = 0;//Reset the elapsed time for next tick;
			}
			tasks[i]->elapsedTime += GCD; 
			GCD = findGCD(GCD, tasks[i]->period);
		}
		while(!TimerFlag);
		TimerFlag = 0;
    }
    return 1;
}
