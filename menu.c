#include <stdbool.h>

#define MENU_STARTGAME 0
#define MENU_OPTIONS 1
#define MENU_QUIT 2

static int menu_highlight = 0; // Which menu option is highlighted
static int menu_hltime = 0; // Time in hightlight color cycle

static bool menu_w_held = 0; // Used so holding these keys doesn't skip past multiple buttons
static bool menu_s_held = 0;

static const int button_spacing = 60;
static const int button_upscaled_w = 336;
static const int button_upscaled_h = 51;

static const int button_source_x = 100;

//------------------------------------------------------------------------------------------------------------------------------------------
// MENU TICK FUNCTION (Called about 30 times per second by the main loop)
//------------------------------------------------------------------------------------------------------------------------------------------

int m_tick(struct r* rs, int* rslen, bool keyW, bool keyA, bool keyS, bool keyD, bool keySpace) {
	
	// Use this variable to toggle the highlight color every 10 frames
	if (menu_hltime >= 20)
		menu_hltime = 0;
	else
		menu_hltime += 1;
	
	// Render everything
	struct r button1_r = {
		.source_x=button_source_x, .source_y=2564, .source_w=112, .source_h=17,
		.dest_x=550, .dest_y=10, .dest_w=button_upscaled_w, .dest_h=button_upscaled_h,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
	};
	struct r text_start_r = {
		.source_x=button_source_x, .source_y=2564+17, .source_w=55, .source_h=17,
		.dest_x=550, .dest_y=10, .dest_w=165, .dest_h=button_upscaled_h,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
	};
	struct r button2_r = {
		.source_x=button_source_x, .source_y=2564, .source_w=112, .source_h=17,
		.dest_x=550, .dest_y=10+button_spacing, .dest_w=button_upscaled_w, .dest_h=button_upscaled_h,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
	};
	struct r text_options_r = {
		.source_x=button_source_x, .source_y=2564+2*17, .source_w=55, .source_h=17,
		.dest_x=550, .dest_y=10+button_spacing, .dest_w=165, .dest_h=button_upscaled_h,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
	};
	struct r button3_r = {
		.source_x=button_source_x, .source_y=2564, .source_w=112, .source_h=17,
		.dest_x=550, .dest_y=10+button_spacing*2, .dest_w=button_upscaled_w, .dest_h=button_upscaled_h,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
	};
	struct r text_quit_r = {
		.source_x=button_source_x, .source_y=2564+3*17, .source_w=55, .source_h=17,
		.dest_x=550, .dest_y=10+button_spacing*2, .dest_w=165, .dest_h=button_upscaled_h,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
	};
	struct r highlight_r = {
		.source_x=button_source_x+(14*(menu_hltime > 10)), .source_y=2564-17, .source_w=16, .source_h=17,
		.dest_x=550, .dest_y=10+(button_spacing*menu_highlight), .dest_w=48, .dest_h=button_upscaled_h,
		.visible=1, .flip_horizontal=0, .flip_vertical=0
	};
	
	rs[0] = button1_r;
	rs[1] = text_start_r;
	rs[2] = button2_r;
	rs[3] = text_options_r;
	rs[4] = button3_r;
	rs[5] = text_quit_r;
	rs[6] = highlight_r;
	*rslen = 7;
	
	
	// Allow changing the highlighted button with W and S keys:
	if ((keyW && !menu_w_held) && menu_highlight > 0)
		menu_highlight -= 1;
	else if ((keyS && !menu_s_held) && menu_highlight < 2)
		menu_highlight += 1;
	
	// Designated actions for each choice when Space is pressed:
	if (keySpace) {
		switch (menu_highlight) {
			case MENU_STARTGAME:
				return MAIN_GAME;
				break;
			case MENU_OPTIONS:
				return MAIN_GAME; // TODO make a real options menu
				break;
			case MENU_QUIT:
				return MAIN_QUIT;
				break;
		}
	}
	
	menu_w_held = keyW; // This just has to be done after menu_w_held is checked
	menu_s_held = keyS;
	
	return MAIN_MENU; // Remain in the menu state
}
