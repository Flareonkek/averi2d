
// The only reason this gets its own file is to keep all this mess out of game.c
// Pretend you're in game.c, in the sense that you can use all the stuff in it before it #includes this file

// ----------------------------------------------------------------------------------------------------------------------------------
// GOD MODE DATA and HELPER FUNCTIONS -----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

bool god_mode = false;

// Starting coordinates of the first vector in the current ortho-form
static int editor_start_x = 0;
static int editor_start_y = 0;

// Data for the current ortho-form being worked on, "editor_form"
static struct oform editor_form = {
	.x = 0,
	.y = 0,
	.tile_type = 0,
	.len = 0,
	.vectmags = NULL,
	.vectdirs = NULL
};

// Data for all finished features created
int n_new_forms = 0;
struct oform new_forms[20];
const static int editor_max_new_forms = 10; // (Exceed this and it'll warn you)

// Starting coordinates position data of the vector being worked on
static int editor_new_x = 0;
static int editor_new_y = 0;
static bool editor_vertical = false;
static bool editor_negative = false;
// Current vector data
static int editor_newedge_mag = 0;
static bool editor_newcorner_dir = false; // (false means right, true means left)

const int edge_min_mag = 2*tileDim;

static bool editor_autofinish_available = false;

static int editor_newform_clr = 0; // tile_type of the new form for use in EDITOR_COLORING mode

enum {
	EDITOR_READY, // Not actively making a shape
	EDITOR_SHAPING, // Actively making a shape
	EDITOR_COLORING, // Choosing a tile_type for the shape
	EDITOR_PLEASE_SAVE // Ask the user to save when they've created a lot of forms, and don't let them keep working
};
int editor_mode = EDITOR_READY;

// Add the working-feature to new_forms and reset working-feature data for the next feature
static void god_add_form(void) {
	//printf("ADDING EDITOR FEATURE\n\t{.x=%i, .y=%i, .tile_type=%i, .len=%i,...}\nTO new_forms\n", editor_form.x, editor_form.y, editor_form.tile_type, editor_form.len);
	new_forms[n_new_forms] = (struct oform) {
		.x = editor_form.x,
		.y = editor_form.y,
		.tile_type = editor_form.tile_type,
		.len = editor_form.len,
		.vectmags = editor_form.vectmags,
		.vectdirs = editor_form.vectdirs
	};
	editor_form.vectmags = NULL;
	editor_form.vectdirs = NULL;
	n_new_forms++;
}

// Add all new forms to the level file and reset new_forms
// extern const char* the_level_filename; (already available as we #include this after it in game.c)
static void god_save_new_forms(void) {
	FILE* fp;
	fp = fopen(the_level_filename, "a"); // "a" for appending
	
	time_t raw_time;
	time(&raw_time);
	// Convert to local time structure
	struct tm *info;
	info = localtime(&raw_time);
	// Format time into string (buffer)
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d, at %X", info); // %Y is year, %m month, %d day, %X local time
    
	fprintf(fp, "\n# Geometry saved %s:\n\n", buffer);
	for (int i = 0; i < n_new_forms; i++) {
		fprintf(fp, "A.%i.%i.%i.\n", new_forms[i].x, new_forms[i].y, new_forms[i].tile_type);
		for (int j = 0; j < new_forms[i].len; j++) {
			fprintf(fp, "%i %c ", new_forms[i].vectmags[j], new_forms[i].vectdirs[j] ? 'L':'R');
		}
		fprintf(fp, ".\n\n");
	}
	
	fclose(fp);
}

// ----------------------------------------------------------------------------------------------------------------------------------
// GOD MODE TICK FUNCTIONS ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

