
#include "g_common.c"
#include "g_animal.c"
#include "g_ortho_forms.c"
#include "g_mobile_forms.c"
#include "g_simple_forms.c"
#include "g_level_loading.c"
#include "g_ai.c"

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME DATA
//------------------------------------------------------------------------------------------------------------------------------------------

int frameNo = 0;

// Camera data
int camera_x = 0;
int camera_y = 0;

// Collision rectangles data in immediate area
const int crs_size = 2500;
struct coll_rect crs[2500];
int crlen = 0;

// The Level
struct level* the_level = NULL;
const char* the_level_filename = NULL;

void g_load_level(const char* level_filename) {
	if (the_level != NULL)
		gll_unload_level(the_level);
	the_level = gll_load_level(level_filename);
	the_level_filename = level_filename;
	
	for (int i = 0; i < 8; i++) {
		colorstates[i] = false;
	}
}

#include "g_level_editing.c"

//----------------------------------------------------------------------------------------------------------------------------------------------------
// GAME TICK FUNCTION (Called about 30 times per second by the main loop)
//----------------------------------------------------------------------------------------------------------------------------------------------------

int g_tick() {
	
	if (the_level == NULL) {
		the_level = gll_load_level("testlevel.txt");
		the_level_filename = "testlevel.txt";
	}
	
	// Level geometry --------------------------------------------------------------------------------------------------------------------------------
	
	// Determine the current visible quadrant --------------------------------------------------------------------------------------------------------
	
	int quadcolumns = the_level->n_quadrants / the_level->quad_row_length;
	
	int quad_col = (camera_x-the_level->lolx) / seg_w; // (seg_w because quads overlap and are spaced the same as segments)
	if (quad_col < 0) quad_col = 0;
	if (quad_col > (the_level->quad_row_length - 1)) quad_col = (the_level->quad_row_length - 1);
	
	int quad_row = (camera_y-the_level->loty) / seg_h;
	if (quad_row < 0) quad_row = 0;
	if (quad_row > (quadcolumns - 1)) quad_row = (quadcolumns - 1);
	
	// The player's current visible and collidable quadrant
	int cq_index = (quad_row * the_level->quad_row_length) + quad_col; // Index of the current quadrant
	struct level_quadrant cq = the_level->quadrants[ cq_index ]; // The current quadrant
	
	quad_col = ((hazel.x-900)-the_level->lolx) / seg_w; // (seg_w because quads overlap and are spaced the same as segments)
	if (quad_col < 0) quad_col = 0;
	if (quad_col > (the_level->quad_row_length - 1)) quad_col = (the_level->quad_row_length - 1);
	
	quad_row = ((hazel.y-320)-the_level->loty) / seg_h;
	if (quad_row < 0) quad_row = 0;
	if (quad_row > (quadcolumns - 1)) quad_row = (quadcolumns - 1);
	
	// Hazel's current collidable quadrant
	int hazel_q_index = (quad_row * the_level->quad_row_length) + quad_col; // Index of hazel's quadrant
	struct level_quadrant hazel_q = the_level->quadrants[ hazel_q_index ]; // Hazel's current quadrant
	
	// Mobile forms tick (must occur BEFORE the quadrant's crs are loaded, as it changes certain the_level->lcrs, which must happen THIS FRAME)
	mobile_forms_tick(the_level, cq);
	
	// Display and note collision data for all level geometry indexed by the current quadrant --------------------------------------------------------
	
	rslen = 0;
	crlen = 0;
	
	// Add the quadrant's render instructions
	for (int i = 0; i < cq.n_rinds; i++) {
		add_r(the_level->lrs[cq.rinds[i]], &rslen, rs, rs_size);
	}
	
	// Add the quadrant's collision rectangles
	for (int i = 0; i < cq.n_crinds; i++) {
		add_cr(the_level->lcrs[cq.crinds[i]], &crlen, crs, crs_size);
	}
	// If hazel is in a different quadrant, process its collision rectangles for her also
	if (hazel_q_index != cq_index) {
		for (int i = 0; i < hazel_q.n_crinds; i++) {
			add_cr(the_level->lcrs[hazel_q.crinds[i]], &crlen, crs, crs_size);
		}
	}
	
	// Add the quadrant's levers
	for (int i = 0; i < cq.n_linds; i++) {
		struct lever this = the_level->lls[ cq.linds[i] ];
		// (Displayed differently depending on the matching colorstate)
		if (colorstates[ this.color ]) { // (This lever is flipped on)
			struct r tr = {
				.source_x= 101,
				.source_y= 446+((lever_h+1)*this.color),
				.source_w= lever_w,
				.source_h= lever_h,
				.dest_x= this.x,
				.dest_y= this.y-lever_h,
				.dest_w= 0, // (Just don't scale it)
				.dest_h= 0,
				.flip_horizontal= true,
				.flip_vertical= false
			};
			add_r(tr, &rslen, rs, rs_size);
		} else { // (This lever is flipped off)
			struct r tr = {
				.source_x= 101,
				.source_y= 446+((lever_h+1)*this.color),
				.source_w= lever_w,
				.source_h= lever_h,
				.dest_x= this.x,
				.dest_y= this.y-lever_h,
				.dest_w= 0, // (Just don't scale it)
				.dest_h= 0,
				.flip_horizontal= false,
				.flip_vertical= false
			};
			add_r(tr, &rslen, rs, rs_size);
		}
	}
	
	// Display & collision-register all non-indexed level geometry that has been created by the player using cheats ----------------------------------
	
	for (int i = 0; i < n_new_forms; i++) {
		struct entryform this = new_forms[i];
		ortho_form(
			this.x, this.y,
			this.vectmags, this.vectdirs, this.perlen,
			this.tile_type,
			rs, &rslen, rs_size, crs, &crlen, crs_size);
	}
	
	if (keyWdown(kd) && keySdown(kd) && !(keyWdown(kp) || keySdown(kp))) { // BIG OL' DEBUG PRINTOUT
		printf("################################ DEBUG PRINTOUT ################################\n");
		printf("rslen %i\n", rslen);
		printf("crlen %i\n", crlen);
		//printf("crs %p\n", crs);
		//printf("the_level->n_lrs %i, ->nlcrs %i\n", the_level->n_lrs, the_level->n_lcrs);
		//printf("the_level-->nlcrs %i, ->lcrs[", the_level->n_lcrs); for(int i=0;i<the_level->n_lcrs;i++) printf("%i.x=%i ", i, the_level->lcrs[i].x); printf("]\n");
		//printf("n_new_forms %i, &n_new_forms %p\n", n_new_forms, &n_new_forms);
		//printf("new_forms %p\n", new_forms);
		//printf("Quadrant map:\n"); for(int i=0;i<the_level->n_quadrants;i++) printf("%i%c",i,((i+1)%the_level->quad_row_length?' ':'\n'));
		printf("Current quadrant: %i, containing %i crs\n", cq_index, cq.n_crinds);
		//printf("crs in this quadrant: [\n"); for(int i=0;i<cq.n_crinds;i++) printf(" %i@%i,.x=%i ", i, cq.crinds[i], the_level->lcrs[cq.crinds[i]].x); printf("]\n");
		printf("Hazel's quadrant: %i, containing %i crs\n", hazel_q_index, hazel_q.n_crinds);
		//printf("crs in her quadrant: [\n"); for(int i=0;i<hazel_q.n_crinds;i++) printf(" %i@%i,.x=%i ", i, hazel_q.crinds[i], the_level->lcrs[hazel_q.crinds[i]].x); printf("]\n");
		//printdemo_level(the_level);
	}
	
	// God/Level-edit mode tick ----------------------------------------------------------------------------------------------------------------------
	
	// G key toggles god mode (Though you cannot leave god-mode in the middle of editing a shape)
	if (keyGdown(kd) && !keyGdown(kp) && (!editor_mode || !god_mode))
		god_mode = !god_mode;
		
	if (god_mode) { // Noclip and can edit level
		god_tick(kd, kp);
		god_render(&rslen, rs, rs_size);
	}
	
	else
	
	// Normal game tick ------------------------------------------------------------------------------------------------------------------------------
	
	{ // Normal physics and animation
		
		//printf("Averi X %i, VX %i, Y %i, VY %i%s\n", averi.x, averi.vx, averi.y, averi.vy, keySPACEdown(kd)?" SPACE":"");
		if (nibble_timer > 0) {
			nibble_timer--;
			if (nibble_timer == 20)
				add_sound(AUDIO_BITE, sounds, &slen, sounds_size);
		} else {
			// AVERI TICK
			animal_tick(&averi, kd, kp, crs, &crlen);
			
			// HAZEL TICK
			unsigned short hkd; // Simulated keyboard state for Hazel
			ai_tick(&hkd, crs, &crlen); // AI controls hkd, which will control Hazel
			animal_tick(&hazel, hkd, 0U, crs, &crlen);
			//animal_tick(&hazel, kd, kp, crs, &crlen, sounds, &slen, sounds_size); // (To control her myself with the same keys)
		}
		
		// If Averi is standing by Hazel, W can be pressed to nibble her ear
		if (averi.y == hazel.y && abs(averi.x-hazel.x) < 40 && (keyWdown(kd) && !keyWdown(kp)) && !nibble_timer ) {
			nibble_timer = 40;
			averi.vx = 0;
			hazel.vx = 0;
			ai_paranoia = 10;
			add_sound(AUDIO_STEP, sounds, &slen, sounds_size);
		}
		
		
		// If Averi is standing by a lever, W or S can be pressed to flip it
		for (int i = 0; i < cq.n_linds; i++) {
			struct lever this = the_level->lls[ cq.linds[i] ];
			if (averi.x+averi.w >= this.x && averi.x-averi.w <= (this.x+lever_w) &&
				averi.y+averi.h >= this.y && averi.y <= (this.y+lever_w) &&
				( (keyWdown(kd) && !keyWdown(kp)) || (keySdown(kd) && !keySdown(kp)) )
				)
			{
				colorstates[this.color] = !colorstates[this.color];
				add_sound(colorstates[this.color]?AUDIO_TYPEWRITER_F:AUDIO_TYPEWRITER_E, sounds, &slen, sounds_size);
			}
		}
		
		// If anyone falls out of the bottom of the world, drop them above the spawn area as if by a wormhole or something
		if (averi.y > the_level->loby+800) {
			averi.x = 900;
			averi.y = the_level->loty-800;
		}
		if (hazel.y > the_level->loby+800) {
			hazel.x = 900;
			hazel.y = the_level->loty-800;
		}
		
		// Place the camera to center on Averi
		camera_x = averi.x - (ideal_w/2);
		camera_y = averi.y - (ideal_h/2);
		
		static int camera_shift_x = 0;
		static int camera_shift_y = 0;
		static const int cam_shift_spd = 2;
		static const int max_cam_shift = 60;
		static int manual_shift_x = 0;
		static int manual_shift_y = 0;
		static const int manual_shift_spd = 50;
		
		// Gently shift the camera with her velocity (So you see further ahead where you're going)
		if (camera_shift_x < averi.vx*4 && camera_shift_x < max_cam_shift)
			camera_shift_x += cam_shift_spd;
		else if (camera_shift_x > averi.vx*4 && camera_shift_x > -max_cam_shift)
			camera_shift_x -= cam_shift_spd;
		if (camera_shift_y < averi.vy*2 && camera_shift_y < max_cam_shift)
			camera_shift_y  += cam_shift_spd;
		else if (camera_shift_y > averi.vy*2 && camera_shift_y > -max_cam_shift)
			camera_shift_y -= cam_shift_spd;
		
		// Apply manual camera shift by use of arrow keys
		if (keyUPdown(kd) && !keyDOWNdown(kd)) {
			// Arrow up
			if (manual_shift_y > -ideal_h/3) manual_shift_y -= manual_shift_spd;
		} else if (!keyUPdown(kd) && keyDOWNdown(kd)) {
			// Arrow down
			if (manual_shift_y < ideal_h/3) manual_shift_y += manual_shift_spd;
		} else {
			// No vertical arrow
			if (manual_shift_y > manual_shift_spd) manual_shift_y -= manual_shift_spd;
			else if (manual_shift_y <-manual_shift_spd) manual_shift_y += manual_shift_spd;
			else manual_shift_y = 0;
		}
		if (keyLEFTdown(kd) && !keyRIGHTdown(kd)) {
			// Arrow left
			if (manual_shift_x > -ideal_h/2) manual_shift_x -= manual_shift_spd;
		} else if (!keyLEFTdown(kd) && keyRIGHTdown(kd)) {
			// Arrow right
			if (manual_shift_x < ideal_h/2) manual_shift_x += manual_shift_spd;
		} else {
			// No horizontal arrow
			if (manual_shift_x > manual_shift_spd) manual_shift_x -= manual_shift_spd;
			else if (manual_shift_x <-manual_shift_spd) manual_shift_x += manual_shift_spd;
			else manual_shift_x = 0;
		}
		
		// Apply all shifts to the camera position
		camera_x += camera_shift_x + manual_shift_x;
		camera_y += camera_shift_y + manual_shift_y;
	}
		
	// Character rendering instructions --------------------------------------------------------------------------------------------------------------
	
	if (!nibble_timer) {
	// Render Averi,
		add_r(
			(struct r) {
				.source_x=0, .source_y=(91*averi.state), .source_w=100, .source_h=90,
				.dest_x=averi.x-50, .dest_y=averi.y, .dest_w=0, .dest_h=0,
				.flip_horizontal=averi.right_face, .flip_vertical=false
			}, &rslen, rs, rs_size);
		// and her tail
		if (!averi.state) { // (only when she's standing still, at averi.state 0)
			add_r(
				(struct r) {
					.source_x=0, .source_y=(2737 + 35*averi.tail_state), .source_w=100, .source_h=35,
					.dest_x=averi.x-50, .dest_y=averi.y+44, .dest_w=0, .dest_h=0,
					.flip_horizontal=averi.right_face, .flip_vertical=false
				}, &rslen, rs, rs_size);
		}
		// Render Hazel
		add_r(
			(struct r) {
				.source_x=296, .source_y=(91*hazel.state), .source_w=100, .source_h=90,
				.dest_x=hazel.x-50, .dest_y=hazel.y, .dest_w=0, .dest_h=0,
				.flip_horizontal=hazel.right_face, .flip_vertical=false
			}, &rslen, rs, rs_size
		);
	} else {
		// Render Hazel with Averi in the same sprite
		add_r(
			(struct r) {
				.source_x=296, .source_y=(91*(nibble_timer>20?30:31)), .source_w=100, .source_h=90,
				.dest_x=hazel.x-50, .dest_y=hazel.y, .dest_w=0, .dest_h=0,
				.flip_horizontal=hazel.right_face, .flip_vertical=false
			}, &rslen, rs, rs_size
		);
	}
	

	
	
	// Camera offsetting -----------------------------------------------------------------------------------------------------------------------------
	
	// Offset every render-instruction by the camera's position
	for (int i = 0; i < rslen; i++) {
		rs[i].dest_x -= camera_x;
		rs[i].dest_y -= camera_y;
	}
	
	// Final steps -----------------------------------------------------------------------------------------------------------------------------------
	
	frameNo += 1;
	
	return MAIN_GAME; // Remain in the game state
}
