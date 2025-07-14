#include "tm4c123gh6pm.h"
#include "sound.h"
#include "os.h"

typedef struct {
  uint16_t frequency;  //Frequency corresponding to the note
  uint16_t duration;   //How long to play the note in millisecond
} Note;

const Note GameOverTune[] = {
  {1046, 150},  // C5
  {987,  150},  // B4
  {932,  150},  // A# 
  {880,  150},  // A
  {830,  150},  // G#
  {784,  300},  // G4
  {0, 0}
};

void Sound_Init(void){
  SYSCTL_RCGCPWM_R |= 0x02;       // Activate PWM1
  SYSCTL_RCGCGPIO_R |= 0x20;      // Activate Port F
  while((SYSCTL_PRGPIO_R & 0x20) == 0){}; // Ready

  GPIO_PORTF_AFSEL_R |= 0x04;     // Enable alt funct on PF2
  GPIO_PORTF_PCTL_R &= ~0x00000F00;
  GPIO_PORTF_PCTL_R |= 0x00000500; // Configure PF2 as M1PWM6
  GPIO_PORTF_AMSEL_R &= ~0x04;    // Disable analog on PF2
  GPIO_PORTF_DIR_R |= 0x04;       // PF2 output
  GPIO_PORTF_DEN_R |= 0x04;       // enable digital I/O on PF2

  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV;     
  SYSCTL_RCC_R = (SYSCTL_RCC_R & ~SYSCTL_RCC_PWMDIV_M) + SYSCTL_RCC_PWMDIV_64; 

  PWM1_3_CTL_R = 0;               // Generator 3 disable
  PWM1_3_GENA_R = 0xC8;           // low on LOAD, high on CMPA down
  PWM1_3_LOAD_R = 1000;           // Set period (1.25kHz)
  PWM1_3_CMPA_R = 500;            // 50% duty
  PWM1_3_CTL_R |= 0x01;           // enable generator

 // PWM1_ENABLE_R |= 0x40;          // enable M1PWM6 (bit 6)
}


//******************* Test Buzzer **********
//void Sound_On(void){
//  PWM1_ENABLE_R |= 0x40; // Enable M1PWM6
//}

//void Sound_Off(void){
//  PWM1_ENABLE_R &= ~0x40; // Disable M1PWM6
//}

void PlaySound(uint32_t frequency){
  uint32_t period = 80000000 / 64 / frequency;

  PWM1_3_LOAD_R = period;
  PWM1_3_CMPA_R = period / 2;
  PWM1_ENABLE_R |= 0x40; // Enable M1PWM6 (PF2)
}

void StopSound(void){
  PWM1_ENABLE_R &= ~0x40;         // Disable M1PWM6
}


//void PlayGameOverTune(void){
//  int i = 0;
//  while (GameOverTune[i].frequency != 0){
//    PlaySound(GameOverTune[i].frequency);
//    OS_Sleep(GameOverTune[i].duration);
//    StopSound();
//    OS_Sleep(20);
//    i++;
//  }
//}

void GetScoreSound(void){
  PlaySound(1568);            //G5,1568Hz
  for(volatile int i = 0; i < 400000; i++){} 
  StopSound();
}

void PlayGameOverSound(void){
  int i = 0;
  while (GameOverTune[i].frequency != 0){
    PlaySound(GameOverTune[i].frequency);

    for(int j = 0; j < 800000; j++){}

    StopSound();
			
    for(int k = 0; k < 400000; k++){}

    i++;
  }
}
