
struct coll_rect source_of (int arg) {
	switch (arg) {
		// Fleurs-de-lis
		case 21:
			return (struct coll_rect) {.x=136, .y=0, .w=19, .h=30};
			break;
		case 22:
			return (struct coll_rect) {.x=155, .y=0, .w=19, .h=30};
			break;
		case 23:
			return (struct coll_rect) {.x=136, .y=30, .w=19, .h=30};
			break;
		case 24:
			return (struct coll_rect) {.x=155, .y=30, .w=19, .h=30};
			break;
		// Sun symbols
		case 25:
			return (struct coll_rect) {.x=136, .y=60, .w=19, .h=19};
			break;
		case 26:
			return (struct coll_rect) {.x=155, .y=60, .w=19, .h=19};
			break;
		case 27:
			return (struct coll_rect) {.x=136, .y=79, .w=19, .h=19};
			break;
		case 28:
			return (struct coll_rect) {.x=155, .y=79, .w=19, .h=19};
			break;
		// Leaf symbols
		case 29:
			return (struct coll_rect) {.x=136, .y=98, .w=18, .h=18};
			break;
		case 30:
			return (struct coll_rect) {.x=155, .y=98, .w=18, .h=18};
			break;
		case 31:
			return (struct coll_rect) {.x=136, .y=116, .w=18, .h=18};
			break;
		case 32:
			return (struct coll_rect) {.x=155, .y=116, .w=18, .h=18};
			break;
		// Levers
		case 33:
			return (struct coll_rect) {.x=136, .y=168, .w=34, .h=43};
			break;
		case 34:
			return (struct coll_rect) {.x=136, .y=211, .w=34, .h=43};
			break;
		case 35:
			return (struct coll_rect) {.x=136, .y=254, .w=34, .h=43};
			break;
		case 36:
			return (struct coll_rect) {.x=136, .y=297, .w=34, .h=43};
			break;
		case 37:
			return (struct coll_rect) {.x=136, .y=340, .w=34, .h=43};
			break;
		case 38:
			return (struct coll_rect) {.x=136, .y=383, .w=34, .h=43};
			break;
		case 39:
			return (struct coll_rect) {.x=136, .y=426, .w=34, .h=43};
			break;
		case 40:
			return (struct coll_rect) {.x=136, .y=469, .w=34, .h=43};
			break;
		case 41:
			return (struct coll_rect) {.x=136, .y=512, .w=34, .h=43};
			break;
			
	}
	return (struct coll_rect) {.x=0, .y=0, .w=100, .h=100};
}

const int lever_h = 42;
const int lever_w = 43;

struct lever {
	int x, y;
	int color;
};

void simple_form( // ------------------------------------------------------------------------------------------------------------
	int start_x, int start_y, // Where to place the form
	int tile_type, // (There are many different decorative designs on the sprite sheet; set it to anything above 20)
	struct r* rs, int* rslen, int rs_size, // Pointers and limit to add onto the caller's rendering data
	struct coll_rect* crs, int* crlen, int crs_size) // Pointers  & limit to add onto the caller's collision data
{
	// Add rendering (###and, for certain forms, collision?###) data to the pointed struct arrays
	
	struct coll_rect source_rect = source_of(tile_type);
	
	struct r result_r = {
		.source_x=source_rect.x,
		.source_y=source_rect.y,
		.source_w=source_rect.w,
		.source_h=source_rect.h,
		.dest_x=start_x,
		.dest_y=start_y-source_rect.h,
		.dest_w=source_rect.w,
		.dest_h=source_rect.h,
		.flip_horizontal=false,
		.flip_vertical=false
	};
	
	add_r(result_r, rslen, rs, rs_size);
}
