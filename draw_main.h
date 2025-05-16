#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <span>
#include "main_state.h"

#define USE_MASK unsigned long long
#define USE_RAX (1ull << 0)
#define USE_RBX (1ull << 1)
#define USE_RCX (1ull << 2)
#define USE_RDX (1ull << 3)
#define USE_RSI (1ull << 4)
#define USE_RDI (1ull << 5)
#define USE_RSP (1ull << 6)
#define USE_RBP (1ull << 7)
#define USE_R8 (1ull << 8)
#define USE_R9 (1ull << 9)
#define USE_R10 (1ull << 10)
#define USE_R11 (1ull << 11)
#define USE_R12 (1ull << 12)
#define USE_R13 (1ull << 13)
#define USE_R14 (1ull << 14)
#define USE_R15 (1ull << 15)

#define USE_CF (1ull << 16)
#define USE_PF (1ull << 17)
#define USE_AF (1ull << 18)
#define USE_ZF (1ull << 19)
#define USE_SF (1ull << 20)
#define USE_DF (1ull << 21)
#define USE_OF (1ull << 22)

#define USE_UNKNOWN (1ull << 23)

// xmm0: 1ull << 32 ... xmm31: 1ull << 63

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
	unsigned long long start_pos;
	unsigned long long last_pos;
	int position;
};
union DragTracker {
	char type;
	VerticalDrag vertical;
	HorizontalDrag horizontal;
};

std::vector<USE_MASK> default_displayed_positions();
std::vector<int> default_displayed_position_widths();

struct MainViewPort {
	SDL_Rect frame;
	float vertical_scroll_instructions; // 1.0 means one instruction is skipped at the top of the canvas
	float side_scroll_pixels;
	float zoom_vertical; // height of one instruction in pixels
	MainState* main_state;
	std::vector<USE_MASK> displayed_positions{ default_displayed_positions() };
	std::vector<int> displayed_position_widths {default_displayed_position_widths()};
	DragTracker drag_tracker{ .type = DRAG_NONE };
};

void draw_main(SDL_Renderer* renderer, TTF_Font* font, MainViewPort& view_port);

// returns true if the event had an effect
bool main_view_port_handle_event(MainViewPort& view_port, SDL_Event& event, bool ignore_keyboard, bool ignore_mouse);

