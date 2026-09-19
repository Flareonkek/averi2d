
#include "common.h"
#include "sysimp.c"
//#include "TEMP_DEMOsysimp.c" // JUST TO SHOW OFF LOADING
#include "menu.c"
#include "game.c"

//------------------------------------------------------------------------------------------------------------------------------------------
// The MAIN FUNCTION
//------------------------------------------------------------------------------------------------------------------------------------------

int main(void) {
	
	unsigned short kd = 0; // Which keys are down
	unsigned short kp = 0; // Which keys were down in the previous frame
	
	const int rs_size = 5000; // Used to make sure we don't read or write out of bounds with rs
	struct r rs[5000]; // Array of struct r (things to render on screen)
	int rslen = 0; // How many need to be rendered currently
	
	si_start("resource/gameveri_sheet_0910.png");
	
	int state = MAIN_MENU;
	
	//----------------------------------------------------------------------------------------------------------------------------------
	// Main loop, runs about 30 times per second (si_draw() regulates the frame duration)
	//----------------------------------------------------------------------------------------------------------------------------------
	
	while (si_isRunning() && state != MAIN_QUIT) {
		
		// READ KEYBOARD INPUT
		kd = si_keys();
		
		// TICK PROGRAM LOGIC
		switch (state) {
			case MAIN_MENU:
				int menu_instruction;
				state = m_tick(rs, &rslen, rs_size, kd, kp, &menu_instruction); // Menu tick
				if (menu_instruction) {
					switch (menu_instruction) {
						case 1:
							g_load_level("level1.txt");
							break;
						case 2:
							g_load_level("level2.txt");
							break;
					}
				}
				break;
			case MAIN_GAME:
				state = g_tick(rs, &rslen, rs_size, kd, kp); // Game tick
				break;
		}
		
		// RENDER rs ONTO THE SCREEN
		si_draw(rs, rslen);
		
		// RECORD HELD KEYS FOR NEXT FRAME'S REFERENCE
		kp = kd;
	}
	
	si_end();
	
	return 0; // Program ends reporting no errors
}
