#include "draw_main.h"
#include <bit>

#define DBG(msg) (std::cout << msg << "\n")

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

USE_MASK get_use_mask_in(const Instruction instruction, const InstructionFootprint& footprint) {
	USE_MASK result = USE_MASK(footprint.always_read_registers) | (USE_MASK(footprint.always_read_flags) << 16);
	for (int i = 0; i != 4; i++) if (footprint.operands[i].head & OF_IN) {
		unsigned long long op = instruction.operands[i];
		if (OP_IS_REGISTER(op))				result |= 1ull << OP_REGISTER(op);
		else if (OP_IS_HIGH_BYTE(op))		result |= 1ull << OP_HIGH_BYTE(op);
		else if (OP_IS_SIMD_REGISTER(op))	result |= 1ull << 32 + OP_SIMD_REGISTER(op);
	}
	return result;
}
USE_MASK get_use_mask_in_pointer(const Instruction instruction, const InstructionFootprint& footprint) {
	USE_MASK result = 0;
	for (int i = 0; i != 4; i++) {
		unsigned long long op = instruction.operands[i];
		unsigned char head = footprint.operands[i].head;
		if ((head & OF_DEREF_TYPE) == OF_IN_DEREF || (head & OF_DEREF_TYPE) == OF_IO_DEREF)
			result |= 1ull << (footprint.operands[i].head & OF_PTR_REGISTER);
		else if ((head & OF_IN) && OP_IS_MEMORY_LOCATION(op) && OP_MEMORY_BASE_REGISTER(op) < 16)
			result |= 1ull << OP_MEMORY_BASE_REGISTER(op);
	}
	return result;
}
USE_MASK get_use_mask_in_index(const Instruction instruction, const InstructionFootprint& footprint) {
	USE_MASK result = 0;
	for (int i = 0; i != 4; i++) {
		unsigned long long op = instruction.operands[i];
		unsigned char head = footprint.operands[i].head;
		if ((head & OF_IN) && OP_IS_MEMORY_LOCATION(op) && OP_MEMORY_INDEX_REGISTER(op) < 16)
			result |= 1ull << OP_MEMORY_INDEX_REGISTER(op);
	}
	return result;
}
USE_MASK get_use_mask_out(const Instruction instruction, const InstructionFootprint& footprint) {
	USE_MASK result = USE_MASK(footprint.always_written_registers) | (USE_MASK(footprint.always_written_flags) << 16);
	for (int i = 0; i != 4; i++) if (footprint.operands[i].head & OF_OUT) {
		unsigned long long op = instruction.operands[i];
		if (OP_IS_REGISTER(op))				result |= 1ull << OP_REGISTER(op);
		else if (OP_IS_HIGH_BYTE(op))		result |= 1ull << OP_HIGH_BYTE(op);
		else if (OP_IS_SIMD_REGISTER(op))	result |= 1ull << 32 + OP_SIMD_REGISTER(op);
	}
	return result;
}
USE_MASK get_use_mask_out_pointer(const Instruction instruction, const InstructionFootprint& footprint) {
	USE_MASK result = 0;
	for (int i = 0; i != 4; i++) {
		unsigned long long op = instruction.operands[i];
		unsigned char head = footprint.operands[i].head;
		if ((head & OF_DEREF_TYPE) == OF_OUT_DEREF || (head & OF_DEREF_TYPE) == OF_IO_DEREF)
			result |= 1ull << (footprint.operands[i].head & OF_PTR_REGISTER);
		else if ((head & OF_OUT) && OP_IS_MEMORY_LOCATION(op) && OP_MEMORY_BASE_REGISTER(op) < 16)
			result |= 1ull << OP_MEMORY_BASE_REGISTER(op);
	}
	return result;
}
USE_MASK get_use_mask_out_index(const Instruction instruction, const InstructionFootprint& footprint) {
	USE_MASK result = 0;
	for (int i = 0; i != 4; i++) {
		unsigned long long op = instruction.operands[i];
		unsigned char head = footprint.operands[i].head;
		if ((head & OF_OUT) && OP_IS_MEMORY_LOCATION(op) && OP_MEMORY_INDEX_REGISTER(op) < 16)
			result |= 1ull << OP_MEMORY_INDEX_REGISTER(op);
	}
	return result;
}

struct DrawState {
	float x;
	float y;
	int text_width;
	int text_height;
	int nr_hits_left;
	int start_pos;
	float w;
	float h;
	bool done;

