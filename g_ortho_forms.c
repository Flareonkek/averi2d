
// ----------------------------------------------------------------------------------------------------------------------------------
// ORTHO_FORM: Add collision and rendering instructions for an orthogonal solid shape to crs and rs, respectively -------------------
// ----------------------------------------------------------------------------------------------------------------------------------

// Constants and helper functions for this file

// Dimensions of a tile set on the sprite sheet
static const int tilebaseW = 12;
static const int tilebaseH = 24;

// Height and width of all tile corners
const int tileDim = 6;

// Functions for drawing the right sprite for each tile type
int tilebaseX(int tile_type) {
	// There are three tile sprites in a row on the sheet
	int col_n = tile_type % 3;
	return col_n * tilebaseW;
}
int tilebaseY(int tile_type) {
	// There are seven tile sprite rows on the sheet
	int row_n = ( tile_type - (tile_type % 3) ) / 3;
	return row_n * tilebaseH;
}

// Make a shape from an array of vectors forming the shape's perimeter. All vertices are 90 degree turns to the left or right.
// The first vector, at (start_x, start_y), is horizontal and positive (pointing east), and there are perimeterlen vectors.
// Each vertex has its length (per_mag) and direction (per_dir) describing the line to the next vertex and the turn-direction
// (true for left, false for right) to face the cardinal-direction that the next one will point.
// The vertices must be arranged clockwise to form a closed perimeter.
// Also, the caller passes in pointers to its rendering and collision data for this function to add to them.

static void ortho_form( // ------------------------------------------------------------------------------------------------------------
	int start_x, int start_y, // Where to start making the shape
	int per_mags[], bool per_dirs[], int perimeterlen, // The vectors forming the shape's perimeter
	int tile_type, // (There are 21 different tile designs on the sprite sheet, set it to anything 0~20)
	struct r* rs, int* rslen, int rs_size, // Pointers and limit to add onto the caller's rendering data
	struct coll_rect* crs, int* crlen, int crs_size) // Pointers  & limit to add onto the caller's collision data
{
	
	int x = start_x; int y = start_y; // Keep track of location throughout the for-loop
	bool vertical = 0; bool negative = 0; // Keep track of point-direction throughout the for-loop
	
	int n_colliders = 0; // Initialize empty
	int first_collider_i = (*crlen); // (add new colliders at and after this index)
	struct bottom {int x; int y; int len;};
	struct bottom bottoms[perimeterlen]; // x and y are the origin, that is, the eastern point of the negative-direction line
	int n_bottoms = 0;
	int vxs[perimeterlen]; // Vertex Xs (We'll split all collision rectangles at these X-points)
	int vys[perimeterlen]; // Corresponding Ys (For collision rectangle expansion)
	
	// Run through vertices initializing a collider on each top edge, and a bottom on each bottom edge --------------------------------
	
	for (int i = 0; i < perimeterlen; i++) { // For each ortho_vertex...
		int vect_mag = per_mags[i]; // This vector's magnitude
		bool vect_lft = per_dirs[i]; // This vector's end direction, true if left-turn, false if right-turn
		vxs[i] = x;
		vys[i] = y;
		
		if (vertical) {
			// Update position
			if (negative) y -= vect_mag;
			else y += vect_mag;
		} else { //...it's horizontal
			if (negative) {
				bottoms[n_bottoms] = (struct bottom) {x, y, vect_mag};
				n_bottoms++;
				x -= vect_mag; // Update position
			} else { //...it's a horizontal positive vertex
				// Add a collision rectangle
				if (first_collider_i + 1 >= crs_size) {
					printf("ERROR in g_ortho_forms.c: Finna overflow crs with this %i-vector shape, with crs already at %i out of %i items full\n", perimeterlen, first_collider_i, crs_size);
					exit(1);
				}
				crs[first_collider_i+n_colliders] = (struct coll_rect) {x, y, vect_mag, 6}; // Provisory height of 6
				//printf("New collider: crs[%i] (x=%i, y=%i, w=%i, h=%i)\n", first_collider_i+n_colliders, crs[first_collider_i+n_colliders].x, crs[first_collider_i+n_colliders].y, crs[first_collider_i+n_colliders].w, crs[first_collider_i+n_colliders].h);
				n_colliders++;
				x += vect_mag; // Update position
			}
		}
		// Set these for the next vertex:
		negative = ( vertical == negative? (vect_lft) : !(vect_lft) );
		vertical = !vertical;
	}
	
