#include <stdbool.h>
#include <stdio.h>
#include "g_ortho_forms.c"

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME DATA
//------------------------------------------------------------------------------------------------------------------------------------------

//Note: for file-scope data like these, static just means only game.c can use them

static int frameNo = 0;

// Player character data
static bool averi_airborne = 0;
static int averi_climbing = 0;
static const int averiW = 28; // It's actually half her width, her X is the center of her sprite and its 28 pixels to either side.
static const int averiH = 90; // Her full height, her Y is at the top of her sprite.
static int averiX = 900; // Her X location
static int averiVx = 0; // Her X-axis velocity
static int averiY = 260;
static int averiVy = 0;
static const int runspd_max = 15;
static int averiState = 0; // Which sprite to draw for animation
static int tailState = 3;
static bool tailSwing = 0; // Remember which way it was swinging last time it was
static bool averiRightFace = 0;
static const int avg_stride = 8; // How many pixels of travel each frame represents
static int animDX = 0; // X-travel since last walk/run frame change (So animations look right at any speed)

// Camera data
static int camera_x = 0;
static int camera_y = 0;
static int camera_shift_x = 0;
static int camera_shift_y = 0;
static const int cam_shift_spd = 2;

// Collision rectangles data in immediate area
static struct coll_rect crs[100];
static int crlen = 0;

//------------------------------------------------------------------------------------------------------------------------------------------
// HELPER FUNCTIONS for this file
//------------------------------------------------------------------------------------------------------------------------------------------
int approach_zero(int arg, int amt) {
	if (arg < -amt) return arg + amt;
	if (arg > amt) return arg - amt;
	return 0;
}

// Check if two coll_rects overlap each other
bool collides(struct coll_rect cr1, struct coll_rect cr2) {
	if (cr1.x + cr1.w <= cr2.x)
		return 0;
	if (cr2.x + cr2.w <= cr1.x)
		return 0;
	
	if (cr1.y + cr1.h <= cr2.y)
		return 0;
	if (cr2.y + cr2.h <= cr1.y)
		return 0;
	
	return 1;
}

// Check if a coll_rect collides with any in an array of coll_rects
bool any_collides(struct coll_rect cr1, struct coll_rect* crs, int crlen) {
	for (int i = 0; i < crlen; i++) {
		struct coll_rect cr = crs[i];
		if (collides(cr1, cr))
			return 1;
	}
	//printf("No collisions detected for cr {%i, %i, %i, %i}\n", cr1.x, cr1.y, cr1.w, cr1.h);
	return 0;
}

// Would averi's current y-velocity cause an overlap with cr?
bool averiVyColl(struct coll_rect cr) {
	if ((averiY+averiVy) + averiH <= cr.y)
		return 0;
	if (cr.y + cr.h <= (averiY+averiVy))
		return 0;
	
	if (averiX + averiW <= cr.x)
		return 0;
	if (cr.x + cr.w <= averiX - averiW)
		return 0;
	return 1;
}

// Would averi's current x-velocity cause an overlap with cr?
bool averiVxColl(struct coll_rect cr) {
	if ((averiX+averiVx) + averiW <= cr.x)
		return 0;
	if (cr.x + cr.w <= (averiX+averiVx) - averiW)
		return 0;
	
	if (averiY + averiH <= cr.y)
		return 0;
	if (cr.y + cr.h <= averiY)
		return 0;
	return 1;
}

// Would averi's current compound velocity cause an overlap with cr?
bool averiVxyColl(struct coll_rect cr) {
	if ((averiX+averiVx) + averiW <= cr.x)
		return 0;
	if (cr.x + cr.w <= (averiX+averiVx) - averiW)
		return 0;
	
	if ((averiY+averiVy) + averiH <= cr.y)
		return 0;
	if (cr.y + cr.h <= (averiY+averiVy))
		return 0;
	return 1;
}


