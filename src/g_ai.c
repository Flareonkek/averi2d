
// Averi's X and Y, or if she is out of sight: her last known whereabouts
static int ai_avoid_x = 0;
static int ai_avoid_y = 0;

static int ai_paranoia = 0; // Determines whether Hazel will consider running from Averi
static int ai_fear = 0; // Determines whether Hazel will actively run from Averi

// Can animal a1 see animal a2 (or is her line-of-sight blocked by a solid object?)
bool sightline(struct animal* a1, struct animal* a2, struct coll_rect* crs, int crlen) {
	
	// (First ensure a1 is to the left, and a2 to the right, by swapping them if needed)
	if (a1->x > a2->x) {
		struct animal* temp = a1;
		a1 = a2;
		a2 = temp;
	}
	
	if (a1->x == a2->x) { // (a1 and a2 are at the same X-position)
		// First ensure that a1 is above, and a2 below
		if (a1->y > a2->y) {
			struct animal* temp = a1;
			a1 = a2;
			a2=temp;
		}
		// Now check all crs like so
		for (int i = 0; i < crlen; i++) {
			struct coll_rect cr = crs[i];
			if ((cr.y > a1->y+a1->h && cr.y+cr.h < a2->y) && (cr.x+cr.w > a1->x && cr.x < a1->x)) {
				//printf("Sightline blocked (Y) by CR{%i,%i,%i,%i}\n", cr.x, cr.y, cr.w, cr.h);
				//printf("(%i > %i && %i < %i)\n", cr.y, a1->y+a1->h, cr.y+cr.h, a2->y);
				return false;
			}
		}
		return true;
	}
	
	if ((a1->y) == (a2->y)) { // If they are at the same height ------------------------------------------------------------------
		// Check for crs between them like so
		for (int i = 0; i < crlen; i++) {
			struct coll_rect cr = crs[i];
			if (cr.x+cr.w > (a1->x) && cr.x < (a2->x) &&
				cr.y < (a1->y) && cr.y+cr.h > (a1->y)
			) {
				//printf("Sightline blocked (A) by CR{%i,%i,%i,%i}\n", cr.x, cr.y, cr.w, cr.h);
				return false; // The cr blocks the sightline
			}
		}
		//printf("Sightline clear (A)\n");
		
	} else if ((a1->y) < (a2->y)) { // If a2 is lower than a1 --------------------------------------------------------------------
		// Calculate linear SlopeX+B formula for the line-of-sight
		float slope = ((a2->y) - (a1->y)) / (float)((a2->x) - (a1->x));
		int b = (a1->y);
		// Check all the crs...
		for (int i = 0; i < crlen; i++) {
			struct coll_rect cr = crs[i];
			// If it is possible for cr to be blocking the line-of-sight
			if (cr.x+cr.w > (a1->x) && cr.x < (a2->x)) {
				// (Firstly, trim cr so it can't collide with the extended sightlines beyond that stretch between a1 and a2)
				if (cr.x < (a1->x)) {
					cr.w = (cr.x+cr.w)-(a1->x);
					cr.x = (a1->x);
				}
				if (cr.x+cr.w > (a2->x)) {
					cr.w = (a2->x)-cr.x;
				}
				// Check if cr's bottom-left-corner is beneath whilst its top-right corner is above the line-of-sight
				if (cr.y+cr.h > (slope*(cr.x-a1->x))+b && cr.y < (slope*(cr.x+cr.w-a1->x))+b) {
					//printf("Sightline blocked (B) by CR{%i,%i,%i,%i} (treated like {%i,%i,%i,%i})\tf=%fX+%i\n", crs[i].x, cr.y, crs[i].w, cr.h, cr.x, cr.y, cr.w, cr.h, slope, b);
					//printf("a1: (%i,%i) a2: (%i,%i)\n", a1->x, a1->y, a2->x, a2->y);
					return false;
				}
			}
		}
		//printf("Sightline clear (B)\tf=%fX+%i\n", slope, b);
		
	} else { // ( (a1->y) > (a2->y) ) a2 is higher than a1 -----------------------------------------------------------------------
		// Calculate linear SlopeX+B formula for the line-of-sight
		float slope = ((a2->y) - (a1->y)) / (float)((a2->x) - (a1->x));
		int b = (a1->y);
		// Check all the crs...
		for (int i = 0; i < crlen; i++) {
			struct coll_rect cr = crs[i];
			// If it is possible for cr to be blocking the line-of-sight
			if (cr.x+cr.w > (a1->x) && cr.x < (a2->x)) {
				// (Firstly, trim cr so it can't collide with the extended sightlines beyond that stretch between a1 and a2)
				if (cr.x < (a1->x)) {
					cr.w = (cr.x+cr.w)-(a1->x);
					cr.x = (a1->x);
				}
				if (cr.x+cr.w > (a2->x)) {
					cr.w = (a2->x)-cr.x;
				}
				// Check if cr's top-left-corner is above whilst its bottom-right corner is beneath the line-of-sight
				if (cr.y < (slope*(cr.x-a1->x))+b && cr.y+cr.h > (slope*(cr.x+cr.w-a1->x))+b) {
					//printf("Sightline blocked (C) by CR{%i,%i,%i,%i} (treated like {%i,%i,%i,%i})\tf=%fX+%i\n", crs[i].x, cr.y, crs[i].w, cr.h, cr.x, cr.y, cr.w, cr.h, slope, b);
					//printf("a1: (%i,%i) a2: (%i,%i)\n", a1->x, a1->y, a2->x, a2->y);
					return false;
				}
			}
		}
		//printf("Sightline clear (C)\tf=%fX+%i\n", slope, b);
	}
	return true;
}


