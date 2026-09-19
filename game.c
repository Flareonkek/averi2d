
#include "g_ortho_forms.c"
#include "g_level_loading.c"
#include "g_averi.c"

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME DATA
//------------------------------------------------------------------------------------------------------------------------------------------

int frameNo = 0;

// Camera data
int camera_x = 0;
int camera_y = 0;

// Collision rectangles data in immediate area
const int crs_size = 5000;
struct coll_rect crs[5000];
int crlen = 0;

// The Level
struct level* the_level = NULL;
const char* the_level_filename = NULL;

void g_load_level(const char* level_filename) {
	if (the_level != NULL)
		gll_unload_level(the_level);
	the_level = gll_load_level(level_filename);
	the_level_filename = level_filename;
}

#include "g_level_editing.c"

//----------------------------------------------------------------------------------------------------------------------------------------------------
// GAME TICK FUNCTION (Called about 30 times per second by the main loop)
//----------------------------------------------------------------------------------------------------------------------------------------------------

int g_tick(struct r* rs, int* rslen, int rs_size, unsigned short kd, unsigned short kp) {
	
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
	
	int cq_index = (quad_row * the_level->quad_row_length) + quad_col; // Index of the current quadrant
	struct level_quadrant cq = the_level->quadrants[ cq_index ]; // The current quadrant
	
	// Display and note collision data for all level geometry indexed by the current quadrant --------------------------------------------------------
	
	*rslen = 0;
	crlen = 0;
	
	// Add the quadrant's render instructions
	for (int i = 0; i < cq.n_rinds; i++) {
		add_r(the_level->lrs[cq.rinds[i]], rslen, rs, rs_size);
	}
	
	// Add the quadrant's collision rectangles
	for (int i = 0; i < cq.n_crinds; i++) {
		add_cr(the_level->lcrs[cq.crinds[i]], &crlen, crs, crs_size);
	}
	//if (!(frameNo%30)) printf("(game.c) %i rinds and %i crinds in this quadrant\n", cq.n_rinds, cq.n_crinds);
	
	// Display & collision-register all non-indexed level geometry that has been created by the player using cheats ----------------------------------
	
	for (int i = 0; i < n_new_forms; i++) {
		struct oform this = new_forms[i];
		ortho_form(
			this.x, this.y,
			this.vectmags, this.vectdirs, this.len,
			this.tile_type,
			rs, rslen, rs_size, crs, &crlen, crs_size);
	}
	
	if (keySPACEdown(kd) && !keySPACEdown(kp) && keySdown(kd)) { // BIG OL' DEBUG PRINTOUT
		printf("################################ DEBUG PRINTOUT ################################\n");
		printf("rslen %i, &rslen %p\n", (*rslen), rslen);
		printf("rs %p\n", rs);
		printf("crlen %i, &crlen %p\n", crlen, &crlen);
		printf("crs %p\n", crs);
		printf("the_level->n_lrs %i, ->nlcrs %i\n", the_level->n_lrs, the_level->n_lcrs);
		printf("n_new_forms %i, &n_new_forms %p\n", n_new_forms, &n_new_forms);
		printf("new_forms %p\n", new_forms);
		//printf("clock() is %li\n", clock());
	}
	
	// God/Level-edit mode tick ----------------------------------------------------------------------------------------------------------------------
	
	// G key toggles god mode (Though you cannot leave god-mode in the middle of editing a shape)
	if (keyGdown(kd) && !keyGdown(kp) && (!editor_mode || !god_mode))
		god_mode = !god_mode;
		
	if (god_mode) { // Noclip and can edit level
		god_tick(kd, kp);
		god_render(rslen, rs, rs_size);
	}
	
	else
	
	// Normal game tick ------------------------------------------------------------------------------------------------------------------------------
	
	{ // Normal averi physics and animation
		averi_tick(kd, kp, crs, &crlen);
		
		// Place the camera to center on Averi
		camera_x = averiX - (ideal_w/2);
		camera_y = averiY - (ideal_h/2);
		
		static int camera_shift_x = 0;
		static int camera_shift_y = 0;
		static const int cam_shift_spd = 2;
		static const int max_cam_shift = 60;
		static int manual_shift_x = 0;
		static int manual_shift_y = 0;
		static const int manual_shift_spd = 50;
		
		// Gently shift the camera with her velocity (So you see further ahead where you're going)
		if (camera_shift_x < averiVx*4 && camera_shift_x < max_cam_shift)
			camera_shift_x += cam_shift_spd;
		else if (camera_shift_x > averiVx*4 && camera_shift_x > -max_cam_shift)
			camera_shift_x -= cam_shift_spd;
		if (camera_shift_y < averiVy*2 && camera_shift_y < max_cam_shift)
			camera_shift_y  += cam_shift_spd;
		else if (camera_shift_y > averiVy*2 && camera_shift_y > -max_cam_shift)
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
		
	// Averi rendering instructions ------------------------------------------------------------------------------------------------------------------
	
	add_r(
		(struct r) {
			.source_x=0, .source_y=(90*averiState), .source_w=100, .source_h=90,
			.dest_x=averiX-50, .dest_y=averiY, .dest_w=0, .dest_h=0,
			.flip_horizontal=averiRightFace, .flip_vertical=false
		}, rslen, rs, rs_size);
		
	if (!averiState) { // Tail is only rendered separately when she's standing still, at averiState 0
		add_r(
			(struct r) {
				.source_x=0, .source_y=(2697 + 35*tailState), .source_w=100, .source_h=35,
				.dest_x=averiX-50, .dest_y=averiY+44, .dest_w=0, .dest_h=0,
				.flip_horizontal=averiRightFace, .flip_vertical=false
			}, rslen, rs, rs_size);
	}
	
	// Camera offsetting -----------------------------------------------------------------------------------------------------------------------------
	
	// Offset every render-instruction by the camera's position
	for (int i = 0; i < (*rslen); i++) {
		rs[i].dest_x -= camera_x;
		rs[i].dest_y -= camera_y;
	}
	
	// Final steps -----------------------------------------------------------------------------------------------------------------------------------
	
	frameNo += 1;
	
	return MAIN_GAME; // Remain in the game state
}
