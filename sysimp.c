#include "raylib.h"
#include "render.h"
#include <stdio.h>

//------------------------------------------------------------------------------------
// File-scope variables
//------------------------------------------------------------------------------------

const int screenDefWidth = 1800; // ideal: 1800
const int screenDefHeight = 720; // ideal: 720
float scale_factor = 1.0;
int disp_x, disp_y; // Top-left corner of display area on window. disp_x may be negative to even up the cut-off area between the sides
Texture2D spriteTexture;

//int dpt = 0; // Was using this earlier to figure out issues. 'Debug Print Timer'

//------------------------------------------------------------------------------------------------------------------------------------------
// System implementation (Raylib) functions, prefix si_
//------------------------------------------------------------------------------------------------------------------------------------------


// Initialize window and stuff --------------------------------------------------------------
void si_start(const char* sprite_sheet)
{
	
	SetConfigFlags(FLAG_WINDOW_RESIZABLE); //Set before InitWindow
	
    InitWindow(screenDefWidth, screenDefHeight, "Averi game prototype 2026-07-20");
	
    spriteTexture = LoadTexture(sprite_sheet); //global Texture2D from LoadTexture("path to .png")
	
    SetTargetFPS(30);
}

// Check if program should keep running -----------------------------------------------------
int si_isRunning(void) {
	return !WindowShouldClose();
}

// Get Keyboard inputs ----------------------------------------------------------------------
int si_keys(bool* keyWa, bool* keyAa, bool* keySa, bool* keyDa, bool* keySpacea)
{
	int r = 0;
	if (IsKeyDown(KEY_W)) *keyWa = 1; else *keyWa = 0;
	if (IsKeyDown(KEY_A)) *keyAa = 1; else *keyAa = 0;
	if (IsKeyDown(KEY_S)) *keySa = 1; else *keySa = 0;
	if (IsKeyDown(KEY_D)) *keyDa = 1; else *keyDa = 0;
	if (IsKeyDown(KEY_SPACE)) *keySpacea = 1; else *keySpacea = 0;
}

int si_get_width(void) {
	return GetScreenWidth();
}

int si_get_height(void) {
	return GetScreenHeight();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// DRAW FUNCTION (Called repeatedly by the main loop, also shoots to maintain a framerate of 30FPS)
//------------------------------------------------------------------------------------------------------------------------------------------
void si_draw(struct r* rs, int rslen)
{
	
	// ADJUST DISPLAY AREA in case window is resized
	if (si_get_width() / si_get_height() > max_aspect_ratio) {
		// If the screen is now wider than max_aspect_ratio: scale by height, shift display right to be centered in the window
		scale_factor = (float)si_get_height() / (float)ideal_h;
		disp_x = (si_get_width() - (2.5*si_get_height()) ) / 2;
		disp_y = 0;
	} else if (si_get_width() / si_get_height() < min_aspect_ratio) {
		// If the screen is now taller than it is wide: scale by width, shift display left to cut off as much as min_aspect_ratio permits
		scale_factor = (float)si_get_width() / ((float)ideal_h*min_aspect_ratio);
		disp_x = (si_get_width() - (ideal_w * scale_factor)) / 2;
		disp_y = (si_get_height() - si_get_width()) / 2; // ...and shift display down to be centered
	} else {
		// If the screen is between the min and max aspect ratios, scale by height and shift left to cut off the sides as needed
		scale_factor = (float)si_get_height() / (float)ideal_h;
		disp_x = (si_get_width() - (ideal_w * scale_factor)) / 2;
		disp_y = 0;
	}
	
    BeginDrawing();
    
        ClearBackground(GetColor(0x052c46ff)); // (dark blue)
		
		DrawRectangle(disp_x, disp_y, scale_factor * ideal_w, scale_factor * ideal_h, GetColor(0x90EE90ff)); // Display area (mint green)
		
		// Draw the floor (Hard-coded here for now...)
		int floor_y = 350;
		DrawRectangleV(
			(Vector2){ 0+disp_x, floor_y*scale_factor+disp_y }, // Top-left corner coordinates
			(Vector2){ ideal_w*scale_factor, (ideal_h-floor_y)*scale_factor }, // Rectangle dimensions
			GRAY);
		
		// Draw each r from the sprite sheet
		for (int ri = 0; ri < rslen; ri++) {
			if (rs[ri].visible)
				DrawTexturePro(
					spriteTexture, // Texture2D: what to draw
					// Rectangle on the Texture2D, what part of it to draw:
					(Rectangle){rs[ri].source_x, rs[ri].source_y, (rs[ri].flip_horizontal ? -rs[ri].source_w : rs[ri].source_w), rs[ri].source_h},
					// Rectangle on the screen: where to draw it
					(Rectangle){ rs[ri].dest_x*scale_factor + disp_x, rs[ri].dest_y*scale_factor + disp_y, rs[ri].source_w*scale_factor, rs[ri].source_h*scale_factor },
					(Vector2){0,0}, 0.0f, WHITE // SNCA
					);
			}

    EndDrawing();
    
    /* Was using earlier to troubleshoot, uncomment if needed again...
    if (dpt >= 120) {
		dpt = 0;
		printf("scale_factor %f, disp_x %d, disp_y %d, si_get_width() %d, si_get_height() %d\n", scale_factor, disp_x, disp_y, si_get_width(), si_get_height());
	} else dpt++;
	*/
}

// Just shut it down -------------------------------------------------------------------------
void si_end(void)
{
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(spriteTexture);  // Unload sprite texture

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
}
