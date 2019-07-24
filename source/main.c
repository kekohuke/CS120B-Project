/*
 * test.c
 *
 * Created: 2019/7/6 18:06:56
 * Author : Coco
 */ 
#include <avr/io.h>
#include <avr/eeprom.h>
#include "scheduler.h"
#include "bit.h"
#include "io.c"
#include "io.h"
#include "keypad.h"
#include "shift.c"
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
//#include "lcd_8bit_task.h"
//#include "queue.h"
//#include "seven_seg.h"
//#include "stack.h"
#include "timer.h"
//#include "usart.h"




unsigned char point = 0;
static task task1, task2,task3, task4, task5;
task *tasks[] = {&task1, &task2, &task3, &task4, &task5};
enum pauseButtonSM_States {pauseButton_wait, pauseButton_press, pauseButton_release};
enum charactors {me,me_eating, box,block, coin};
void writedata(unsigned char row, unsigned char col, unsigned char simbol, unsigned char special_charactor)
{
		LCD_Cursor(col+(row-1)*16);
		if(!special_charactor){
			LCD_WriteData(simbol);
		}else{
			LCD_Char(simbol);
		}
	//	displaystar();
		
		
}
void defineChars()
{
	unsigned char mes[8] = {0x00, 0x00, 0x0F, 0x15, 0x16, 0x1E, 0x1F, 0x1F };  /* Custom char set for alphanumeric LCD Module */
	unsigned char mes_eating[8] =  {0x00, 0x00, 0x0F, 0x15, 0x17, 0x1F, 0x1F, 0x1F };
	unsigned char boxs[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x05, 0x07 };
	unsigned char blocks[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
	unsigned char coins[8] = {0x00, 0x00,0x04,0x0A, 0x11, 0x0A, 0x04,0x00 };
	LCD_Custom_Char(me, mes);
	LCD_Custom_Char(me_eating, mes_eating);
	LCD_Custom_Char(box, boxs);
	LCD_Custom_Char(block,blocks);
	LCD_Custom_Char(coin,coins);
	
}
unsigned char coins_number = 8;
unsigned char coins_begin = 0;
unsigned char star[4] = {1, 10, 6, 14};
unsigned char coins[100] = {3,4,5,6,7+16,8+16,9+16,10+16};

unsigned char i = 1;
unsigned char position = 17;
void initialdisplay()
{
	LCD_ClearScreen();
}

void movestar()
{
	for(i = 0; i < 4; i++)
	{
		if(star[i] == 1)
			star[i] = 16;
		else star[i]--;
	}
}
void movecoins()
{
		
	for(i = 0; i < coins_number; i++)
	{
		if(coins[i] != 0){
			if(coins[i] == 1)
				coins[i] = 16;
			else coins[i]--;
		}
	}
}

void displaycoin()
{
	for(i = coins_begin; i < coins_number; i++)
	{
		if(coins[i]==0)
		{
			if(i == coins_begin+1)
				coins_begin++;
		}
		else
			writedata(1, coins[i],coin,1 );
	}
}

void displaystar()
{
	for(i = 0; i < 2; i++)
	{
		writedata(1, star[i],'*',0 );
	}
	for(i = 2; i < 4; i++)
	{
		writedata(2, star[i],'*',0 );
	}
}
unsigned int press = '\0';
unsigned char failflag = 0;
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
			if(failflag){
				state = Fail;
				failflag = 0;
			}
			for(i = 0 ; i < 4;i++)
				if(position == (star[i]+(i>1)*16))
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
	int j;
	switch(state)
	{
		case Start:
			break;
		case WaitforStart:
			if(last_state != state){
			coins_number = 8;
			coins_begin = 0;
			star[0] = 1;
			star[1] = 10;
			star[2] = 6;
			star[3] = 14;
			for(j = 0; j< coins_number;j++)
			{
				coins[j] = 3+(j>3)*16+j*rand()%2;
			}
				const unsigned char string[32] ="Press 1 to start                ";
				LCD_DisplayString(1, string);
			}
			break;
		case GameStart:
			initialdisplay();
			position = 17;
			displaystar();
			writedata(1,position,me,1);
			displaycoin();
			point = 0;
			LCD_Cursor(0);
			break;
		case Gaming:
			break;
		case Fail:
			if(last_state != state){
				const unsigned char string2[31] ="Your Score:      Highest: ";
				LCD_DisplayString(1, string2);
				LCD_Cursor(14);
				LCD_WriteData('0'+point/10);
				LCD_Cursor(15);
				LCD_WriteData('0'+point%10);
				if(point > (eeprom_read_byte((uint8_t*)1)*10+eeprom_read_byte((uint8_t*)2)))
				{
					eeprom_write_byte((uint8_t*)1, point/10);
					eeprom_write_byte((uint8_t*)2, point%10);
				}
				LCD_Cursor(28);
				LCD_WriteData('0'+eeprom_read_byte((uint8_t*)1));
				LCD_Cursor(29);
				LCD_WriteData('0'+eeprom_read_byte((uint8_t*)2));
			}
	}
	return state;
}
enum MovingComponentStates {MCstart, MCwait, MCmoving};
unsigned char randnum= 0;
unsigned char soundFlag = 0;
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
			if((position > 7 && position < 17)|| (position > 23 &&position < 33)){
				initialdisplay();
				writedata(1,position,' ',0);
				position --;
				writedata(1,position,me,1);
				movecoins();
				movestar();
				randnum = rand()%3;
				if((randnum  ==0)&&star[0] != 16&& star[1]!= 16){
					coins[coins_number] = 16;
					coins_number++;
				}
				if((randnum ==1)&&star[2] != 16 && star[3] != 16){
					coins[coins_number] = 32;
					coins_number++;
				}
				displaystar();
				displaycoin();
				LCD_Cursor(0);
			
			}
			for(i = coins_begin; i < coins_number; i++){
				if(position == coins[i]){//get points!!!!!!!!!!!!!!!!!
					coins[i] = 0;
					writedata(1,position,me_eating,1);
					LCD_Cursor(0);

					point++;
				}
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

			if(position  >= 17){
				writedata(1,position,' ',0);
				position -= 16;
				writedata(1,position,me,1);
				LCD_Cursor(0);
			}

		}else if (press == '8'){

			if(position <17 ){
				writedata(1,position,' ',0);
				position += 16;
				writedata(1,position,me,1);
				LCD_Cursor(0);
			}

		}else if (press == '9'){

			if(position != 16 && position != 31){
				writedata(1,position,' ',0);
				position ++;
				writedata(1,position,me,1);
				LCD_Cursor(0);
			}

		}else if (press == '7')
		{

			if(position != 1 && position != 17){
				writedata(1,position,' ',0);
				position --;
				writedata(1,position,me,1);
				LCD_Cursor(0);
			}

		}

		break;
		
		case pauseButton_release:
		break;
		default:
		break;
	}
	return state;
}
enum GameTimeSMStates {WaitTime, StartCount, Counting, CountEnd};
unsigned char GameTimeCount = 0;
int GameTimeSM(int state)
{
	switch(state)
	{
		case WaitTime:
			if(tasks[0]->state == Gaming)
				state = StartCount;
			break;
		case StartCount:
			state = Counting;
			break;
		case Counting:
			if(tasks[0]->state == Gaming && GameTimeCount > 100)
			{
				state = CountEnd;
			}else if(tasks[0]->state != Gaming){ state = WaitTime;}
			break;
		case CountEnd:
			state = WaitTime;
			break;
		default:
			state = WaitTime;
			break;
	}
	switch(state)
	{
		case WaitTime:
			break;
		case StartCount:
			GameTimeCount = 0;
			break;
		case Counting:
			GameTimeCount++;
			break;
		case CountEnd:
			if(tasks[0]->state == Gaming)
				failflag =1;
			break;
	}
	
	return state;
}