	SDL_Color foreground_color;
	SDL_Color background_color;
	SDL_Color inactive_color;
	
};
void draw_position(SDL_Renderer* renderer, DrawState& state,
	USE_MASK position, USE_MASK below, USE_MASK above,
	USE_MASK in, USE_MASK in_pointer, USE_MASK in_index,
	USE_MASK out, USE_MASK out_pointer, USE_MASK out_index)
{
	// DBG(position << " " << ((position & in) ? "in " : "") << ((position & in_pointer) ? "in* " : "") << ((position & in_index) ? "[in] " : "") << ((position & out) ? "out " : "") << ((position & out_pointer) ? "out* " : "") << ((position & out_index) ? "[out] " : ""));

	float x = state.x;
	float x1 = state.x + state.w / 6;
	float x2 = state.x + state.w * 5 / 6;
	float x3 = state.x + state.w;
	float y = state.y;
	float y1 = state.y + (state.h - state.text_height) / 2 - 1;
	float y2 = y1 + state.text_height + 1;
	float y3 = state.y + state.w;

	// lower half gray ?
	// upper half gray ?
	SDL_FRect upper_half = {x, y, state.w, state.h / 2};
	SDL_FRect lower_half = { x, y + state.h / 2, state.w, (state.h + 1) / 2 };
	if (below & position) SDL_SetRenderDrawColor(renderer, state.background_color.r, state.background_color.g, state.background_color.b, state.background_color.a);
	else SDL_SetRenderDrawColor(renderer, state.inactive_color.r, state.inactive_color.g, state.inactive_color.b, state.inactive_color.a);
	SDL_RenderFillRect(renderer, &lower_half);
	if (above & position) SDL_SetRenderDrawColor(renderer, state.background_color.r, state.background_color.g, state.background_color.b, state.background_color.a);
	else SDL_SetRenderDrawColor(renderer, state.inactive_color.r, state.inactive_color.g, state.inactive_color.b, state.inactive_color.a);
	SDL_RenderFillRect(renderer, &upper_half);

	// divider
	SDL_SetRenderDrawColor(renderer, state.foreground_color.r, state.foreground_color.g, state.foreground_color.b, state.foreground_color.a);
	SDL_RenderLine(renderer, x3 - 1, y, x3 - 1, y3 - 1);

	// maybe draw bridge
	bool contains_bridge;
	bool bridge_start;
	bool bridge_end;
	if (position & (in | in_pointer | in_index | out | out_pointer | out_index)) state.nr_hits_left--;
	if (state.start_pos < 0) {
		if (position & (in | in_pointer | in_index | out | out_pointer | out_index)) {
			contains_bridge = true;
			bridge_start = true;
			state.start_pos = x1+1;
		}
		else {
			bridge_start = false;
			bridge_end = false;
			contains_bridge = false;
		}
	}
	else {
		// not right
		if (state.done) {
			contains_bridge = false;
			bridge_start = false;
		}
		else {
			contains_bridge = true;
			bridge_start = false;
		}
	}
	if (contains_bridge) {
		if (state.start_pos + state.text_width < x2 && state.nr_hits_left == 0) {
			bridge_end = true;
			state.done = true;
		}
		else {
			bridge_end = false;
		}
	}

	if (contains_bridge) {
		SDL_FRect bridge_fill = { x, y1+1, x3-x, y2-y1-1 };
		if (bridge_start) {
			bridge_fill.x = x1;
			bridge_fill.w = x3-x1;
		}
		if (bridge_end) bridge_fill.w = x2 - bridge_fill.x;
		SDL_SetRenderDrawColor(renderer, state.background_color.r, state.background_color.g, state.background_color.b, state.background_color.a);
		SDL_RenderFillRect(renderer, &bridge_fill);
		
		SDL_SetRenderDrawColor(renderer, state.foreground_color.r, state.foreground_color.g, state.foreground_color.b, state.foreground_color.a);
		if (bridge_start) {
			SDL_RenderLine(renderer, x1, y1, x1, y2);
		}
		else {
			SDL_RenderLine(renderer, x, y1, x1, y1);
			SDL_RenderLine(renderer, x, y2, x1, y2);
		}
		if (bridge_end) {
			SDL_RenderLine(renderer, x2, y1, x2, y2);
		}
		else {
			SDL_RenderLine(renderer, x2, y1, x3, y1);
			SDL_RenderLine(renderer, x2, y2, x3, y2);
		}

		// draw top edge / markers
		// draw bottom edge / markers
		if (position & (in | in_pointer | in_index)) {
			SDL_RenderLine(renderer, x1, y, x1, y1);
			SDL_RenderLine(renderer, x2, y, x2, y1);
		}
		else {
			SDL_RenderLine(renderer, x1, y1, x2, y1);
		}
		if (position & in_pointer) SDL_RenderLine(renderer, x1, y1, x2, y);
		if (position & in_index) SDL_RenderLine(renderer, x1, y, x2, y1);
		
		if (position & (out | out_pointer | out_index)) {
			SDL_RenderLine(renderer, x1, y2, x1, y3);
			SDL_RenderLine(renderer, x2, y2, x2, y3);
		}
		else {
			SDL_RenderLine(renderer, x1, y2, x2, y2);
		}
		if (position & out_pointer) SDL_RenderLine(renderer, x1, y3, x2, y2);
		if (position & out_index) SDL_RenderLine(renderer, x1, y2, x2, y3);
	}
	state.x += state.w;
}