	// (Check that the perimeter-walk has brought us back where we began)
	if ( (x != start_x) || (y != start_y) || vertical || negative) {
		printf("ERROR in ortho_form(): UNCLOSED SHAPE\n");
		exit(1);
	}
	
	// Split all colliders that might need it to account for a non-flat bottom under them ---------------------------------------------
	
	for (int i = first_collider_i; i < first_collider_i + n_colliders; i++) {
		//printf("Split-check crs[%i] (x=%i, y=%i, w=%i, h=%i)...\n", i, crs[i].x, crs[i].y, crs[i].w, crs[i].h);
		for (int j = 0; j < perimeterlen; j++) {
			// If there is a vertex in the middle of the collider
			if ( (vxs[j]>crs[i].x) && (vxs[j]<(crs[i].x+crs[i].w)) ) {
				if (first_collider_i + n_colliders >= crs_size) {
					printf("ERROR in g_ortho_forms.c: Finna overflow crs during splitting\n");
					exit(1);
				}
				//printf("\tSplitting crs[%i]...\n", i);
				// Then split the collider in two (by adding a new one and shrinking the original)
				crs[first_collider_i + n_colliders].x = vxs[j];
				crs[first_collider_i + n_colliders].y = crs[i].y;
				crs[first_collider_i + n_colliders].w = (crs[i].x+crs[i].w) - vxs[j];
				crs[first_collider_i + n_colliders].h = crs[i].h;
				//printf("\tNew collider: crs[%i] (x=%i, y=%i, w=%i, h=%i)\n", first_collider_i+n_colliders, crs[first_collider_i+n_colliders].x, crs[first_collider_i+n_colliders].y, crs[first_collider_i+n_colliders].w, crs[first_collider_i+n_colliders].h);
				n_colliders++;
				/* The following is NO GOOD: add_cr adds to index *crlen, we need to add to first_collider_i + n_colliders.
				add_cr(
					(struct coll_rect) {
						.x = vxs[j],
						.y = crs[i].y,
						.w = (crs[i].x+crs[i].w) - vxs[j],
						.h = crs[i].h
					},
					crlen, crs, crs_size
				);
				*/
				crs[i].w = vxs[j] - crs[i].x;
				//printf("\tcrs[%i] shrunk to (x=%i, y=%i, w=%i, h=%i)\n", i, crs[i].x, crs[i].y, crs[i].w, crs[i].h);
			}
		}
	}
	
	// Now expand the colliders down to the bottom edges, making them fully fledged collision rectangles ------------------------------
	
	for (int i = first_collider_i; i < first_collider_i + n_colliders; i++) {
		struct coll_rect t = crs[i];
		for (int j = 0; j < n_bottoms; j++) {
			struct bottom b = bottoms[j];
			// Check if b is the correct bottom for t to stop at. That is, if
			// the location is in range, and either [t is yet to be set or the bottom is higher than a previous tentative]
			if ((b.x-b.len <= t.x && b.x >= t.x+t.w && b.y > t.y) && (crs[i].h == 6 || crs[i].y+crs[i].h > b.y)) {
				// Then move the collider's bottom there
				crs[i].h = b.y-t.y;
			}
		}
		// Render the collision rectangles with their top and bottom trimmed to fill in the shape
		add_r(
			(struct r) {
				.source_x=111+tilebaseX(tile_type), .source_y=23+tilebaseY(tile_type), .source_w=1, .source_h=1, // pixel from concave corner
				.dest_x=crs[i].x, .dest_y=crs[i].y+tileDim,
				.dest_w=crs[i].w, .dest_h=crs[i].h-2*tileDim,
				.flip_horizontal=false, .flip_vertical=false
			},
			rslen, rs, rs_size
		);
	}
	
