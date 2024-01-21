// SpaceInvaders.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the ECE319K Lab 10

// Last Modified: 1/2/2023 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php

// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// buttons connected to PE0-PE3
// 32*R resistor DAC bit 0 on PB0 (least significant bit)
// 16*R resistor DAC bit 1 on PB1
// 8*R resistor DAC bit 2 on PB2 
// 4*R resistor DAC bit 3 on PB3
// 2*R resistor DAC bit 4 on PB4
// 1*R resistor DAC bit 5 on PB5 (most significant bit)
// LED on PD1
// LED on PD0


#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "Random.h"
#include "TExaS.h"
#include "../inc/ADC.h"
#include "Images.h"
#include "../inc/wave.h"
#include "Timer1.h"
#include "Timer0.h"
//#include "Sounds.h"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds
void EdgeCounterInit(void);  
void MovePlayer(uint32_t target); 
void destroyEnemy(uint8_t index);  
void shootBullet();
int8_t checkDestroy(void); 
void GameIntro(void); 
void GameInit(void); 
void GameDraw(void); 
void GameMove(void); 
void GameOver(void); 


typedef enum {Running, Paused, Over} gameStatus;
typedef enum {dead, alive} lifeStatus;
gameStatus status;

typedef enum {English, Spanish} Language_t;
Language_t myLanguage;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char *Phrases[3][2]={
  {Hello_English,Hello_Spanish},
  {Goodbye_English,Goodbye_Spanish},
  {Language_English,Language_Spanish}
};

struct Sprite{
	int16_t x, y; 
	uint8_t w, h;  
	lifeStatus life; 
	const uint16_t *image; 
	const unsigned short *black; 
}; 
typedef struct Sprite Sprite_t; 

Sprite_t player = {55, 159, 18, 8, alive, PlayerShip0}; 
Sprite_t bunker = {55, 151, 18, 5, alive, Bunker0};  	
Sprite_t pBullet = {62, 159, 4, 8, alive, bullet}; 

uint32_t convert(uint32_t x) {
	return x/35;
}

uint32_t ADCMail, ADCStatus; 
void Timer1A_Handler(void){ // can be used to perform tasks in background
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER1A timeout
   // execute user task
	ADCMail = ADC_In(); 
}


// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
      time--;
    }
    count--;
  }
}

void EdgeCounter_Init(void){  volatile long delay;     
  SYSCTL_RCGCGPIO_R |= 0x10;           // activate port E PIN 0 AND 3
  while((SYSCTL_PRGPIO_R&0x10)==0){}; // allow time for clock to start
  delay = 100;                  //    allow time to finish activating
  //GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  //GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0                              // 2) GPIO Port F needs to be unlocked
  //GPIO_PORTF_AMSEL_R &= ~0x11;  // 3) disable analog on PF4,0
                                // 4) configure PF4,0 as GPIO
  //GPIO_PORTF_PCTL_R &= ~0x000F000F;
  GPIO_PORTE_DIR_R &= ~0x9;    // 5) make PF4,0 in
  //GPIO_PORTF_AFSEL_R &= ~0x9;  // 6) disable alt funct on PF4,0
	//GPIO_PORTF_PUR_R |= 0x9;     
  GPIO_PORTE_DEN_R |= 0x9;     // 7) enable digital I/O on PF4,0
  GPIO_PORTE_IS_R &= ~0x9;     //    PF4,0 is edge-sensitive
  GPIO_PORTE_IBE_R &= ~0x9;    //    PF4,0 is not both edges
  GPIO_PORTE_IEV_R |= 0x9;     //    PF4,0 rising edge event (Neg logic)
  GPIO_PORTE_ICR_R = 0x9;      //    clear flag1-0
  GPIO_PORTE_IM_R |= 0x9;      // 8) arm interrupt on PF4,0
                                // 9) GPIOF priority 2
  NVIC_PRI1_R = (NVIC_PRI1_R&0xFF0FFFFF)|0x00000000;
  NVIC_EN0_R = 1<<4;   // (h) enable interrupt 4 in NVIC         
}