void draw_main(SDL_Renderer* renderer, TTF_Font* font, MainViewPort& view_port)
{
	auto& instructions = view_port.main_state->instructions;
	auto& extensions = view_port.main_state->extensions;

	int first_instruction_drawn = int(view_port.vertical_scroll_instructions);
	if (first_instruction_drawn > instructions.size()) first_instruction_drawn = instructions.size();
	int nr_instructions_drawn = int(float(view_port.frame.h) / view_port.zoom_vertical + view_port.vertical_scroll_instructions - float(first_instruction_drawn)) + 1;
	if (first_instruction_drawn + nr_instructions_drawn > instructions.size()) nr_instructions_drawn = instructions.size() - first_instruction_drawn;
	int vertical_scroll_pixels = int((view_port.vertical_scroll_instructions - float(first_instruction_drawn)) * view_port.zoom_vertical);
	
	SDL_Rect view_translate = {
		view_port.frame.x - view_port.side_scroll_pixels,
		view_port.frame.y - vertical_scroll_pixels,
		view_port.frame.w + view_port.side_scroll_pixels,
		view_port.frame.h + vertical_scroll_pixels
	};
	SDL_SetRenderViewport(renderer, &view_translate);

	// the clip rectangle is relative to the moved viewport, so we need to un-move it.
	// and no, swapping around SDL_SetRenderViewPort and SDL_SetRenderClipRect does not help.

	SDL_Rect clip = {
		view_port.frame.x - view_translate.x,
		view_port.frame.y - view_translate.y,
		view_port.frame.w,
		view_port.frame.h
	};
	SDL_SetRenderClipRect(renderer, &clip);

	// std::cout << "frame: " << view_port.frame.x << " " << view_port.frame.y << " " << view_port.frame.w << " " << view_port.frame.h << "\n";
	// std::cout << "port: " << view_translate.x << " " << view_translate.y << "\n";

	// now draw the instructions [first_instruction_drawn ..< first_instruction_drawn+nr_instructions_drawn].
	// (0,0) is top left of first instruction.

	USE_MASK use_mask = 0xffffffff00ffffff;
	for (int i = instructions.size() - 1; i >= first_instruction_drawn + nr_instructions_drawn; i--) {
		Instruction instruction = instructions[i];
		InstructionFootprint& footprint = extensions[instruction.extension].instructions[instruction.index];
		USE_MASK in = get_use_mask_in(instruction, footprint) 
			| get_use_mask_in_pointer(instruction, footprint)
			| get_use_mask_in_index(instruction, footprint)
			| get_use_mask_out_pointer(instruction, footprint)
			| get_use_mask_out_index(instruction, footprint);
		USE_MASK out = get_use_mask_out(instruction, footprint);
		use_mask = (use_mask & ~out) | in;
	}

	for (int i = first_instruction_drawn + nr_instructions_drawn - 1; i != first_instruction_drawn - 1; i--) {
		Instruction instruction = instructions[i];
		InstructionFootprint& footprint = extensions[instruction.extension].instructions[instruction.index];
		const char* name = &extensions[instruction.extension].names[footprint.name];
		USE_MASK in = get_use_mask_in(instruction, footprint);
		USE_MASK in_pointer = get_use_mask_in_pointer(instruction, footprint);
		USE_MASK in_index = get_use_mask_in_index(instruction, footprint);
		USE_MASK out_pointer = get_use_mask_out_pointer(instruction, footprint);
		USE_MASK out_index = get_use_mask_out_index(instruction, footprint);
		USE_MASK out = get_use_mask_out(instruction, footprint);
		
		USE_MASK use_mask_below = use_mask;
		use_mask = (use_mask & ~out) | in | in_pointer | in_index | out_pointer | out_index;
		USE_MASK use_mask_above = use_mask;

		TTF_Text* ttf_text = TTF_CreateText(NULL, font, name, 0);
		int text_width, text_height;
		TTF_GetTextSize(ttf_text, &text_width, &text_height);
		TTF_DestroyText(ttf_text);

		DrawState state = {
			.x = 0,
			.y = (i - first_instruction_drawn) * view_port.zoom_vertical,
			.text_width = text_width,
			.text_height = text_height,
			.nr_hits_left = std::popcount((in | in_pointer | in_index | out | out_pointer | out_index) & 0xffffull),
			.start_pos = -1,
			.w = view_port.zoom_vertical,
			.h = view_port.zoom_vertical,
			.done = false,
			.foreground_color = {255,255,255,255},
			.background_color = {0,0,0,255},
			.inactive_color = {100,100,100,255}
		};
		if (state.nr_hits_left == 0) state.start_pos = 0;
		for (int r = 0;r != 16;r++) {
			draw_position(renderer, state,
				1ull << r, use_mask_below, use_mask_above, 
				in, in_pointer, in_index,
				out, out_pointer, out_index);
		}

		SDL_Color color = {255, 255, 255, 255};
		SDL_Surface* surface = TTF_RenderText_Blended(font, name, 0, color);
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);
		SDL_FRect dst_rect = { 
			state.start_pos, 
			state.y + (view_port.zoom_vertical-text_height)/2, 
			text_width, 
			text_height
		};
		SDL_RenderTexture(renderer, texture, NULL, &dst_rect);
		SDL_DestroyTexture(texture);
	}


	// SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);

	/*
	int text_width, text_height;
	const char* text = "Test";
	TTF_Text* ttf_text = TTF_CreateText(NULL, font, text, 4);
	TTF_GetTextSize(ttf_text, &text_width, &text_height);
	SDL_Color color = { 255, 255, 255, 255 }; // white
	SDL_Surface* surface = TTF_RenderText_Blended(font, text, 4, color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_DestroySurface(surface); // We no longer need the surface
	SDL_FRect dstRect = { 0, 0, text_width, text_height };
	SDL_RenderTexture(renderer, texture, NULL, &dstRect);
	SDL_DestroyTexture(texture);
	*/

	SDL_SetRenderViewport(renderer, NULL);
	SDL_SetRenderClipRect(renderer, NULL);
}