void god_tick(unsigned short k, unsigned short k_held) {
	if (editor_mode == EDITOR_READY)
	{
		// Free movement without physics
		if (keyCTRLdown(k)) {
			// Move one pixel at a time if you're holding Ctrl
			if (keyWdown(k) && !keyWdown(k_held)) averiY -= 1;
			if (keySdown(k) && !keySdown(k_held)) averiY += 1;
			if (keyAdown(k) && !keyAdown(k_held)) averiX -= 1;
			if (keyDdown(k) && !keyDdown(k_held)) averiX += 1;
		} else {
			// Move at high or low speed depending on if you're holding Shift
			int spd = (keySHIFTdown(k)? 5 : 30);
			if (keyWdown(k)) averiY -= spd;
			if (keySdown(k)) averiY += spd;
			if (keyAdown(k)) averiX -= spd;
			if (keyDdown(k)) averiX += spd;
		}
		
		// Put the editor reticule here
		editor_start_x = averiX;
		editor_start_y = averiY+averiH;
		// And since we're in EDITOR_READY mode, start editor_newx and _newy off at the same place
		editor_new_x = editor_start_x;
		editor_new_y = editor_start_y;
		
		// Begin making a new shape by pressing space
		if (keySPACEdown(k) && !keySPACEdown(k_held)) {
			editor_mode = EDITOR_SHAPING;
			editor_newedge_mag = edge_min_mag; // (New edge starts at the minimum valid magnitude)
		}
		
	}
	else if (editor_mode == EDITOR_SHAPING) {
		
		// Pressing backspace/delete discards the current vector
		if (keyDELdown(k) && !keyDELdown(k_held)) {
			if (editor_form.len == 0) { // If you haven't placed any edges yet, backspace will take you back to EDITOR_READY mode,
				editor_mode = EDITOR_READY;
			} else { // Otherwise, it will undo the last vertex and give you back the last vertex's working data
				// Deduce what the previous state of editor_negative must have been
				if (editor_negative) {
					editor_negative = (editor_form.vectdirs[editor_form.len-1] != editor_vertical);
				} else {
					editor_negative = (editor_form.vectdirs[editor_form.len-1] == editor_vertical);
				}
				// (The previous state of editor_vertical is even easier to deduce)
				editor_vertical = !editor_vertical;
				
				// Now that we're oriented like the previous vector, we can just undo the change in editor_new_x/_y that it would have caused
				if (editor_vertical) {
					editor_new_y -= (editor_negative? -editor_form.vectmags[editor_form.len-1] : editor_form.vectmags[editor_form.len-1]);
				} else { // (editor horizontal)
					editor_new_x -= (editor_negative? -editor_form.vectmags[editor_form.len-1] : editor_form.vectmags[editor_form.len-1]);
				}
				// Now we just put the old vector's magnitude and direction back into the working variables,
				editor_newedge_mag = editor_form.vectmags[editor_form.len-1];
				editor_newcorner_dir = editor_form.vectdirs[editor_form.len-1];
				// and discard the old vector from editor_form.
				editor_form.len--;
			}
		}
		
		// SCALE EDGE LENGTH with W/S
		if (keyCTRLdown(k)) {
			// Scaling by one pixel at a time
			if (keySdown(k) && !keySdown(k_held) && editor_newedge_mag > 12) editor_newedge_mag -= 1;
			if (keyWdown(k) && !keyWdown(k_held)) editor_newedge_mag += 1;
		} else {
			// Normal and quick scaling
			int spd = (keySHIFTdown(k)? 1 : 5);
			if (keySdown(k)) {
				if (editor_newedge_mag > spd + edge_min_mag)
					editor_newedge_mag -= spd;
				else
					editor_newedge_mag = edge_min_mag;
			}
			if (keyWdown(k)) editor_newedge_mag += spd;
		}

		// CHOOSE END-DIRECTION with A/D
		if (keyAdown(k) && !keyDdown(k))
			editor_newcorner_dir = true;
		else if (keyDdown(k) && !keyAdown(k))
			editor_newcorner_dir = false;
			
		// Pressing space finalizes the vector, and we either immediately begin the next vector, or finish the shape and return to EDITOR_READY
		if (keySPACEdown(k) && !keySPACEdown(k_held)) {
			// Add the new vector to the editor_form
			oform_addvect(editor_newedge_mag, editor_newcorner_dir, &editor_form);
			
			// Update editor_new_x and _new_y
			if (editor_vertical) {
				editor_new_y = editor_negative ? (editor_new_y-editor_newedge_mag) : (editor_new_y+editor_newedge_mag);
			} else { // (editor horizontal)
				editor_new_x = editor_negative ? (editor_new_x-editor_newedge_mag) : (editor_new_x+editor_newedge_mag);
			}
			
			// Set these in readiness for the next vector
			editor_negative = ( editor_vertical == editor_negative? (editor_newcorner_dir) : !(editor_newcorner_dir) );
			editor_vertical = !editor_vertical;
			
			editor_newedge_mag = 12; // (Next edge, if there is one, starts at the minimum valid magnitude)
			
			// Check if we've closed the shape, and if so, finalize it
			if (!(editor_vertical || editor_negative) &&
				(editor_new_x == editor_start_x) &&
				(editor_new_y == editor_start_y) ) {
					printf("EDITOR FORM .len=%i,\n", editor_form.len);
					editor_form.x = editor_start_x;
					editor_form.y = editor_start_y;
					editor_form.tile_type = 20; // (Will get overwritten anyway)
					god_add_form();
					editor_form.len = 0;
					editor_mode = EDITOR_COLORING;
					editor_newform_clr = 0;
			}
		}
		
		// Check if auto-finish is possible, indicate, and possibly do it if so ----------------------------------------------------------------------
		
		if (editor_vertical) {
				if (editor_new_x == editor_start_x && // If we're on the starting x-position,
				    abs(editor_new_y - editor_start_y) >= edge_min_mag && // there's room for a vector up/down to the starting y-position,
				    (editor_new_y > editor_start_y)==(editor_negative) ) // and we're facing the right way to do it...
				{
					// ...then we can autofinish by making one new vertical vector
					editor_autofinish_available = true;
					if (keyTABdown(k) && !keyTABdown(k_held)) {
						// (The new vector)
						editor_newedge_mag = abs(editor_new_y - editor_start_y);
						editor_newcorner_dir = !editor_negative;
						/* (Don't wanna do this without the user's explicit confirmation)
						// Add the vector
						vectlist_addseg(editor_newedge_mag, editor_newcorner_dir, &editor_form);
						// Update this
						editor_new_y = editor_negative ? (editor_new_y-editor_newedge_mag) : (editor_new_y+editor_newedge_mag);
						// Set these in readiness for the next vector
						editor_negative = ( editor_vertical == editor_negative? (editor_newcorner_dir) : !(editor_newcorner_dir) );
						editor_vertical = !editor_vertical;
						// Finish the form
						printf("(Autofinish) EDITOR FORM .len=%i,\n", editor_form.len);
						editor_form.x = editor_start_x;
						editor_form.y = editor_start_y;
						editor_form.tile_type = 19;
						god_add_form();
						editor_form.len = 0;
						editor_mode = EDITOR_READY;
						*/
					}
				} else
					editor_autofinish_available = false;
		} else if ( // But if we're horizontal,
			abs(editor_new_y - editor_start_y) >= edge_min_mag && // there's room for a vector up/down to the starting y-position,
			abs(editor_new_x - editor_start_x) >= edge_min_mag && // and also for a vector left/right to the starting x-position,
			(editor_new_x < editor_start_x && editor_new_y < editor_start_y && !editor_negative || // And we're in the right position, facing the right way to,
			 editor_new_x > editor_start_x && editor_new_y > editor_start_y && editor_negative)
			) {
				// ...then we can autofinish by making two new vectors, first to get us directly above/below start position, and then to get us up/down there
				editor_autofinish_available = true;
				if (keyTABdown(k) && !keyTABdown(k_held)) {
					// (The first new vector)
					editor_newedge_mag = abs(editor_new_x - editor_start_x);
					editor_newcorner_dir = false;
					// Add the vector
					oform_addvect(editor_newedge_mag, editor_newcorner_dir, &editor_form);
					// Update this
					editor_new_x = editor_negative ? (editor_new_x-editor_newedge_mag) : (editor_new_x+editor_newedge_mag);
					// Set these in readiness for the next vector
					editor_negative = ( editor_vertical == editor_negative? (editor_newcorner_dir) : !(editor_newcorner_dir) );
					editor_vertical = true; // We're now vertical for sure
					
					// (The second new vector) (Does not actually finish shape, its awaiting confirmation here)
					editor_newedge_mag = abs(editor_new_y - editor_start_y);
					editor_newcorner_dir = !editor_negative;
				}
		} else
			editor_autofinish_available = false;
		
	} else if (editor_mode == EDITOR_COLORING) {
		if (n_new_forms) {
			if (keyWdown(k) && !keyWdown(k_held)) editor_newform_clr -= 3;
			if (keySdown(k) && !keySdown(k_held)) editor_newform_clr += 3;
			if (keyAdown(k) && !keyAdown(k_held)) editor_newform_clr--;
			if (keyDdown(k) && !keyDdown(k_held)) editor_newform_clr++;
			if (editor_newform_clr < 0) editor_newform_clr += 12;
			if (editor_newform_clr >11) editor_newform_clr -= 12;
			new_forms[n_new_forms-1].tile_type = editor_newform_clr;
		} else {
			printf("Warning in g+level_editing.c: somehow in coloring mode with no new form\n");
			editor_mode = EDITOR_READY;
		}
		if (keySPACEdown(k) && !keySPACEdown(k_held)) {
			if (n_new_forms > editor_max_new_forms)
				editor_mode = EDITOR_PLEASE_SAVE;
			else
				editor_mode = EDITOR_READY;
		}
	} else { // (EDITOR_PLEASE_SAVE)
		if (keyWdown(k)) averiY -= 2;
		if (keySdown(k)) averiY += 2;
		if (keyAdown(k)) averiX -= 2;
		if (keyDdown(k)) averiX += 2;
		if (n_new_forms <= editor_max_new_forms)
			editor_mode = EDITOR_READY;
	}
	
	// Save new features and reload level by pressing F6
	if (keyF6down(k) && !keyF6down(k_held)) {
		god_save_new_forms(); // Saves the new features onto the level's .txt file
		printf("RELOADING LEVEL; the_level_filename is %s\n", the_level_filename);
		// Reset averiX/Y and n_new_forms to their default states
		n_new_forms = 0;
		//averiX = 900; // Default, same as in g_averi.c
		//averiY = 260;
		g_load_level(the_level_filename);
	}
}

