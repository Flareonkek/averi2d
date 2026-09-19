
// ---------------------------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------- COMMON.H - COMMON DATA AND FUNCTIONS ------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------------------
// Stuff needed across many different files, pertaining to the whole program overall

#ifndef MAIN_MENU

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> // (Used to make timestamps for edits in g_level_editing.c)

// These are just aliases for integer literals, used to express more clearly that an int is referring main.c's state
#define MAIN_QUIT -1
#define MAIN_MENU 0
#define MAIN_GAME 1

// Absolute value of an integer
int abs (int argint) {
	if (argint > 0) return argint;
	return -argint;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// Rendering instructions datatype, function and constants --------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

// The core files will ask sysimp.c to render things by giving it these:
struct r {
	int source_x, source_y, source_w, source_h, // Describe a rectangle on the sprite sheet, what part of it to draw
	dest_x, dest_y, // Describe a location on the screen on which to draw that sprite
	dest_w, dest_h; // Allow rescaling the sprite, set to 0 to keep original size.
	bool flip_horizontal, flip_vertical;
};

// Concise way to add struct r to an array of them, which checks it doesn't overfill an array of 5000
void add_r(struct r arg_r, int* arg_rslen, struct r* arg_rs, int arg_rs_size) {
	if (*arg_rslen >= arg_rs_size-1) {
		printf("WARNING: Attempt to overfill arg_rs (size %i) with add_r() (argrslen is %i)\n", arg_rs_size, *arg_rslen);
		return;
	}
	arg_rs[*arg_rslen] = arg_r;
	(*arg_rslen)++;
}


// Core rendering will be expressed as if the screen were 1800 x 720; sysimp.c scales it to the actual size
const int ideal_w = 1800; // The sides will be cut off on less wide screens (basically all screens)
const int ideal_h = 720; // But all 720 height units will always be visible

/* ----------------------------------------- How display scaling works -----------------------------------
Different users may have differently sized screens, and can also resize the game's window in any way.
Rendering instructions will be expressed with respect to a theoretical/ideal display size of 1800 x 720,
and si_draw() will scale it bigger or smaller to match the actual size of the game window.
However, if the ratio of the window's width to its height is different than the ideal display's, it will
not stretch/squash the image, but rather, it may cut off the sides, but no further than to the point
where the display's aspect ratio is 1:1, and scale the image as much as needed to fit in the window,
possibly leaving some portions of the window unused.

So when placing things on the screen with rendering instructions, imagine the display is anywhere from
720 to 1800 pixels wide (anything less than 540 or greater than 1260 on the x-axis might get cut off),
and always 720 pixels tall (no part of the display's height is ever cut off).
*/// -----------------------------------------------------------------------------------------------------

const float max_aspect_ratio = 2.5; // (The aspect ratio of 1800:720, the widest the display can be)
const float min_aspect_ratio = 1.0; // (The aspect ratio of 720:720, the least wide the display can be)

// ----------------------------------------------------------------------------------------------------------------------------------
// Collision rectangle dataype and function -----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

// A collision rectangle, defines a solid area for game logic
struct coll_rect {
	int x, y, w, h; // X and Y of its top-left corner, Width and Height projecting out east and south, respectively
};

// Concise way to add struct coll_rect to an array of them, which checks it doesn't overfill an array of 5000
void add_cr(struct coll_rect arg_cr, int* arg_crlen, struct coll_rect* arg_crs, int arg_crs_size) {
	if (*arg_crlen >= arg_crs_size-1) {
		printf("WARNING: Attempt to overfill arg_crs (size %i) with add_cr() (argcrslen is %i)\n", arg_crs_size, *arg_crlen);
		return;
	}
	arg_crs[*arg_crlen] = arg_cr;
	(*arg_crlen)++;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// KEYBOARD STATE FUNCTIONS ---------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------
// Functions for interpreting the state of many keyboard inputs from a single, concise variable
// The data type "unsigned short" can store values anywhere from 0 to 65535, so it must comprise of at least 16 bits.

// To bitwise-and a single bit's value with the corresponding power-of-two will yield that bit alone
unsigned short k_powers_of_2[16] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};

/* (This is how the above would look in binary, and it expresses the idea more clearly,
 *  but I used decimal literals as they're more certain to be supported on all compilers)

unsigned short k_powers_of_2[16] = {
	0b0000000000000001,
	0b0000000000000010,
	0b0000000000000100,
	0b0000000000001000,
	0b0000000000010000,
	0b0000000000100000,
	0b0000000001000000,
	0b0000000010000000,
	0b0000000100000000,
	0b0000001000000000,
	0b0000010000000000,
	0b0000100000000000,
	0b0001000000000000,
	0b0010000000000000,
	0b0100000000000000,
	0b1000000000000000,
};

 * The program uses each bit to represent a certain key as follows:
 * 0  1  2  3  4  5     6  7    8    9     10    11    12        13   14   15  16
 * W  A  S  D  G  Space Up Left Down Right Shift Ctrl  Del/Back  Tab  Esc  F6  -
 */

bool keyWdown(unsigned short argk) {
	return (argk & k_powers_of_2[0]);
}
bool keyAdown(unsigned short argk) {
	return (argk & k_powers_of_2[1]);
}
bool keySdown(unsigned short argk) {
	return (argk & k_powers_of_2[2]);
}
bool keyDdown(unsigned short argk) {
	return (argk & k_powers_of_2[3]);
}
bool keyGdown(unsigned short argk) {
	return (argk & k_powers_of_2[4]);
}
bool keySPACEdown(unsigned short argk) {
	return (argk & k_powers_of_2[5]);
}
bool keyUPdown(unsigned short argk) {
	return (argk & k_powers_of_2[6]);
}
bool keyLEFTdown(unsigned short argk) {
	return (argk & k_powers_of_2[7]);
}
bool keyDOWNdown(unsigned short argk) {
	return (argk & k_powers_of_2[8]);
}
bool keyRIGHTdown(unsigned short argk) {
	return (argk & k_powers_of_2[9]);
}
bool keySHIFTdown(unsigned short argk) {
	return (argk & k_powers_of_2[10]);
}
bool keyCTRLdown(unsigned short argk) {
	return (argk & k_powers_of_2[11]);
}
bool keyDELdown(unsigned short argk) {
	return (argk & k_powers_of_2[12]);
}
bool keyTABdown(unsigned short argk) {
	return (argk & k_powers_of_2[13]);
}
bool keyESCdown(unsigned short argk) {
	return (argk & k_powers_of_2[14]);
}
bool keyF6down(unsigned short argk) {
	return (argk & k_powers_of_2[15]);
}

// ----------------------------------------------------------------------------------------------------------------------------------
// VECTOR LIST: used to store an ortho_form -----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

// A form of solid level geometry comprised of vectors connected by 90-degree turns
struct oform {
	// Where the first vector starts from
	int x;
	int y;
	int tile_type;
	
	// Vector list
	int len; // Number of vectors
	int* vectmags; // Vector magnitudes
	bool* vectdirs; // Vector directions: false for right, true for left
};
/* ------------------------------------------- How oforms work -------------------------------------------
The first vector is always horizontal, pointing east.
Its magnitude projects out to the east, however many units (vectmags[0])
then, at the end, there is a 90-degree left or right turn (vectdirs[0]),
so that the next vector will be horizontal going either north or south,
and the next vector also has its own length and magnitude (vectmags[1] and vectdir[1]),
and so on until the circuit is complete. For a complete and usable oform,
the vectors must form a closed shape.
*/// -----------------------------------------------------------------------------------------------------

// Adds a vector, of magnitude 'argmag', and end-direction 'argdir', to the oform pointed to by 'argoform'
void oform_addvect(int argmag, bool argdir, struct oform* argoform) {
	// Allocate new arrays 1 thing longer than the originals
	int* newvectmags = malloc( sizeof(int)*(argoform->len + 1) );
	bool* newvectdirs = malloc( sizeof(bool)*(argoform->len + 1) );
	
	if (newvectmags == NULL || newvectdirs == NULL) {
		printf("Error, unable to allocate oform array(s)\n");
		exit(1);
	}
	
	// Set the last things, the new ones, of the new arrays, to our arguments
	newvectmags[argoform->len] = argmag;
	newvectdirs[argoform->len] = argdir;
	if (argoform->len > 0) {
		// Copy each thing from the original into the new array
		for (int i = 0; i < argoform->len; i++) {
			newvectmags[i] = (argoform->vectmags)[i];
			newvectdirs[i] = (argoform->vectdirs)[i];
		}
	// Free up the old arrays (if there were any) and point arglist to the new ones
	free(argoform->vectmags);
	free(argoform->vectdirs);
	}
	argoform->vectmags = newvectmags;
	argoform->vectdirs = newvectdirs;
	argoform->len++;
}

// Functions for rendering text sprites, given their own file for organization
#include "text.c"

#endif
