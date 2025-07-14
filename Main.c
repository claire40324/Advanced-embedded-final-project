#include <stdint.h>
#include <stdio.h>
#include "PLL.h"
#include "LCD.h"
#include "os.h"
#include "joystick.h"
#include "sound.h"
#include "bitmaps.h"
#include "FIFO.h"
#include "PORTE.h"
#include "tm4c123gh6pm.h"
// main picture in game setting maximum will have four threads to control the cubes,
// on LCD screen, it can have 36 semaphores(total 36 cubes)

// define in lcd.c and os.c 
// initialize as 0
extern Sema4Type LCDFree; 
// define in os.c
// initialize as 0
extern Sema4Type CubeCnt;


// 0 -> main page, showing highest and new score, settings, start game
// 1 -> playing the game
// 2 -> setting page 
int state = 2;
// Constants
#define BGCOLOR     					LCD_BLACK
#define CROSSSIZE            			5

//------------------Defines and Variables-------------------
uint16_t origin[2]; // the original ADC value of x,y if the joystick is not touched
int16_t x = 63;  // horizontal position of the crosshair, initially 63
int16_t y = 63;  // vertical position of the crosshair, initially 63
int16_t prevx = 63;
int16_t	prevy = 63;
uint8_t select;  // joystick push

#define LFSR_POLY_MASK 0xB8
int cubeCount = 0; //
int game_type = 0;
// 0 -> five cubes, 1-> one cube
int game_mode = 0;
// use to differentiate 50, 100, 200 seconds
// 0 -> 50 sec 
// 1 -> 100 sec
// 2 -> 200 sec
// 3 -> infinity
int p_game_type = 0;
int p_game_mode = 0;
int selector = 0; 
// which choice we select, different page have differenct choices
// state 0 -> onle 0 or 1 can select( 0 -> settings, 1 -> starting game )
// state 1 -> gaming, we don't select
// state 2 -> 0- 10 can select
// 0 -> sound on/off
// 2 -> 50 - trial
// 3 -> 100 - forge
// 4 -> 200 - Dominion
// 5 -> ~~~ - Endurance
// 7 -> five
// 8 -> solus
// 10 -> Start Game
int sound = 1;  // 1 -> open sound, 0 -> mute
int oneOff_0 = 0;  // use to start new panel thread
int oneOff_1 = 0;  // use to start new game thread and protect adding too much threads
int oneOff_2 = 0;  // use to start new setting thread
int nrounds = 0;
int mainY = 0;
int mainX = 0;  
int db_sw1 = 0;  // use to debounce 
int db_sw2 = 0;  // use to debounce 
int HighScore = 0; 
int scores;
//---------------------User debugging-----------------------

#define TEST_TIMER 0		// Change to 1 if testing the timer
#define TEST_PERIOD 4000000  // Defined by user
#define PERIOD 800000  		// Defined by user

unsigned long Count;   		// number of times thread loops


//--------------------------------------------------------------
void CrossHair_Init(void){
	BSP_LCD_FillScreen(LCD_BLACK);	// Draw a black screen
	BSP_Joystick_Input(&origin[0], &origin[1], &select); // Initial values of the joystick, used as reference
}

