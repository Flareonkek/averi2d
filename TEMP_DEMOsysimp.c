
#include "raylib.h"

//------------------------------------------------------------------------------------
// File-scope variables
//------------------------------------------------------------------------------------

// Window starting dimensions (It can get resized at any time though)
const int screenDefWidth = 1800; // ideal: 1800
const int screenDefHeight = 720; // ideal: 720
float scale_factor = 1.0;
int disp_x, disp_y; // Top-left corner of display area on window. disp_x may be negative to even up the cut-off area between the sides
Texture2D spriteTexture;

//------------------------------------------------------------------------------------------------------------------------------------------
// System implementation (Raylib) functions, prefix si_
//------------------------------------------------------------------------------------------------------------------------------------------


// Initialize window and stuff --------------------------------------------------------------
void si_start(const char* sprite_sheet) {
	
	SetConfigFlags(FLAG_WINDOW_RESIZABLE); //Set before InitWindow
	
    InitWindow(screenDefWidth, screenDefHeight, "Averi game prototype 2026-09-17");
    
	//SetExitKey(KEY_NULL); // Disable the default escape-key closing the window action
	
    spriteTexture = LoadTexture(sprite_sheet); //global Texture2D from LoadTexture("path to .png")
	
    SetTargetFPS(30);
}

// Check if program should keep running -----------------------------------------------------
int si_isRunning(void) {
	return !WindowShouldClose();
}

// Get Keyboard inputs ----------------------------------------------------------------------
int si_keys(void) {
	// Use each bit of the 16-bit unsigned short to represent a key
	unsigned short sum = 0;
	if (IsKeyDown(KEY_W)) sum += k_powers_of_2[0];
	if (IsKeyDown(KEY_A)) sum += k_powers_of_2[1];
	if (IsKeyDown(KEY_S)) sum += k_powers_of_2[2];
	if (IsKeyDown(KEY_D)) sum += k_powers_of_2[3];
	if (IsKeyDown(KEY_G)) sum += k_powers_of_2[4];
	if (IsKeyDown(KEY_SPACE)) sum += k_powers_of_2[5];
	if (IsKeyDown(KEY_UP)) sum += k_powers_of_2[6];
	if (IsKeyDown(KEY_LEFT)) sum += k_powers_of_2[7];
	if (IsKeyDown(KEY_DOWN)) sum += k_powers_of_2[8];
	if (IsKeyDown(KEY_RIGHT)) sum += k_powers_of_2[9];
	if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) sum += k_powers_of_2[10];
	if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) sum += k_powers_of_2[11];
	if (IsKeyDown(KEY_BACKSPACE) || IsKeyDown(KEY_DELETE)) sum += k_powers_of_2[12];
	if (IsKeyDown(KEY_TAB)) sum += k_powers_of_2[13];
	if (IsKeyDown(KEY_ESCAPE)) sum += k_powers_of_2[14];
	if (IsKeyDown(KEY_F6)) sum += k_powers_of_2[15];
	return sum;
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
	
	// ################ TEMP_DEMO changes ################
	disp_x += si_get_width() / 4;
	disp_y += si_get_height() / 4;
	scale_factor /= 2.0;
	// ###################################################
	
    BeginDrawing();
    
        ClearBackground(GetColor(0x052c46ff)); // (dark blue)
		
		DrawRectangle(disp_x, disp_y, scale_factor * ideal_w, scale_factor * ideal_h, GetColor(0x90EE90ff)); // Display area (mint green)
		
		// Draw each r from the sprite sheet
		for (int ri = 0; ri < rslen; ri++) {
			if (true) {
					int dw, dh; // If .dest_ dimension isn't 0, use it, otherwise draw the sprite at its original size
					if (rs[ri].dest_w) dw = rs[ri].dest_w; else dw = rs[ri].source_w;
					if (rs[ri].dest_h) dh = rs[ri].dest_h; else dh = rs[ri].source_h;
					DrawTexturePro(
						spriteTexture, // Texture2D: what to draw
						// Rectangle on the Texture2D, what part of it to draw:
						(Rectangle){rs[ri].source_x, rs[ri].source_y, (rs[ri].flip_horizontal ? -rs[ri].source_w : rs[ri].source_w), (rs[ri].flip_vertical ? -rs[ri].source_h : rs[ri].source_h)},
						// Rectangle on the screen: where to draw it
						(Rectangle){ rs[ri].dest_x*scale_factor + disp_x, rs[ri].dest_y*scale_factor + disp_y,
							dw*scale_factor, dh*scale_factor },
						(Vector2){0,0}, 0.0f, WHITE // SNCA
						);
			}
		}
		/* ############### TEMP_DEMO commenting ###############
		// Draw blinds so you can't see objects appearing & disappearing past the edges of the display area
		Color blind_color = GetColor(0x052c46ff);
		if (disp_y) {
			int bh = disp_y; // Blind height
			DrawRectangle(0, 0, si_get_width(), bh, blind_color);
			DrawRectangle(0, si_get_height()-bh, si_get_width(), bh, blind_color);
		} else {
			int bw = disp_x; // Blind width
			DrawRectangle(0, 0, bw, si_get_height(), blind_color);
			DrawRectangle(si_get_width()-bw, 0, bw, si_get_height(), blind_color);
		}
		*/
		//DrawFPS(0, 0); // For testing purposes, show the FPS in the corner (Raylib function)

    EndDrawing();
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
