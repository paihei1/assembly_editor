#pragma once
#include <span>
#include "instructions.h"

#define REGISTER_SWAP 1
#define INSTRUCTION_REORDER 2
struct RegisterSwap {
	char type; // REGISTER_SWAP
	char explicit_swap_at_start; // 0 means no, 1 means mov pos1 -> pos2, 2 means mov pos2 -> pos1, 3 means swap
	char explicit_swap_at_end;
	int first_instruction_affected;
	int last_instruction_affected;
	unsigned long long pos1;
	unsigned long long pos2;
};
struct InstructionReorder {
	char type; // INSTRUCTION_REORDER;
	int instruction;
	int number_places_down;
};


union Change {
	char type;
	RegisterSwap horizontal;
	InstructionReorder vertical;
};

Change build_horizontal_change(std::span<Instruction> instructions, std::vector<InstructionSetExtension>& extensions, int index, unsigned char pos1, unsigned char pos2);
// pos1, pos2: 0b001xxxxx SIMD, 0b0001rrrr Normal register, 0b000001hh High Byte
// currently only supports normal registers
// index: number of instructions before the point where two swap instructions are inserted

Change build_vertical_change(std::span<Instruction> instructions, std::vector<InstructionSetExtension>& extensions, int instruction, int number_places_down);

// void horizontal_change(std::vector<Instruction>& instructions, int instruction, int operand, int new_register);

void apply_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, Change* change);
void undo_last_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, Change* change);