//------------------------------------------------------------------------------------------------------------------------------------------
// Averi's physics and animations
//------------------------------------------------------------------------------------------------------------------------------------------
static void averi_tick(bool keyW, bool keyA, bool keyS, bool keyD, bool keySpace, struct coll_rect* crs, int* crlen) {
	
	// TODO implement going down onto ledge when you stand (averiVx == 0) on that awkward area where you currently look like you're standing on air ####################
	
	if (averi_climbing) {
		// averi_climbing of 1 means she's hanging on the ledge. 0 means she isn't,
		// values greater than 1 mean she's progressing through steps of pulling herself up and jumping over the ledge
		if (averi_climbing > 1) {
			averi_climbing++;
			if (averi_climbing == 5)
				averiY -= 12; // Going into state 22
			else if (averi_climbing == 7)
				averiY -= 11; // Going into state 23
			else if (averi_climbing == 9)
				averiY -= 10; // Going into state 24
			else if (averi_climbing == 11)
				averiY -= 9; // Going into state 25
			else if (averi_climbing == 13) {
				averiY -= 8; // Going into normal airborne fall
				averiVy = -16;
				averi_climbing = 0;
				averi_airborne = 1;
			}
		} else if (keyW || keySpace) {
			averi_climbing = 2;
			averiY -= 13; // Going into state 21
		} else if (keyS)
			averi_climbing = 0; // Let go
	} else {
		// Influence Averi's velocity according to keyboard input ---------------------------------------------------------------
		if (keyA && !keyD) {
			// Try to accelerate left (on 2 out of every 3 frames)
			if (averiVx > -runspd_max && frameNo % 3)
				averiVx -= 1;
			averiRightFace = 0;
		}
		else if (keyD && !keyA) {
			// Try to accelerate right (on 2/3 frames)
			if (averiVx < runspd_max && frameNo % 3)
				averiVx += 1;
			averiRightFace = 1;
		}
		else if ((frameNo % 3)) averiVx = approach_zero(averiVx, 1); // Slow down cause not trying to go anywhere
		
		if (averi_airborne) // If in the air, enact gravity
			averiVy += (keySpace? 1: 3); // (3x faster if space is being held)
		else if (keySpace) // Otherwise, jumping is possible
			averiVy = -20;
		
		// Apply velocity -------------------------------------------------------------------------------------------------------
		
		// Collision check with every cr
		for (int i = 0; i < *crlen; i++) {
			struct coll_rect cr = crs[i]; // For each collision rectangle,
			if (averiVxyColl(cr)) { // If Averi is on course to collide with it...
				if (averiVxColl(cr)) {
					if (averiVyColl(cr)) { // (Either axes velocity would cause a collision)
						// Go as many twentieths of the intended course as possible without clipping into the corner and halt velocity
						int oldVx = averiVx, oldVy = averiVy;
						int vicesimi = 20;
						while (averiVxyColl(cr)) {
							vicesimi--;
							averiVx = (oldVx*vicesimi)/20;
							averiVy = (oldVy*vicesimi)/20;
						}
						averiX += averiVx;
						averiY += averiVy;
						averiVx = averiVy = 0; // Halt both axes velocity
					}
					else { // (Only her x-axis velocity would cause a collision on its own)
						// Place her right up against the wall/surface and halt Vx
						if (averiVx > 0) averiX = cr.x-averiW;
						else if (averiVx < 0) averiX = cr.x+cr.w+averiW;
						// LEDGE GRAB CHECK
						if ( (averiY+averiH > cr.y) && (cr.y > averiY) && (averi_airborne) ) {
							averiVy = 0;
							averi_climbing = 1;
							averiY = cr.y - 27;
							// Prevent a funny graphical glitch
							if (averiVx > 0) averiRightFace = 1;
							else averiRightFace = 0;
						}
						averiVx = 0;
					}
				}
				else { // (Only her y-axis velocity would cause a collision on its own)
					// Place her right atop the floor or up against the ceiling and halt Vy
					if (averiVy > 0)
						averiY = cr.y-averiH;
					else if (averiVy < 0) averiY = cr.y+cr.h;
					averiVy = 0;
				}
			}
		}
		
		// Enact velocity onto position
		averiX += averiVx;
		animDX += averiVx;
		averiY += averiVy; // Fall
		
		// Unless we find her to be standing on something, assume she's in the air
		averi_airborne = 1;
		for (int i = 0; i < *crlen; i++) { // Now check every collision rectangle...
			struct coll_rect cr = crs[i];
			if (averiY+averiH == cr.y && ((averiX-averiW < cr.x+cr.w) && (averiX+averiW > cr.x)) ) {
				// If she's standing on any cr, then she isn't in the air
				averi_airborne = 0;
				// If she's standing still, and sticking out over the edge, and there's room down on the side, go into a ledge hang there
				if (!averiVx) {
					struct coll_rect hang_right_cr = {(cr.x+cr.w), cr.y-27, averiW*2, averiH};
					struct coll_rect hang_left_cr = {cr.x-(averiW*2), cr.y-27, averiW*2, averiH};
					
					if (averiX > (cr.x+cr.w) && !any_collides(hang_right_cr, crs,  *crlen)) {
						averiX = (cr.x+cr.w)+averiW;
						averiVy = 0;
						averi_climbing = 1;
						averiY = cr.y - 27;
						averiRightFace = 0;
					} else if (averiX < cr.x && !any_collides(hang_left_cr, crs,  *crlen)) {
						averiX = cr.x - averiW;
						averiVy = 0;
						averi_climbing = 1;
						averiY = cr.y - 27;
						averiRightFace = 1;
					}
				}
			}
		}
	}
	
	// Averi animations -----------------------------------------------------------------------------------------------------
	
	if (averi_climbing) {
		// Ledge hanging/climbing animations --------------------------------------------------------------------------------
		if (averi_climbing < 2) averiState = 20;
		else if (averi_climbing < 5) averiState = 21;
		else if (averi_climbing < 7) averiState = 22;
		else if (averi_climbing < 9) averiState = 23;
		else if (averi_climbing < 11) averiState = 24;
		else averiState = 25;
	} else if (averi_airborne) {
		// Jumping/falling animations ---------------------------------------------------------------------------------------
		if (averiVy < -2)
			averiState = 16; // Rising
		else if (averiVy < 2)
			averiState = 17; // Peak
		else if (averiVy < 6)
			averiState = 18; // Descent
		else
			averiState = 19; // Fast descent
	}
	else if ((averiRightFace && averiVx < 0) || (!averiRightFace && averiVx > 0)) {
		// Sliding and turning around ---------------------------------------------------------------------------------------
		if (averiState < 11 || averiState > 15)
			averiState = 11;
		else if (averiState != 15 && !(frameNo % 3))
			averiState += 1;
		// When you're about to have slid to a halt...
		if (averiVx == 1 || averiVx == -1) {
			averiVx = (averiVx == 1? -1 : 1); // Skip over 0-velocity (Or she would snap into stand-still frame)
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
		if (averiVx == 0) {
			averiState = 0;
			animDX = 0;
		} else if (averiState == 0) // But if she is moving even a bit and yet in the stand-still sprite,
			averiState = 1; // then don't wait for animDX, kickstart the walk animation immediately
	}
	
	// Tail swinging animation ----------------------------------------------------------------------------------------------
	
	if (averiState)
		tailState = 3; // Return tail to neutral position when she moves (As then it's not swinging)
	else if (!(frameNo % 7)) {
		if (tailSwing)
			tailState -= 1;
		else
			tailState += 1;
		if (tailState == 6 || tailState == 0)
			tailSwing = !tailSwing; // Switch direction when tail reaches the end of its swing
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME TICK FUNCTION (Called about 30 times per second by the main loop)
//------------------------------------------------------------------------------------------------------------------------------------------

int g_tick(struct r* rs, int* rslen, bool keyW, bool keyA, bool keyS, bool keyD, bool keySpace, bool keyG) {
	
	*rslen = 0; // I do this here because itll keep going up as the program loops otherwise
	crlen = 0;
	
	// Level geometry -------------------------------------------------------------------------------------------------------
	
	// ###########################################################################################################################################################
	// TODO although it's hard-coded here for now, I'd like to make a system where only the nearby stuff you can see would even get loaded in here, ##############
	// That is, into crs and rs, and when you leave things behind they get freed up. Also a way of designing geometry in-game for fast testing and adjusting. ####
	// ###########################################################################################################################################################
	const int nv_wave = 8;
	struct coll_rect cr_wave[8];
	struct ortho_vertex ofp[8] = {
		{.len=60, .leftTurn=0},
		{.len=60, .leftTurn=1},
		{.len=84, .leftTurn=0},
		{.len=24, .leftTurn=0},
		{.len=120, .leftTurn=0},
		{.len=72, .leftTurn=1},
		{.len=24, .leftTurn=0},
		{.len=12, .leftTurn=0}};
	struct coll_rect wave_colliders[nv_wave];
	ortho_form(1000, 100, ofp, nv_wave, rs, rslen, crs, &crlen);
	
	const int nv_block = 4;
	struct coll_rect cr_block[4];
	struct ortho_vertex ofp2[4] = {
		{.len=200, .leftTurn=0},
		{.len=400, .leftTurn=0},
		{.len=200, .leftTurn=0},
		{.len=400, .leftTurn=0}};
	ortho_form(650, -200, ofp2, nv_block, rs, rslen, crs, &crlen);
	
	const int nv_spool = 12;
	struct coll_rect cr_spool[12];
	struct ortho_vertex ofp3[12] = {
		{.len=160, .leftTurn=0},
		{.len=20, .leftTurn=0},
		{.len=20, .leftTurn=1},
		{.len=20, .leftTurn=1},
		{.len=20, .leftTurn=0},
		{.len=20, .leftTurn=0},
		{.len=160, .leftTurn=0},
		{.len=20, .leftTurn=0},
		{.len=20, .leftTurn=1},
		{.len=20, .leftTurn=1},
		{.len=20, .leftTurn=0},
		{.len=20, .leftTurn=0}};
	ortho_form(260, 20, ofp3, nv_spool, rs, rslen, crs, &crlen);
	
	const int nv_ground = 4;
	struct coll_rect cr_ground[4];
	struct ortho_vertex ofp4[4] = {
		{.len=1800, .leftTurn=0},
		{.len=100, .leftTurn=0},
		{.len=1800, .leftTurn=0},
		{.len=100, .leftTurn=0}};
	ortho_form(0, 350, ofp4, nv_ground, rs, rslen, crs, &crlen);
	
	const int nv_pinwheel = 20;
	const int pwdim = 60;
	struct coll_rect cr_pinwheel[20];
	struct ortho_vertex ofp5[20] = {
		{.len=3*pwdim, .leftTurn=0},
		{.len=2*pwdim, .leftTurn=1},
		{.len=1*pwdim, .leftTurn=1},
		{.len=2*pwdim, .leftTurn=0},
		{.len=1*pwdim, .leftTurn=0},
		// (Those same 5 lines copy-pasted)
		{.len=3*pwdim, .leftTurn=0},
		{.len=2*pwdim, .leftTurn=1},
		{.len=1*pwdim, .leftTurn=1},
		{.len=2*pwdim, .leftTurn=0},
		{.len=1*pwdim, .leftTurn=0},
		
		{.len=3*pwdim, .leftTurn=0},
		{.len=2*pwdim, .leftTurn=1},
		{.len=1*pwdim, .leftTurn=1},
		{.len=2*pwdim, .leftTurn=0},
		{.len=1*pwdim, .leftTurn=0},
		
		{.len=3*pwdim, .leftTurn=0},
		{.len=2*pwdim, .leftTurn=1},
		{.len=1*pwdim, .leftTurn=1},
		{.len=2*pwdim, .leftTurn=0},
		{.len=1*pwdim, .leftTurn=0} };
	ortho_form(1000, -400, ofp5, nv_pinwheel, rs, rslen, crs, &crlen);
	
	const int nv_square = 4;
	struct coll_rect cr_square[4];
	struct ortho_vertex ofp6[4] = {
		{.len=60, .leftTurn=0},
		{.len=60, .leftTurn=0},
		{.len=60, .leftTurn=0},
		{.len=60, .leftTurn=0}};
	ortho_form(1400, 0, ofp6, nv_square, rs, rslen, crs, &crlen);
	
	ortho_form(1500, -400, ofp4, nv_ground, rs, rslen, crs, &crlen); // Upper testing ground
	ortho_form(1700, -699, ofp6, nv_square, rs, rslen, crs, &crlen); // This thing 699u up is exactly as high a thing as you can reach from below
	ortho_form(2450, -699, ofp6, nv_square, rs, rslen, crs, &crlen); // This one 750u away is about as far a thing as you can reach from beside (Not exact, but its close)
	
	averi_tick(keyW, keyA, keyS, keyD, keySpace, crs, &crlen);
		
	// Averi rendering data -------------------------------------------------------------------------------------------------
	
	struct r averi = {
		.source_x=0, .source_y=(90*averiState), .source_w=100, .source_h=90,
		.dest_x=averiX-50, .dest_y=averiY, .dest_w=0, .dest_h=0,
		.visible=1,
		.flip_horizontal=averiRightFace, .flip_vertical=0
		};
	struct r averi_tail = {
		.source_x=0, .source_y=(2337 + 35*tailState), .source_w=100, .source_h=35,
		.dest_x=averiX-50, .dest_y=averiY+44, .dest_w=0, .dest_h=0,
		.visible=(!averiState), // (Tail is only rendered separately at stand-still)
		.flip_horizontal=averiRightFace, .flip_vertical=0
		};
	
	rs[*rslen] = averi;
	rs[*rslen+1] = averi_tail;
	*rslen += 2;
	
	// Place the camera to center on Averi ----------------------------------------------------------------------------------
	camera_x = averiX - (ideal_w/2);
	camera_y = averiY - (ideal_h/2);
	
	// Gently shift the camera with her velocity (So you see further in the direction you're going)
	if (camera_shift_x < averiVx*4) camera_shift_x += cam_shift_spd;
	else if (camera_shift_x > averiVx*4) camera_shift_x -= cam_shift_spd;
	if (camera_shift_y < averiVy*2) camera_shift_y  += cam_shift_spd;
	else if (camera_shift_y > averiVy*2) camera_shift_y -= cam_shift_spd;
	camera_x += camera_shift_x;
	camera_y += camera_shift_y;
	
	// Offset everything by the camera's position
	for (int i = 0; i < (*rslen); i++) {
		rs[i].dest_x -= camera_x;
		rs[i].dest_y -= camera_y;
	}
	
	frameNo += 1;
	return MAIN_GAME; // Remain in the game state
}
