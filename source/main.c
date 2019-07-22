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
static task task1, task2,task3;
task *tasks[] = {&task1, &task2, &task3};
enum pauseButtonSM_States {pauseButton_wait, pauseButton_press, pauseButton_release};
unsigned char star[4] = {0, 9, 5, 13};

unsigned char i = 0;
unsigned char position = 17;

void initialdisplay()
{
	LCD_ClearScreen();
}

void movestar()
{
	for(i = 0; i < 4; i++)
	{
		if(star[i] == 0)
			star[i] = 15;
		else star[i]--;
	}
}
void writedata(unsigned char row, unsigned char col, unsigned char simbol)
{
		LCD_Cursor(col+1+(row-1)*15);
		LCD_WriteData(simbol);
}
void displaystar()
{
	for(i = 0; i < 2; i++)
	{
		writedata(1, star[i],'*' );
	}
	for(i = 2; i < 4; i++)
	{
		writedata(2, star[i],'*' );
	}
}
unsigned int press = '\0';
enum GameStates {Start, WaitforStart, GameStart, Gaming, Fail};
int Game(int state)
{
	int last_state = state;
	switch(state)
	{
		case Start:
			state = WaitforStart;
			break;
		case WaitforStart:
			if(press == '1' )
				state = GameStart;
			break;
		case GameStart:
			state = Gaming;
			break;
		case Gaming:
			if(press == '*' )
				state = WaitforStart;
			for(i = 0 ; i < 4;i++)
				if(position == (star[i]+(i>1)*16)+(i<2)*1)
					state = Fail;
			break;
		case Fail:
			if(press == '1' )
				state = WaitforStart;
			break;
	default:
			state = Start;
			break;
	}
	switch(state)
	{
		case Start:
			break;
		case WaitforStart:
			if(last_state != state){
				position = 17;
				star[0] = 1;
				star[1] = 9;
				star[2] = 5;
				star[3] = 13;
				const unsigned char string[32] ="Press 1 to start                ";
				LCD_DisplayString(1, string);
			}
			break;
		case GameStart:
			initialdisplay();
			displaystar();
			break;
		case Gaming:
			break;
		case Fail:
			if(last_state != state){
			const unsigned char string2[16] ="Game Over";
			LCD_DisplayString(1, string2);}
	}
	return state;
}
enum MovingComponentStates {MCstart, MCwait, MCmoving};

int MCSM(int state)
{
	switch(state)
	{
		case MCstart:
			state = MCwait;
			break;
		case MCwait:
			if(task1.state == Gaming)
				state = MCmoving;
			break;
		case MCmoving:
			if(task1.state == WaitforStart || task1.state == Fail)
				state = MCwait;
			break;
		default:
			state = MCstart;
			break;
	}
	switch(state)
	{
		case MCstart:
			break;
		case MCwait:
			break;
		case MCmoving:
			if((position > 8 && position < 17)|| (position > 24 &&position < 33)){
				initialdisplay();
			movestar();
			displaystar();
			position --;
			}
			
			break;
		default:
			break;
	}
	return state;
}
//Monitors button connected to PA0.
//When button is pressed, shared variable "pause" is toggled.
int pauseButtonSMTick(int state){
	press = GetKeypadKey();
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
		if(press == '5'){
			if(position  >= 17)
				position -= 16;
		}else if (press == '8'){
			if(position <17 )
				position += 16;
		}else if (press == '9'){
			if(position != 16 && position != 31)
				position++;
		}else if (press == '7')
		{
			if(position != 1 && position != 17)
				position --;
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
	DDRC = 0xFF; PORTC  = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	DDRA = 0xF0; PORTA = 0x0F;
//	DDRB = 0xFF; PORTB = 0x00;
	LCD_init();
	//LCD_DisplayString(1,string);
    /* Insert your solution below */
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
    
    //Task 1 (pauseButtonToggleSM)
    
    task1.state =  Start;//Task initial state
    task1.period = 5;//Task Period
    task1.elapsedTime = task1.period;//Task current elapsed time.
    task1.TickFct = &Game;//Function pointer for the tick
    //Set the timer and turn it on
    
    task2.state = MCstart;//Task initial state
    task2.period =5;//Task Period
    task2.elapsedTime = task2.period;//Task current elapsed time.
    task2.TickFct = &MCSM;//Function pointer for the tick
	
	task3.state = pauseButton_wait;//Task initial state
	task3.period =5;//Task Period
	task3.elapsedTime = task3.period;//Task current elapsed time.
	task3.TickFct = &pauseButtonSMTick;//Function pointer for the tick
	unsigned long int GCD = tasks[0]->period;
	unsigned short i;//Scheduler for-loop iterator
	for( i = 1; i < numTasks;i ++)
	{
		GCD = findGCD(tasks[i]-> period, GCD);
	}
    TimerSet(GCD);
    TimerOn();
    
    while (1) {
		for(i = 0; i < numTasks; i++){//Scheduler code
			if(tasks[i]->elapsedTime == tasks[i]->period){//Task is ready to tick
				tasks[i]->state= tasks[i]->TickFct(tasks[i]->state);//set next state
				tasks[i]->elapsedTime = 0;//Reset the elapsed time for next tick;
			}
			tasks[i]->elapsedTime += GCD; 
		}
		LCD_Cursor(position);
		while(!TimerFlag);
		TimerFlag = 0;
    }
    return 1;
}
