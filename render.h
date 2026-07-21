#ifndef MAIN_MENU

// These are just aliases for ints, used to express more clearly that an int is referring main.c's state

#define MAIN_QUIT -1
#define MAIN_MENU 0
#define MAIN_GAME 1

// The core files will ask sysimp.c to render things by giving it these:

struct r {
	int source_x, source_y, source_w, source_h, // Describe a rectangle on the sprite sheet, what part of it to draw
	dest_x, dest_y; // Describe a location on the screen on which to draw that sprite
	bool visible, flip_horizontal, flip_vertical;
};

// Core rendering will be expressed as if the screen were 1800 x 720; sysimp.c scales it to the actual monitor

const int ideal_w = 1800; // The sides will be cut off on less wide screens (basically all screens)
const int ideal_h = 720; // But all 720 height units will always be visible

const float max_aspect_ratio = 2.5; // This is also the "ideal" aspect ratio
const float min_aspect_ratio = 1.0;

#endif
