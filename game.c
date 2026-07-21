#include <stdbool.h>
#include <stdio.h>

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME DATA
//------------------------------------------------------------------------------------------------------------------------------------------

//Note: for file-scope data like these, static just means only game.c can use them
static int frameNo = 0;

static int averiX = 900; // Her X location
static int averiV = 0; // Her velocity
static const int runspd_max = 15;
static int averiState = 0; // Which sprite to draw for animation
static int tailState = 3;
static bool tailSwing = 0; // Remember which way it was swinging last time it was
static bool averiRightFace = 0;
static const int avg_stride = 8; // How many pixels of travel each frame represents
static int animDX = 0; // X-travel since last walk/run frame change (So animations look right at any speed)
static const int averi_groundY = 260;
static int averiY = averi_groundY - 200; // She starts off 200 units up in the air
static int averiVy = 0; // Y-axis velocity
static int averiW = 28;

static const int leftWallX = 540;
static const int rightWallX = 1239;
static const int wallH = 350;
static const int wallW = 21;

// Little helper function
int approach_zero(int arg, int amt) {
	if (arg < -amt) return arg + amt;
	if (arg > amt) return arg - amt;
	return 0;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME TICK FUNCTION (Called about 30 times per second by the main loop)
//------------------------------------------------------------------------------------------------------------------------------------------

int g_tick(struct r* rs, int* rslen, bool keyW, bool keyA, bool keyS, bool keyD, bool keySpace) {
	
	// Influence Averi's velocity according to keyboard input ---------------------------------------------------------------
	
	if (keyA && !keyD) {
		// Try to accelerate left (on 2 out of every 3 frames)
		if (averiV > -runspd_max && frameNo % 3)
			averiV -= 1;
		averiRightFace = 0;
	}
	else if (keyD && !keyA) {
		// Try to accelerate right (on 2/3 frames)
		if (averiV < runspd_max && frameNo % 3)
			averiV += 1;
		averiRightFace = 1;
	}
	else if ((frameNo % 3)) averiV = approach_zero(averiV, 1); // Slow down cause not trying to go anywhere
	
	if (averiY < averi_groundY) // If in the air, enact gravity
		averiVy += (keySpace? 1: 3); // (3x faster if space is being held)
	else if (keySpace) averiVy = -20; // Otherwise, jumping is possible
	
	// Apply velocity -------------------------------------------------------------------------------------------------------
	
	averiX += averiV;
	animDX += averiV;
	
	// Don't let her walk through walls
	if (averiX - averiW <= (leftWallX+wallW)) {
		averiX = averiW + (leftWallX+wallW) + 1;
		averiV = 0;
	}
	if (averiX +  averiW >= rightWallX) {
		averiX = -averiW + rightWallX - 1;
		averiV = 0;
	}
	
	// Also do such for the Y axis
	if (averiY + averiVy >= averi_groundY) {
		averiY = averi_groundY;
		averiVy = 0; // Land on ground
	} else averiY += averiVy; // Fall
	
	// Averi animations -----------------------------------------------------------------------------------------------------
	
	if (averiY < averi_groundY) {
		// Jumping animations -----------------------------------------------------------------------------------------------
		if (averiVy < -2)
			averiState = 16; // Rising
		else if (averiVy < 2)
			averiState = 17; // Peak
		else if (averiVy < 6)
			averiState = 18; // Descent
		else
			averiState = 19; // Fast descent
	}
	else if ((averiRightFace && averiV < 0) || (!averiRightFace && averiV > 0)) {
		// Sliding and turning around ---------------------------------------------------------------------------------------
		if (averiState < 11 || averiState > 15)
			averiState = 11;
		else if (averiState != 15 && !(frameNo % 3))
			averiState += 1;
		// When you're about to have slid to a halt...
		if (averiV == 1 || averiV == -1) {
			averiV = (averiV == 1? -1 : 1); // Skip over 0-velocity (Or she would snap into stand-still frame)
			averiState = 3; // Skip walk frames (The skid animation aligns with the run frames)
			animDX = 0;
		}
	} else {
		// General walking/running/standing ---------------------------------------------------------------------------------
		if (animDX < -avg_stride || animDX > avg_stride) {
			// Every time you moved avg_stride pixels, advance the animation frame and update animDX
			animDX = approach_zero(animDX, avg_stride);
			averiState += 1;
		}
		// If we reach the end of the run cycle, go back to its start
		if (averiState > 10) averiState = 3;
		
		// If she isn't moving, go back to stand-still sprite and reset animDX
		if (averiV == 0) {
			averiState = 0;
			animDX = 0;
		} else if (averiState == 0) // But if she is moving even a bit,
			averiState = 1; // don't wait for animDX to kickstart the walk animation
	}
	
	// Tail swinging animation ----------------------------------------------------------------------------------------------
	
	if (averiState) {
		tailState = 3; // Neutral position
		tailSwing = !tailSwing; // Switch swing direction when she moves or does something
	}
	else if (!(frameNo % 7)) {
		if (tailSwing)
			tailState -= 1;
		else
			tailState += 1;
		if (tailState == 6 || tailState == 0)
			tailSwing = !tailSwing; // Switch direction when tail reaches the end of its swing
	}
		
	// Final rendering data -------------------------------------------------------------------------------------------------
	
	struct r averi = {
		.source_x=0, .source_y=(90*averiState), .source_w=81, .source_h=90,
		.dest_x=averiX-40, .dest_y=averiY,
		.visible=1,
		.flip_horizontal=averiRightFace, .flip_vertical=0
		};
	struct r averi_tail = {
		.source_x=0, .source_y=(2335 + 35*tailState), .source_w=81, .source_h=35,
		.dest_x=averiX-40, .dest_y=averiY+44,
		.visible=(!averiState), // (Tail is only rendered separately at stand-still)
		.flip_horizontal=averiRightFace, .flip_vertical=0
		};
	struct r left_wall = {
		.source_x=133, .source_y=0, .source_w=wallW, .source_h=wallH,
		.dest_x=leftWallX, .dest_y=0,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
		};
	struct r right_wall = {
		.source_x=133, .source_y=0, .source_w=wallW, .source_h=wallH,
		.dest_x=rightWallX, .dest_y=0,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
		};
	
	rs[0] = averi; // Arrays are secretly pointers in C, when tinkering with their members we reach the data out in main.c
	rs[1] = averi_tail;
	rs[2] = left_wall;
	rs[3] = right_wall;
	
	*rslen = 4; // rslen in this file is just a local copy of an address (pointer). Star prefix here reaches data at that address
	
	// ----------------------------------------------------------------------------------------------------------------------
	
	frameNo += 1;
	return MAIN_GAME; // Remain in the game state
}