//******** Producer *************** 
void Producer(void){
#if TEST_TIMER
	PE1 ^= 0x02;	// heartbeat
	Count++;	// Increment dummy variable			
#else
	// Variable to hold updated x and y values
	int16_t newX = x;
	int16_t newY = y;
	int16_t deltaX = 0;
	int16_t deltaY = 0;
	uint16_t rawX, rawY; // To hold raw adc values
	uint8_t select;	// To hold pushbutton status
	rxDataType data;
	
	//read joystick input
	BSP_Joystick_Input(&rawX, &rawY, &select);

	//detect joystick movement in all four direction and move at a speed of 6
	if(rawX > 3000)
		origin[0] +=6; 
	if(rawY < 1800)
		origin[1] +=6; 
	if(rawX < 1400)
		origin[0] -=6; 
	if(rawY > 3000)
		origin[1] -=6; 	
	
	//boundary condition on all four sides of the frame
	if(origin[0] >= 4050)
		origin[0] =  4050;

	if(origin[0] <= 50)
		origin[0] = 50; 

	if(origin[1] <= 50)
		origin[1] = 50;  

	if(origin[1] >= 3550)
		origin[1] =  3550;	

	//scale x,y from 0 - 4095 into 0 - 128
	newX    = ( origin[0] * 128 ) / 4095;
	newY    = ( origin[1] * 128 ) / 4095;	
	
	//store x and y into fifo
	data.x  = newX;
	data.y  = newY;
	RxFifo_Put(data);
		
#endif
}
//******** display bitmap *********
/*0xbf4f51,  // Bittersweet Shimmer (rare red)
0xda2c43,  // Rusty Red (intense warm red)
0x00416a,  // Indigo Dye (deep rare blue)
0x2e5090,  // YInMn Blue (recently discovered pigment)
0xe97451,  // Burnt Sienna (earthy orange)
0x997a8d,  // Mountbatten Pink (military-muted pink)
0x7cb9e8,  // Aero (silky sky blue)
0xeedc82,  // Flax (straw-like rare yellow)
0xf4f0ec,  // Isabelline (creamy off-white)
0x86608e   // Pomp and Power (dusky violet)
*/
uint16_t colors[20] = {
    0xF647, // Tomato
    0xF500, // Orange Red
    0xF493, // Deep Pink
    0xF69B, // Hot Pink
    0xFD00, // Gold
    0xF800, // Dark Orange
    0xF68C, // Khaki
    0xF493, // Deep Pink
    0xBFFF, // Deep Sky Blue
    0xFF7F, // Spring Green
    0xDFFF, // Green Yellow
    0x7F00, // Chartreuse
    0xFF00, // Fuchsia
    0xFBC1, // Light Pink
    0xF500, // Orange Red
    0x2CD3, // Lime Green
    0xFD00, // Gold
    0xF5FA, // Mint Cream
    0xA52A, // Brown
    0xF4A3  // Neon Carrot
};

void DisplayBitmap(uint16_t startX, uint16_t startY, int clearMode) {
    int width = 17;
    int height = 17;
    uint32_t index = 0;

    if (clearMode) {
        for (int y = startY; y < startY + height; y++) {
            for (int x = startX; x < startX + width; x++) {
                BSP_LCD_FillRect(x, y, 1, 1, 0x1AA6); // Clear with default color
            }
        }
        return;
    }

    for (int y = startY; y < startY + height; y++) {
        for (int x = startX; x < startX + width; x++) {
            uint16_t color = bg1[index++];
            BSP_LCD_FillRect(x, y, 1, 1, color); // Draw pixel
        }
    }
}


uint32_t read_adc_value(void);
#define POLY_MASK_32 0xB4BCD35C
#define POLY_MASK_31 0x7A5BC2E3

typedef unsigned int uint;
uint32_t lfsr32, lfsr31;

int shift_lfsr(uint* lfsr, uint polynomial_mask)
{
    int feedback;
    feedback = *lfsr & 1;
    
    if(feedback == 1)
    {
        *lfsr ^=  polynomial_mask;
    }
    return *lfsr;
}

void init_lfsr(void)
{
	// constant
    lfsr32 = 0xABCDEFAB; 
    lfsr31 = read_adc_value();
	// generate seed value 
}

// generate seed value by reading adc value
uint32_t read_adc_value(void) {
    uint32_t adc_value;

    
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
    while (!(SYSCTL_PRADC_R & SYSCTL_PRADC_R0));
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
    while (!(SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R4));

    
    GPIO_PORTE_AMSEL_R |= 0x08;
    GPIO_PORTE_PCTL_R &= ~0x0000F000;
    GPIO_PORTE_DIR_R &= ~0x08;
    GPIO_PORTE_AFSEL_R &= ~0x08;
    GPIO_PORTE_DEN_R &= ~0x08;

    
    ADC0_ACTSS_R &= ~0x0001;
    ADC0_EMUX_R &= ~0x000F;
    ADC0_SSMUX0_R = 0;
    ADC0_SSCTL0_R |= 0x0006;
    ADC0_ACTSS_R |= 0x0001;

    
    ADC0_PSSI_R |= 0x0001;

    
    while ((ADC0_RIS_R & 0x0001) == 0);

    
    adc_value = ADC0_SSFIFO0_R & 0xFFF;

    
    ADC0_ISC_R = 0x0001;

    return adc_value;
}

