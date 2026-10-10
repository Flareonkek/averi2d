
struct animal {
	// Location and sprite
	int x, y, w, h, state,
	// Which sprites correspond to which states:
	// 0: standing still, 1~2: starting run, 3~10: running, 11~15: ground turn, 16~19: aerial turn,
	// 20~23: jump/fall, 24: ledge hang, 25~29: ledge climbing
	
	// (The following are only relevant here within g_animal.c)
	vx, vy, climbing, max_run_spd, tail_state, dx_per_frame, animDX, turn_counter_v;
	bool right_face, airborne, tail_swing, turn_counter_hold, is_bunny;
};

//------------------------------------------------------------------------------------------------------------------------------------------
// FILE-SCOPE DATA
//------------------------------------------------------------------------------------------------------------------------------------------

const int animal_h = 90;
const int animal_w = 28;
const int animal_fw = 56;

struct animal averi = {
	.x=900, .y=260, .w=animal_w, .h=animal_h, .state=0,
	.vx=0, .vy=0, .climbing=0, .max_run_spd=15, .tail_state=3, .dx_per_frame=8, .animDX=0, .turn_counter_v=0,
	.right_face=false, .airborne=false, .tail_swing=false, .turn_counter_hold=false, .is_bunny=false
};
struct animal hazel = {
	.x=1300, .y=260, .w=animal_w, .h=animal_h, .state=0,
	.vx=0, .vy=0, .climbing=0, .max_run_spd=15, .tail_state=3, .dx_per_frame=8, .animDX=0, .turn_counter_v=0,
	.right_face=false, .airborne=false, .tail_swing=false, .turn_counter_hold=false, .is_bunny=true
};

//------------------------------------------------------------------------------------------------------------------------------------------
// HELPER FUNCTIONS for this file
//------------------------------------------------------------------------------------------------------------------------------------------
int approach_zero(int arg, int amt) {
	if (arg < -amt) return arg + amt;
	if (arg > amt) return arg - amt;
	return 0;
}

// Check if two coll_rects overlap each other
bool collides(struct coll_rect cr1, struct coll_rect cr2) {
	if (cr1.x + cr1.w <= cr2.x)
		return 0;
	if (cr2.x + cr2.w <= cr1.x)
		return 0;
	
	if (cr1.y + cr1.h <= cr2.y)
		return 0;
	if (cr2.y + cr2.h <= cr1.y)
		return 0;
	
	return 1;
}

// Check if a coll_rect collides with any in an array of coll_rects
bool any_collides(struct coll_rect cr1, struct coll_rect* crs, int crlen) {
	for (int i = 0; i < crlen; i++) {
		struct coll_rect cr = crs[i];
		if (collides(cr1, cr))
			return 1;
	}
	//printf("No collisions detected for cr {%i, %i, %i, %i}\n", cr1.x, cr1.y, cr1.w, cr1.h);
	return 0;
}

// Would averi's current y-velocity cause an overlap with cr?
bool animalVyColl(struct animal* arga, struct coll_rect cr) {
	if ((arga->y+arga->vy) + arga->h <= cr.y)
		return 0;
	if (cr.y + cr.h <= (arga->y+arga->vy))
		return 0;
	
	if (arga->x + arga->w <= cr.x)
		return 0;
	if (cr.x + cr.w <= arga->x - arga->w)
		return 0;
	return 1;
}

// Would averi's current x-velocity cause an overlap with cr?
bool animalVxColl(struct animal* arga, struct coll_rect cr) {
	if ((arga->x+arga->vx) + arga->w <= cr.x)
		return 0;
	if (cr.x + cr.w <= (arga->x+arga->vx) - arga->w)
		return 0;
	
	if (arga->y + arga->h <= cr.y)
		return 0;
	if (cr.y + cr.h <= arga->y)
		return 0;
	return 1;
}