	// Note how many colliders we added to argument crs -------------------------------------------------------------------------------
	
	(*crlen) += n_colliders;
	
	// Add edges' and corners' rendering instructions to argument rs ------------------------------------------------------------------
	
	bool corner_flip_h, corner_flip_v;
	for (int i = 0; i < perimeterlen; i++) { // For each ortho_vertex...
		int vect_mag = per_mags[i]; // This vector's magnitude
		bool vect_lft = per_dirs[i]; // This vector's end direction, true means left-turn, false means right-turn
		if (vect_mag < 2*tileDim) {
			printf("ERROR in ortho_form(): NOT ENOUGH ROOM FOR CORNER TILE\n");
			exit(1);
		}
		if (vertical) {
			if (vect_mag > 2*tileDim) { // This looks unnecessary but actually prevents a graphical glitch, as struct r .dest_h of 0 means draw without scaling,
				// Add the wall:
				add_r(
					(struct r) {
						.source_x=100+tileDim+tilebaseX(tile_type), .source_y=tilebaseY(tile_type), .source_w=tileDim, .source_h=tileDim,
						.dest_x=(negative? x : x-tileDim), .dest_y=(negative? y-vect_mag+tileDim : y+tileDim),
						.dest_w=0, .dest_h=vect_mag-(2*tileDim), // Stretch height to reach next corner
						.flip_horizontal=negative, .flip_vertical=false
					},
					rslen, rs, rs_size
				);
			}
			
			// Update position
			if (negative) y -= vect_mag;
			else y += vect_mag;
			
			if (negative == vect_lft) {
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
						.source_x=100+tilebaseX(tile_type), .source_y=tileDim+tilebaseY(tile_type), .source_w=tileDim, .source_h=tileDim,
						.dest_x=(negative? x-vect_mag+tileDim : x+tileDim), .dest_y=(negative? y-tileDim : y),
						.dest_w=vect_mag-(2*tileDim), .dest_h=0, // Stretch width to reach next corner
						.flip_horizontal=false, .flip_vertical=negative
					},
					rslen, rs, rs_size
				);
			}
			
			// Update position
			if (negative) {
				x -= vect_mag;
				// Also, the corner sprite should be flipped on both axes
				corner_flip_h = corner_flip_v = 1;
			} else {
				x += vect_mag;
				// Also, the corner sprite shouldn't be flipped on either axis
				corner_flip_h = corner_flip_v = 0;
			}
		}
		
		// Draw the corner sprite
		add_r(
			(struct r) {
				// Concave or convex corner?
				.source_x=(vect_lft? 0 : tileDim)+100+tilebaseX(tile_type), .source_y=(vect_lft? 2*tileDim : tileDim)+tilebaseY(tile_type),
				.source_w=(vect_lft? 2*tileDim : tileDim), .source_h=(vect_lft? 2*tileDim : tileDim),
				// Direction dependent
				.dest_x=((vect_lft || !corner_flip_h)? x-tileDim : x), .dest_y=((vect_lft || corner_flip_v)? y-tileDim : y),
				.dest_w=0, .dest_h=0, // (Do not resize)
				.flip_horizontal=corner_flip_h, .flip_vertical=corner_flip_v
			},
			rslen, rs, rs_size
		);
		
		// Set these for the next vertex:
		negative = ( vertical == negative? (vect_lft) : !(vect_lft) );
		vertical = !vertical;

	}
	// All done -----------------------------------------------------------------------------------------------------------------------
	//printf("(ortho_forms) Added %i coll_rects to argument array\n", n_colliders);
}
