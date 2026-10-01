
const int seg_w = ideal_w;
const int seg_h = ideal_h;

// ----------------------------------------------------------------------------------------------------------------------------------
// LEVEL QUADRANT: Used to by the game to load in a limited part of the level at a time ---------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------
/*
struct level_quadrant {
	// Indices of features of the level contained within this quadrant
	int n_rinds;
	int* rinds;
	
	int n_crinds;
	int* crinds;
	
	int n_linds;
	int* linds;
};
*/
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

static void level_quadrant_add_l_index(int argind, struct level_quadrant* argquad) {
	int* newlinds = malloc( sizeof(int)*(argquad->n_linds + 1) );
	newlinds[argquad->n_linds] = argind;
	if (argquad->n_linds > 0) {
		for (int i = 0; i < argquad->n_linds; i++) {
			newlinds[i] = (argquad->linds)[i];
		}
		free(argquad->linds);
	}
	argquad->linds = newlinds;
	argquad->n_linds++;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// LEVEL: contains all the stuff in the environment, with facilities for segmented loading ------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

/*
struct level {
	// Level rendering-instructions
	int n_lrs;
	struct r* lrs;
	
	// Level collision rectangles
	int n_lcrs;
	struct coll_rect* lcrs;
	
	// Level mobile forms
	int n_lmfs;
	struct mobile_form* lmfs;
	
	// Level levers ## NEW ##
	int n_lls;
	struct lever* lls;
	
	// Level quadrants
	int n_quadrants;
	int quad_row_length;
	struct level_quadrant* quadrants;
	
	int lolx; // Level outline left x
	int loty; // Level outline top y
};
*/

// Level segment, used by load_level to help it make quadrants, four segments per quadrant
struct level_segment {
	int left_x;
	int top_y;
	// Indices of features of the level contained within this segment
	int n_rinds;
	int* rinds;
	int n_crinds;
	int* crinds;
	
	int n_linds; // ## NEW ##
	int* linds;
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

static void level_segment_add_l_index(int argind, struct level_segment* argseg) {
	int* newlinds = malloc( sizeof(int)*(argseg->n_linds + 1) );
	newlinds[argseg->n_linds] = argind;
	if (argseg->n_linds > 0) {
		for (int i = 0; i < argseg->n_linds; i++) {
			newlinds[i] = (argseg->linds)[i];
		}
	free(argseg->linds);
	}
	argseg->linds = newlinds;
	argseg->n_linds++;
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
		PATH_MODE
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
	
	int n_entriesA = 0;
	int n_entriesB = 0;
	int n_entriesC = 0;
	char current_entry_type = 'Z'; // Initialize to some bullshit cause why not
	
	while ( (c=fgetc(fp)) != EOF ) {
		if (comment_mode) {
			if (c == '\n')
				comment_mode = 0;
		} else if (c == '#') {
			comment_mode = 1;
		} else {
			if (mode == TYPE_MODE) {
				if (c == 'A' || c == 'B' || c == 'C')
					current_entry_type = c;
			}
			if (c == '.')
				mode += 1;
			if (current_entry_type == 'A' && mode > VERT_MODE) {
				mode = TYPE_MODE;
				n_entriesA++;
			} else if (current_entry_type == 'B' && mode > PATH_MODE) {
				mode = TYPE_MODE;
				n_entriesB++;
			} else if (current_entry_type == 'C' && mode > TILE_MODE) {
				mode = TYPE_MODE;
				n_entriesC++;
			}
		}
	}
	if (mode != TYPE_MODE) {
		printf("Error, entry form data ends in incomplete entry\n");
		exit(1);
	}
	if (!n_entriesA && !n_entriesB && !n_entriesC) {
		printf("Error, no entries in file to load\n");
		exit(1);
	}
	
	//printf("n_entriesA %i, n_entriesB %i, n_entriesC %i\n", n_entriesA, n_entriesB, n_entriesC);
	
	// Scan in entries ----------------------------------------------------------------------------------------------------------
	struct entryform entriesA[n_entriesA];
	struct entryform entriesB[n_entriesB];
	struct entryform entriesC[n_entriesC];
	
	// Initialize all the entries because stuff needs them to start with zeros
	for (int i = 0; i < n_entriesA; i++) {
		entriesA[i].x = 0;
		entriesA[i].y = 0;
		entriesA[i].tile_type = 0;
		entriesA[i].perlen = 0;
		entriesA[i].pathlen = 0;
	}
	for (int i = 0; i < n_entriesB; i++) {
		entriesB[i].x = 0;
		entriesB[i].y = 0;
		entriesB[i].tile_type = 0;
		entriesB[i].perlen = 0;
		entriesB[i].pathlen = 0;
	}
	for (int i = 0; i < n_entriesC; i++) {
		entriesC[i].x = 0;
		entriesC[i].y = 0;
		entriesC[i].tile_type = 0;
		entriesC[i].perlen = 0;
		entriesC[i].pathlen = 0;
	}
	int current_entryA = 0;
	int current_entryB = 0;
	int current_entryC = 0;
	
	rewind(fp); // Go back to beginning of file stream to read it again
	comment_mode = 0;
	int tempmag = 0;
	bool neg_x_flag = false;
	bool neg_y_flag = false;
	int tempdelta = 0;
	bool neg_delta_flag = false;
	
	// Scan in data, storing it in entries array
	while ( (c=fgetc(fp)) != EOF ) {
		//printf("C is %c\n", c);
		if (comment_mode) {
			if (c == '\n')
				comment_mode = 0;
		} else if (c == '#') {
			comment_mode = 1;
		} else { // (Parsing level data character)
			switch (mode) {
				case TYPE_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c == 'A') {
							current_entry_type = 'A';
						} else if (c == 'B') {
							current_entry_type = 'B';
						} else if (c == 'C') {
							current_entry_type = 'C';
						} else {
							printf("Error, only type A, B, or C entries are valid\n");
							exit(1);
						}
					}
					break;
				case XDIM_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c == '-' && (
							(current_entry_type == 'A' && entriesA[current_entryA].x == 0) ||
							(current_entry_type == 'B' && entriesB[current_entryB].x == 0) ||
							(current_entry_type == 'C' && entriesC[current_entryC].x == 0) )
						) {
							neg_x_flag = 1;
						} else if (c < '0' || c > '9') {
							printf("Error, X dimension must be integer number\n");
							exit(1);
						} else {
							if (current_entry_type == 'A') {
								entriesA[current_entryA].x *= 10;
								entriesA[current_entryA].x += (c-'0');
							} else if (current_entry_type == 'B') {
								entriesB[current_entryB].x *= 10;
								entriesB[current_entryB].x += (c-'0');
							} else if (current_entry_type == 'C') {
								entriesC[current_entryC].x *= 10;
								entriesC[current_entryC].x += (c-'0');
							}
						}
					}
					break;
				case YDIM_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c == '-' && (
							(current_entry_type == 'A' && entriesA[current_entryA].y == 0) ||
							(current_entry_type == 'B' && entriesB[current_entryB].y == 0) ||
							(current_entry_type == 'C' && entriesC[current_entryC].y == 0) )
						) {
							neg_y_flag = 1;
						} else if (c < '0' || c > '9') {
							printf("Error, Y dimension must be integer number\n");
							exit(1);
						} else {
							if (current_entry_type == 'A') {
								entriesA[current_entryA].y *= 10;
								entriesA[current_entryA].y += (c-'0');
							} else if (current_entry_type == 'B') {
								entriesB[current_entryB].y *= 10;
								entriesB[current_entryB].y += (c-'0');
							} else if (current_entry_type == 'C') {
								entriesC[current_entryC].y *= 10;
								entriesC[current_entryC].y += (c-'0');
							}
						}
					}
					break;
				case TILE_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c < '0' || c > '9') {
							printf("Error, tile_type must be integer number\n");
							exit(1);
						} else {
							if (current_entry_type == 'A') {
								entriesA[current_entryA].tile_type *= 10;
								entriesA[current_entryA].tile_type += (c-'0');
							} else if (current_entry_type == 'B') {
								entriesB[current_entryB].tile_type *= 10;
								entriesB[current_entryB].tile_type += (c-'0');
							} else if (current_entry_type == 'C') {
								entriesC[current_entryC].tile_type *= 10;
								entriesC[current_entryC].tile_type += (c-'0');
							}
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
							if (c == 'L' || c == 'R') {
								if (current_entry_type == 'A') {
									entryform_addvect(tempmag, (c=='L'?true:false), &(entriesA[current_entryA])); // (Only type A or B entries could get to this point)
								} else if (current_entry_type == 'B') {
									entryform_addvect(tempmag, (c=='L'?true:false), &(entriesB[current_entryB]));
								}
							} else {
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
				case PATH_MODE:
					if (!is_blankspace(c) && c != '.') {
						if (c < '0' || c > '9') {
							if (c == 'X' || c == 'Y') {
								//printf("Adding path point with argdelta %i, argaxis %b, to entriesB\n", tempdelta*(neg_delta_flag?-1:1), (c=='Y'?true:false));
								entryform_addpathpoint(
									tempdelta*(neg_delta_flag?-1:1),
									(c=='Y'?true:false),
									&(entriesB[current_entryB]) // (Only type B entries would even get to this point)
								);
								// Reset these in readiness for next pathpoint
								neg_delta_flag = false;
								tempdelta = 0;
							} else if (c == '-') {
								neg_delta_flag = true;
							} else {
								printf("Error, axis must be X or Y\n");
								exit(1);
							}
						} else { // (It's a digit)
							//printf("Digit %c found, changing tempdelta (%i) to ", c, tempdelta);
							tempdelta *= 10;
							tempdelta += (c-'0');
							//printf("%i\n", tempdelta);
						}
					}
					break;
			}
			if (c == '.')
				mode += 1;
			if (current_entry_type == 'A' && mode > VERT_MODE) {
				mode = TYPE_MODE;
				// Reset flags and increment current_entryA for the next one
				if (neg_x_flag)
					entriesA[current_entryA].x *= -1;
				if (neg_y_flag)
					entriesA[current_entryA].y *= -1;
				neg_x_flag = neg_y_flag = 0;
				current_entryA++;
			} else if (current_entry_type == 'B' && mode > PATH_MODE) {
				mode = TYPE_MODE;
				// Reset flags and increment current_entryB for the next one
				if (neg_x_flag)
					entriesB[current_entryB].x *= -1;
				if (neg_y_flag)
					entriesB[current_entryB].y *= -1;
				neg_x_flag = neg_y_flag = 0;
				current_entryB++;
			} else if (current_entry_type == 'C' && mode > TILE_MODE) {
				mode = TYPE_MODE;
				// Reset flags and increment current_entryC for the next one
				if (neg_x_flag)
					entriesC[current_entryC].x *= -1;
				if (neg_y_flag)
					entriesC[current_entryC].y *= -1;
				neg_x_flag = neg_y_flag = 0;
				current_entryC++;
			}
		}
	}
	
	fclose(fp);
	
	/* Show that the scanned-in data in entriesA, entriesB, and entriesC is correct
	for ( int i = 0; i < n_entriesA; i++ ) {
		printf("A @ (%i, %i) tile %i:\n", entriesA[i].x, entriesA[i].y, entriesA[i].tile_type);
		for ( int j = 0; j < entriesA[i].perlen; j++) {
			printf(" %i%c", entriesA[i].vectmags[j], entriesA[i].vectdirs[j]?'L':'R');
		}
		printf("\n\n");
	}
	for ( int i = 0; i < n_entriesB; i++ ) {
		printf("B @ (%i, %i) tile %i:\n", entriesB[i].x, entriesB[i].y, entriesB[i].tile_type);
		for ( int j = 0; j < entriesB[i].perlen; j++) {
			printf(" %i%c", entriesB[i].vectmags[j], entriesB[i].vectdirs[j]?'L':'R');
		}
		printf("\nWith path points:\n");
		for ( int j = 0; j < entriesB[i].pathlen; j++) {
			printf(" %i%c", entriesB[i].pathdeltas[j], entriesB[i].pathaxes[j]?'Y':'X');
		}
		printf("\n\n");
	}
	for ( int i = 0; i < n_entriesC; i++ ) {
		printf("C @ (%i, %i) tile %i\n", entriesC[i].x, entriesC[i].y, entriesC[i].tile_type);
		printf("\n");
	}
	*/
	
	// ------------------------------------------------------------------------------------------------------------------------------------------
	// Process the data for segmented loading ---------------------------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------------------------------------------------------------------
	
	// Process the entry forms into the level's lrs, lcrs, and lls ------------------------------------------------------------------------------
	
	struct level* resultlevel = malloc( sizeof(struct level) );
	int temp_lrs_room = 1000;
	resultlevel-> lrs = malloc( sizeof(struct r) * temp_lrs_room ); // Just assuming no one will try to add more than 1000 in a single shape. If they do, itll crash mmmkay???
	resultlevel-> n_lrs = 0;
	
	int temp_lcrs_room = 1000;
	resultlevel-> lcrs = malloc( sizeof(struct coll_rect) * temp_lcrs_room ); // (ditto)
	resultlevel-> n_lcrs = 0;
	
	resultlevel-> lmfs = malloc( sizeof(struct mobile_form) * n_entriesB ); // This we already know how many, as there's just one mobile_form per B entry.
	resultlevel-> n_lmfs = n_entriesB;
	
	int temp_lls_room = 1000;
	resultlevel-> lls = malloc( sizeof(struct lever) * temp_lls_room );
	resultlevel-> n_lls = 0;
	
	void resultarrays_adapt(void) {
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
		// If lls is halfway full, double its room
		if (resultlevel-> n_lls > (temp_lls_room/2)) {
			temp_lls_room *= 2;
			struct lever* newlls = malloc( sizeof(struct lever) * temp_lls_room );
			for (int i = 0; i < resultlevel-> n_lls; i++) {
				newlls[i] = resultlevel-> lls[i];
			}
			free(resultlevel-> lls);
			resultlevel-> lls = newlls;
		}
	}
	
	// Fill up resultlevel's arrays lrs (visual data) and lcrs (collision data) (and lls (lever visual data))
	for (int i = 0; i < n_entriesA; i++) {
		ortho_form(
			entriesA[i].x, entriesA[i].y,
			entriesA[i].vectmags, entriesA[i].vectdirs, entriesA[i].perlen,
			entriesA[i].tile_type,
			(resultlevel-> lrs), &(resultlevel-> n_lrs), temp_lrs_room,
			(resultlevel-> lcrs), &(resultlevel-> n_lcrs), temp_lcrs_room
		);
		resultarrays_adapt();
		//printf("Entry A added to resultlevel, whose ->...\n");
		//printf("\tn_lrs is %i, n_lcrs is %i, n_lmfs is %i\n", resultlevel->n_lrs, resultlevel->n_lcrs, resultlevel->n_lmfs);
	}
	for (int i = 0; i < n_entriesB; i++) {
		int temp_i_lrs = resultlevel->n_lrs;
		int temp_i_lcrs = resultlevel->n_lcrs;
		
		ortho_form(
			entriesB[i].x, entriesB[i].y,
			entriesB[i].vectmags, entriesB[i].vectdirs, entriesB[i].perlen,
			entriesB[i].tile_type,
			(resultlevel-> lrs), &(resultlevel-> n_lrs), temp_lrs_room,
			(resultlevel-> lcrs), &(resultlevel-> n_lcrs), temp_lcrs_room
		);
		resultarrays_adapt();
		
		int temp_f_lrs = resultlevel->n_lrs;
		int temp_f_lcrs = resultlevel->n_lcrs;
		
		resultlevel-> lmfs[i].start_x = entriesB[i].x;
		resultlevel-> lmfs[i].start_y = entriesB[i].y;
		resultlevel-> lmfs[i].x = entriesB[i].x;
		resultlevel-> lmfs[i].y = entriesB[i].y;
		resultlevel-> lmfs[i].pathlen = entriesB[i].pathlen;
		resultlevel-> lmfs[i].pathdeltas = malloc( sizeof(int)*entriesB[i].pathlen );
		for (int j = 0; j < entriesB[i].pathlen; j++) {
			resultlevel-> lmfs[i].pathdeltas[j] = entriesB[i].pathdeltas[j];
		}
		resultlevel-> lmfs[i].pathaxes = malloc( sizeof(bool)*entriesB[i].pathlen );
		for (int j = 0; j < entriesB[i].pathlen; j++) {
			resultlevel-> lmfs[i].pathaxes[j] = entriesB[i].pathaxes[j];
		}
		if (entriesB[i].tile_type >= 12 && entriesB[i].tile_type <= 20)
			resultlevel-> lmfs[i].color = entriesB[i].tile_type-12;
		else
			resultlevel-> lmfs[i].color = -1; // (Default: Non-controlled)
		resultlevel-> lmfs[i].mf_state = 0; // Starts off inactive
		resultlevel-> lmfs[i].n_rinds = temp_f_lrs - temp_i_lrs;
		resultlevel-> lmfs[i].rinds = malloc( sizeof(int)*(temp_f_lrs - temp_i_lrs) );
		for (int j = 0; j < (temp_f_lrs-temp_i_lrs); j++) {
			resultlevel-> lmfs[i].rinds[j] = j + temp_i_lrs;
		}
		resultlevel-> lmfs[i].n_crinds = temp_f_lcrs - temp_i_lcrs;
		resultlevel-> lmfs[i].crinds = malloc( sizeof(int)*(temp_f_lcrs - temp_i_lcrs) );
		for (int j = 0; j < (temp_f_lcrs-temp_i_lcrs); j++) {
			resultlevel-> lmfs[i].crinds[j] = j + temp_i_lcrs;
		}
		//printf("Entry B added to resultlevel, whose ->...\n");
		//printf("\tn_lrs is %i, n_lcrs is %i, n_lmfs is %i\n", resultlevel->n_lrs, resultlevel->n_lcrs, resultlevel->n_lmfs);
	}
	for (int i = 0; i < n_entriesC; i++) {
		if (entriesC[i].tile_type >= 33 && entriesC[i].tile_type <= 41) {
			// (If it's a lever, put it in resultlevel->lls rather than ->lrs)
			if (resultlevel->n_lls >= temp_lls_room-1) {
				printf("Somehow about to overfill resultlevel->lls\n");
				exit(1);
			}
			resultlevel->lls[resultlevel->n_lls] = (struct lever) {
				.x=entriesC[i].x,
				.y=entriesC[i].y,
				.color=entriesC[i].tile_type - 33,
			};
			resultlevel->n_lls++;
		} else { // (It's not a lever)
			simple_form(
				entriesC[i].x, entriesC[i].y,
				entriesC[i].tile_type,
				(resultlevel-> lrs), &(resultlevel-> n_lrs), temp_lrs_room,
				(resultlevel-> lcrs), &(resultlevel-> n_lcrs), temp_lcrs_room
			);
		}
		resultarrays_adapt();
		//printf("Entry C added to resultlevel, with tile_type %i\n", entriesC[i].tile_type);
		//printf("resultlevel now has n_lrs %i, n_lcrs %i, and n_lls %i\n", resultlevel->n_lrs, resultlevel->n_lcrs, resultlevel->n_lls);
	}
	
	// Now that that's done, trim those arrays down to just the exact size they need ------------------------------------------------------------
	
	struct r* newlrs = malloc( sizeof(struct r) * resultlevel-> n_lrs );
	struct coll_rect* newlcrs = malloc( sizeof(struct coll_rect) * resultlevel-> n_lcrs );
	struct lever* newlls = malloc( sizeof(struct lever) * resultlevel-> n_lls );
	
	for (int i = 0; i < resultlevel-> n_lrs; i++) {
		newlrs[i] = resultlevel-> lrs[i];
	}
	for (int i = 0; i < resultlevel-> n_lcrs; i++) {
		newlcrs[i] = resultlevel-> lcrs[i];
	}
	for (int i = 0; i < resultlevel-> n_lls; i++) {
		newlls[i] = resultlevel-> lls[i];
	}
	
	free(resultlevel-> lrs);
	free(resultlevel-> lcrs);
	free(resultlevel-> lls);
	
	resultlevel-> lrs = newlrs;
	resultlevel-> lcrs = newlcrs;
	resultlevel-> lls = newlls;
	
	// Determine and fill in the correct outline: lx (left x), rx, ty (top y,) and by, for each lr and lcr --------------------------------------
	
	int lr_lxs[resultlevel->n_lrs];
	int lr_rxs[resultlevel->n_lrs];
	int lr_tys[resultlevel->n_lrs];
	int lr_bys[resultlevel->n_lrs];
	
	int lcr_lxs[resultlevel->n_lcrs];
	int lcr_rxs[resultlevel->n_lcrs];
	int lcr_tys[resultlevel->n_lcrs];
	int lcr_bys[resultlevel->n_lcrs];
	
	int ll_lxs[resultlevel->n_lls];
	int ll_rxs[resultlevel->n_lls];
	int ll_tys[resultlevel->n_lls];
	int ll_bys[resultlevel->n_lls];
	
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
	
	for (int i = 0; i < resultlevel->n_lls; i++) {
		int left_x = resultlevel->lls[i].x;
		int right_x = left_x + lever_w;
		int top_y = resultlevel->lls[i].y;
		int bottom_y = top_y + lever_h;
		ll_lxs[i] = left_x;
		ll_rxs[i] = right_x;
		ll_tys[i] = top_y;
		ll_bys[i] = bottom_y;
		//printf("lls entry %i extrema [ x: %i ~ %i, y: %i ~ %i ]\n", i, left_x, right_x, top_y, bottom_y);
	}
	
	// Expand the extrema further for mobile forms, accounting for however far they can move
	for (int i = 0; i < resultlevel->n_lmfs; i++) {
		int temp_left_exp = 0;
		int temp_right_exp = 0;
		int temp_up_exp = 0;
		int temp_down_exp = 0;
		int temp_x = 0;
		int temp_y = 0;
		for (int j = 0; j < resultlevel->lmfs[i].pathlen; j++) {
			if (resultlevel->lmfs[i].pathaxes[j]) {
				temp_y += resultlevel->lmfs[i].pathdeltas[j];
				if (temp_y < temp_up_exp) {
					temp_up_exp = temp_y;
				}
				if (temp_y > temp_down_exp) {
					temp_down_exp = temp_y;
				}
			} else {
				temp_x += resultlevel->lmfs[i].pathdeltas[j];
				if (temp_x < temp_left_exp) {
					temp_left_exp = temp_x;
				}
				if (temp_x > temp_right_exp) {
					temp_right_exp = temp_x;
				}
			}
		}
		for (int j = 0; j < resultlevel->lmfs[i].n_rinds; j++) {
			lr_lxs[ resultlevel->lmfs[i].rinds[j] ] += temp_left_exp;
			lr_rxs[ resultlevel->lmfs[i].rinds[j] ] += temp_right_exp;
			lr_tys[ resultlevel->lmfs[i].rinds[j] ] += temp_up_exp;
			lr_bys[ resultlevel->lmfs[i].rinds[j] ] += temp_down_exp;
		}
		for (int j = 0; j < resultlevel->lmfs[i].n_crinds; j++) {
			lcr_lxs[ resultlevel->lmfs[i].crinds[j] ] += temp_left_exp;
			lcr_rxs[ resultlevel->lmfs[i].crinds[j] ] += temp_right_exp;
			lcr_tys[ resultlevel->lmfs[i].crinds[j] ] += temp_up_exp;
			lcr_bys[ resultlevel->lmfs[i].crinds[j] ] += temp_down_exp;
		}
	}
	
	// Now, figure out the outer dimensions of the whole level overall --------------------------------------------------------------------------
	
	//Candidate data
	int left_x = lr_lxs[0];
	int right_x = left_x;
	int top_y = lr_tys[0];
	int bottom_y = top_y;
	// Check each level r
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
		lsegs[i].n_linds = 0;
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
			for (int k = 0; k < resultlevel->n_lls; k++) {
				if (ll_rxs[k] >= slx &&
					ll_lxs[k] <= srx &&
					ll_bys[k] >= sty &&
					ll_tys[k] <= sby )
				{
					level_segment_add_l_index(k, &(lsegs[seg_index]));
					//printf("Found ll %i (%i ~ %i, %i ~ %i)\n", k, lr_lxs[k], lr_rxs[k], lr_tys[k], lr_bys[k]);
					//printf("Adding index %i to segment %i lls\n", k, seg_index);
				}
			}
			
			if (lsegs[seg_index].n_rinds > fullest_seg_n)
				fullest_seg_n = lsegs[seg_index].n_rinds;
			if (lsegs[seg_index].n_crinds > fullest_seg_n)
				fullest_seg_n = lsegs[seg_index].n_crinds;
			if (lsegs[seg_index].n_linds > fullest_seg_n)
				fullest_seg_n = lsegs[seg_index].n_linds;
			
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
		printf("\nlsegs[%i]: its %i linds: ", i, lsegs[i].n_linds);
		for (int j = 0; j < lsegs[i].n_linds; j++) {
			printf("%i, ", lsegs[i].linds[j]);
		}
		printf("\n");
	}
	*/
	
	// Arrange quadrants and fill them with the indices of their proper segment quartets --------------------------------------------------------
	
	//printf("MARKER B, fullest_seg_n is %i\n\n", fullest_seg_n);
	
	for (int i = 0; i < total_quadrants; i++) {
		resultlevel->quadrants[i].n_rinds = 0;
		resultlevel->quadrants[i].n_crinds = 0;
		resultlevel->quadrants[i].n_linds = 0;
	} int lqi = 0; // Working index for resultlevel->quadrants for now
	
	int seg_index_memory_lens = fullest_seg_n*4; // (Each quadrant contains four segments, and could contain no more than 4 times the stuff in the fullest segment)
	
	int lr_seg_index_memory[seg_index_memory_lens];
	int lcr_seg_index_memory[seg_index_memory_lens];
	int ll_seg_index_memory[seg_index_memory_lens];
	
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
			for (int k = 0; k < seg_index_memory_lens; k++) {
				lr_seg_index_memory[i] = 0;
				lcr_seg_index_memory[i] = 0;
				ll_seg_index_memory[i] = 0;
			}
			int lr_sim_i = 0; // Current length of lr_seg_index_memory
			int lcr_sim_i = 0; // Current length of lcr_seg_index_memory
			int ll_sim_i = 0; // Current length of ll_seg_index_memory
			
			// Add the rinds and crinds of topleftseg to seg_index_memory --------------------------
			for (int k = 0; k < (lsegs[topleftseg].n_rinds); k++) {
				lr_seg_index_memory[lr_sim_i] = (lsegs[topleftseg].rinds[k]);
				lr_sim_i++;
			}
			for (int k = 0; k < (lsegs[topleftseg].n_crinds); k++) {
				lcr_seg_index_memory[lcr_sim_i] = (lsegs[topleftseg].crinds[k]);
				lcr_sim_i++;
			}
			for (int k = 0; k < (lsegs[topleftseg].n_linds); k++) {
				ll_seg_index_memory[ll_sim_i] = (lsegs[topleftseg].linds[k]);
				ll_sim_i++;
			}
			
			// Add any new rinds found in toprightseg ----------------------------------------------
			bool already_there;
			for (int k = 0; k < (lsegs[toprightseg].n_rinds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < lr_sim_i; l++) {
					if ( (lsegs[toprightseg].rinds[k]) == lr_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lr_seg_index_memory[lr_sim_i] = (lsegs[toprightseg].rinds[k]);
					lr_sim_i++;
				}
			}
			// Add any new crinds found in toprightseg
			for (int k = 0; k < (lsegs[toprightseg].n_crinds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < lcr_sim_i; l++) {
					if ( (lsegs[toprightseg].crinds[k]) == lcr_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lcr_seg_index_memory[lcr_sim_i] = (lsegs[toprightseg].crinds[k]);
					lcr_sim_i++;
				}
			}
			// Add any new linds found in toprightseg
			for (int k = 0; k < (lsegs[toprightseg].n_linds); k++) {
				already_there = 0; // Assume it's not until its found
				for (int l = 0; l < ll_sim_i; l++) {
					if ( (lsegs[toprightseg].linds[k]) == ll_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					ll_seg_index_memory[ll_sim_i] = (lsegs[toprightseg].linds[k]);
					ll_sim_i++;
				}
			}
			
			// Add any new rinds found in bottomleftseg --------------------------------------------
			for (int k = 0; k < (lsegs[bottomleftseg].n_rinds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < lr_sim_i; l++) {
					if ( (lsegs[bottomleftseg].rinds[k]) == lr_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lr_seg_index_memory[lr_sim_i] = (lsegs[bottomleftseg].rinds[k]);
					lr_sim_i++;
				}
			}
			// Add any new crinds found in bottomleftseg
			for (int k = 0; k < (lsegs[bottomleftseg].n_crinds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < lcr_sim_i; l++) {
					if ( (lsegs[bottomleftseg].crinds[k]) == lcr_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lcr_seg_index_memory[lcr_sim_i] = (lsegs[bottomleftseg].crinds[k]);
					lcr_sim_i++;
				}
			}
			// Add any new linds found in bottomleftseg
			for (int k = 0; k < (lsegs[bottomleftseg].n_linds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < ll_sim_i; l++) {
					if ( (lsegs[bottomleftseg].linds[k]) == lr_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					ll_seg_index_memory[ll_sim_i] = (lsegs[bottomleftseg].linds[k]);
					ll_sim_i++;
				}
			}
			
			// Add any new rinds found in bottomrightseg -------------------------------------------
			for (int k = 0; k < (lsegs[bottomrightseg].n_rinds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < lr_sim_i; l++) {
					if ( (lsegs[bottomrightseg].rinds[k]) == lr_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lr_seg_index_memory[lr_sim_i] = (lsegs[bottomrightseg].rinds[k]);
					lr_sim_i++;
				}
			}
			// Add any new crinds found in bottomrightseg
			for (int k = 0; k < (lsegs[bottomrightseg].n_crinds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < lcr_sim_i; l++) {
					if ( (lsegs[bottomrightseg].crinds[k]) == lcr_seg_index_memory[l] ) {
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					lcr_seg_index_memory[lcr_sim_i] = (lsegs[bottomrightseg].crinds[k]);
					lcr_sim_i++;
				}
			}
			// Add any new linds found in bottomrightseg
			for (int k = 0; k < (lsegs[bottomrightseg].n_linds); k++) {
				already_there = 0; // Assume it's not until its found
				for(int l = 0; l < ll_sim_i; l++) {
					if ( (lsegs[bottomrightseg].linds[k]) == ll_seg_index_memory[l] ) { // HO LEE SHIIIIIIT
						already_there = 1;
						break;
					}
				}
				if (!already_there) {
					ll_seg_index_memory[ll_sim_i] = (lsegs[bottomrightseg].linds[k]);
					ll_sim_i++;
				}
			}
			
			/*
			printf("Quadrant %i's ll_seg_index_memory[%i]: ", lqi, ll_sim_i);
			for (int k = 0; k < ll_sim_i; k++) {
				printf("%i ", ll_seg_index_memory[k]);
			} printf("\n");
			*/
			
			// Add all level geometry indexed to seg_index_memory -----------------------------------------------------------
			
			for (int k = 0; k < lr_sim_i; k++) {
				int l = lr_seg_index_memory[k];
				//printf("The %ith index of lr_seg_index_memory is %i, adding that to quadrant %i...\n", k, l, lqi);
				level_quadrant_add_r_index(l, &(resultlevel->quadrants[lqi]));
			}
			for (int k = 0; k < lcr_sim_i; k++) {
				int l = lcr_seg_index_memory[k];
				level_quadrant_add_cr_index(l, &(resultlevel->quadrants[lqi]));
			}
			for (int k = 0; k < ll_sim_i; k++) {
				int l = ll_seg_index_memory[k];
				//printf("The %ith index of ll_seg_index_memory is %i, adding that to quadrant %i...\n", k, l, lqi);
				level_quadrant_add_l_index(l, &(resultlevel->quadrants[lqi]));
			}
			lqi++;
			
			// (End of this quadrant's work)
		}
	}
	
	/*
	printf("About to return pointer resultlevel, with n_lrs %i, n_lcrs %i, n_lls %i, n_quadrants %i, lolx %i, and loty %i,\n",
		resultlevel->n_lrs, resultlevel->n_lcrs, resultlevel->n_lls, resultlevel->n_quadrants, resultlevel->lolx, resultlevel->loty);
	for (int i = 0; i < resultlevel->n_lrs; i++) {
		printf("lrs[%i]: at (%i, %i)\n", i, resultlevel->lrs[i].dest_x, resultlevel->lrs[i].dest_y);
	}
	for (int i = 0; i < resultlevel->n_lcrs; i++) {
		printf("lcrs[%i]: at (%i, %i)\n", i, resultlevel->lcrs[i].x, resultlevel->lcrs[i].y);
	}
	for (int i = 0; i < resultlevel->n_lcrs; i++) {
		printf("lls[%i]: at (%i, %i)\n", i, resultlevel->lls[i].x, resultlevel->lls[i].y);
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
	for (int i = 0; i < arglevel->n_lls; i++) {
		printf("lls[%i]: at (%i, %i)\n", i, arglevel->lls[i].x, arglevel->lls[i].y);
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
		printf("quadrant %i contains lls:\n", i);
		for (int j = 0; j < arglevel->quadrants[i].n_linds; j++) {
			printf("%i%s", arglevel->quadrants[i].linds[j], j==(arglevel->quadrants[i].n_linds-1)? "\n": ", ");
		}
	}
}

void gll_unload_level(struct level* arglevel) {
	// Free all the data now that you're done with it
	free(arglevel->lrs);
	free(arglevel->lcrs);
	free(arglevel->lls);
	
	for (int i = 0; i < arglevel->n_quadrants; i++) {
		if (arglevel->quadrants[i].n_rinds)
			free(arglevel->quadrants[i].rinds);
		if (arglevel->quadrants[i].n_crinds)
			free(arglevel->quadrants[i].crinds);
		if (arglevel->quadrants[i].n_linds)
			free(arglevel->quadrants[i].linds);
	}
	free(arglevel->quadrants);
	
	free(arglevel);
}
