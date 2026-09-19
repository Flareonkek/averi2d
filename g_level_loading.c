
const int seg_w = ideal_w;
const int seg_h = ideal_h;

// ----------------------------------------------------------------------------------------------------------------------------------
// LEVEL QUADRANT: Used to by the game to load in a limited part of the level at a time ---------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

struct level_quadrant {
	// Indices of features of the level contained within this quadrant
	int n_rinds;
	int* rinds;
	int n_crinds;
	int* crinds;
};

static void level_quadrant_add_r_index(int argind, struct level_quadrant* argquad) {
	int* newrinds = malloc( sizeof(int)*(argquad->n_rinds + 1) );
	newrinds[argquad->n_rinds] = argind;
	if (argquad->n_rinds > 0) {
		for (int i = 0; i < argquad->n_rinds; i++) {
			newrinds[i] = (argquad->rinds)[i];
		}
		free(argquad->rinds);
	}
	argquad->rinds = newrinds;
	argquad->n_rinds++;
}

static void level_quadrant_add_cr_index(int argind, struct level_quadrant* argquad) {
	int* newcrinds = malloc( sizeof(int)*(argquad->n_crinds + 1) );
	newcrinds[argquad->n_crinds] = argind;
	if (argquad->n_crinds > 0) {
		for (int i = 0; i < argquad->n_crinds; i++) {
			newcrinds[i] = (argquad->crinds)[i];
		}
		free(argquad->crinds);
	}
	argquad->crinds = newcrinds;
	argquad->n_crinds++;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// LEVEL: contains all the stuff in the environment, with facilities for segmented loading ------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

struct level {
	int n_lrs;
	struct r* lrs;
	int n_lcrs;
	struct coll_rect* lcrs;
	int n_quadrants;
	int quad_row_length;
	struct level_quadrant* quadrants;
	//int n_segments_per_row;
	//int n_segment_rows;
	//struct gll_level_segment* segments;
	int lolx; // Level outline left x
	int loty; // Level outline top y
};

// Level segment, used by load_level to help it make quadrants, four segments per quadrant
struct level_segment {
	int left_x;
	int top_y;
	// Indices of features of the level contained within this segment
	int n_rinds;
	int* rinds;
	int n_crinds;
	int* crinds;
};

static void level_segment_add_r_index(int argind, struct level_segment* argseg) {
	int* newrinds = malloc( sizeof(int)*(argseg->n_rinds + 1) );
	newrinds[argseg->n_rinds] = argind;
	if (argseg->n_rinds > 0) {
		for (int i = 0; i < argseg->n_rinds; i++) {
			newrinds[i] = (argseg->rinds)[i];
		}
	free(argseg->rinds);
	}
	argseg->rinds = newrinds;
	argseg->n_rinds++;
}

static void level_segment_add_cr_index(int argind, struct level_segment* argseg) {
	int* newcrinds = malloc( sizeof(int)*(argseg->n_crinds + 1) );
	newcrinds[argseg->n_crinds] = argind;
	if (argseg->n_crinds > 0) {
		for (int i = 0; i < argseg->n_crinds; i++) {
			newcrinds[i] = (argseg->crinds)[i];
		}
	free(argseg->crinds);
	}
	argseg->crinds = newcrinds;
	argseg->n_crinds++;
}

// A little helper function used when reading the .txt file
bool is_blankspace(char argc) {
	if (argc == ' ')
		return 1;
	if (argc == '\t')
		return 1;
	if (argc == '\n')
		return 1;
	if (argc == '\r') // "Carriage return", I've never seen one but online docs say windows may inject them into filestreams
		return 1;
	return 0;
}

struct level* gll_load_level(const char* filename) {
	
	// ------------------------------------------------------------------------------------------------------------------------------
	// Read level data from .txt file -----------------------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------------------------------------------------------
	
	// Modes for char reading
	enum {
		TYPE_MODE,
		XDIM_MODE,
		YDIM_MODE,
		TILE_MODE,
		VERT_MODE,
	};
	bool comment_mode = 0;
	// Start in TYPE_MODE
	int mode = TYPE_MODE;
	
	FILE* fp;
	//const char* filename = "ortho_form_data.txt";
	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Error, unable to open file \"%s\"\n", filename);
		exit(1);
	}
	
	int c;
	
	// Preliminary scan to set n_entries ----------------------------------------------------------------------------------------
	int n_entries = 0;
	
	while ( (c=fgetc(fp)) != EOF ) {
		if (comment_mode) {
			if (c == '\n')
				comment_mode = 0;
		} else if (c == '#') {
			comment_mode = 1;
		} else {
			if (c == '.')
				mode += 1;
			if (mode > VERT_MODE) {
				mode = TYPE_MODE;
				n_entries++;
			}
		}
	}
	if (mode != TYPE_MODE) {
		printf("Error, ortho form data ends in incomplete entry\n");
		exit(1);
	}
	if (!n_entries) {
		printf("Error, no entries in file to load\n");
		exit(1);
	}
	
	// Scan in entries ----------------------------------------------------------------------------------------------------------
	struct oform entries[n_entries];
	
	// Initialize all the entries because stuff needs them to start with zeros
	for (int i = 0; i < n_entries; i++) {
		entries[i].x = 0;
		entries[i].y = 0;
		entries[i].tile_type = 0;
		entries[i].len = 0;
	}
	int current_entry = 0;
	
	rewind(fp); // Go back to beginning of file stream to read it again
	comment_mode = 0;
	int tempmag = 0;
	bool neg_x_flag = 0;
	bool neg_y_flag = 0;
	
	// Scan in data, storing it in entries array
	while ( (c=fgetc(fp)) != EOF ) {
		//printf("C is %c\n", c);
		if (comment_mode) {
			if (c == '\n')
				comment_mode = 0;
		} else if (c == '#') {
			comment_mode = 1;
		} else {
			switch (mode) {
				case TYPE_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c != 'A') {
							printf("Error, only type A entries are allowed for now\n");
							exit(1);
						}
					}
					break;
				case XDIM_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c == '-' && entries[current_entry].x == 0)
							neg_x_flag = 1;
						else if (c < '0' || c > '9') {
							printf("Error, X dimension must be integer number\n");
							exit(1);
						} else {
							entries[current_entry].x *= 10;
							entries[current_entry].x += (c-'0');
						}
					}
					break;
				case YDIM_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c == '-' && entries[current_entry].y == 0)
							neg_y_flag = 1;
						else if (c < '0' || c > '9') {
							printf("Error, Y dimension must be integer number\n");
							exit(1);
						} else {
							entries[current_entry].y *= 10;
							entries[current_entry].y += (c-'0');
						}
					}
					break;
				case TILE_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c < '0' || c > '9') {
							printf("Error, tile_type must be integer number\n");
							exit(1);
						} else {
							entries[current_entry].tile_type *= 10;
							entries[current_entry].tile_type += (c-'0');
						}
					}
					break;
				case VERT_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c < '0' || c > '9') {
							if (tempmag < 12) {
								printf("Error, vertex length must be integer number >= 12\n");
								exit(1);
							}
							if (c == 'L')
								oform_addvect(tempmag, 1, &(entries[current_entry]));
							else if (c == 'R')
								oform_addvect(tempmag, 0, &(entries[current_entry]));
							else {
								printf("Error, vertex direction must be L or R\n");
								exit(1);
							}
							tempmag = 0;
						} else {
							tempmag *= 10;
							tempmag += (c-'0');
						}
					}
					break;
			}
			if (c == '.')
				mode += 1;
			if (mode > VERT_MODE) {
				mode = TYPE_MODE;
				// Reset flags and increment current_entry for next entry
				if (neg_x_flag)
					entries[current_entry].x *= -1;
				if (neg_y_flag)
					entries[current_entry].y *= -1;
				neg_x_flag = neg_y_flag = 0;
				current_entry++;
			}
		}
	}
	
	fclose(fp);	
	
	/* Show that the scanned-in data in entries[] is correct
	for ( int i = 0; i < n_entries; i++ ) {
		printf("A @ (%i, %i) tile %i:\n", entries[i].x, entries[i].y, entries[i].tile_type);
		for ( int j = 0; j < entries[i].len; j++) {
			printf(" %i%c", entries[i].vectmags[j], entries[i].vectdirs[j]?'L':'R');
		}
		printf("\n\n");
	}
	*/
	
	// ------------------------------------------------------------------------------------------------------------------------------------------
	// Process the data for segmented loading ---------------------------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------------------------------------------------------------------
	
	// Process the forms into the level's lrs and lcrs ------------------------------------------------------------------------------------------
	
	struct level* resultlevel = malloc( sizeof(struct level) );
	int temp_lrs_room = 1000;
	resultlevel-> lrs = malloc( sizeof(struct r) * temp_lrs_room ); // Just assuming no one will try to add more than 1000 in a single shape. If they do itll crash of course
	resultlevel-> n_lrs = 0;
	int temp_lcrs_room = 1000;
	resultlevel-> lcrs = malloc( sizeof(struct coll_rect) * temp_lcrs_room ); // (ditto)
	resultlevel-> n_lcrs = 0;
	
	// Fill up resultlevel's arrays
	for (int i = 0; i < n_entries; i++) {
		ortho_form(
			entries[i].x, entries[i].y,
			entries[i].vectmags, entries[i].vectdirs, entries[i].len,
			entries[i].tile_type,
			(resultlevel-> lrs), &(resultlevel-> n_lrs), temp_lrs_room,
			(resultlevel-> lcrs), &(resultlevel-> n_lcrs), temp_lcrs_room
		);
		// If lrs or lcrs are halfway full or more, double their room
		if (resultlevel-> n_lrs > (temp_lrs_room/2)) {
			temp_lrs_room *= 2;
			struct r* newlrs = malloc( sizeof(struct r) * temp_lrs_room );
			for (int i = 0; i < resultlevel-> n_lrs; i++) {
				newlrs[i] = resultlevel-> lrs[i];
			}
			free(resultlevel-> lrs);
			resultlevel-> lrs = newlrs;
		}
		if (resultlevel-> n_lcrs > (temp_lcrs_room/2)) {
			temp_lcrs_room *= 2;
			struct coll_rect* newlcrs = malloc( sizeof(struct coll_rect) * temp_lcrs_room );
			for (int i = 0; i < resultlevel-> n_lcrs; i++) {
				newlcrs[i] = resultlevel-> lcrs[i];
			}
			free(resultlevel-> lcrs);
			resultlevel-> lcrs = newlcrs;
		}
	}
	// Now that that's done, trim those arrays down to just the exact size they need
	struct r* newlrs = malloc( sizeof(struct r) * resultlevel-> n_lrs );
	struct coll_rect* newlcrs = malloc( sizeof(struct coll_rect) * resultlevel-> n_lcrs );
	for (int i = 0; i < resultlevel-> n_lrs; i++) {
		newlrs[i] = resultlevel-> lrs[i];
	}
	for (int i = 0; i < resultlevel-> n_lcrs; i++) {
		newlcrs[i] = resultlevel-> lcrs[i];
	}
	free(resultlevel-> lrs);
	free(resultlevel-> lcrs);
	resultlevel-> lrs = newlrs;
	resultlevel-> lcrs = newlcrs;
	
	// Determine and fill in the correct outline: lx (left x), rx, ty (top y,) and by, for each lr and lcr --------------------------------------
	
	int lr_lxs[resultlevel->n_lrs];
	int lr_rxs[resultlevel->n_lrs];
	int lr_tys[resultlevel->n_lrs];
	int lr_bys[resultlevel->n_lrs];
	
	int lcr_lxs[resultlevel->n_lcrs];
	int lcr_rxs[resultlevel->n_lcrs];
	int lcr_tys[resultlevel->n_lcrs];
	int lcr_bys[resultlevel->n_lcrs];
	
	for (int i = 0; i < resultlevel->n_lrs; i++) {
		int left_x = resultlevel->lrs[i].dest_x;
		int right_x;
		if (resultlevel->lrs[i].dest_w)
			right_x = left_x + resultlevel->lrs[i].dest_w;
		else
			right_x = left_x + resultlevel->lrs[i].source_w;
		int top_y = resultlevel->lrs[i].dest_y;
		int bottom_y;
		if (resultlevel->lrs[i].dest_h)
			bottom_y = top_y + resultlevel->lrs[i].dest_h;
		else
			bottom_y = top_y + resultlevel->lrs[i].source_h;
		lr_lxs[i] = left_x;
		lr_rxs[i] = right_x;
		lr_tys[i] = top_y;
		lr_bys[i] = bottom_y;
	}
	
	for (int i = 0; i < resultlevel->n_lcrs; i++) {
		int left_x = resultlevel->lcrs[i].x;
		int right_x = resultlevel->lcrs[i].x + resultlevel->lcrs[i].w;
		int top_y = resultlevel->lcrs[i].y;
		int bottom_y = resultlevel->lcrs[i].y + resultlevel->lcrs[i].h;
		lcr_lxs[i] = left_x;
		lcr_rxs[i] = right_x;
		lcr_tys[i] = top_y;
		lcr_bys[i] = bottom_y;
		//printf("lcr entry %i extrema [ x: %i ~ %i, y: %i ~ %i ]\n", i, left_x, right_x, top_y, bottom_y);
	}
	
	// Now, figure out the outer dimensions of the whole level overall --------------------------------------------------------------------------
	
	//Candidate data
	int left_x = lr_lxs[0];
	int right_x = left_x;
	int top_y = lr_tys[0];
	int bottom_y = top_y;
	// Check each entry
	for (int i = 0; i < resultlevel->n_lrs; i++) {
		// Change these as better candidates are found
		if (lr_lxs[i] < left_x)   left_x   = lr_lxs[i];
		if (lr_rxs[i] > right_x)  right_x  = lr_rxs[i];
		if (lr_tys[i] < top_y)    top_y    = lr_tys[i];
		if (lr_bys[i] > bottom_y) bottom_y = lr_bys[i];
	}
	
	//printf("\nWhole level extrema [ x: %i ~ %i, y: %i ~ %i ]\n\n", left_x, right_x, top_y, bottom_y);
	
	resultlevel-> lolx = left_x;
	resultlevel-> loty = top_y;
	
	// Determine the rows and columns and number of segments, then quadrants, needed to contain the level ---------------------------------------
	
	int segcols = 1 + (right_x - left_x) / seg_w;
	int segrows = 1 + (bottom_y - top_y) / seg_h;
	if (segcols < 2) segcols = 2;
	if (segrows < 2) segrows = 2;
	int total_segments = segrows * segcols;
	
	struct level_segment lsegs[total_segments];
	// ALWAYS zero out all the mallocated-array length-counters for shit with _addindex type functions, or you'll corrupt memory
	for (int i = 0; i < total_segments; i++) {
		lsegs[i].n_rinds = 0;
		lsegs[i].n_crinds = 0;
	}
	
	//printf("Seg dimensions (%i, %i)\n%i seg columns, %i seg rows\n%i total segs\n\n", seg_w, seg_h, segcols, segrows, total_segments);
	
	int quadcols = segcols - 1;
	if (quadcols == 0) quadcols = 1;
	int quadrows = segrows - 1;
	if (quadrows == 0) quadrows = 1;
	int total_quadrants = quadcols * quadrows;
	
	//printf("%i quad columns, %i quad rows\n%i total quads\n\n", quadcols, quadrows, total_quadrants);
		
	resultlevel-> n_quadrants = total_quadrants;
	resultlevel-> quad_row_length = quadcols;
	resultlevel-> quadrants = malloc( sizeof(struct level_quadrant) * total_quadrants );

	// Arrange segments on each axis and point them to the objects they encompass ---------------------------------------------------------------
	
	int seg_index = 0;
	int fullest_seg_n = 0;
	for (int j = 0; j < segrows; j++) {
		for (int i = 0; i < segcols; i++) {
			int slx = i * seg_w + left_x; // Segment left x
			int srx = slx + seg_w;        // Segment right x
			int sty = j * seg_h + top_y;  // Segment top y
			int sby = sty + seg_h;        // Segment bottom y
			//printf("SCANNING FOR FEATURES IN SEGMENT %i, AT (%i ~ %i, %i ~ %i)\n", seg_index, slx, srx, sty, sby);
			// Check every r and cr as to whether or not they can be seen inside this segment
			//printf("Checking all %i lrs...\n", resultlevel->n_lrs);
			for (int k = 0; k < resultlevel->n_lrs; k++) {
				if (lr_rxs[k] >= slx &&
					lr_lxs[k] <= srx &&
					lr_bys[k] >= sty &&
					lr_tys[k] <= sby )
				{
					level_segment_add_r_index(k, &(lsegs[seg_index]));
					//printf("Found lr %i (%i ~ %i, %i ~ %i)\n", k, lr_lxs[k], lr_rxs[k], lr_tys[k], lr_bys[k]);
					//printf("Adding index %i to segment %i lrs\n", k, seg_index);
				}
			}
			//printf("\nChecking all %i lcrs...\n", resultlevel->n_lcrs);
			for (int k = 0; k < resultlevel->n_lcrs; k++) {
				if (lcr_rxs[k] >= slx &&
					lcr_lxs[k] <= srx &&
					lcr_bys[k] >= sty &&
					lcr_tys[k] <= sby )
				{
					level_segment_add_cr_index(k, &(lsegs[seg_index]));
					//printf("Found lcr %i (%i ~ %i, %i ~ %i)\n", k, lcr_lxs[k], lcr_rxs[k], lcr_tys[k], lcr_bys[k]);
					//printf("Adding index %i to segment %i lcrs\n", k, seg_index);
				}
			}
			
			if (lsegs[seg_index].n_rinds > fullest_seg_n)
				fullest_seg_n = lsegs[seg_index].n_rinds;
			if (lsegs[seg_index].n_crinds > fullest_seg_n)
				fullest_seg_n = lsegs[seg_index].n_crinds;
			
			seg_index++;
		}
	}
	//printf("\n\n");
	
	/*
	for (int i = 0; i < total_segments; i++) {
		printf("lsegs[%i]: its %i rinds: ", i, lsegs[i].n_rinds);
		for (int j = 0; j < lsegs[i].n_rinds; j++) {
			printf("%i, ", lsegs[i].rinds[j]);
		}
		printf("\nand its %i crinds: ", lsegs[i].n_crinds);
		for (int j = 0; j < lsegs[i].n_crinds; j++) {
			printf("%i, ", lsegs[i].crinds[j]);
		}
		printf("\n");
	}
	*/
	// Arrange quadrants and fill them with the indices of their proper segment quartets --------------------------------------------------------
	
	//printf("MARKER B, fullest_seg_n is %i\n\n", fullest_seg_n);
	
	for (int i = 0; i < total_quadrants; i++) {
		resultlevel->quadrants[i].n_rinds = 0;
		resultlevel->quadrants[i].n_crinds = 0;
	} int lqi = 0; // Working index for resultlevel->quadrants for now
	
	int seg_index_memory_lens = fullest_seg_n*4;
	
	int lr_seg_index_memory[seg_index_memory_lens];
	int lcr_seg_index_memory[seg_index_memory_lens];
	
	// For each and quadrant we're adding to the level...
	for (int j = 0; j < quadrows; j++) {
		for (int i = 0; i < quadcols; i++) {
			
			// Determine the indices of the four  segments implicated in this quadrant --------------------------------------

			int topleftseg = i + (segcols*j);
			int bottomleftseg = topleftseg + segcols;
			int toprightseg = topleftseg + 1;
			int bottomrightseg = toprightseg + segcols;
			
			//printf("Considering quadrant %i (comprising segments %i, %i, %i, and %i)...\n", lqi, topleftseg, toprightseg, bottomleftseg, bottomrightseg);
			
			// Check all four segments and add their contents to seg_index_memory -------------------------------------------
			
			// Zero out stuff that needs it
			for (int i = 0; i < seg_index_memory_lens; i++) {
				lr_seg_index_memory[i] = 0;
				lcr_seg_index_memory[i] = 0;
			}
			int lr_sim_i = 0; // Current length of lr_seg_index_memory
			int lcr_sim_i = 0; // Current length of lcr_seg_index_memory
			
			// Add the rinds and crinds of topleftseg to seg_index_memory
			for (int i = 0; i < (lsegs[topleftseg].n_rinds); i++) {
				lr_seg_index_memory[lr_sim_i] = (lsegs[topleftseg].rinds[i]);
				lr_sim_i++;
			}
			for (int i = 0; i < (lsegs[topleftseg].n_crinds); i++) {
				lcr_seg_index_memory[lcr_sim_i] = (lsegs[topleftseg].crinds[i]);
				lcr_sim_i++;
			}
			// Add any new rinds found in toprightseg
			bool already_there;
			for (int i = 0; i < (lsegs[toprightseg].n_rinds); i++) {
				already_there = 0; // Assume it's not until its found
				for(int j = 0; j < lr_sim_i; j++) {
					if ( (lsegs[toprightseg].rinds[i]) == lr_seg_index_memory[j] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lr_seg_index_memory[lr_sim_i] = (lsegs[toprightseg].rinds[i]);
					lr_sim_i++;
				}
			}
			// Add any new crinds found in toprightseg
			for (int i = 0; i < (lsegs[toprightseg].n_crinds); i++) {
				already_there = 0; // Assume it's not until its found
				for(int j = 0; j < lcr_sim_i; j++) {
					if ( (lsegs[toprightseg].crinds[i]) == lcr_seg_index_memory[j] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lcr_seg_index_memory[lcr_sim_i] = (lsegs[toprightseg].crinds[i]);
					lcr_sim_i++;
				}
			}
			// Add any new rinds found in bottomleftseg
			for (int i = 0; i < (lsegs[bottomleftseg].n_rinds); i++) {
				already_there = 0; // Assume it's not until its found
				for(int j = 0; j < lr_sim_i; j++) {
					if ( (lsegs[bottomleftseg].rinds[i]) == lr_seg_index_memory[j] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lr_seg_index_memory[lr_sim_i] = (lsegs[bottomleftseg].rinds[i]);
					lr_sim_i++;
				}
			}
			// Add any new crinds found in bottomleftseg
			for (int i = 0; i < (lsegs[bottomleftseg].n_crinds); i++) {
				already_there = 0; // Assume it's not until its found
				for(int j = 0; j < lcr_sim_i; j++) {
					if ( (lsegs[bottomleftseg].crinds[i]) == lcr_seg_index_memory[j] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lcr_seg_index_memory[lcr_sim_i] = (lsegs[bottomleftseg].crinds[i]);
					lcr_sim_i++;
				}
			}
			// Add any new rinds found in bottomrightseg
			for (int i = 0; i < (lsegs[bottomrightseg].n_rinds); i++) {
				already_there = 0; // Assume it's not until its found
				for(int j = 0; j < lr_sim_i; j++) {
					if ( (lsegs[bottomrightseg].rinds[i]) == lr_seg_index_memory[j] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lr_seg_index_memory[lr_sim_i] = (lsegs[bottomrightseg].rinds[i]);
					lr_sim_i++;
				}
			}
			// Add any new crinds found in bottomrightseg
			for (int i = 0; i < (lsegs[bottomrightseg].n_crinds); i++) {
				already_there = 0; // Assume it's not until its found
				for(int j = 0; j < lcr_sim_i; j++) {
					if ( (lsegs[bottomrightseg].crinds[i]) == lcr_seg_index_memory[j] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lcr_seg_index_memory[lcr_sim_i] = (lsegs[bottomrightseg].crinds[i]);
					lcr_sim_i++;
				}
			}
			
			// Add all level geometry indexed to seg_index_memory -----------------------------------------------------------
			
			for (int i = 0; i < lr_sim_i; i++) {
				int j = lr_seg_index_memory[i];
				//printf("The %ith index of lr_seg_index_memory is %i, adding that to quadrant %i...\n", i, j, lqi);
				level_quadrant_add_r_index(j, &(resultlevel->quadrants[lqi]));
			}
			for (int i = 0; i < lcr_sim_i; i++) {
				int j = lcr_seg_index_memory[i];
				level_quadrant_add_cr_index(j, &(resultlevel->quadrants[lqi]));
			}
			lqi++;
			
			// (End of this quadrant's work)
		}
	}
	/*
	printf("About to return pointer resultlevel, with n_lrs %i, n_lcrs %i, n_quadrants %i, lolx %i, and loty %i,\n",
		resultlevel->n_lrs, resultlevel->n_lcrs, resultlevel->n_quadrants, resultlevel->lolx, resultlevel->loty);
	for (int i = 0; i < resultlevel->n_lrs; i++) {
		printf("lrs[%i]: at (%i, %i)\n", i, resultlevel->lrs[i].dest_x, resultlevel->lrs[i].dest_y);
	}
	for (int i = 0; i < resultlevel->n_lcrs; i++) {
		printf("lcrs[%i]: at (%i, %i)\n", i, resultlevel->lcrs[i].x, resultlevel->lcrs[i].y);
	}
	*/
	return resultlevel;
}

void printdemo_level(struct level* arglevel) {
	for (int i = 0; i < arglevel->n_lrs; i++) {
		printf("lrs[%i]: at (%i, %i)\n", i, arglevel->lrs[i].dest_x, arglevel->lrs[i].dest_y);
	}
	for (int i = 0; i < arglevel->n_lcrs; i++) {
		printf("lcrs[%i]: at (%i, %i)\n", i, arglevel->lcrs[i].x, arglevel->lcrs[i].y);
	}
	for (int i = 0; i < arglevel->n_quadrants; i++) {
		printf("quadrant %i contains lrs:\n", i);
		for (int j = 0; j < arglevel->quadrants[i].n_rinds; j++) {
			printf("%i%s", arglevel->quadrants[i].rinds[j], j==(arglevel->quadrants[i].n_rinds-1)? "\n": ", ");
		}
		printf("quadrant %i contains lcrs:\n", i);
		for (int j = 0; j < arglevel->quadrants[i].n_crinds; j++) {
			printf("%i%s", arglevel->quadrants[i].crinds[j], j==(arglevel->quadrants[i].n_crinds-1)? "\n": ", ");
		}
	}
}

void gll_unload_level(struct level* arglevel) {
	// Free all the data now that you're done with it
	free(arglevel->lrs);
	free(arglevel->lcrs);
	
	for (int i = 0; i < arglevel->n_quadrants; i++) {
		if (arglevel->quadrants[i].n_rinds)
			free(arglevel->quadrants[i].rinds);
		if (arglevel->quadrants[i].n_crinds)
			free(arglevel->quadrants[i].crinds);
	}
	free(arglevel->quadrants);
	
	free(arglevel);
}
