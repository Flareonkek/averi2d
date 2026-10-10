
// Color-signal states
// (For blue, yellow, red, violet, indigo, green, turqoise, and pink, respectively)
bool colorstates[8] = {false, false, false, false, false, false, false, false};

int nibble_timer = 0; // Used for the animation where Averi bites Hazel's ear

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
	
	// Level levers
	int n_lls;
	struct lever* lls;
	
	// Level quadrants
	int n_quadrants;
	int quad_row_length;
	struct level_quadrant* quadrants;
	
	int lolx; // Level outline left x
	int loty; // Level outline top y
	int loby; // Level outline bottom y
};

struct level_quadrant {
	// Indices of features of the level contained within this quadrant
	int n_rinds;
	int* rinds;
	
	int n_crinds;
	int* crinds;
	
	int n_linds;
	int* linds;
};
