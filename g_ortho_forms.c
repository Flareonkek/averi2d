
//------------------------------------------------------------------------------------------------------------------------------------------
// Orthogonal (Polygons made of only 90-degree angles) Level geometry
//------------------------------------------------------------------------------------------------------------------------------------------

//static int delete_dis = 0;

// A point used to create an ortho_form
struct ortho_vertex {
	int len; // How long is the line from this ortho_vertex to the next one?
	bool leftTurn; // Will the corner at the end of that line turn left, or right?
};

// A collision rectangle, defines a solid area for game logic
struct coll_rect {
	int x, y, w, h;
};

static void ortho_form(int start_x, int start_y, struct ortho_vertex perimeter[], int perimeterlen, struct r* rs, int* rslen, struct coll_rect* crs, int* crlen) {
	// Make a shape from an array of ortho_vertices forming the shape's perimeter. All vertices are 90 degree turns to the left or right.
	// The first vertex, at (start_x, start_y), is horizontal and positive (pointing east),
	// each vertex has its length and direction describing the line to the next vertex and the direction that one will point.
	// The vertices must be arranged clockwise to form a closed perimeter.
	// Also, the caller passes in an array of collision rectangles and this method adds its own to it, incrementing *crlen
	const int tileDim = 6;
	int x = start_x; int y = start_y; // Keep track of location throughout the for-loop
	bool vertical = 0; bool negative = 0; // Keep track of point-direction throughout the for-loop
	struct ortho_vertex this;
	
	int n_colliders = 0; // Initialize empty
	struct coll_rect colliders[perimeterlen/2];
	struct bottom {int x; int y; int len;};
	struct bottom bottoms[perimeterlen/2]; // x and y are the origin, that is, the eastern point of the negative-direction line
	int n_bottoms = 0;
	int vxs[perimeterlen]; // Vertex Xs (We'll split all collision rectangles at these X-points)
	int vys[perimeterlen]; // Corresponding Ys (For collision rectangle expansion)
	
	// Run through vertices initializing a collider on each top edge
	for (int i = 0; i < perimeterlen; i++) { // For each ortho_vertex...
		this = perimeter[i];
		vxs[i] = x;
		vys[i] = y;
		
		if (vertical) {
			
			// Update position
			if (negative) y -= this.len;
			else y += this.len;
			
		} else { //...it's horizontal
			
			if (negative) {
				bottoms[n_bottoms] = (struct bottom) {x, y, this.len};
				n_bottoms++;
				
				x -= this.len; // Update position
			} else { //...it's a horizontal positive vertex
				// Add a collision rectangle
				colliders[n_colliders] = (struct coll_rect) {x, y, this.len, 6}; // Height of 6: TEMPORARY TEST
				n_colliders++;
				
				x += this.len; // Update position
			}
			
		}
		// Set these for the next vertex:
		negative = ( vertical == negative? (this.leftTurn) : !(this.leftTurn) );
		vertical = !vertical;
	}
	
	/*
	if (delete_dis < 3) {
		for (int i = 0; i < n_colliders; i++)
			printf("colliders[%i]: (%i, %i, %i, %i)\n", i, colliders[i].x, colliders[i].y, colliders[i].w, colliders[i].h);
		printf("(Original n_colliders: %i)\n", n_colliders);
	}
	*/
	
	if ( (x != start_x) || (y != start_y) || vertical || negative) printf("ERROR in ortho_form(): UNCLOSED SHAPE");
	
	// Split all colliders that might need to be split
	for (int i = 0; i < n_colliders; i++) {
		for (int j = 0; j < perimeterlen; j++) {
			// If there is a vertex in the middle of the collider
			if ( (vxs[j]>colliders[i].x) && (vxs[j]<(colliders[i].x+colliders[i].w)) ) {
				// Then split the collider in two by adding a new one and shrinking the original
				colliders[n_colliders].x = vxs[j];
				colliders[n_colliders].y = colliders[i].y;
				colliders[n_colliders].w = (colliders[i].x+colliders[i].w) - vxs[j];
				colliders[n_colliders].h = colliders[i].h;
				n_colliders++;
				colliders[i].w = vxs[j] - colliders[i].x;
			}
		}
		//if (delete_dis < 3) printf("colliders[%i]: (%i, %i, %i, %i)\n", i, colliders[i].x, colliders[i].y, colliders[i].w, colliders[i].h);
	}
	
	//if (delete_dis < 3) printf("(Post-split n_colliders: %i)\n", n_colliders);
	
	// Now expand the colliders down to the bottom edges, making them fully fledged collision rectangles
	for (int i = 0; i < n_colliders; i++) {
		struct coll_rect t = colliders[i];
		for (int j = 0; j < n_bottoms; j++) {
			struct bottom b = bottoms[j];
			// Check if b is the correct bottom for t to stop at. That is, if
			// the location is in range, and either [t is yet to be set or the bottom is higher than a previous tentative]
			if ((b.x-b.len <= t.x && b.x >= t.x+t.w && b.y > t.y) && (colliders[i].h == 6 || colliders[i].y+colliders[i].h > b.y)) {
				// Then move the collider's bottom there
				colliders[i].h = b.y-t.y;
				//if (delete_dis < 3) printf("Setting colliders[%i].h to %i\n", i, colliders[i].h);
			}
		}
		// Render the collision rectangles with their top and bottom trimmed to fill in the shape
		rs[*rslen] = (struct r) {
			.source_x=111, .source_y=23, .source_w=1, .source_h=1, // gray pixel from concave corner
			.dest_x=colliders[i].x, .dest_y=colliders[i].y+tileDim,
			.dest_w=colliders[i].w, .dest_h=colliders[i].h-2*tileDim,
			.visible=1, .flip_horizontal=0, .flip_vertical=0
		};
		(*rslen)++;
	}
	
	// Add colliders to the caller's array
	for (int i = 0; i < n_colliders; i++)
		crs[i+(*crlen)] = colliders[i];
	(*crlen) += n_colliders;
	
	// Draw the edges and corners
	bool corner_flip_h, corner_flip_v;
	for (int i = 0; i < perimeterlen; i++) { // For each ortho_vertex...
		this = perimeter[i];
		if (this.len < 2*tileDim) printf("ERROR in ortho_form(): NOT ENOUGH ROOM FOR CORNER TILE");
		//if (delete_dis < 3 && 0)
		//	printf("ORTHO_VERTEX from (%i, %i), length %i, %s, [%s direction], %s\n", x, y, this.len, (vertical? "Vertical":"Horizontal"), (negative?"-":"+"), (this.leftTurn?"L":"R"));
		if (vertical) {
			if (this.len > 2*tileDim) {
				// Add the wall:
				rs[*rslen] = (struct r) {
					.source_x=100+tileDim, .source_y=0, .source_w=tileDim, .source_h=tileDim,
					.dest_x=(negative? x : x-tileDim), .dest_y=(negative? y-this.len+tileDim : y+tileDim),
					.dest_w=0, .dest_h=this.len-(2*tileDim), // Stretch height to reach next corner
					.visible=1, .flip_horizontal=negative, .flip_vertical=0
				};
				(*rslen)++;
			}
			
			// Update position
			if (negative) y -= this.len;
			else y += this.len;
			
			if (negative == this.leftTurn) {
				// The corner sprite should be flipped vertically and NOT horizontally
				corner_flip_h = 0; corner_flip_v = 1;
			} else {
				// The corner sprite should be flipped horizontally and NOT vertically
				corner_flip_h = 1; corner_flip_v = 0;
			}
		} else { // (...it's horizontal)
			if (this.len > 2*tileDim) {
			// Add the floor/ceiling
				rs[*rslen] = (struct r) {
					.source_x=100, .source_y=tileDim, .source_w=tileDim, .source_h=tileDim,
					.dest_x=(negative? x-this.len+tileDim : x+tileDim), .dest_y=(negative? y-tileDim : y),
					.dest_w=this.len-(2*tileDim), .dest_h=0, // Stretch width to reach next corner
					.visible=1, .flip_horizontal=0, .flip_vertical=negative
				};
				(*rslen)++;
			}
			
			// Update position
			if (negative) {
				x -= this.len;
				// Also, the corner sprite should be flipped on both axes
				corner_flip_h = corner_flip_v = 1;
			} else {
				x += this.len;
				// Also, the corner sprite shouldn't be flipped on either axis
				corner_flip_h = corner_flip_v = 0;
			}
		}
		
		// Draw the corner sprite
		rs[*rslen] = (struct r) {
			// Concave or convex corner?
			.source_x=(this.leftTurn? 100 : 100+tileDim), .source_y=(this.leftTurn? 2*tileDim : tileDim),
			.source_w=(this.leftTurn? 2*tileDim : tileDim), .source_h=(this.leftTurn? 2*tileDim : tileDim),
			// Direction dependent
			.dest_x=((this.leftTurn || !corner_flip_h)? x-tileDim : x), .dest_y=((this.leftTurn || corner_flip_v)? y-tileDim : y),
			.dest_w=0, .dest_h=0, // (Do not resize)
			.visible=1, .flip_horizontal=corner_flip_h, .flip_vertical=corner_flip_v
		};
		(*rslen)++;
		
		// Set these for the next vertex:
		negative = ( vertical == negative? (this.leftTurn) : !(this.leftTurn) );
		vertical = !vertical;

	}
	//delete_dis++;
	//if (delete_dis <= 3) printf("\n");
}