int next_rand(int bounds)
{
	// using constant (lfsr32) ^ seeed value to make even random number
	return ((shift_lfsr(&lfsr32, POLY_MASK_32) ^ shift_lfsr(&lfsr31, POLY_MASK_31))&0xFFFF) % bounds;
}




// use to check whether we hit the cubes or not,
// return 1-> means we hit the cube
// return 0-> otherwise
int update(int xcord, int ycord){
	
	// update xcord and ycord to 6*6 grid index
	int newx = (xcord * 6) / 128;
	int newy = (ycord * 66)  / 1280;

	// if x and y change
	if(mainX != newx || mainY != newy){
		// check if this cube someone is using it 
		// if someone using it->means we hit the cube
		// checksemapt will return 0, if someone using it
		if(!checkSemaPt(newx,newy)){		
			score(newx,newy);
			scores++;
			// remove this cube on LCD screen
			BSP_LCD_FillRect((newx * 21), (newy * 19), 17, 17, 0x1AA6); 
			if(sound){
				GetScoreSound();}
			
			mainX = newx;
			mainY = newy;
			return 1;
		}
		
		mainX = newx;
		mainY = newy;

	}
	return 0;
}


//******** Consumer *************** 
void Consumer(void){
	
	while(state == 1){	

		rxDataType dataC;
		RxFifo_Get(&dataC);

		// grab semaphore 
		OS_Wait(&LCDFree);
		// if we're not in gaming state
		if(state != 1){
			// release lock
			OS_Signal(&LCDFree);
			break;
		} 
		// if we doesn't hit the cube
		if(!update(dataC.x, dataC.y)){
			// erase crosshair
			BSP_LCD_DrawCrosshair(prevx, prevy, 0x1AA6);
			// draw new crosshair
			BSP_LCD_DrawCrosshair(dataC.x, dataC.y, LCD_WHITE);
			prevx = dataC.x;
			prevy = dataC.y;
		}	

		OS_Signal(&LCDFree);
		OS_Suspend();
	}
	// grab sempahore again to erase crosshair
	OS_Wait(&LCDFree);
	// earse cross hair to background colors
	BSP_LCD_DrawCrosshair(prevx, prevy, 0x1AA6);
	// release lcd lock
	OS_Signal(&LCDFree);
	// kill this thread to prevent stack overflow
	OS_Kill();
}

