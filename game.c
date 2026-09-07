#include <stdbool.h>
#include <stdio.h>
#include "g_ortho_forms.c"
#include "g_level_loading.c"
#include "g_averi.c"

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME DATA
//------------------------------------------------------------------------------------------------------------------------------------------

//Note: for file-scope data like these, static just means only game.c can use them

static int frameNo = 0;

// Camera data
static int camera_x = 0;
static int camera_y = 0;
static int camera_shift_x = 0;
static int camera_shift_y = 0;
static const int cam_shift_spd = 2;
const int max_cam_shift = 60;

// Collision rectangles data in immediate area (Do not load in more than 5000 at a time)
static struct coll_rect crs[5000];
static int crlen = 0;

// The Level and an array for all segments in visible area
struct level* testlevel = NULL;
int seg_index_memory[1000];

//------------------------------------------------------------------------------------------------------------------------------------------
// GAME TICK FUNCTION (Called about 30 times per second by the main loop)
//------------------------------------------------------------------------------------------------------------------------------------------

int g_tick(struct r* rs, int* rslen, bool keyW, bool keyA, bool keyS, bool keyD, bool keySpace, bool keyG) {
	
	*rslen = 0; // I do this here because itll keep going up as the program loops otherwise
	crlen = 0;
	
	if (testlevel == NULL) {
		testlevel = load_level("testlevel.txt");
		printdemo_level(testlevel);
	}
	
	// Level geometry -------------------------------------------------------------------------------------------------------
	
	
	// Determine the current visible segment quartet --------------------------------------------------------------
	
	int left_seg_col = (camera_x-testlevel->lolx) / seg_w;
	if (left_seg_col < 0) left_seg_col = 0;
	if (left_seg_col > (testlevel->n_segments_per_row - 2)) left_seg_col = (testlevel->n_segments_per_row - 2);
	
	int top_seg_row = (camera_y-testlevel->loty) / seg_h;
	if (top_seg_row < 0) top_seg_row = 0;
	if (top_seg_row > (testlevel->n_segment_rows - 2)) top_seg_row = (testlevel->n_segment_rows - 2);
	
	// Segment indices for the visible four segments
	int topleftseg = left_seg_col + (testlevel->n_segments_per_row) * top_seg_row;
	int bottomleftseg = topleftseg + testlevel->n_segments_per_row;
	int toprightseg = topleftseg + 1;
	int bottomrightseg = bottomleftseg + 1;
	
	// Check all four segments and add their contents to seg_index_memory -----------------------------------------
	
	// Zero out seg_index_memory
	for (int i = 0; i < 1000; i++)
		seg_index_memory[i] = 0;
		
	int sim_i = 0; // Current length of seg_index_memory
	
	// Add the contents of topleftseg to seg_index_memory
	for (int i = 0; i < (testlevel->segments[topleftseg].len); i++) {
		seg_index_memory[sim_i] = (testlevel->segments[topleftseg].data[i]);
		sim_i++;
	}
	// Add any new contents found in toprightseg
	bool already_there;
	for (int i = 0; i < (testlevel->segments[toprightseg].len); i++) {
		already_there = 0; // Assume it's not until its found
		for(int j = 0; j < sim_i; j++) {
			if ( (testlevel->segments[toprightseg].data[i]) == seg_index_memory[j] ) {
				already_there = 1;
				break;
			}
		}
		if (!already_there) {
			seg_index_memory[sim_i] = (testlevel->segments[toprightseg].data[i]);
			sim_i++;
		}
	}
	// Add any new contents found in bottomleftseg
	for (int i = 0; i < (testlevel->segments[bottomleftseg].len); i++) {
		already_there = 0; // Assume it's not until its found
		for(int j = 0; j < sim_i; j++) {
			if ( (testlevel->segments[bottomleftseg].data[i]) == seg_index_memory[j] ) {
				already_there = 1;
				break;
			}
		}
		if (!already_there) {
			seg_index_memory[sim_i] = (testlevel->segments[bottomleftseg].data[i]);
			sim_i++;
		}
	}
	// Add any new contents found in bottomrightseg
	for (int i = 0; i < (testlevel->segments[bottomrightseg].len); i++) {
		already_there = 0; // Assume it's not until its found
		for(int j = 0; j < sim_i; j++) {
			if ( (testlevel->segments[bottomrightseg].data[i]) == seg_index_memory[j] ) {
				already_there = 1;
				break;
			}
		}
		if (!already_there) {
			seg_index_memory[sim_i] = (testlevel->segments[bottomrightseg].data[i]);
			sim_i++;
		}
	}
	
	// Display all level geometry indexed to seg_index_memory -----------------------------------------------------
	
	for (int i = 0; i < sim_i; i++) {
		int j = seg_index_memory[i];
		struct vectlist this = testlevel->features[j];
		ortho_form_2(
			this.x, this.y,
			this.vectmags, this.vectdirs, this.len,
			this.tile_type,
			rs, rslen, crs, &crlen);
	}
	
	
	if (keySpace && !space_held && keyS) { // BIG OL' DEBUG PRINTOUT
		printf("################################ DEBUG PRINTOUT ################################\n");
		/*
		printf("Top seg row %i, left seg col %i\n", top_seg_row, left_seg_col);
		printf("camera_y %i\n", camera_y);
		printf("testlevel->loty %i\n", testlevel->loty);
		printf("seg_h %i\n", seg_h);
		printf("camera_y-testlevel->loty %i\n", camera_y-testlevel->loty);
		printf("(camera_y-testlevel->loty)/seg_h %i\n\n", (camera_y-testlevel->loty)/seg_h);
		*/
		printf("Full segment map:\n");
		for (int i = 0; i < testlevel->n_segment_rows*testlevel->n_segments_per_row; i++) {
			printf("%i %s%c", i, (i>9?" ":"  "), (((i+1)%testlevel->n_segments_per_row)?' ':'\n') );
		}
		
		printf("Current segment quartet:\n%i %i\n%i %i\n", topleftseg, toprightseg, bottomleftseg, bottomrightseg);
		/*
		printf("\nSegment contents:\n");
		for (int i = 0; i < testlevel->n_segment_rows*testlevel->n_segments_per_row; i++) {
			printf("[");
			for (int j = 0; j < testlevel->segments[i].len; j++) {
				printf("%i ", testlevel->segments[i].data[j]);
			}
			printf("]%c", (((i+1)%testlevel->n_segments_per_row)?' ':'\n') );
		}
		printf("\n");
		*/
		printf("Visible features:\n");
		for (int i = 0; i < sim_i; i++)
			printf("%i ", seg_index_memory[i]);
		printf("\n");
		
		//printdemo_level(testlevel);
	}
	
	averi_tick(keyW, keyA, keyS, keyD, keySpace, crs, &crlen, frameNo);
		
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
	
	// Camera offsetting ----------------------------------------------------------------------------------------------------
	
	// Place the camera to center on Averi
	camera_x = averiX - (ideal_w/2);
	camera_y = averiY - (ideal_h/2);
	
	// Gently shift the camera with her velocity (So you see further ahead where you're going)
	if (camera_shift_x < averiVx*4) camera_shift_x += cam_shift_spd;
	else if (camera_shift_x > averiVx*4) camera_shift_x -= cam_shift_spd;
	if (camera_shift_y < averiVy*2) camera_shift_y  += cam_shift_spd;
	else if (camera_shift_y > averiVy*2) camera_shift_y -= cam_shift_spd;
	
	// (Limit the camera shift before applying it)
	if (camera_shift_x > max_cam_shift) camera_shift_x = max_cam_shift;
	if (camera_shift_x <-max_cam_shift) camera_shift_x =-max_cam_shift;
	if (camera_shift_y > max_cam_shift) camera_shift_y = max_cam_shift;
	if (camera_shift_y <-max_cam_shift) camera_shift_y =-max_cam_shift;
	
	camera_x += camera_shift_x;
	camera_y += camera_shift_y;
	
	// Offset everything by the camera's position
	for (int i = 0; i < (*rslen); i++) {
		rs[i].dest_x -= camera_x;
		rs[i].dest_y -= camera_y;
	}
	
	// Final steps ----------------------------------------------------------------------------------------------------------
	
	frameNo += 1;
	
	return MAIN_GAME; // Remain in the game state
}
