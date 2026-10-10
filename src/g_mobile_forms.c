
struct mobile_form {
	// Starting position coordinates
	int start_x;
	int start_y;
	// The coordinates of each waypoint along the mobile form's path
	int pathlen;
	bool* pathaxes;
	int* pathdeltas;
	// Color-mechanism that controls it, or -1 if not controlled
	int color;
	// The current step in the motion cycle the form is on, or 0 if inactive
	int mf_state;
	int x;
	int y;
	// Indices of the form's visual and collision data
	int n_rinds;
	int* rinds;
	int n_crinds;
	int* crinds;
};

bool any_collides_mf (
	struct coll_rect cr1, // The coll_rect to check
	struct level* arglevel,
	struct level_quadrant* argcq )
{
	for (int h = 0; h < argcq->n_crinds; h++) {
		int i = argcq->crinds[h];
		struct coll_rect cr = arglevel->lcrs[i];
		if (collides(cr1, cr))
			return 1;
	}
	return 0;
}

bool any_collides_mf_besides (
	struct coll_rect cr1,
	struct level* arglevel,
	struct level_quadrant* argcq,
	int argmf_index )
{
	int ignored_crind_ii = arglevel->lmfs[argmf_index].crinds[0];
	//int ignored_crind_if = arglevel->lmfs[argmf_index].crinds[arglevel->lmfs[argmf_index].n_crinds];// NOPE!!!!
	int ignored_crind_if = ignored_crind_ii+arglevel->lmfs[argmf_index].n_crinds;
	//printf("Checking any_collides_mf_besides %i~%i...\n", ignored_crind_ii, ignored_crind_if);
	for (int h = 0; h < argcq->n_crinds; h++) {
		int i = argcq->crinds[h];
		if (i < ignored_crind_ii || i >= ignored_crind_if) {
			struct coll_rect cr = arglevel->lcrs[i];
			if (collides(cr1, cr)) {
				//printf("ACK! cr1:[%i,%i,%i,%i] with lcrs[%i]:[%i,%i,%i,%i]\n", cr1.x,cr1.y,cr1.w,cr1.h, i, cr.x,cr.y,cr.w,cr.h);
				return true;
			}
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Mobile form tick - Called for each mobile form, each tick -----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------

/*
	void mobile_form_tick(
		struct mobile_form* argmf, // The mobile form being ticked
		struct r* arglrs, struct coll_rect* arglcrs, int arg_n_lcrs, // The whole level's rs, crs, and number of crs
		int* argqcrinds, int arg_n_qcrinds, // The current quadrant's cr indices
		bool* colorstates)
*/
void mobile_forms_tick (struct level* the_level, struct level_quadrant cq) {
	// (This flag is so if she's standing on two coll_rects, like, they're right next to eachother, it won't go trying to push her twice)
	bool push_type1_flag_averi = false;
	bool push_type1_flag_hazel = false;
	for (int w = 0; w < the_level->n_lmfs; w++)
	{
		struct mobile_form* argmf = &(the_level->lmfs[w]);
		struct coll_rect* arglcrs = the_level->lcrs;
		int arg_n_lcrs = the_level->n_lcrs;
		int* argqcrinds = cq.crinds;
		int arg_n_qcrinds = cq.n_crinds;
		
		int spd = 5;
		int mf_goal_x, mf_goal_y;
		
		// Initial determination of mf_state ------------------------------------------------------------------------------------------------------------------------
		// (mf_state of 0 means idle, at origin. Positive numbers mean going to next path-point, while negative mean going to previous path point, of the number)
		
		bool do_sound = true;
		if (argmf->color != -1) { // If argmf is controlled by a color mechanism,
			// Its state depends on that color's state
			if (colorstates[argmf->color]) {
				// Kick it on if its just off
				if (argmf->mf_state == 0)
					argmf->mf_state = 1;
				// Otherwise make its state positive
				if (argmf->mf_state < 0)
					argmf->mf_state = -argmf->mf_state;
			} else {
				// Make its state negative
				if (argmf->mf_state > 0)
					argmf->mf_state = -argmf->mf_state;
			}
		} else { // If argmf is non-controlled, it starts up immediately
			if (!argmf->mf_state)
				argmf->mf_state = 1;
			do_sound = false; // (and makes no sound)
		}
		
		if (argmf->mf_state != 0) { // If the mobile form is supposed to move... ------------------------------------------------------------------------------------
			int xdelta = 0;
			int ydelta = 0;
			// Set mf_goal_ position
			mf_goal_x = argmf->start_x;
			mf_goal_y = argmf->start_y;
			if (argmf->mf_state >= 0) {
				// If mf_state is positive, set the goal as the next path-node
				for (int j = 0; j < argmf->mf_state; j++) {
					if (argmf->pathaxes[j])
						mf_goal_y += (argmf->pathdeltas[j]);
					else
						mf_goal_x += (argmf->pathdeltas[j]);
				}
			} else {
				// If mf_state is negative, set the goal as the previous path-node
				for (int j = 0; j > argmf->mf_state+1; j--) {
					if (argmf->pathaxes[j])
						mf_goal_y += (argmf->pathdeltas[abs(j)]);
					else
						mf_goal_x += (argmf->pathdeltas[abs(j)]);
				}
			}
			
			
			// If this is a non-controlled form which has reached the goal position,
			if (argmf->color == -1 && argmf->x == mf_goal_x && argmf->y == mf_goal_y) {
				// increment mf_state or, if at end of path, reset to repeat it
				if (argmf->mf_state >= argmf->pathlen)
					argmf->mf_state = 1;
				else
					argmf->mf_state++;
			} else if (argmf->color != -1 && argmf->x == mf_goal_x && argmf->y == mf_goal_y) { // If this is a controlled form, which has reached the goal position
				// increment mf_state if its not at the end of its path
				if (argmf->mf_state < argmf->pathlen)
					argmf->mf_state++;
			} else { // (This thing needs to move)
				// Plot our intended motion (towards the mf_goal_ coordinates) --------------------------------------------------------------------------------------
				if (argmf->x < mf_goal_x) {
					if (argmf->x + spd < mf_goal_x)
						xdelta = spd;
					else
						xdelta = mf_goal_x - argmf->x;
				}
				if (argmf->x > mf_goal_x) {
					if (argmf->x - spd > mf_goal_x)
						xdelta = -spd;
					else
						xdelta = mf_goal_x - argmf->x;
				}
				if (argmf->y < mf_goal_y) {
					if (argmf->y + spd < mf_goal_y)
						ydelta = spd;
					else
						ydelta = mf_goal_y - argmf->y;
				}
				if (argmf->y > mf_goal_y) {
					if (argmf->y - spd > mf_goal_y)
						ydelta = -spd;
					else
						ydelta = mf_goal_y - argmf->y;
				}
				
				// (Now xdelta and ydelta represent our intended motion)
				
				// #####################################
				// ##           PUSH TYPE 1           ##
				// #####################################
				
				// See if Averi is standing or hanging on any of the mobile_form's coll_rects, and if so, try to move her along with it -----------------------------
				
				for (int i = 0; i < argmf->n_crinds; i++) {
					
					struct coll_rect cr = the_level->lcrs[ argmf->crinds[i] ];
					
					if ( !push_type1_flag_averi && // If Averi hasn't already been pushed in this way, and...
						 ((averi.x+averi.w > cr.x && averi.x-averi.w < (cr.x+cr.w)) &&
						  (averi.y+averi.h == cr.y) // ...she's standing atop a cr
						 ) || // OR
						 ( averi.climbing && // she's hanging off one of its sides
						   (averi.x+averi.w == cr.x || averi.x-averi.w == cr.x+cr.w) &&
						   (averi.y <= cr.y-27 && averi.y >= cr.y-83) // (-83 is just an empirical finding that allows you to jump from the hang effectively)
						 )
					   )
					{
						push_type1_flag_averi = true;
						//printf("Push type 1\n");
						averi.x += xdelta; // Move her left/right with it
						averi.y += ydelta; // Move her up/down with it
						
						// But now roll it back if those changes caused her to clip
						struct coll_rect averi_tcr = {
							.x=averi.x-averi.w,
							.y=averi.y,
							.w=averi.w*2,
							.h=averi.h
						};
						while (
							any_collides_mf_besides(
								averi_tcr,
								the_level,
								&cq, w)
							)
						{
							averi.climbing = 0;
							if (xdelta > 0) {
								averi.x--;
								averi_tcr.x--;
							} else if (xdelta < 0) {
								averi.x++;
								averi_tcr.x++;
							}
							if (ydelta > 0) {
								averi.y--;
								averi_tcr.y--;
							} else if (ydelta < 0) {
								averi.y++;
								averi_tcr.y++;
							}
						}
						
						
					}
					//printf("averi.y %i (+H=%i) cr.y %i\n", averi.y, averi.y+averi.h, cr.y);
					// (Ditto for Hazel) ---------------------------------------------------------------------------------------------------
					if ( !push_type1_flag_hazel && // If Hazel hasn't already been pushed in this way, and...
						 ((hazel.x+hazel.w > cr.x && hazel.x-hazel.w < (cr.x+cr.w)) &&
						  (hazel.y+hazel.h == cr.y) // ...she's standing atop a cr
						 ) || // OR
						 ( hazel.climbing && // she's hanging off one of its sides
						   (hazel.x+hazel.w == cr.x || hazel.x-hazel.w == cr.x+cr.w) &&
						   (hazel.y <= cr.y-27 && hazel.y >= cr.y-83) // (-83 is just an empirical finding that allows you to jump from the hang effectively)
						 )
					   )
					{
						push_type1_flag_hazel = true;
						//printf("Push type 1\n");
						hazel.x += xdelta; // Move her left/right with it
						hazel.y += ydelta; // Move her up/down with it
						
						// But now roll it back if those changes caused her to clip
						struct coll_rect hazel_tcr = {
							.x=hazel.x-hazel.w,
							.y=hazel.y,
							.w=hazel.w*2,
							.h=hazel.h
						};
						while (
							any_collides_mf_besides(
								hazel_tcr,
								the_level,
								&cq, w)
							)
						{
							hazel.climbing = 0;
							if (xdelta > 0) {
								hazel.x--;
								hazel_tcr.x--;
							} else if (xdelta < 0) {
								hazel.x++;
								hazel_tcr.x++;
							}
							if (ydelta > 0) {
								hazel.y--;
								hazel_tcr.y--;
							} else if (ydelta < 0) {
								hazel.y++;
								hazel_tcr.y++;
							}
						}
						
						
					}
				}
				
				// #####################################
				// ##           PUSH TYPE 2           ##
				// #####################################
				
				// Push averi if she's in the way and can be pushed, or if she's blocked by something else, diminish xdelta and/or ydelta ---------------------------
				
				//printf("MF checks begin with averi.y of %i\n", averi.y);
				struct coll_rect averi_cr = {
					.x=averi.x-averi.w,
					.y=averi.y,
					.w=averi.w*2,
					.h=averi.h
				};
				struct coll_rect hazel_cr = { // ## NEW ##
					.x=hazel.x-hazel.w,
					.y=hazel.y,
					.w=hazel.w*2,
					.h=hazel.h
				};
				
				// For each coll_rect of this mobile_form...
				for (int i = 0; i < argmf->n_crinds; i++) {
					
					struct coll_rect cr = the_level->lcrs[ argmf->crinds[i] ];
					// Here is what the coll_rect WILL BE when we move,
					struct coll_rect future_mf = {
						.x=cr.x+xdelta,
						.y=cr.y+ydelta,
						.w=cr.w,
						.h=cr.h
					};
					//printf("Checks begin for cr with future_mf of x=%i, y=%i, w=%i, h=%i\n", future_mf.x, future_mf.y, future_mf.w, future_mf.h);
					//printf("\twith averi_cr of x=%i, y=%i (+h=%i), w=%i, h=%i\n", averi_cr.x, averi_cr.y, averi_cr.y+averi_cr.h, averi_cr.w, averi_cr.h);
					
					averi_cr.x = averi.x-averi.w;
					averi_cr.y = averi.y;
					hazel_cr.x = hazel.x-hazel.w;
					hazel_cr.y = hazel.y;
					
					while ( collides(future_mf, averi_cr) && (xdelta||ydelta) ) {
						if (averi.climbing)
							averi.climbing = 0;
						//printf("Changing averi.x or averi.y for potential collision...\n\t(pre) %i, %i\n", averi.x, averi.y);
						if (xdelta > 0) {
							averi.x++;
							averi_cr.x++;
							if (any_collides_mf(averi_cr, the_level, &cq)) {
								averi.x--;
								averi_cr.x--;
								xdelta--;
							}
						} else if (xdelta < 0) {
							averi.x--;
							averi_cr.x--;
							if (any_collides_mf(averi_cr, the_level, &cq)) {
								averi.x++;
								averi_cr.x++;
								xdelta++;
							}
						}
						if (ydelta > 0) {
							averi.y++;
							averi_cr.y++;
							if (any_collides_mf(averi_cr, the_level, &cq)) {
								averi.y--;
								averi_cr.y--;
								ydelta--;
							}
						} else if (ydelta < 0) {
							averi.y--;
							averi_cr.y--;
							if (any_collides_mf(averi_cr, the_level, &cq)) {
								averi.y++;
								averi_cr.y++;
								ydelta++;
							}
						}
						//printf("\t(new) %i, %i\n", averi.x, averi.y);
					}
					while ( collides(future_mf, hazel_cr) && (xdelta||ydelta) ) {
						if (hazel.climbing)
							hazel.climbing = 0;
						//printf("Changing hazel.x or hazel.y for potential collision...\n\t(pre) %i, %i\n", hazel.x, hazel.y);
						if (xdelta > 0) {
							hazel.x++;
							hazel_cr.x++;
							if (any_collides_mf(hazel_cr, the_level, &cq)) {
								hazel.x--;
								hazel_cr.x--;
								xdelta--;
							}
						} else if (xdelta < 0) {
							hazel.x--;
							hazel_cr.x--;
							if (any_collides_mf(hazel_cr, the_level, &cq)) {
								hazel.x++;
								hazel_cr.x++;
								xdelta++;
							}
						}
						if (ydelta > 0) {
							hazel.y++;
							hazel_cr.y++;
							if (any_collides_mf(hazel_cr, the_level, &cq)) {
								hazel.y--;
								hazel_cr.y--;
								ydelta--;
							}
						} else if (ydelta < 0) {
							hazel.y--;
							hazel_cr.y--;
							if (any_collides_mf(hazel_cr, the_level, &cq)) {
								hazel.y++;
								hazel_cr.y++;
								ydelta++;
							}
						}
						//printf("\t(new) %i, %i\n", hazel.x, hazel.y);
					}
				}
				//printf("MF checks end with averi.y of %i (+h=%i)\n\n", averi.y, averi.y+averi.h);
				
				// Move the mobile form xdelta and ydelta -----------------------------------------------------------------------------------------------------------
				argmf->x += xdelta;
				for (int i = 0; i < argmf->n_rinds; i++) {
					the_level->lrs[ argmf->rinds[i] ].dest_x += xdelta;
					the_level->lrs[ argmf->rinds[i] ].dest_y += ydelta;
				}
				argmf->y += ydelta;
				for (int i = 0; i < argmf->n_crinds; i++) {
					the_level->lcrs[ argmf->crinds[i] ].x += xdelta;
					the_level->lcrs[ argmf->crinds[i] ].y += ydelta;
					//printf("mf cr .y is %i\n", arglcrs[ argmf->crinds[i] ].y);
				}
				if (xdelta==0 && ydelta==0) do_sound = false;
			}
		}
		if (do_sound && argmf->mf_state && !(argmf->x == mf_goal_x && argmf->y == mf_goal_y)) {
			add_sound(AUDIO_SYNTHNOISE, sounds, &slen, sounds_size);
		}
	}
}