int tt = 0;
int cc = 0;
// use to add cube on lcd, maximum will have four cube on screen and each is on thread 
void addTile() {
	int worked = 0;
	int xnew = 0;  // next position we're going to
	int ynew = 0;
	int x = next_rand(5);  // random value( 0 - 4 )
	int y = next_rand(5);  // random value( 0 - 4 )
	int direction = next_rand(4);  // only have four directions
	int initial = 0;
	int rands = next_rand(19); // randomly choose one color from 20 options
	int color = colors[rands];

	while(state == 1){
		worked = 0; // use to determine if we find a way to move 
	
		OS_Wait(&LCDFree);
		// means this thread haven't been hit, still alive
 		if(RunPt->terminate == 0) {
			// try to move -> and get semaphore for this position
    		if (direction == 0 && x < 5 && checkSemaPt(x+1, y)) {
        		xnew = x + 1;
        		worked = 1;
    		} 
			// try to move <- and get semaphore for this position
    		else if (direction == 1 && x > 0 && checkSemaPt(x-1, y)) {
        		xnew = x - 1;
        		worked = 1;
    		} 
			// try to move up and get semaphore for this position
    		else if (direction == 2 && y < 5 && checkSemaPt(x, y+1)) {
        		ynew = y + 1;
        		worked = 1;
    		} 
			// try to move down and get semaphore for this position
    		else if (direction == 3 && y > 0 && checkSemaPt(x, y-1)) {
        		ynew = y - 1;
        		worked = 1;
    		}
    
    		direction = (direction + 1) % 4;
		}
		// this thread is being hit
		else {
			scores++;
			OS_Signal(&LCDFree);
			break;	
		}
		// if we try up, down, ->, <-, and can't find a way to move
		if(!worked){
			OS_Signal(&LCDFree);
			OS_Suspend();
			continue;
		}

		// after finding one direction we can walk, it might be hit immidiately
		// if hit or state is not in gaming 
		if(RunPt->terminate == 1 || state != 1){
			OS_Signal(&LCDFree);
			break;
		// if cube is no hit we need to update new cube position
		}else{	
			// if the first time generating cube 
			if(!initial){initial++;}
			// if it's not the first time generating cube, remove the previous cube
			else{
				BSP_LCD_FillRect((x * 21), (y * 19), 21, 21, 0x1AA6);
			}
			// draw new cube
			BSP_LCD_FillRect((xnew * 21), (ynew * 19), 21, 21, color);
		
		}
		OS_Signal(&LCDFree);
		// update position
		x = xnew;
		y = ynew;
		// change to next color
	  	rands = (rands + 1)% 20;
		color = colors[rands];
	
		// put this thread to sleep
		OS_Sleep(3000);

	}
	// if state != 1, do following things or cube being hit
	OS_Wait(&CubeCnt);
	cubeCount--;
	OS_Signal(&CubeCnt);

	OS_Wait(&LCDFree);
	// remove previouse cube if we're still in gaming state
	// because other thread might preempt and if we still clean that cube,
	// will cause problem like removing certain thing on setting or panel page 
	// 這個條件表示：
	// 遊戲還在進行（state == 1）
	// 這顆 cube 至少畫過一次（initial > 0）
	// 這表示畫面上可能還留有這顆 cube 的圖形，但這個 thread 即將結束，因此要清除它。
	if(state == 1 && initial > 0){
		BSP_LCD_FillRect((x * 21), (y * 19)+1, 21, 21, 0x1AA6);
	}
	OS_Signal(&LCDFree);
		
	// free the cube semaphore 
	if( state != 1){
		OS_FreePt(x,y);
	}else{
		// if we're in solu, keep adding one cube
		if(game_type == 1){
			OS_AddThread(&addTile,128,1);	
		}
	}
	OS_Kill();	
}



void stop(){
	if(state == 1){
		state = 0;
		// to let updater to add panel thread
		oneOff_0 = 0; 
		// update the highest score
		if(scores > HighScore){
			HighScore = scores;
		}
		// play gameover sound
		if(sound)
			PlayGameOverSound();
	}
}



void panel(){
	OS_Wait(&LCDFree);
	BSP_LCD_FillScreen(LCD_BLACK);
	OS_Signal(&LCDFree);
	int y = 0;
  	int prevy = -1;
	char newScoreStr[20];
  	char HighScoreStr[20];
  	while(state == 0){
		OS_Wait(&LCDFree);
		if(state != 0)
		{
			OS_Signal(&LCDFree);
			break;}
		
		// draw every option
		BSP_LCD_String(3/2, 4, "Highest Score");
		//BSP_LCD_Value(2, 8, 430);
		sprintf(HighScoreStr, "  %d", HighScore);
  		BSP_LCD_String(2, 8, HighScoreStr);	

		BSP_LCD_String (5, 6, "New Score");
		sprintf(newScoreStr, "  %d", scores);
  		BSP_LCD_String(6, 8, newScoreStr);
		//BSP_LCD_Value(6, 8, 234);
		BSP_LCD_String(9, 7, "Settings");	
		BSP_LCD_String(11, 6, "Start Game");
	
		// means we select setting page
		if(selector == 0){
      		y = 9 * 10;   // Settings at number 9 line
  		} 
		// means we select gaming page
		else {
      		y = 11 * 10;  // Start Game at number 11 line
  		}
		// draw little cube
		BSP_LCD_FillRect(21, y, 6, 6, 0xea2a);  

		// earse previous little cube
  		if(prevy != y)
		{
     		BSP_LCD_FillRect(21, prevy, 6, 6, 0x0000);  
  		}
		prevy = y;
	
		OS_Signal(&LCDFree);
		OS_Suspend();
	}
	OS_Kill();
}