unsigned char get_mouse_pos_register(MainViewPort& view_port, float mouse_x, float mouse_y) {
	float result = (mouse_x - view_port.frame.x + view_port.side_scroll_pixels) / view_port.zoom_vertical;
	if (result < 0) return 0x10;
	if (result > 15) return 0x1f;
	return 0x10 + int(result);
}
int get_mouse_pos_instruction(MainViewPort& view_port, float mouse_x, float mouse_y) {
	float result = (mouse_y - view_port.frame.y) / view_port.zoom_vertical + view_port.vertical_scroll_instructions;
	if (result < 0) return 0;
	if (result > view_port.main_state->instructions.size() - 1) return view_port.main_state->instructions.size() - 1;
	return int(result);
}
bool in_viewport(MainViewPort& view_port, float mouse_x, float mouse_y) {
	return mouse_x - view_port.frame.x >= 0 && mouse_x - view_port.frame.x < view_port.frame.w &&
		mouse_y - view_port.frame.y >= 0 && mouse_y - view_port.frame.y < view_port.frame.h;
}
bool update_drag(MainViewPort& view_port, float mouse_x, float mouse_y) {
	switch (view_port.drag_tracker.type) {
	case DRAG_HORIZONTAL: {
		unsigned char new_position = get_mouse_pos_register(view_port, mouse_x, mouse_y);
		if (new_position == view_port.drag_tracker.horizontal.last_pos) return false;
		if (view_port.main_state->temporary_change.type) undo_last_change(view_port.main_state->instructions, view_port.main_state->extensions, &view_port.main_state->temporary_change);
		view_port.drag_tracker.horizontal.last_pos = new_position;
		if (new_position == view_port.drag_tracker.horizontal.start_pos) view_port.main_state->temporary_change.type = 0;
		else {
			DBG("building horizontal change: " << view_port.drag_tracker.horizontal.position << " " << int(view_port.drag_tracker.horizontal.start_pos) << " " << int(new_position));
			view_port.main_state->temporary_change = build_horizontal_change(
				view_port.main_state->instructions,
				view_port.main_state->extensions,
				view_port.drag_tracker.horizontal.position,
				view_port.drag_tracker.horizontal.start_pos,
				new_position);
			apply_change(view_port.main_state->instructions, view_port.main_state->extensions, &view_port.main_state->temporary_change);
		}
		return true;
	}
	case DRAG_VERTICAL: {
		int new_position = get_mouse_pos_instruction(view_port, mouse_x, mouse_y);
		if (new_position == view_port.drag_tracker.vertical.last_instruction) return false;
		if (view_port.main_state->temporary_change.type) undo_last_change(view_port.main_state->instructions, view_port.main_state->extensions, &view_port.main_state->temporary_change);
		view_port.drag_tracker.vertical.last_instruction = new_position;
		if (new_position == view_port.drag_tracker.vertical.start_instruction) view_port.main_state->temporary_change.type = 0;
		else {
			DBG("building vertical change " << view_port.drag_tracker.vertical.start_instruction << "" << new_position - view_port.drag_tracker.vertical.start_instruction);
			view_port.main_state->temporary_change = build_vertical_change(
				view_port.main_state->instructions,
				view_port.main_state->extensions,
				view_port.drag_tracker.vertical.start_instruction,
				new_position - view_port.drag_tracker.vertical.start_instruction);
			apply_change(view_port.main_state->instructions, view_port.main_state->extensions, &view_port.main_state->temporary_change);
		}
		return true;
	}
	}
	return false;
}

