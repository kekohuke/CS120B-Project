/*
 * test.c
 *
 * Created: 2019/7/6 18:06:56
 * Author : Coco
 */ 
#include <avr/io.h>
#include "scheduler.h"
//#include "bit.h"
//#include "io.c"
//#include "io.h"
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
enum pauseButtonSM_States {pauseButton_wait, pauseButton_press, pauseButton_release};


//Monitors button connected to PA0.
//When button is pressed, shared variable "pause" is toggled.
int pauseButtonSMTick(int state){
	unsigned char press = GetKeypadKey();
	switch(state){
		case pauseButton_wait:
			state = (press == '\0')? pauseButton_wait: pauseButton_press; break;
		case pauseButton_press:
			state = pauseButton_release; break;
		case pauseButton_release:
			state = (press == '\0')? pauseButton_wait: pauseButton_release; break;
		default:
			state = pauseButton_wait;break;
	}
	switch(state){
		case pauseButton_wait:
			break;
		case pauseButton_press:
			switch(press){
				case '\0': PORTB = 0x1F;break;//All 5 LEDs on
				case '1': PORTB = 0x01; break;//hexequivalent
				case '2': PORTB =0x02; break;
				case '3': PORTB =0x03; break;
				case '4': PORTB =0x04; break;
				case '5': PORTB =0x05; break;
				case '6': PORTB =0x06; break;
				case '7': PORTB =0x07; break;
				case '8': PORTB =0x08; break;
				case 'A': PORTB =0x0A; break;
				case 'B': PORTB =0x0B; break;
				case 'C': PORTB =0x0C; break;
				case 'D': PORTB =0x0D; break;
				case '*': PORTB =0x0E; break;
				case '0': PORTB =0x00; break;
				case '#': PORTB =0x0F; break;
				default: PORTB = 0x1B; break;//Should never occur, Middle LED off.
			}
			break;
		case pauseButton_release:
			break;
		default:
			break;
	}
	return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
	/*unsigned char x;
	DDRB = 0xFF; PORTB = 0x00;*/
	DDRC = 0xF0; PORTC  = 0x0F;
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	
    /* Insert your solution below */
    static task task1;
    task *tasks[] = {&task1};
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