void settings(){
	OS_Wait(&LCDFree);
	BSP_LCD_FillScreen(LCD_BLACK);
	OS_Signal(&LCDFree);
	int y = 0;
	int prevy =0;
  	while(state == 2){
		
		OS_Wait(&LCDFree);
		// if we enter blocked state and another thread could change it to another state
		// so if we've been signal, we need to check it again
		if(state != 2)
		{
			OS_Signal(&LCDFree);
			break;}
		if(sound){
			BSP_LCD_String(1, 7, "Sound  On");
		}else{
			BSP_LCD_String(1, 7, "Sound Off");		
		}
	
		//BSP_LCD_String(2, 7, "100 - Forge");	
		BSP_LCD_String(3, 7, "50 - Trial");
		BSP_LCD_String(4, 7, "100 - Forge");	
		BSP_LCD_String(5, 7, "200 - Dominion");
		BSP_LCD_String(6, 7, "~~~ - Endurance");
	
		BSP_LCD_String(8, 7, "five");
		BSP_LCD_String(9, 7, "Solus");	
		BSP_LCD_String(11, 7, "Start Game");	

		// draw little cubes to corresponding line
		// selector = 0 → y = 10 (option 1) 10 pixels apart
		y = (selector + 1) * 10;

		BSP_LCD_FillRect(21,(3 + game_mode)*10, 6,6,  0x850d);	
		BSP_LCD_FillRect(21,(8 + game_type)*10, 6,6,  0x850d);		
		BSP_LCD_FillRect(21,y, 6,6,  0xea2a);

		// if changed, clean the little cubes
		if(prevy != y){
			BSP_LCD_FillRect(21,prevy, 6,6,  0x0000);		
		}
		if(p_game_mode != game_mode){
			BSP_LCD_FillRect(21,(3 + p_game_mode)*10, 6,6,  0x0000);	
		}		
		if(p_game_type != game_type){
			BSP_LCD_FillRect(21,(8 + p_game_type)*10, 6,6,  0x0000);	
		}

		prevy = y;
		p_game_mode = game_mode;
		p_game_type = game_type;
		
		OS_Signal(&LCDFree);
		OS_Suspend();
	}
	OS_Kill();
}

// up button
void SW1Push(void){
	// debounce, if (now - the last time we push button) > 10 seconds -> valid push
	if((OS_MsTime() - db_sw2) > 10){
		// record time when we push the button
		db_sw2 = OS_MsTime();
	}else{
		return; // the time is too close, ignore behavior
	}
	
	// if sound open-> play sound
	if(sound){
		GetScoreSound();
	}
	
	// in state == 0, only 0 or 1 can select
	if(state == 0)
	{
		// simulate pushing button, once user push button meaning they change
		// to select other choice, so they push button we need to change selector
		// better way -> if(selector == 0){ selector ^= 1;}
		if(selector == 0){
			selector = 1;
		}
		else{
			selector = 0;
		}
	}
	
	// int setting page 
	// better way -> 
	if (state == 2) {
		do {
			selector = (selector + 1) % 11;
		} while (selector == 1 || selector == 5 || selector == 6 || selector == 9);
	}
}


// down button
void SW2Push(void){
	// debounce, if (now - the last time we push button) > 10 seconds -> valid push
	if((OS_MsTime() - db_sw2) > 10){
		// record time when we push the button
		db_sw2 = OS_MsTime();
	}else{
		return;  // the time is too close, ignore behavior
	}
	
	// only 0 ( settings )or 1 (starting game) can select
	if(state == 0)
	{
		// sw1 push will change selector and use sw2 to check
		// if selector == 1 -> meaning we're in gaming state
		if(selector == 1) 
		{
			oneOff_1 = 0; // means we want to start new game, refreshing page
			state = 1; // gaming state
		}
		else
		{
			oneOff_2 = 0; // means we want to start new setting, refreshing page
			state = 2; // setting page 
		}
	}
	
	if (state == 2) {
		switch (selector) {
			// sound on/off
			case 0:
				sound ^= 1;
				break;
			// game mode 0-> 50sec, 1 ->100 sec
			case 2:
			case 3:
			case 4:
			case 5:
				game_mode = selector - 2;
				break;
			// game type 0-> five cubes, 1-> 1 cube
			case 7:
			case 8:
				game_type = selector - 7;
				break;
			// starting game 10
			default:
				oneOff_1 = 0;
				state = 1;
				break;
		}
	}	

	// if we're in game right now
	if(state == 1 && oneOff_1){
		stop();
	}
}