int8_t target, bulletShot = 0;  
uint32_t numShoots, numPauses, score, fallingEdges, highscore=0; 
void GPIOPortE_Handler(void){
	fallingEdges++;
	if ((GPIO_PORTE_RIS_R & 0x01)==0x01) {
		numShoots++; 
		if (fallingEdges == 1 && numShoots == 1) {
			myLanguage = English; 
		} else {
			target = checkDestroy();
			bulletShot = 1; 
		}
	}
	if ((GPIO_PORTE_RIS_R & 0x08)==0x08) {
		numPauses++; 
		if (fallingEdges == 1 && numPauses == 1) {
			myLanguage = Spanish;
			numPauses--; 
		} else {
			if (fallingEdges > 1) {
				if (numPauses % 2 == 1) {
					status = Paused;	
					Wave_Highpitch(); 
				} else {
					status = Running; 
					Wave_Highpitch(); 
				}
			}
		}
	}
	GPIO_PORTE_ICR_R=0x09;
}

/* 
  while(1){
    Delay100ms(20);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }  
*/  
Sprite_t arr[6];   
int main(void){  
	//initialize variables at run-time
	numShoots=0, numPauses=0, score=0, fallingEdges=0;  
  DisableInterrupts();
  TExaS_Init(NONE);       // Bus clock is 80 MHz 
	Random_Init(13); 
  Output_Init();
	EdgeCounter_Init();
	EnableInterrupts();  
	GameIntro();
	//DisableInterrupts(); 
	GameInit(); 
  ST7735_FillScreen(0x0000);            // set screen to black	
	ST7735_DrawBitmap(player.x, player.y, player.image, player.w, player.h); 
	ST7735_DrawBitmap(bunker.x, bunker.y, bunker.image, bunker.w, bunker.h);
	//EdgeCounter_Init();
	ADC_Init(); 
	//method where I have all sprites moving at 30HZ. however, I update the screen at computer clock speed. 
	Timer0_Init(&GameMove, 80000000/30); 
	Timer1_Init(8000000, 2);
	Wave_Init(); 
	EnableInterrupts(); 
	status = Running; 
	while(1){
		if (status == Running) {
			uint32_t target = convert(ADCMail);  
			while (player.x != target) { 
				MovePlayer(target); 
			}
			GameDraw();  
		}
		if (status == Over) {
			DisableInterrupts(); 
			Delay100ms(25); 
			GameOver(); 
		}
	}
}

void GameOver(void) {
	Output_Clear(); 
	ST7735_SetCursor(1, 1);
	if (myLanguage == English) {
		ST7735_OutString("GAME OVER");
	} else {
		ST7735_OutString("JUEGO TERMINADO");
	}
  ST7735_SetCursor(1, 2);
	/*if (score >= highscore) {
		highscore = score;
		ST7735_OutString("New High Score:");
	} else {
		ST7735_OutString("Score");
	}*/
	if (myLanguage == English) {
		ST7735_OutString("Score");
	} else {
		ST7735_OutString("Puntaje");
	}
  ST7735_SetCursor(1, 3);
  ST7735_OutUDec(score); 
	
}

void GameIntro(void) {              // delay 5 sec at 80 MHz
	ST7735_FillScreen(0x0000);   // set screen to black
	ST7735_SetCursor(1, 1);
	ST7735_OutString("SPACE INVADERS");
	ST7735_SetCursor(1, 2);
	ST7735_OutString("Pick English (SW1) "); 
	ST7735_SetCursor(1, 3);
	ST7735_OutString("Or Espa\xA4ol (SW4)"); 
	while (fallingEdges == 0) {
		//Wave_Sound1(); 
		//Wave_Sound2(); 
		//Wave_Sound3(); 
	}
	ST7735_FillScreen(0x0000);
	Delay100ms(1); 
}

