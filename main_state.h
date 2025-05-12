#pragma once
#include "changes.h"

struct MainState {
	std::vector<InstructionSetExtension> extensions;
	std::vector<Instruction> instructions;
	std::vector<Change> undo_redo_list;
	int change_count;
	Change temporary_change{ .type = 0 };
};
MainState init_example_main_state();