
// Functions for rendering text from the sprite-sheet

int sheet_x_from_ascii(int ascii) {
	if (ascii < '!' || ascii > '~') // Invalid codes
		return 184; // (Boxed question mark's x)
		
	if (ascii < 60) //60: <
		return 100 + (7 * (ascii - 33));
	else if (ascii < 87) //87: W
		return 100 + (7 * (ascii - 60));
	else if (ascii < 115) //115: s
		return 100 + (7 * (ascii - 87));
	else //if (ascii < 127)
		return 100 + (7 * (ascii - 115));
}

int sheet_y_from_ascii(int ascii) {
	if (ascii < '!' || ascii > '~') // Invalid codes
		return 2952 + (3*12); // (Boxed question mark's y)
		
	if (ascii < 60)
		return 2952; // Top row of text sprites on the sheet
	else if (ascii < 87)
		return 2952 + 12; // Next row is 13 down etc.
	else if (ascii < 115)
		return 2952 + (2*12);
	else //if (ascii < 127)
		return 2952 + (3*12);
}

int sheet_char_w[95] = // The width of my hand-drawn sprite for each ASCII code
{
//  !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;
	1, 3, 5, 5, 5, 5, 1, 3, 3, 5, 5, 2, 3, 1, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, 2, 3,
//  <  =  >  ?  @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 1, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 6, 5, 5,
//  W  X  Y  Z  [  \  ]  ^  _  `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r
	6, 5, 5, 5, 3, 5, 3, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, 2, 5, 5, 2, 7, 5, 5, 5, 5, 5,
//  s  t  u  v  w  x  y  z  {  |  }  ~  [out-of-range]
	5, 4, 5, 5, 6, 5, 5, 5, 4, 1, 4, 5, 7
};

struct r sheet_letter(int ascii, int dest_x, int dest_y, int scalefactor) {
	return (struct r) {
		.source_x=sheet_x_from_ascii(ascii), .source_y=sheet_y_from_ascii(ascii), .source_w=7, .source_h=12,
		.dest_x=dest_x, .dest_y=dest_y, .dest_w=7*scalefactor, .dest_h=12*scalefactor,
		.flip_horizontal=false, .flip_vertical=false
	};
}

void render_text(char* argtext, int dest_x, int dest_y, int scalefactor, int* rslen, struct r* rs, int rs_size) {
	int current_x = dest_x;
	int current_y = dest_y;
	for (int i = 0; argtext[i] != '\0'; i++) {
		char c = argtext[i];
		if (c == '\n') {
			current_y += 12*scalefactor;
			current_x = dest_x;
		} else if (c == ' ')
			current_x += 5*scalefactor;
		else {
			// Add a render instruction for the character
			add_r(sheet_letter(c, current_x, current_y, scalefactor), rslen, rs, rs_size);
			// Move current_x over by the character's width in readiness for the next char
			if (c < '!' || c > '~')
				current_x += (sheet_char_w[94] + 1)*scalefactor; // Boxed question mark's width
			else
				current_x += (sheet_char_w[c-((int)'!')] + 1)*scalefactor;
		}
	}
}

// This shit sucks and probably isn't the right way to do this but I don't have internet access right now to look up how printf() does its %i shit
// pass in the int you want, and a char* of length 12, which will be made to represent that int.
void int_to_string_12(int argint, char* argstr) {
	// (In ASCII, the digits are in order 0123456789, so you can almost add 1 to the char like it were an int)
	// Start off with 12 spaces, the last two must not be used though
	char reg[12] = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
	int max_place_used = 0;
	void inc_reg_digit(int place) {
		if (place > max_place_used)
			max_place_used = place;
		if (place > 9) {
			printf("Warning: number %i too big to display in int_to_string()\n", argint);
			return;
		}
		reg[place] += 1;
		if (reg[place] > '9') {
			reg[place] = '0';
			if (reg[place+1] == ' ') reg[place+1] = '1';
			else inc_reg_digit(place+1);
			if (place+1 > max_place_used)
				max_place_used = place+1;
		}
	}
	bool negative = false;
	if (argint < 0) {
		negative = true;
		argint *= -1;
	}
	for (int i = 0; i < argint; i++)
		inc_reg_digit(0);
		
	// reg = {'3', '2', '1', ....}
	
	//char result[12] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
	
	int res_ind = 0;
	if (negative) {
		res_ind = 1;
		//result[0] = '-';
		argstr[0] = '-';
	}
	for (int i = max_place_used; i >= 0; i--) {
		//result[res_ind] = reg[i];
		argstr[res_ind] = reg[i];
		res_ind++;
	}
	//result[res_ind] = '\0';
	argstr[res_ind] = '\0';
	
	//printf("reg: [%s], max_place_used %i\n", reg, max_place_used);
	//printf("res: [%s]\n", result);
}