bool main_view_port_handle_event(MainViewPort& view_port, SDL_Event& event, bool ignore_keyboard, bool ignore_mouse) {
	float scroll_speed = 10;
	int mouse_button_drag = 1;

	switch (event.type) {
	case SDL_EVENT_MOUSE_WHEEL:
		if (ignore_mouse || !in_viewport(view_port, event.wheel.mouse_x, event.wheel.mouse_y)) return false;
		view_port.side_scroll_pixels += scroll_speed * event.wheel.x;
		view_port.vertical_scroll_instructions += scroll_speed * event.wheel.y / view_port.zoom_vertical;
		update_drag(view_port, event.wheel.mouse_x, event.wheel.mouse_y);
		return true;
	case SDL_EVENT_MOUSE_BUTTON_DOWN: {
		if (ignore_mouse || !in_viewport(view_port, event.button.x, event.button.y)) return false;

		if (view_port.drag_tracker.type) {
			if (view_port.main_state->temporary_change.type) undo_last_change(view_port.main_state->instructions, view_port.main_state->extensions, &view_port.main_state->temporary_change);
			view_port.main_state->temporary_change.type = 0;
		}
		if (event.button.button != mouse_button_drag) return false;

		float pos_x = (event.button.x - view_port.frame.x + view_port.side_scroll_pixels) / view_port.zoom_vertical;
		float pos_y = (event.button.y - view_port.frame.y) / view_port.zoom_vertical + view_port.vertical_scroll_instructions;
		float pos_x_p = pos_x - int(pos_x);
		float pos_y_p = pos_y - int(pos_y);
		if (pos_x < 0 || pos_x >= 16 || pos_y < 0 || pos_y >= view_port.main_state->instructions.size()) view_port.drag_tracker.type = DRAG_NONE;
		else if (pos_x_p > pos_y_p == pos_x_p + pos_y_p < 1.) {
			view_port.drag_tracker.horizontal = {
				.type = DRAG_HORIZONTAL,
				.start_pos = unsigned char(pos_x + 0x10),
				.last_pos = unsigned char(pos_x + 0x10),
				.position = int(pos_y + 0.5)
			};
		}
		else {
			view_port.drag_tracker.vertical = {
				.type = DRAG_VERTICAL,
				.start_instruction = int(pos_y),
				.last_instruction = int(pos_y)
			};
		}
		return true;
	}
	case SDL_EVENT_MOUSE_BUTTON_UP:
		if (ignore_mouse || !in_viewport(view_port, event.button.x, event.button.y) || event.button.button != mouse_button_drag || !view_port.drag_tracker.type) return false;
		update_drag(view_port, event.button.x, event.button.y);
		if (view_port.main_state->temporary_change.type) {
			view_port.main_state->undo_redo_list.resize(view_port.main_state->change_count);
			view_port.main_state->undo_redo_list.push_back(view_port.main_state->temporary_change);
			view_port.main_state->change_count++;
			view_port.main_state->temporary_change.type = 0;
		}
		view_port.drag_tracker.type = DRAG_NONE;
		return true;
	case SDL_EVENT_MOUSE_MOTION:
		if (ignore_mouse || !in_viewport(view_port, event.button.x, event.button.y)) return false;
		return update_drag(view_port, event.motion.x, event.motion.y);
	}
	return false;
}