// Would averi's current compound velocity cause an overlap with cr?
bool animalVxyColl(struct animal* arga, struct coll_rect cr) {
	if ((arga->x+arga->vx) + arga->w <= cr.x)
		return 0;
	if (cr.x + cr.w <= (arga->x+arga->vx) - arga->w)
		return 0;
	
	if ((arga->y+arga->vy) + arga->h <= cr.y)
		return 0;
	if (cr.y + cr.h <= (arga->y+arga->vy))
		return 0;
	return 1;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Averi's physics and animations
//------------------------------------------------------------------------------------------------------------------------------------------
static void animal_tick(struct animal* a, unsigned short k, unsigned short k_held, struct coll_rect* crs, int* crlen) {
	
	bool do_sound = !a->is_bunny;
	
	extern int frameNo;
	
	if (a->climbing) {
		// a->climbing of 1 means she's hanging on a ledge, 0 means she isn't.
		// Values greater than 1 mean she's progressing through steps of pulling herself up and jumping over the ledge
		struct coll_rect jump_from_ledge_cr = {a->x-a->w, a->y-50, a->w*2, a->h};
		if (a->climbing > 1) {
			a->climbing++;
			if (a->climbing == 5)
				a->y -= 12; // Going into a->state 26
			else if (a->climbing == 7)
				a->y -= 11; // Going into state 27
			else if (a->climbing == 9)
				a->y -= 10; // Going into state 28
			else if (a->climbing == 11)
				a->y -= 9; // Going into state 29
			else if (a->climbing == 13) {
				a->y -= 8; // Going into normal aerial state
				a->vy = -16; // Rising with slightly less velocity than a normal jump
				a->climbing = 0;
				a->airborne = 1;
				if (do_sound) add_sound(AUDIO_JUMP_A, sounds, &slen, sounds_size);
			}
		} else if ( // While hanging (a->climbing == 1), player can press Space or W to jump up...
			( (keyWdown(k) && !keyWdown(k_held))||(keySPACEdown(k) && !keySPACEdown(k_held)) ) && !any_collides(jump_from_ledge_cr, crs, *crlen) ) {
			a->climbing = 2;
			a->y -= 13; // Going into state 25
		} else if (keySdown(k)) // ...or S to let go of the ledge
			a->climbing = 0;
	} else {
		// Influence Averi's velocity according to keyboard input ---------------------------------------------------------------------
		if (keyAdown(k) && !keyDdown(k)) {
			// Try to accelerate left (on 2 out of every 3 frames, or every frame if you're a bunny)
			if (a->vx > -a->max_run_spd && (frameNo % 3 || a->is_bunny))
				a->vx -= 1;
			a->right_face = 0;
		}
		else if (keyDdown(k) && !keyAdown(k)) {
			// Try to accelerate right (on 2/3 frames or every frame if IAB)
			if (a->vx < a->max_run_spd && (frameNo % 3 || a->is_bunny))
				a->vx += 1;
			a->right_face = 1;
		}
		else if ((frameNo % 3)) a->vx = approach_zero(a->vx, 1); // Slow down cause we're not trying to go anywhere
		
		if (a->airborne) // If in the air, enact gravity onto velocity
			a->vy += (keySPACEdown(k)? 1: 3); // (3x faster if space is released)
		else if (keySPACEdown(k) && !keySPACEdown(k_held)) { // Otherwise, jumping is possible
			a->vy = a->is_bunny? -28: -20; // (Bunnies jump higher)
			if (do_sound) add_sound(AUDIO_JUMP_A, sounds, &slen, sounds_size);
		}
			
		if (a->vy > 60) a->vy = 60; // Terminal velocity is 60
		
		// Apply velocity -------------------------------------------------------------------------------------------------------------
		//printf("x %i, vx %i, y %i, vy %i, airborne %b", a->x, a->vx, a->y, a->vy, a->airborne);
		// Collision check with every cr
		for (int i = 0; i < *crlen; i++) {
			struct coll_rect cr = crs[i]; // For each collision rectangle,
			if (animalVyColl(a, cr)) { // If Averi's Y velocity alone would result in collision:
				// Reduce Vy accordingly
				if (a->vy > 0) {
					int potential_new_vy = cr.y - (a->y+a->h);
					if (potential_new_vy < a->vy)
						a->vy = potential_new_vy;
				} else if (a->vy < 0) {
					int potential_new_vy = (cr.y+cr.h) - a->y;
					if (potential_new_vy > a->vy)
						a->vy = potential_new_vy;
				}
			}
			if (animalVxColl(a, cr)) { // If her X velocity alone would result in collision:
				// Reduce Vx accordingly
				if (a->vx > 0) {
					int potential_new_vx = cr.x - (a->x+a->w);
					if (potential_new_vx < a->vx)
						a->vx = potential_new_vx;
				} else if (a->vx < 0) {
					int potential_new_vx = (cr.x+cr.w) - (a->x-a->w);
					if (potential_new_vx > a->vx)
						a->vx = potential_new_vx;
				}
				// LEDGE GRAB CHECK
				struct coll_rect ledge_cr = {(a->x+a->vx)-a->w, cr.y-27, a->w*2, a->h};
				if ( (cr.y>a->y) && // The ledge is below the top of averi's head,
					 (cr.y<a->y+(a->h/2)) && // but above her waist, and...
					 a->airborne &&
					 (a->vy >= 0) && // She's not still going up (since that may suffice to clear the obstacle without a grab, and the grab then would annoy the player)
					 !any_collides(ledge_cr, crs, *crlen)
					 && !keySdown(k) )
				{
					//printf("\nLedge grab initiates, x %i, vx %i, y %i, vy %i, airborne %b", a->x, a->vx, a->y, a->vy, a->airborne);
					// Ensure she's facing properly for the ledge grab state
					a->right_face = (cr.x > a->x);
					// Enter the ledge grab state
					a->vy = 0;
					a->climbing = 1;
					a->y = cr.y - 27;
					a->x += a->vx;
					a->vx = 0;
				}
			}
			if (animalVxyColl(a, cr)) { // If, after accounting for each axis alone, her remaining velocity would result in collision:
				// Reduce Vy however much is needed until its resolved
				if (a->vy > 0) {
					int potential_new_vy = cr.y - (a->y+a->h);
					if (potential_new_vy < a->vy)
						a->vy = potential_new_vy;
				} else if (a->vy < 0) {
					int potential_new_vy = (cr.y+cr.h) - a->y;
					if (potential_new_vy > a->vy)
						a->vy = potential_new_vy;
				}
			}
		}
		//printf(" POST COLLISION CHECK: x %i, vx %i, y %i, vy %i, airborne %b\n", a->x, a->vx, a->y, a->vy, a->airborne);
		
		// Enact velocity onto position
		a->x += a->vx;
		a->animDX += a->vx;
		a->y += a->vy; // Fall
		
		// Determine if she's standing on anything
		a->airborne = true; // Unless we find her to be standing on something, assume she's in the air
		for (int i = 0; i < *crlen; i++) { // Now check every collision rectangle...
			struct coll_rect cr = crs[i];
			if (a->y+a->h == cr.y && ((a->x-a->w < cr.x+cr.w) && (a->x+a->w > cr.x)) ) {
				// If she's standing on any cr, then she isn't in the air
				a->airborne = false;
				// If she's standing still, and sticking out over the edge, and there's room down on the side, go into a ledge hang there
				if (!a->vx) {
					struct coll_rect hang_right_cr = {(cr.x+cr.w), cr.y-27, a->w*2, a->h};
					struct coll_rect hang_left_cr = {cr.x-(a->w*2), cr.y-27, a->w*2, a->h};
					
					if (a->x > (cr.x+cr.w) && !any_collides(hang_right_cr, crs,  *crlen)) {
						a->x = (cr.x+cr.w)+a->w;
						a->vy = 0;
						a->climbing = 1;
						a->y = cr.y - 27;
						a->right_face = 0;
					} else if (a->x < cr.x && !any_collides(hang_left_cr, crs,  *crlen)) {
						a->x = cr.x - a->w;
						a->vy = 0;
						a->climbing = 1;
						a->y = cr.y - 27;
						a->right_face = 1;
					}
				}
			}
		}
	}
	
	// Averi animations ---------------------------------------------------------------------------------------------------------------
	
	if (a->climbing) {
		// Ledge hanging/climbing animations --------------------------------------------------------------------------------
		if (a->climbing < 2) a->state = 24;
		else if (a->climbing < 5) a->state = 25;
		else if (a->climbing < 7) a->state = 26;
		else if (a->climbing < 9) a->state = 27;
		else if (a->climbing < 11) a->state = 28;
		else a->state = 29;
	} else if (a->airborne) {
		// Jumping/falling animations ---------------------------------------------------------------------------------------
		if ((a->right_face && a->vx < 0) || (!a->right_face && a->vx > 0)) {
			// (Turning around in midair)
			if (!a->turn_counter_hold) // When you first begin a turnaround, set a->turn_counter_v
				a->turn_counter_v = abs(a->vx);
			// During the turnaround, animation goes by fraction of a->turn_counter_v decelerated under
			if (abs(a->vx) < a->turn_counter_v/4)
				a->state = 19;
			else if (abs(a->vx) < a->turn_counter_v*2/4)
				a->state = 18;
			else if (abs(a->vx) < a->turn_counter_v*3/4)
				a->state = 17;
			else //(abs(a->vx) < a->turn_counter_v*4/4)
				a->state = 16;
		} else {
			// (Not turning around in midair)
			if (a->vy < -2)
				a->state = 20; // Rising
			else if (a->vy < 2)
				a->state = 21; // Peak
			else if (a->vy < 6)
				a->state = 22; // Descent
			else
				a->state = 23; // Fast descent
		}
	} else if ((a->right_face && a->vx < 0) || (!a->right_face && a->vx > 0)) {
		// Sliding and turning around on the ground -------------------------------------------------------------------------
		if (!a->turn_counter_hold) // When you first begin a turnaround, set a->turn_counter_v
			a->turn_counter_v = abs(a->vx);
		// During the turnaround, animation goes by fraction of a->turn_counter_v decelerated under
		if (abs(a->vx) < a->turn_counter_v/5)
			a->state = 15;
		else if (abs(a->vx) < a->turn_counter_v*2/5)
			a->state = 14;
		else if (abs(a->vx) < a->turn_counter_v*3/5)
			a->state = 13;
		else if (abs(a->vx) < a->turn_counter_v*4/5)
			a->state = 12;
		else //(abs(a->vx) < a->turn_counter_v*5/5)
			a->state = 11;
		
		// When you're about to have slid to a halt...
		if (a->vx == 1 || a->vx == -1) {
			a->vx = (a->vx == 1? -1 : 1); // Skip over 0-velocity (Or she would snap into stand-still frame)
			a->state = 3; // Skip walk frames (The skid animation aligns with the run frames)
			a->animDX = 0;
		}
	} else {
		// General walking/running/standing ---------------------------------------------------------------------------------
		if (a->animDX < -a->dx_per_frame || a->animDX > a->dx_per_frame) {
			// Every time you moved a->dx_per_frame pixels, advance the animation frame and update a->animDX
			a->animDX = approach_zero(a->animDX, a->dx_per_frame);
			a->state += 1;
		}
		// If we reach the end of the run cycle, go back to its start
		if (a->state > 10) a->state = 3;
		
		// If she isn't moving, go back to stand-still sprite and reset a->animDX
		if (a->vx == 0) {
			a->state = 0;
			a->animDX = 0;
		} else if (a->state == 0) // But if she is moving even a bit and yet in the stand-still sprite,
			a->state = 1; // then don't wait for a->animDX, kickstart the walk animation immediately
			
		if (do_sound && (a->state==3 || a->state==6)) {
			add_sound(AUDIO_STEP, sounds, &slen, sounds_size);
		}
	}
	
	// Tail swinging animation ----------------------------------------------------------------------------------------------
	
	if (a->state)
		a->tail_state = 3; // Return tail to neutral position when she moves (As then it's not swinging)
	else if (!(frameNo % 7)) {
		if (a->tail_swing)
			a->tail_state -= 1;
		else
			a->tail_state += 1;
		if (a->tail_state == 6 || a->tail_state == 0)
			a->tail_swing = !a->tail_swing; // Switch direction when tail reaches the end of its swing
	}
	
	// Hold checks (must be done last, after all logic that uses them) ------------------------------------------------------

	a->turn_counter_hold = (a->right_face && a->vx < 0) || (!a->right_face && a->vx > 0);
}
