#include <stdio.h>
#include <stdbool.h>
#include "render.h"
#include "sysimp.c"
#include "menu.c"
#include "game.c"

//------------------------------------------------------------------------------------------------------------------------------------------
// The MAIN FUNCTION
//------------------------------------------------------------------------------------------------------------------------------------------
int main(void) {
	
	bool keyW = 0;
	bool keyA = 0;
	bool keyS = 0;
	bool keyD = 0;
	bool keySpace = 0;
	
	struct r rs[100]; // Array of struct r (things to render on screen)
	// (If theres more than 100 at once it would probably glitch or something, but you'll never need that many)
	int rslen = 0; // How many need to be rendered currently
	
	const char* sprite_sheet = "resource/gameveri_sheet_0801.png";
	si_start(sprite_sheet);
	
	int state = MAIN_MENU;//MAIN_GAME;
	
	//----------------------------------------------------------------------------------------------------------------------------------
	// Main loop, runs about 30 times per second (si_draw() regulates the frame duration)
	//----------------------------------------------------------------------------------------------------------------------------------
	
	while (si_isRunning() && state != MAIN_QUIT) {
		
		si_keys(&keyW, &keyA, &keyS, &keyD, &keySpace); // READ KEYBOARD INPUT
		
		// TICK PROGRAM LOGIC
		switch (state) {
			case MAIN_MENU:
				state = m_tick(rs, &rslen, keyW, keyA, keyS, keyD, keySpace);
				break;
			case MAIN_GAME:
				state = g_tick(rs, &rslen, keyW, keyA, keyS, keyD, keySpace);
				break;
		}
		si_draw(rs, rslen); // RENDER EVERYTHING ONTO THE SCREEN
	}
	
	si_end();
	
	return 0; // Program ends reporting no errors
}