enum JumpStates {WaitforJump, Jumping, Landing};
	
int JumpSM(int state)
{
	switch(state){
		case WaitforJump:
			if(position <17)
				state = Jumping;
			break;
		case Jumping:
			state = Landing;
			break;
		case Landing:
			state = WaitforJump;
			break;
		default:
			state = WaitforJump;
	}
	switch(state){
		case WaitforJump:
			break;
		case Jumping:
			break;
		case Landing:
			writedata(1,position,' ',0);
			position +=16;
			writedata(1,position,me,1);
			LCD_Cursor(0);
			break;
	}
	return state;
}


int main(void) {
    /* Insert DDR and PORT initializations */
	/*unsigned char x;
	DDRB = 0xFF; PORTB = 0x00;*/
	DDRC = 0xFF; PORTC  = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	DDRA = 0xF0; PORTA = 0x0F;
//	DDRB = 0xFF; PORTB = 0x00;
	defineChars();
	LCD_init();
	eeprom_write_byte((uint8_t*)1,0);
	eeprom_write_byte((uint8_t*)2,0);
	//LCD_DisplayString(1,string);
    /* Insert your solution below */
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
    
    //Task 1 (pauseButtonToggleSM)sili guo
    
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
	task3.period = 5;//Task Period
	task3.elapsedTime = task3.period;//Task current elapsed time.
	task3.TickFct = &pauseButtonSMTick;//Function pointer for the tick

	task4.state = WaitTime;//Task initial state
	task4.period = 50;//Task Period
	task4.elapsedTime = task4.period;//Task current elapsed time.
	task4.TickFct = &GameTimeSM;//Function pointer for the tick

	task5.state = WaitforJump;//Task initial state
	task5.period = 40;//Task Period
	task5.elapsedTime = task5.period;//Task current elapsed time.
	task5.TickFct = &JumpSM;//Function pointer for the tick
	

	unsigned long int GCD = tasks[0]->period;
	unsigned short i;//Scheduler for-loop iterator
	
	for( i = 1; i < numTasks;i ++)
	{
		GCD = findGCD(tasks[i]-> period, GCD);
	}
    TimerSet(GCD);
    TimerOn();
    
       uint8_t led_pattern[8]={
                        0b10000001,
                        0b11000011,
                        0b11100111,
                        0b11111111,
                        0b01111110,
                        0b00111100,
                        0b00011000,
                        0b00000000,
                     };
		HC595Write(led_pattern[2]);  
    while (1) {
		for(i = 0; i < numTasks; i++){//Scheduler code
			if(tasks[i]->elapsedTime == tasks[i]->period){//Task is ready to tick
				tasks[i]->state= tasks[i]->TickFct(tasks[i]->state);//set next state
				tasks[i]->elapsedTime = 0;//Reset the elapsed time for next tick;
			}
			tasks[i]->elapsedTime += GCD; 
		}
		while(!TimerFlag);
		TimerFlag = 0;
    }
    return 1;
}
