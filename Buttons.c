#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "Sound.h"

volatile uint32_t FallingEdges = 0;

void EdgeCounter_Init(void){       
	SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
	GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4 in (built-in button)
	GPIO_PORTF_DEN_R |= 0x11;     //     enable digital I/O on PF4
	GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4
	GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4 is edge-sensitive
	GPIO_PORTF_IBE_R &= ~0x11;    //     PF4 is not both edges
	GPIO_PORTF_IEV_R &= ~0x11;    //     PF4 falling edge event
	GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4
	GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF0FFFFF)|0x00400000; // (g) priority 5
	NVIC_EN0_R = 1<<30;      // (h) enable interrupt 30 in NVIC
}


void GPIOPortF_Handler(void){

	if ((GPIO_PORTF_RIS & 0x01)==0x01) {
		playSound(Shoot); 
	}
	//if ((GPIO_PORTF_RIS & 0x10)==0x10) {
		//playSound(Drop); 
	//}
	GPIO_PORTF_ICR_R=0x11;
}
	