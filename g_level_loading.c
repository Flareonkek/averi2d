#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "render.h"

const int seg_w = ideal_w;
const int seg_h = ideal_h;

// A little helper function used in this file

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

// ----------------------------------------------------------------------------------------------------------------------------------
// VECTOR LIST: used to store an ortho_form -----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

struct vectlist {
	// Where the first vector starts from
	int x;
	int y;
	
	int tile_type;
	
	// Vector list
	int len;
	int* vectmags; // Vector lengths
	bool* vectdirs; // Vector directions
	
	// Overall rectangle that would contain the shape
	int olx; // Outline left x
	int orx; // Outline right x
	int oty; // Outline top y
	int oby; // Outline bottom y
};

void vectlist_addseg(int argmag, bool argdir, struct vectlist* arglist) {
	// Allocate new arrays 1 thing longer than the originals
	int* newvectmags = malloc( sizeof(int)*(arglist->len + 1) );
	bool* newvectdirs = malloc( sizeof(bool)*(arglist->len + 1) );
	
	if (newvectmags == NULL || newvectdirs == NULL) {
		printf("Error, unable to allocate vectlist array(s)\n");
		exit(1);
	}
	
	// Set the last things, the new ones, of the new arrays, to our arguments
	newvectmags[arglist->len] = argmag;
	newvectdirs[arglist->len] = argdir;
	if (arglist->len > 0) {
		// Copy each thing from the original into the new array
		for (int i = 0; i < arglist->len; i++) {
			newvectmags[i] = (arglist->vectmags)[i];
			newvectdirs[i] = (arglist->vectdirs)[i];
		}
	// Free up the old arrays (if there were any) and point arglist to the new ones
	free(arglist->vectmags);
	free(arglist->vectdirs);
	}
	arglist->vectmags = newvectmags;
	arglist->vectdirs = newvectdirs;
	arglist->len++;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// LEVEL SEGMENT: used to by the game to load in a limited part of the level at a time ----------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

struct level_segment {
	int left_x;
	int top_y;
	// Indices of features of the level contained within this segment
	int len;
	int* data;
};

void level_segment_addindex(int argind, struct level_segment* argseg) {
	// Allocate a new array 1 int longer than the original
	int* newdata = malloc( sizeof(int)*(argseg->len + 1) );
	// Set the last int, the new one, of the new array, to argint
	newdata[argseg->len] = argind;
	if (argseg->len > 0) {
		// Copy each int from the original into the new array
		for (int i = 0; i < argseg->len; i++) {
			newdata[i] = (argseg->data)[i];
		}
	// Free up the old array and point the arglist to the new one
	free(argseg->data);
	}
	(*argseg).data = newdata;
	argseg->len++;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// LEVEL: contains all the stuff in the environment, with facilities for segmented loading ------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------

struct level {
	int n_features;
	struct vectlist* features;
	int n_segments_per_row;
	int n_segment_rows;
	struct level_segment* segments;
	int lolx; // Level outline left x
	int loty; // Level outline top y
};

struct level* load_level(const char* filename) {
	
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
	
	// Scan in entries ----------------------------------------------------------------------------------------------------------
	struct vectlist entries[n_entries];
	
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
								vectlist_addseg(tempmag, 1, &(entries[current_entry]));
							else if (c == 'R')
								vectlist_addseg(tempmag, 0, &(entries[current_entry]));
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
	
	// ------------------------------------------------------------------------------------------------------------------------------------------
	// Process the data for segmented loading ---------------------------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------------------------------------------------------------------
	
	/* Show that the scanned-in data in entries[] is correct
	for ( int i = 0; i < n_entries; i++ ) {
		printf("A @ (%i, %i) tile %i:\n", entries[i].x, entries[i].y, entries[i].tile_type);
		for ( int j = 0; j < entries[i].len; j++) {
			printf(" %i%c", entries[i].vectmags[j], entries[i].vectdirs[j]?'L':'R');
		}
		printf("\n\n");
	}
	*/
	
	// Determine and fill in the correct outline: olx (outline left x), orx, oty (outline top y,) and oby, for each entry -----------------------
	
	for ( int i = 0; i < n_entries; i++) {
		// Candidate data
		int left_x = entries[i].x;
		int right_x = left_x;
		int top_y = entries[i].y;
		int bottom_y = top_y;
		// Perimeter walking data
		int x = left_x;
		int y = top_y;
		bool vertical = 0;
		bool negative = 0;
		// Walk the perimeter of the entry
		for ( int j = 0; j < entries[i].len; j++ ) {
			int this_len = entries[i].vectmags[j];
			bool this_dir = entries[i].vectdirs[j];
			// Update position
			if (vertical) {
				if (negative) y -= this_len;
				else y += this_len;
			} else { //...it's horizontal
				if (negative) x -= this_len;
				else x += this_len;
			}
			// Set these for the next vertex:
			negative = ( vertical == negative? (this_dir) : !(this_dir) );
			vertical = !vertical;
			// Change these as better candidates are found
			if (x < left_x)   left_x   = x;
			if (x > right_x)  right_x  = x;
			if (y < top_y)    top_y    = y;
			if (y > bottom_y) bottom_y = y;
		}
		// Now we have the most extreme coordinates, and can do the needful for this entry
		entries[i].olx = left_x;
		entries[i].orx = right_x;
		entries[i].oty = top_y;
		entries[i].oby = bottom_y;
		//printf("Entry %i extrema [ x: %i ~ %i, y: %i ~ %i ]\n", i, left_x, right_x, top_y, bottom_y);
	}
	
	// Now, figure out the outer dimensions of the whole level overall --------------------------------------------------------------------------
	
	//Candidate data
	int left_x = entries[0].olx;
	int right_x = left_x;
	int top_y = entries[0].oty;
	int bottom_y = top_y;
	// Check each entry
	for (int i = 0; i < n_entries; i++) {
		struct vectlist this = entries[i];
		// Change these as better candidates are found
		if (this.olx < left_x)   left_x   = this.olx;
		if (this.orx > right_x)  right_x  = this.orx;
		if (this.oty < top_y)    top_y    = this.oty;
		if (this.oby > bottom_y) bottom_y = this.oby;
	}
	
	//printf("\nWhole level extrema [ x: %i ~ %i, y: %i ~ %i ]\n\n", left_x, right_x, top_y, bottom_y);
	
	// Determine the rows and columns and number of segments needed to contain the level --------------------------------------------------------
	
	// These ought to be floor-divisions, idk if they really are but an extra segment wouldnt hurt anything anyway
	int segcols = 1 + (right_x - left_x) / seg_w;
	int segrows = 1 + (bottom_y - top_y) / seg_h;
	int total_segments = segrows * segcols;
	
	struct level_segment lsegs[total_segments];
	// Zero the .len fields so we can segment_addindex() to these without any problems
	for (int i = 0; i < total_segments; i++)
		lsegs[i].len = 0;
	
	//printf("Seg dimensions (%i, %i)\n%i seg columns, %i seg rows\n%i total segs\n\n", seg_w, seg_h, segcols, segrows, total_segments);

	// Arrange segments on each axis and point them to the objects they encompass ---------------------------------------------------------------
	
	int seg_index = 0;
	for (int j = 0; j < segrows; j++) {
		for (int i = 0; i < segcols; i++) {
			int slx = i * seg_w + left_x; // Segment left x
			int srx = slx + seg_w;        // Segment right x
			int sty = j * seg_h + top_y;  // Segment top y
			int sby = sty + seg_h;        // Segment bottom y
			//printf("SCANNING FOR FEATURES IN SEGMENT %i, AT (%i ~ %i, %i ~ %i)\n", seg_index, slx, srx, sty, sby);
			// Check every entry as to whether or not it can be seen inside this segment
			for (int k = 0; k < n_entries; k++) {
				//printf("Checking feature %i (%i ~ %i, %i ~ %i)\n", k, entries[k].olx, entries[k].orx, entries[k].oty, entries[k].oby);
				if (entries[k].orx >= slx &&
					entries[k].olx <= srx &&
					entries[k].oby >= sty &&
					entries[k].oty <= sby )
				{
					// If it can, make the segment point to it
					level_segment_addindex(k, &(lsegs[seg_index]));
					//printf("Found feature %i (%i ~ %i, %i ~ %i)\n", k, entries[k].olx, entries[k].orx, entries[k].oty, entries[k].oby);
					//printf("Adding index %i to segment %i\n", k, seg_index);
				} //else printf("Passing up feature %i (%i ~ %i, %i ~ %i)\n", k, entries[k].olx, entries[k].orx, entries[k].oty, entries[k].oby);
					
			}
			seg_index++;
		}
	}
	printf("\n\n");
	
	// Put the level on the heap and return a pointer to it -------------------------------------------------------------------------------------
	
	struct level* resultlevel = malloc( sizeof(struct level) );
	
	resultlevel-> n_features = n_entries;
	//resultlevel-> features = entries; // Nope, that'll get trashed when it goes out of scope
	resultlevel-> features = malloc( sizeof(struct vectlist) * n_entries );
	for (int i = 0; i < n_entries; i++)
		resultlevel-> features[i] = entries[i];
	resultlevel-> n_segments_per_row = segcols;
	resultlevel-> n_segment_rows = segrows;
	//resultlevel-> segments = lsegs; // Same deal with this
	resultlevel-> segments = malloc( sizeof(struct level_segment) * total_segments );
	for (int i = 0; i < total_segments; i++)
		resultlevel-> segments[i] = lsegs[i];
	resultlevel-> lolx = left_x;
	resultlevel-> loty = top_y;
	
	return resultlevel;
}

void printdemo_level(struct level* arglevel) {
	for ( int i = 0; i < arglevel->n_features; i++ ) {
		printf("Feature %i:\n", i);
		printf("A @ (%i, %i) tile %i:\n", arglevel->features[i].x, arglevel->features[i].y, arglevel->features[i].tile_type);
		for ( int j = 0; j < arglevel->features[i].len; j++) {
			printf(" %i%c", arglevel->features[i].vectmags[j], arglevel->features[i].vectdirs[j]?'L':'R');
		}
		
		printf("\n\n");
	}
	
	int n_segs = arglevel->n_segments_per_row * arglevel->n_segment_rows;
	printf("\n%i SEGMENTS, (In %i rows of %i each):\n", n_segs, arglevel->n_segment_rows, arglevel->n_segments_per_row);
	for (int i = 0; i < n_segs; i++) {
		printf("[");
		for (int j = 0; j < arglevel->segments[i].len; j++) {
			printf("%i ", arglevel->segments[i].data[j]);
		}
		printf("]%c", (((i+1)%arglevel->n_segments_per_row)?' ':'\n') );
	}
	printf("\n");
	
}

void unload_level(struct level* arglevel) {
	// Free all the data now that you're done with it
	for ( int i = 0; i < arglevel->n_features; i++ ) {
		free(arglevel->features[i].vectmags);
		free(arglevel->features[i].vectdirs);
	}
	free(arglevel->features);
	
	for ( int i = 0; i < (arglevel->n_segments_per_row*arglevel->n_segment_rows); i++ ) {
		//printf("%sttempting to free the segments[%i].data (starts with %i)\n",(arglevel->segments[i].len?"A":"Not a") , i, arglevel->segments[i].data[0]);
		if (arglevel->segments[i].len)
			free(arglevel->segments[i].data);
	}
	free(arglevel->segments);
	free(arglevel);
}
