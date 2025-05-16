#include "main_state.h"

MainState init_example_main_state() {
	MainState result{
		.extensions = {
			read_instruction_set_extension("inputs/builtins.txt"), // has to be first
			read_instruction_set_extension("inputs/binary.txt"),
			read_instruction_set_extension("inputs/bits.txt"),
			read_instruction_set_extension("inputs/cmp.txt"),
			read_instruction_set_extension("inputs/jmp.txt"),
			read_instruction_set_extension("inputs/mov.txt"),
		},
		.instructions = { {} , {} },
		.undo_redo_list = {},
		.change_count = 0
	};
	load_instruction("ADD32 RAX RBX", result.instructions[0], result.extensions);
	load_instruction("MOV64 RCX RAX", result.instructions[1], result.extensions);
	return result;
}