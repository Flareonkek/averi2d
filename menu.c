
enum {
	MENU_START1,
	MENU_START2,
	MENU_QUIT,
};

static int menu_highlight = 0; // Which menu option is highlighted
static int menu_hltime = 0; // Time in hightlight color cycle

static const int button_spacing = 60;
static const int button_upscaled_w = 336;
static const int button_upscaled_h = 51;

static const int button_source_x = 100;

//------------------------------------------------------------------------------------------------------------------------------------------
// MENU TICK FUNCTION (Called about 30 times per second by the main loop)
//------------------------------------------------------------------------------------------------------------------------------------------

int m_tick(struct r* rs, int* rslen, int rs_size, unsigned short kd, unsigned short kp, int * instruction) {
	
	*rslen = 0; // Prepare to render the menu only
	
	// Use this variable to toggle the highlight color every 10 frames
	if (menu_hltime >= 20)
		menu_hltime = 0;
	else
		menu_hltime += 1;
	
	// Render all the buttons
	
	char* btext[3] = {"Start level 1", "Start level 2", "Quit"};
	
	for (int bi = 0; bi < 3; bi++)
	{
		// The round-cornered white rectangle of the button
		rs[*rslen] = (struct r) {
			.source_x=button_source_x, .source_y=2935, .source_w=112, .source_h=17,
			.dest_x=550, .dest_y=10+button_spacing*bi, .dest_w=button_upscaled_w, .dest_h=button_upscaled_h,
			.flip_horizontal=false, .flip_vertical=false
		};
		(*rslen)++;
		// The text of the button
		render_text(btext[bi], 600, 19+button_spacing*bi, 3, rslen, rs, rs_size);
	}
	
	// Render the highlight/cursor thing
	
	struct r highlight_r = {
		.source_x=button_source_x+(14*(menu_hltime > 10)), .source_y=2918, .source_w=16, .source_h=17,
		.dest_x=550, .dest_y=10+(button_spacing*menu_highlight), .dest_w=48, .dest_h=button_upscaled_h,
		.flip_horizontal=false, .flip_vertical=false
	};
	rs[*rslen] = highlight_r;
	(*rslen)++;
	
	// Allow changing the highlighted button with W/Up-arrow and S/Down-arrow keys:
	
	bool cursorup = (keyWdown(kd)||keyUPdown(kd)) && !(keyWdown(kp)||keyUPdown(kp));
	bool cursordown = (keySdown(kd) || keyDOWNdown(kd)) && !(keySdown(kp)||keyDOWNdown(kp));
	
	if (cursorup && menu_highlight > 0)
		menu_highlight -= 1;
	else if (cursordown && menu_highlight < 2)
		menu_highlight += 1;
	
	// Designated actions for each choice when Space is pressed:
	if (keySPACEdown(kd)) {
		switch (menu_highlight) {
			case MENU_START1:
				(*instruction) = 1;
				return MAIN_GAME;
				break;
			case MENU_START2:
				(*instruction) = 2;
				return MAIN_GAME; // TODO make a real options menu
				break;
			case MENU_QUIT:
				return MAIN_QUIT;
				break;
		}
	}
	
	(*instruction) = 0; // Default instruction (to do nothing) for main game-loop
	
	return MAIN_MENU; // Remain in the menu state
}
