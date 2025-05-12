#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <span>
#include "main_state.h"

#define DRAG_NONE 0
#define DRAG_VERTICAL 1
#define DRAG_HORIZONTAL 2
struct VerticalDrag {
	char type; // DRAG_VERTICAL
	int start_instruction;
	int last_instruction;
};
struct HorizontalDrag {
	char type; // DRAG_HORIZONTAL
	unsigned char start_pos;
	unsigned char last_pos;
	int position;
};
union DragTracker {
	char type;
	VerticalDrag vertical;
	HorizontalDrag horizontal;
};

struct MainViewPort {
	SDL_Rect frame;
	float vertical_scroll_instructions; // 1.0 means one instruction is skipped at the top of the canvas
	float side_scroll_pixels;
	float zoom_vertical; // height of one instruction in pixels
	MainState* main_state;
	DragTracker drag_tracker{ .type = DRAG_NONE };
};

void draw_main(SDL_Renderer* renderer, TTF_Font* font, MainViewPort& view_port);

// returns true if the event had an effect
bool main_view_port_handle_event(MainViewPort& view_port, SDL_Event& event, bool ignore_keyboard, bool ignore_mouse);