void GameInit(void) {
	uint32_t randList[3]; 
	for (int8_t i = 0; i < 3; i++) {
		randList[i] = Random32() % 6;
	}
	arr[0].image = SmallEnemy10pointA; 
	arr[1].image = SmallEnemy10pointB; 
	arr[2].image = SmallEnemy20pointA; 
	arr[3].image = SmallEnemy20pointB; 
	arr[4].image = SmallEnemy30pointA; 
	arr[5].image = SmallEnemy30pointB; 
	for (int8_t i = 0; i < 6; i++)	{
		arr[i].x = 20*i; 
		arr[i].y = 10; 
		arr[i].w = 16; 
		arr[i].h = 10; 
		arr[i].black = BlackEnemy;
		if (i == randList[0]) {
			arr[i].life = alive; 
			//arr[i].image = SmallEnemy10pointA; 
		} else if (i == randList[1]) {
			arr[i].life = alive; 
			//arr[i].image = SmallEnemy10pointA; 
		} else if (i == randList[2]) {
			arr[i].life = alive; 
			//arr[i].image = SmallEnemy10pointA;  
		} else {
			arr[i].life = dead; 
			arr[i].image = BlackEnemy;  
		}		
	}
}
	
	/*for (int32_t i = 6; i < 12; i++)	{
		arr[i].x = 20*(i-6); 
		arr[i].y = 23; 
		arr[i].w = 16; 
		arr[i].h = 10; 
		arr[i].life = alive; 
		arr[i].image = SmallEnemy20pointA; 
		arr[i].black = BlackEnemy; 
	}
	
	for (int32_t i = 12; i < 18; i++)	{
		arr[i].x = 20*(i-12); 
		arr[i].y = 36; 
		arr[i].w = 16; 
		arr[i].h = 10; 
		arr[i].life = alive; 
		arr[i].image = SmallEnemy30pointA; 
		arr[i].black = BlackEnemy; 
	} */
	

void GameDraw(void) {
	ST7735_DrawBitmap(pBullet.x , pBullet.y, pBullet.image, pBullet.w, pBullet.h);
	ST7735_DrawBitmap(bunker.x, bunker.y, bunker.image, bunker.w, bunker.h);
	for (int8_t i = 0; i < 6; i++) {
		ST7735_DrawBitmap(arr[i].x, arr[i].y, arr[i].image, arr[i].w, arr[i].h);  
	}
}	

void GameMove(void) {
	static uint32_t change = 0; 
	if (status == Running) {
	if (bulletShot == 1) { 
		Wave_Shoot(); 
		//Wave_Highpitch(); 
		shootBullet(); 
		bulletShot = 0; 
		pBullet.y = 159; 
	}
	//check if all enemies dead
	uint8_t anyAlive = 0; 
	for (int8_t i = 0; i < 6; i++) {
		if (arr[i].life == alive) {
			anyAlive = 1; 
		}
		if (arr[i].y >= 151) {
			status = Over; 
		}
		arr[i].y++; 
	}
	if (anyAlive == 0) { 
		Timer0A_Stop(); 
		change++;
		Timer0_Init(&GameMove, 80000000/30 - (change * 50000)); 
		GameInit(); 
	}
}
}

void MovePlayer(uint32_t target) {
	if (player.x < target) {
		player.x++; 
		pBullet.x++; 
	} else {
		player.x--;
		pBullet.x--; 
	}
	ST7735_DrawBitmap(player.x , player.y, player.image, player.w, player.h);
}

void shootBullet(void) {
	while (pBullet.y > 0) {
		if ((target != -1) && (pBullet.y == arr[target].y)) {
			Wave_Killed(); 
			destroyEnemy(target); 
			arr[target].life = dead;  
			score+=(target+1); 
		}
		pBullet.y--; 
		ST7735_DrawBitmap(pBullet.x , pBullet.y, pBullet.image, pBullet.w, pBullet.h);
	}
}

// enemy boundary ranges x = {0-16} || {20-36} || {40-56} || {60-76} || {80-96} || {100-116} at 
// if in range, check if enemy exists
// if exists explode at enemy.y
// use this technique to determine if an enemy has been hit 
// int current x and y of bullet  
// returns index of enemy to destroy, -1 if no enemy to destory
int8_t checkDestroy() { 
	for (int8_t i = 0; i < 6; i++) {
		if (pBullet.x >= (20*i) && pBullet.x <= (20*i+16)) {
			if (arr[i].life == alive) {
				return i;  
			}
		} 
	}
	return -1;  
}

//destroys enemy of given index by replacing it with a black box of size w x h
void destroyEnemy(uint8_t index) {
	arr[index].image = BlackEnemy; 
}