int xv = 0;
void Updater(){
	
	while(1){
		
		while(state == 1){
			
			// we want to start new game 
			if(!oneOff_1){
				// checking if this semaphore another threads are waiting 
				// return this lock waiting count
				// if someone is waiting, infinite loop
				while(GetNumberOfWaitingThreads(&LCDFree) != 0){} 
				// grab the lock
				OS_Wait(&LCDFree);
				// draw blue background in gaming 
				BSP_LCD_FillRect(0,0, 128, 118,  0x1AA6); 
				// finish drawing
				OS_Signal(&LCDFree);
				OS_InitSemaphore(&CubeCnt, 1); // ***can initial in start() ?
				OS_InitSemaphores();  // initialize 36 semaphores for lcd
				scores = 0;
				// grab semaphore to set cubecount
				OS_Wait(&CubeCnt);
				cubeCount = 0;
				OS_Signal(&CubeCnt);
				// release semaphore
				// ***if our game type is solus, what about five cubes?
				// we add it more cubes after initialization game in else part
				if(game_type == 1){
					OS_AddThread(&addTile,128,1);
				}
				RxFifo_Init();
				OS_AddThread(&Consumer,128,1);
				if(game_mode == 0){nrounds = 50;}
				if(game_mode == 1){nrounds = 100;}
				if(game_mode == 2){nrounds = 200;}
				BeginningSound();
				oneOff_1++;

				OS_Suspend();
			}
			// execution of game 
			else{
				// if we exceed 0.5 sec
				if(OS_MsTime() > 500){
					OS_ClearMsTime(); // clear ms time to 0 to reset
					nrounds--;		  // deduct nround every 0.5 sec
				}
				//ex: if we have 50 nrounds, it weill become 0 after 25 seconds
				// grab semaphore once we need to change value to cubeCount
				OS_Wait(&CubeCnt);
				// keep checking if cubecount < 4 and game type is five cubes
				// so we need to keep adding cube thread
				if(cubeCount < 4 && game_type == 0){		
					OS_AddThread(&addTile,128,1);	
					cubeCount++;			
				}
				// release semaphore 
				OS_Signal(&CubeCnt);
				// grab semaphore to draw things on LCD
				OS_Wait(&LCDFree);
				BSP_LCD_Message (1, 0, 1, "X > ",  nrounds);
				BSP_LCD_Message (1, 0, 13, "s > ", scores);	
				// release semaphore
				OS_Signal(&LCDFree);
				// if nrounds end and game mode not equal to infinity		
				if(nrounds == 0 && game_mode != 3){
					stop();
				}
			}
		
			OS_Suspend();
		}
	
		if(state == 2 && oneOff_2 == 0){
			while(GetNumberOfWaitingThreads(&LCDFree) != 0){}
			OS_AddThread(&settings,128,1);
			oneOff_2++;
		}
		if(state == 0 && oneOff_0 == 0){
			while(GetNumberOfWaitingThreads(&LCDFree) != 0){}
			OS_AddThread(&panel,128,1); 
			oneOff_0++;
		}	
	
		OS_Suspend(); 
	}
}

// entry point
void start(){
	state = 0;
	OS_InitSemaphore(&LCDFree, 1);
	OS_InitSemaphore(&CubeCnt, 1);
	OS_AddThread(&Updater,128,1); // thread always in the system
	OS_AddSW1Task(&SW1Push, 4);   // add interupt thread
	OS_AddSW2Task(&SW2Push, 4);	
	

}

//******** Main *************** 
int main(void){
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
#if TEST_TIMER
	PortE_Init();       // profile user threads
	Count = 0;
	OS_AddPeriodicThread(&Producer, TEST_PERIOD, 1);
	while(1){}
#else
	OS_Init(); 
	Sound_Init();
  	BSP_LCD_Init();        // initialize LCD
	BSP_Joystick_Init();   // initialize Joystick
  	CrossHair_Init();      
	RxFifo_Init();
	OS_AddPeriodicThread(&Producer,PERIOD, 1);
	
  	init_lfsr();
	start();
	  
	OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here           // this never executes
#endif

} 




	
	