char* hud_strs[4] = {
	"Editor ready\n[Shift]: Move slower\n[Ctrl]: Move one pixel at a time\n[Space]: Begin new feature here",
	"Placing vector\n[W] & [S]: Lengthen/shorten\n(Shift/Ctrl for fine adjustment)\n[A] & [D]: Change turn-direction\n[Backspace/Del]: Undo",
	"Choose tile colour\n[W] [A] [S] [D]: Select\n[Space]: Confirm selection",
	"You've made a lot of new forms.\nPlease save and reload before\nmaking any more.\n[F6]: Save level and reload"
};

char* averi_hud_strs[4] = {
	"Editor ready",
	"Editing shape",
	"Selecting tile",
	"Editor worried"
};

// God-mode rendering ------------------------------------------------------------------------------------------------------------------------------------------

// DISPLAY the HUD, the new SHAPE-IN-FORMATION, the RETICULES, and COORDINATES
static int special_shift_x = 0;
static int special_shift_y = 0;

void god_render(int* rslen, struct r* rs, int rs_size) {
	// Display as much of editor_form as has been created yet --------------------------------------------------------------------------------------------------
	{
		int preview_x = editor_start_x;
		int preview_y = editor_start_y;
		bool preview_vert = false;
		bool preview_neg = false;
		for (int i = 0; i < editor_form.len; i++) { // For each ortho_vertex...
			bool corner_flip_h, corner_flip_v;
			int vect_mag = editor_form.vectmags[i]; // This vector's magnitude
			bool vect_lft = editor_form.vectdirs[i]; // This vector's end direction, true means left-turn, false means right-turn
			if (vect_mag < 2*tileDim) {
				printf("ERROR in god_render(): NOT ENOUGH ROOM FOR CORNER TILE\n");
				exit(1);
			}
			if (preview_vert) {
				if (vect_mag > 2*tileDim) { // Looks unnecessary but actually prevents a graphical glitch, as struct r .dest_h of 0 means draw without scaling,
					// Add the wall:
					add_r(
						(struct r) {
							.source_x=100+tileDim+tilebaseX(20), .source_y=tilebaseY(20), .source_w=tileDim, .source_h=tileDim,
							.dest_x=(preview_neg? preview_x : preview_x-tileDim), .dest_y=(preview_neg? preview_y-vect_mag+tileDim : preview_y+tileDim),
							.dest_w=0, .dest_h=vect_mag-(2*tileDim), // Stretch height to reach next corner
							.flip_horizontal=preview_neg, .flip_vertical=false
						},
						rslen, rs, rs_size
					);
				}
				// Update position
				if (preview_neg) preview_y -= vect_mag;
				else preview_y += vect_mag;
				
				if (preview_neg == vect_lft) {
					// The corner sprite should be flipped vertically and NOT horizontally
					corner_flip_h = 0; corner_flip_v = 1;
				} else {
					// The corner sprite should be flipped horizontally and NOT vertically
					corner_flip_h = 1; corner_flip_v = 0;
				}
			} else { // (...it's horizontal)
				if (vect_mag > 2*tileDim) {
				// Add the floor/ceiling
					add_r(
						(struct r) {
							.source_x=100+tilebaseX(20), .source_y=tileDim+tilebaseY(20), .source_w=tileDim, .source_h=tileDim,
							.dest_x=(preview_neg? preview_x-vect_mag+tileDim : preview_x+tileDim), .dest_y=(preview_neg? preview_y-tileDim : preview_y),
							.dest_w=vect_mag-(2*tileDim), .dest_h=0, // Stretch width to reach next corner
							.flip_horizontal=false, .flip_vertical=preview_neg
						},
						rslen, rs, rs_size
					);
				}
				
				// Update position
				if (preview_neg) {
					preview_x -= vect_mag;
					// Also, the corner sprite should be flipped on both axes
					corner_flip_h = corner_flip_v = 1;
				} else {
					preview_x += vect_mag;
					// Also, the corner sprite shouldn't be flipped on either axis
					corner_flip_h = corner_flip_v = 0;
				}
			}
			// Draw the corner sprite
			add_r(
				(struct r) {
					// Concave or convex corner?
					.source_x=(vect_lft? 0 : tileDim)+100+tilebaseX(20), .source_y=(vect_lft? 2*tileDim : tileDim)+tilebaseY(20),
					.source_w=(vect_lft? 2*tileDim : tileDim), .source_h=(vect_lft? 2*tileDim : tileDim),
					// Direction dependent
					.dest_x=((vect_lft || !corner_flip_h)? preview_x-tileDim : preview_x),
					.dest_y=((vect_lft || corner_flip_v)? preview_y-tileDim : preview_y),
					.dest_w=0, .dest_h=0, // (Do not resize)
					.flip_horizontal=corner_flip_h, .flip_vertical=corner_flip_v
				},
				rslen, rs, rs_size
			);
			// Set these for the next vertex:
			preview_neg = ( preview_vert == preview_neg? (vect_lft) : !(vect_lft) );
			preview_vert = !preview_vert;
		}
	}
	
	// HUD elements around editor_start coordinates ------------------------------------------------------------------------------------------------------------
	
	// Display a notice above Averi's head
	render_text(averi_hud_strs[editor_mode], averiX-50, averiY-24, 2, rslen, rs, rs_size);
	
	// Display editor start reticule
	add_r(
		(struct r) {
			.source_x=212, .source_y=2940, .source_w=12, .source_h=12,
			.dest_x=editor_start_x-6, .dest_y=editor_start_y-6, .dest_w=0, .dest_h=0,
			.flip_horizontal=false, .flip_vertical=false
		},
		rslen, rs, rs_size
	);
	// Display editor start coordinates over the reticule
	char xcoord[12];
	char ycoord[12];
	int_to_string_12(editor_start_x, xcoord);
	int_to_string_12(editor_start_y, ycoord);
	render_text(xcoord, editor_start_x+5, editor_start_y-24, 2, rslen, rs, rs_size);
	render_text(ycoord, editor_start_x+5, editor_start_y, 2, rslen, rs, rs_size);
	
	// HUD elements around editor_new coordinates and end of working vector ------------------------------------------------------------------------------------
	
	// The end of the working reticule's offset with respect to editor_new_x/_y
	int arshiftx = 0; int arshifty = 0;
	
	if (editor_mode == EDITOR_SHAPING) {
		// Display the new edge that's being, or has been, made
		if (editor_vertical) {
		// Depict the wall with tile_type 20
			add_r(
				(struct r) {
					.source_x=100+tileDim+tilebaseX(20), .source_y=tilebaseY(20), .source_w=tileDim, .source_h=tileDim,
					.dest_x=(editor_negative? editor_new_x : editor_new_x-tileDim),
					.dest_y=(editor_negative? editor_new_y-editor_newedge_mag+tileDim : editor_new_y+tileDim),
					.dest_w=0, .dest_h=editor_newedge_mag-(2*tileDim), // Scale height by editor_newedge_mag
					.flip_horizontal=editor_negative, .flip_vertical=false
				},
				rslen, rs, rs_size
			);
		} else { // (It's horizontal)
			// Depict the floor/ceiling with tile_type 20
			add_r(
				(struct r) {
					.source_x=100+tilebaseX(20), .source_y=tileDim+tilebaseY(20), .source_w=tileDim, .source_h=tileDim,
					.dest_x=(editor_negative? editor_new_x-editor_newedge_mag+tileDim : editor_new_x+tileDim),
					.dest_y=(editor_negative? editor_new_y-tileDim : editor_new_y),
					.dest_w=editor_newedge_mag-(2*tileDim), .dest_h=0, // Scale width by editor_newedge_mag
					.flip_horizontal=false, .flip_vertical=editor_negative
				},
				rslen, rs, rs_size
			);
		}
		
		// Display auxiliary editor reticule at the end of the working-vector
		if (editor_vertical) {
			arshifty = (editor_negative? -editor_newedge_mag : editor_newedge_mag);
		} else { // (editor is horizontal)
			arshiftx = (editor_negative? -editor_newedge_mag : editor_newedge_mag);
		}
		
		// Display the working-vector's corner
		{
			// Figure out whether we flip it along each axis
			bool cflip_h, cflip_v; // (horizontally, vertically)
			if (editor_vertical) {
				if (editor_negative == editor_newcorner_dir) {
					cflip_h = false;
					cflip_v = true;
				} else {
					cflip_h = true;
					cflip_v = false;
				}
			} else { // (editor is horizontal)
				if (editor_negative) {
					cflip_h = cflip_v = true;
				} else {
					cflip_h = cflip_v = false;
				}
			}
			// Display the new corner with tile_type 20
			add_r(
				(struct r) {
					// Concave or convex corner?
					.source_x=(editor_newcorner_dir? 0 : tileDim)+100+tilebaseX(20), .source_y=(editor_newcorner_dir? 2*tileDim : tileDim)+tilebaseY(20),
					.source_w=(editor_newcorner_dir? 2*tileDim : tileDim), .source_h=(editor_newcorner_dir? 2*tileDim : tileDim),
					// Direction dependent
					.dest_x=((editor_newcorner_dir || !cflip_h)? editor_new_x-tileDim : editor_new_x) + arshiftx,
					.dest_y=((editor_newcorner_dir || cflip_v)? editor_new_y-tileDim : editor_new_y) + arshifty,
					.dest_w=0, .dest_h=0, // (Do not resize)
					.flip_horizontal=cflip_h, .flip_vertical=cflip_v
				},
				rslen, rs, rs_size
			);
		}
		if (editor_new_x+arshiftx == editor_start_x &&
		    editor_new_y+arshifty == editor_start_y &&
		    editor_newcorner_dir != editor_negative) {
			render_text("[Space]: Finish shape", editor_new_x+5+arshiftx, editor_new_y-48+arshifty, 2, rslen, rs, rs_size);
		} else {
			// Display an auxiliary reticule at the working coordinates
			add_r(
				(struct r) {
					.source_x=212, .source_y=2940, .source_w=12, .source_h=12,
					.dest_x=editor_new_x-6+arshiftx, .dest_y=editor_new_y-6+arshifty, .dest_w=0, .dest_h=0,
					.flip_horizontal=false, .flip_vertical=false
				},
				rslen, rs, rs_size
			);
			// Display the working coordinates over the new reticule
			char new_x_readout[12];
			char new_y_readout[12];
			int_to_string_12(editor_new_x+arshiftx, new_x_readout);
			int_to_string_12(editor_new_y+arshifty, new_y_readout);
			render_text(new_x_readout, editor_new_x+5+arshiftx, editor_new_y-24+arshifty, 2, rslen, rs, rs_size);
			render_text(new_y_readout, editor_new_x+5+arshiftx, editor_new_y+arshifty,    2, rslen, rs, rs_size);
			if (editor_autofinish_available)
				render_text("[Tab]: Auto-finish", editor_new_x+5+arshiftx, editor_new_y-36+arshifty, 1, rslen, rs, rs_size);
		}
	}
	
	// Shift the camera to center over the working reticule ----------------------------------------------------------------------------------------------------
	
	special_shift_x = editor_new_x - (averiX) + arshiftx;
	special_shift_y = editor_new_y - (averiY+averiH) + arshifty;
	
	// Just overwrite whatever game.c was going to do with camera_x/_y
	camera_x = averiX - (ideal_w/2) + special_shift_x;
	camera_y = averiY - (ideal_h/2) + special_shift_y;
	
	// Display editing help at the top left of the screen (Done last so as to use the real camera_x/_y values)
	render_text(hud_strs[editor_mode], camera_x+550, camera_y, 2, rslen, rs, rs_size);
	if (n_new_forms) // (Also show a little notice in the top right if you have unsaved work)
		render_text("[F6]: Save new features\nto level file", camera_x+1000, camera_y, 2, rslen, rs, rs_size);
	else
		render_text("(Level file up-to-date)", camera_x+1000, camera_y+15, 1, rslen, rs, rs_size);
}