// These arrays might be useful if I were smart enough to make the AI think and manoeuvre around well
int jumpgains_fox[20] = {
	-20, -39, -57, -74, -90, -105, -119, -132, -144, -155, -165, -174, -182, -189,
	-195, -200, -204, -207, -209, -210
};
int jumpgains_bunny[28] = {
	-28, -55, -81, -106, -130, -153, -175, -196, -216, -235, -253, -270, -286, -301,
	-315, -328, -340, -351, -361, -370, -378, -385, -391, -396, -400, -403, -405, -406
};

void ai_tick(unsigned short* argk, struct coll_rect* crs, int* crlen) {
	
	extern int frameNo;
	// This is initialized in here cause I prioritise making game.c more concise
	*argk = 0;
	
	/* Hazel is controlled similarly as the players character, as if by a keyboard,
	 * Use each bit of argk to represent a certain key as follows:
	 * 0  1  2  3  4  5     6  7    8    9     10    11    12        13   14   15  16
	 * W  A  S  D  G  Space Up Left Down Right Shift Ctrl  Del/Back  Tab  Esc  F6  -
	 */
	
	if (ai_paranoia > 0) {
		
		if (abs(averi.x-hazel.x) < 800 && abs(averi.y-hazel.y) < 500 // (Hazel can only see averi if she's within in this range)
			&& sightline(&hazel, &averi, crs, *crlen) ) // (Hazel needs a direct line of sight to see Averi)
		{
			ai_avoid_x = averi.x;
			ai_avoid_y = averi.y;
			ai_paranoia = 500;
			ai_fear = 50;
		} else { // (Hazel cannot see Averi, and so imagines she is no longer chasing her)
			ai_paranoia--;
			if (ai_fear > 0) ai_fear--;
		}
		
	}
	
	if (!(frameNo % 15)) {
		printf("NPC Hazel %s see you. ", abs(averi.x-hazel.x) < 800 && abs(averi.y-hazel.y) < 400 && sightline(&hazel, &averi, crs, *crlen) ? "CAN":"CANNOT");
		printf("Paranoia: %i, Fear: %i\n", ai_paranoia, ai_fear);
	}
	
	if (ai_fear) {
		*argk += k_powers_of_2[5]; // SPACE
		if (ai_avoid_x > hazel.x || (ai_avoid_x==hazel.x&&hazel.right_face))
			*argk += k_powers_of_2[1]; // A
		else
			*argk += k_powers_of_2[3]; // D
		
	} else if (abs(averi.x-hazel.x) < 400 && sightline(&hazel, &averi, crs, *crlen) && !hazel.climbing) {
		// Turn and face Averi if she's nearby and you're just standing there
		hazel.right_face = averi.x > hazel.x;
	}
}
