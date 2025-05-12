#include <cassert>
#include "changes.h"
#include <algorithm>
const InstructionFootprint& footprint(const std::vector<InstructionSetExtension> extensions, const Instruction& instruction) {
	return extensions[instruction.extension].instructions[instruction.index];
}


struct TwoRegisterMove {
	bool pos1_moves_to_pos2;
	bool pos2_moves_to_pos1;
	unsigned long long pos1;
	unsigned long long pos2;
};

bool move_swap_up_through(std::vector<InstructionSetExtension>& extensions, Instruction& instruction, TwoRegisterMove& move) {
	// TODO if this itself is a swap/mov between the positions
	// TODO what about high bytes

	bool pos1_is_read = false;
	bool pos2_is_read = false;
	bool pos1_is_written = false;
	bool pos2_is_written = false;
	bool pos1_io = false;
	bool pos2_io = false;

	unsigned char mask = 0;
	if ((move.pos1 & 0xf8) == 0x10) mask |= 1 << (move.pos1 & 0x7);
	if ((move.pos2 & 0xf8) == 0x10) mask |= 1 << (move.pos2 & 0x7);
	if ((footprint(extensions, instruction).always_read_registers | footprint(extensions, instruction).always_written_registers) & mask) return false;
	

	for (int i = 0; i != 4; i++) {
		unsigned char op_head = footprint(extensions, instruction).operands[i].head;
		unsigned long long op = instruction.operands[i];
		if (op_head && (op_head & OF_TYPE) == OF_DEREF &&
			(op_head & OF_PTR_REGISTER | 0x10) == move.pos1 ||
				(op_head & OF_PTR_REGISTER | 0x10) == move.pos2) return false;
		
		if (!op) continue;

		if (op == move.pos1) {
			if ((op_head & OF_TYPE) == OF_OUT) pos1_is_written = true;
			if ((op_head & OF_TYPE) == OF_IN) pos1_is_read = true;
			if ((op_head & OF_TYPE) == OF_IO) pos1_io = true;

			if (OP_IS_REGISTER(move.pos2) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_REGISTER))) return false;
			if (OP_IS_HIGH_BYTE(move.pos2) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_BYTE))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos2) && OP_SIMD_REGISTER(move.pos2) < 16 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_LOW_SIMD))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos2) && OP_SIMD_REGISTER(move.pos2) > 15 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_SIMD))) return false;
		}
		if (op == move.pos2) {
			if ((op_head & OF_TYPE) == OF_OUT) pos2_is_written = true;
			if ((op_head & OF_TYPE) == OF_IN) pos2_is_read = true;
			if ((op_head & OF_TYPE) == OF_IO) pos2_io = true;

			if (OP_IS_REGISTER(move.pos1) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_REGISTER))) return false;
			if (OP_IS_HIGH_BYTE(move.pos1) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_BYTE))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos1) && OP_SIMD_REGISTER(move.pos1) < 16 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_LOW_SIMD))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos1) && OP_SIMD_REGISTER(move.pos1) > 15 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_SIMD))) return false;
		}
		if (OP_IS_MEMORY_LOCATION(op)) {
			if ((OP_MEMORY_BASE_REGISTER(op) < 16 && (OP_MEMORY_BASE_REGISTER(op) | 0x10) == move.pos1) ||
				(OP_MEMORY_INDEX_REGISTER(op) < 16 && (OP_MEMORY_INDEX_REGISTER(op) | 0x10) == move.pos1)) {

				pos1_is_read = true;

				if ((move.pos2 & 0xf0) != 0x10) return false;
			}
			if ((OP_MEMORY_BASE_REGISTER(op) < 16 && (OP_MEMORY_BASE_REGISTER(op) | 0x10) == move.pos2) ||
				(OP_MEMORY_INDEX_REGISTER(op) < 16 && (OP_MEMORY_INDEX_REGISTER(op) | 0x10) == move.pos2)) {

				pos2_is_read = true;

				if ((move.pos1 & 0xf0) != 0x10) return false;
			}
		}
	}
	pos1_is_written = pos1_is_written || !move.pos1_moves_to_pos2 && !pos1_io;
	pos2_is_written = pos2_is_written || !move.pos2_moves_to_pos1 && !pos2_io;
	if (pos1_is_written && pos2_is_written) { // B
		move.pos1_moves_to_pos2 = false;
		move.pos2_moves_to_pos1 = false;
	}
	else if (pos2_is_written) { // C
		move.pos1_moves_to_pos2 = true;
		move.pos2_moves_to_pos1 = false;
	}
	else if (pos1_is_written) { // D
		move.pos1_moves_to_pos2 = false;
		move.pos2_moves_to_pos1 = true;
	}
	else { // A
		move.pos1_moves_to_pos2 = true;
		move.pos2_moves_to_pos1 = true;
	}
	return true;
}
struct DownState {
	bool pos1_must_be_dead;
	bool pos2_must_be_dead;
	int change_pos_if_dead;
	unsigned long long pos1;
	unsigned long long pos2;
};
bool update_down_state(std::vector<InstructionSetExtension>& extensions, std::span<Instruction> instructions, int instruction_index, DownState& move) {
	// TODO if this itself is a swap/mov between the positions
	// TODO what about high bytes
	Instruction& instruction = instructions[instruction_index];

	bool pos1_is_read = false;
	bool pos2_is_read = false;
	bool pos1_is_written = false;
	bool pos2_is_written = false;
	bool pos1_io = false;
	bool pos2_io = false;

	unsigned char mask = 0;
	if ((move.pos1 & 0xf8) == 0x10) mask |= 1 << (move.pos1 & 0x7);
	if ((move.pos2 & 0xf8) == 0x10) mask |= 1 << (move.pos2 & 0x7);
	if ((footprint(extensions, instruction).always_read_registers | footprint(extensions, instruction).always_written_registers) & mask) return false;


	for (int i = 0; i != 4; i++) {
		unsigned char op_head = footprint(extensions, instruction).operands[i].head;
		unsigned long long op = instruction.operands[i];
		if (op_head && (op_head & OF_TYPE) == OF_DEREF &&
			(op_head & OF_PTR_REGISTER | 0x10) == move.pos1 ||
			(op_head & OF_PTR_REGISTER | 0x10) == move.pos2) return false;

		if (!op) continue;

		if (op == move.pos1) {
			if ((op_head & OF_TYPE) == OF_OUT) pos1_is_written = true;
			if ((op_head & OF_TYPE) == OF_IN) pos1_is_read = true;
			if ((op_head & OF_TYPE) == OF_IO) pos1_io = true;

			if (OP_IS_REGISTER(move.pos2) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_REGISTER))) return false;
			if (OP_IS_HIGH_BYTE(move.pos2) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_BYTE))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos2) && OP_SIMD_REGISTER(move.pos2) < 16 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_LOW_SIMD))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos2) && OP_SIMD_REGISTER(move.pos2) > 15 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_SIMD))) return false;
		}
		if (op == move.pos2) {
			if ((op_head & OF_TYPE) == OF_OUT) pos2_is_written = true;
			if ((op_head & OF_TYPE) == OF_IN) pos2_is_read = true;
			if ((op_head & OF_TYPE) == OF_IO) pos2_io = true;

			if (OP_IS_REGISTER(move.pos1) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_REGISTER))) return false;
			if (OP_IS_HIGH_BYTE(move.pos1) && ((op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_BYTE))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos1) && OP_SIMD_REGISTER(move.pos1) < 16 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_LOW_SIMD))) return false;
			if (OP_IS_SIMD_REGISTER(move.pos1) && OP_SIMD_REGISTER(move.pos1) > 15 && (!(op_head & OF_SIMD_REGISTER) || !(op_head & OF_CAN_BE_HIGH_SIMD))) return false;
		}
		if (OP_IS_MEMORY_LOCATION(op)) {
			if ((OP_MEMORY_BASE_REGISTER(op) < 16 && (OP_MEMORY_BASE_REGISTER(op) | 0x10) == move.pos1) ||
				(OP_MEMORY_INDEX_REGISTER(op) < 16 && (OP_MEMORY_INDEX_REGISTER(op) | 0x10) == move.pos1)) {

				pos1_is_read = true;

				if ((move.pos2 & 0xf0) != 0x10) return false;
			}
			if ((OP_MEMORY_BASE_REGISTER(op) < 16 && (OP_MEMORY_BASE_REGISTER(op) | 0x10) == move.pos2) ||
				(OP_MEMORY_INDEX_REGISTER(op) < 16 && (OP_MEMORY_INDEX_REGISTER(op) | 0x10) == move.pos2)) {

				pos2_is_read = true;

				if ((move.pos1 & 0xf0) != 0x10) return false;
			}
		}
	}
	if (!move.pos1_must_be_dead && !pos2_is_read) {
		move.pos2_must_be_dead = !pos2_is_written;
		if (!pos2_io) return true;
	}
	else if (!move.pos2_must_be_dead && !pos1_is_read) {
		move.pos1_must_be_dead = !pos1_is_written;
		if (!pos1_io) return true;
	}
	
	move.pos1_must_be_dead = !pos1_is_written || pos2_io;
	move.pos2_must_be_dead = !pos2_is_written || pos1_io;
	move.change_pos_if_dead = instruction_index;

	return true;
}

Change build_horizontal_change(std::span<Instruction> instructions, std::vector<InstructionSetExtension>& extensions, int index, unsigned char pos1, unsigned char pos2) {
	Change result_change {};
	RegisterSwap& result = result_change.horizontal;
	result = {
		.type = REGISTER_SWAP,
		.explicit_swap_at_start = 3, // 0 means no, 1 means mov pos1 -> pos2, 2 means mov pos2 -> pos1, 3 means swap
		.explicit_swap_at_end = 3,
		.first_instruction_affected = 0,
		.last_instruction_affected = (int)instructions.size()-1,
		.pos1 = (unsigned long long)pos1 | 0x10ull,
		.pos2 = (unsigned long long)pos2 | 0x10ull
	};
	TwoRegisterMove up_state = {
		.pos1_moves_to_pos2 = true,
		.pos2_moves_to_pos1 = true,
		.pos1 = pos1 | 0x10ull,
		.pos2 = pos2 | 0x10ull
	};
	for (int i = index - 1; i != -1; i--) {
		if (!move_swap_up_through(extensions, instructions[i], up_state)) {
			result.first_instruction_affected = i + 1;
			result.explicit_swap_at_start = (int)up_state.pos1_moves_to_pos2 + 2 * (int)up_state.pos2_moves_to_pos1;
			break;
		}
		if (!up_state.pos1_moves_to_pos2 && !up_state.pos2_moves_to_pos1) {
			result.first_instruction_affected = i;
			result.explicit_swap_at_start = 0;
			break;
		}
	}
	DownState down_state = {
		.pos1_must_be_dead = true,
		.pos2_must_be_dead = true,
		.change_pos_if_dead = index-1,
		.pos1 = pos1 | 0x10ull,
		.pos2 = pos2 | 0x10ull
	};
	for (int i = index; i != instructions.size(); i++) {
		if (!update_down_state(extensions, instructions, i, down_state)) {
			result.explicit_swap_at_end = 3;
			result.last_instruction_affected = down_state.change_pos_if_dead;
			break;
		}
		if (!down_state.pos1_must_be_dead && !down_state.pos2_must_be_dead) {
			result.explicit_swap_at_end = 0;
			result.last_instruction_affected = down_state.change_pos_if_dead;
			break;
		}
	}
	return result_change;
}

void swap_registers(Instruction& instruction, unsigned long long pos1, unsigned long long pos2) {
	unsigned long long pos1_reg = (pos1 & 0x10ull) ? pos1 & 0xfull : pos1 & 0x3ull;
	unsigned long long pos2_reg = (pos2 & 0x10ull) ? pos2 & 0xfull : pos2 & 0x3ull;
	for (int i = 0; i != 4; i++) if (unsigned long long& op = instruction.operands[i]) {
		if (OP_IS_HIGH_BYTE(op) && OP_HIGH_BYTE(op) == pos1_reg) op = pos2_reg | 0x4ull;
		else if (OP_IS_HIGH_BYTE(op) && OP_HIGH_BYTE(op) == pos2_reg) op = pos1_reg | 0x4ull;
		else if (OP_IS_REGISTER(op) && OP_REGISTER(op) == pos1_reg) op = pos2_reg | 0x10ull;
		else if (OP_IS_REGISTER(op) && OP_REGISTER(op) == pos2_reg) op = pos1_reg | 0x10ull;
		else if (OP_IS_MEMORY_LOCATION(op)) {
			// memory location	00000000 00000000 01rrrrri iiiissss oooooooo oooooooo oooooooo oooooooo
			if (OP_MEMORY_BASE_REGISTER(op) == pos1) op = op & ~0x00003e0000000000ull | (pos2 << 41);
			else if (OP_MEMORY_BASE_REGISTER(op) == pos2) op = op & ~0x00003e0000000000ull | (pos1 << 41);

			if (OP_MEMORY_INDEX_REGISTER(op) == pos1) op = op & ~0x000001f000000000ull | (pos2 << 36);
			else if (OP_MEMORY_INDEX_REGISTER(op) == pos2) op = op & 0x000001f000000000ull | (pos1 << 36);
		}
	}
}
void swap_out_registers(Instruction& instruction, InstructionFootprint& footprint, unsigned long long pos1, unsigned long long pos2) {
	unsigned long long pos1_reg = (pos1 & 0x10ull) ? pos1 & 0xfull : pos1 & 0x3ull;
	unsigned long long pos2_reg = (pos2 & 0x10ull) ? pos2 & 0xfull : pos2 & 0x3ull;
	for (int i = 0; i != 4; i++) if (unsigned long long& op = instruction.operands[i]) if ((footprint.operands[i].head & OF_TYPE) == OF_OUT) {
		if (OP_IS_HIGH_BYTE(op) && OP_HIGH_BYTE(op) == pos1_reg) op = pos2_reg | 0x4ull;
		else if (OP_IS_HIGH_BYTE(op) && OP_HIGH_BYTE(op) == pos2_reg) op = pos1_reg | 0x4ull;
		else if (OP_IS_REGISTER(op) && OP_REGISTER(op) == pos1_reg) op = pos2_reg | 0x10ull;
		else if (OP_IS_REGISTER(op) && OP_REGISTER(op) == pos2_reg) op = pos1_reg | 0x10ull;
		else if (OP_IS_MEMORY_LOCATION(op)) {
			// memory location	00000000 00000000 01rrrrri iiiissss oooooooo oooooooo oooooooo oooooooo
			if (OP_MEMORY_BASE_REGISTER(op) == pos1) op = op & ~0x00003e0000000000ull | (pos2 << 41);
			else if (OP_MEMORY_BASE_REGISTER(op) == pos2) op = op & ~0x00003e0000000000ull | (pos1 << 41);

			if (OP_MEMORY_INDEX_REGISTER(op) == pos1) op = op & ~0x000001f000000000ull | (pos2 << 36);
			else if (OP_MEMORY_INDEX_REGISTER(op) == pos2) op = op & 0x000001f000000000ull | (pos1 << 36);
		}
	}
}

void apply_horizontal_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, RegisterSwap& change) {
	for (int i = change.first_instruction_affected; i <= change.last_instruction_affected; i++) swap_registers(instructions[i], change.pos1, change.pos2);
	if (!change.explicit_swap_at_end) {
		Instruction& i = instructions[change.last_instruction_affected];
		swap_out_registers(i, extensions[i.extension].instructions[i.index], change.pos1, change.pos2);
	}
	else {
		instructions.emplace(instructions.cbegin() + change.last_instruction_affected + 1);
		if (change.explicit_swap_at_end == 1)
			load_instruction(std::string("MOV64 ") + REGISTER_NAMES[change.pos1 & 0xfull] + " " + REGISTER_NAMES[change.pos2 & 0xfull], 
				instructions[change.last_instruction_affected + 1], extensions);
		else if (change.explicit_swap_at_end == 2)
			load_instruction(std::string("MOV64 ") + REGISTER_NAMES[change.pos2 & 0xfull] + " " + REGISTER_NAMES[change.pos1 & 0xfull],
				instructions[change.last_instruction_affected + 1], extensions);
		else load_instruction(std::string("XCHG64 ") + REGISTER_NAMES[change.pos1 & 0xfull] + " " + REGISTER_NAMES[change.pos2 & 0xfull],
				instructions[change.last_instruction_affected + 1], extensions);

	}
	if (!change.explicit_swap_at_start) {
		Instruction& i = instructions[change.first_instruction_affected];
		swap_registers(i, change.pos1, change.pos2);
		swap_out_registers(i, extensions[i.extension].instructions[i.index], change.pos1, change.pos2);
	}
	else {
		instructions.emplace(instructions.cbegin() + change.first_instruction_affected);
		if (change.explicit_swap_at_start == 1)
			load_instruction(std::string("MOV64 ") + REGISTER_NAMES[change.pos1 & 0xfull] + " " + REGISTER_NAMES[change.pos2 & 0xfull],
				instructions[change.first_instruction_affected], extensions);
		else if (change.explicit_swap_at_start == 2)
			load_instruction(std::string("MOV64 ") + REGISTER_NAMES[change.pos2 & 0xfull] + " " + REGISTER_NAMES[change.pos1 & 0xfull],
				instructions[change.first_instruction_affected], extensions);
		else load_instruction(std::string("XCHG64 ") + REGISTER_NAMES[change.pos1 & 0xfull] + " " + REGISTER_NAMES[change.pos2 & 0xfull],
			instructions[change.first_instruction_affected], extensions);

	}

}
void undo_last_horizontal_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, RegisterSwap& change) {
	if (change.explicit_swap_at_start) instructions.erase(instructions.cbegin() + change.first_instruction_affected);
	else {
		Instruction& i = instructions[change.first_instruction_affected];
		swap_out_registers(i, extensions[i.extension].instructions[i.index], change.pos1, change.pos2);
		swap_registers(i, change.pos1, change.pos2);
	}
	if (change.explicit_swap_at_end) instructions.erase(instructions.cbegin() + change.last_instruction_affected + 1);
	else {
		Instruction& i = instructions[change.first_instruction_affected];
		swap_out_registers(i, extensions[i.extension].instructions[i.index], change.pos1, change.pos2);
	}
	for (int i = change.first_instruction_affected; i <= change.last_instruction_affected; i++) swap_registers(instructions[i], change.pos1, change.pos2);
}
unsigned long long read_places(InstructionFootprint& footprint, Instruction& instruction) {
	unsigned long long result = footprint.always_read_flags | (unsigned long long(footprint.always_read_registers) << 16);
	for (int i = 0; i != 4; i++) if (footprint.operands[i].head) {
		if ((footprint.operands[i].head & OF_TYPE) == OF_DEREF) {
			result |= 1ull << ((footprint.operands[i].head & OF_PTR_REGISTER) + 16);
			if ((footprint.operands[i].head & OF_DEREF_TYPE) != OF_OUT_DEREF) result |= 1 << 8;
		}
		if (OP_IS_REGISTER(instruction.operands[i]) && (footprint.operands[i].head & OF_TYPE) != OF_OUT)
			result |= 1ull << (OP_REGISTER(instruction.operands[i]) + 16);
		else if (OP_IS_HIGH_BYTE(instruction.operands[i]) && (footprint.operands[i].head & OF_TYPE) != OF_OUT)
			result |= 1ull << (OP_HIGH_BYTE(instruction.operands[i]) + 16);
		else if (OP_IS_SIMD_REGISTER(instruction.operands[i]) && (footprint.operands[i].head & OF_TYPE) != OF_OUT)
			result |= 1ull << (OP_SIMD_REGISTER(instruction.operands[i]) + 32);
		else if (OP_IS_RIP_RELATIVE(instruction.operands[i]) && (footprint.operands[i].head & OF_TYPE) != OF_OUT)
			result |= 1 << 8;
		else if (OP_IS_MEMORY_LOCATION(instruction.operands[i])) {
			if (OP_MEMORY_BASE_REGISTER(instruction.operands[i]) < 16) result |= 1ull << (OP_MEMORY_BASE_REGISTER(instruction.operands[i]) + 16);
			if (OP_MEMORY_INDEX_REGISTER(instruction.operands[i]) < 16) result |= 1ull << (OP_MEMORY_INDEX_REGISTER(instruction.operands[i]) + 16);
			if ((footprint.operands[i].head & OF_TYPE) != OF_OUT) result |= 1 << 8;
		}
	}
	return result;
}
unsigned long long written_places(InstructionFootprint& footprint, Instruction& instruction) {
	unsigned long long result = footprint.always_read_flags | (unsigned long long(footprint.always_read_registers) << 16);
	for (int i = 0; i != 4; i++) if (footprint.operands[i].head && (footprint.operands[i].head & OF_TYPE) != OF_IN) {
		if ((footprint.operands[i].head & OF_TYPE) == OF_DEREF && (footprint.operands[i].head & OF_DEREF_TYPE) != OF_IN_DEREF) result |= 1 << 8;
		else if (OP_IS_REGISTER(instruction.operands[i])) result |= 1ull << (OP_REGISTER(instruction.operands[i]) + 16);
		else if (OP_IS_HIGH_BYTE(instruction.operands[i])) result |= 1ull << (OP_HIGH_BYTE(instruction.operands[i]) + 16);
		else if (OP_IS_SIMD_REGISTER(instruction.operands[i])) result |= 1ull << (OP_SIMD_REGISTER(instruction.operands[i]) + 32);
		else if (OP_IS_RIP_RELATIVE(instruction.operands[i])) result |= 1 << 8;
		else if (OP_IS_MEMORY_LOCATION(instruction.operands[i])) result |= 1 << 8;
	}
	return result;
}
bool can_swap_instructions(std::vector<InstructionSetExtension>& extensions, Instruction& instruction_1, Instruction& instruction_2) {
	auto& footprint_1 = extensions[instruction_1.extension].instructions[instruction_1.index];
	auto& footprint_2 = extensions[instruction_2.extension].instructions[instruction_2.index];

	if (footprint_1.jump_type || footprint_2.jump_type) return false;
	if ((read_places(footprint_1, instruction_1) & written_places(footprint_2, instruction_2)) ||
		(read_places(footprint_2, instruction_2) & written_places(footprint_1, instruction_1))) return false;
	return true;
}
Change build_vertical_change(std::span<Instruction> instructions, std::vector<InstructionSetExtension>& extensions, int instruction, int number_places_down) {
	Change result_change{};
	InstructionReorder& result = result_change.vertical;
	result.type = INSTRUCTION_REORDER;
	result.instruction = instruction;
	result.number_places_down = number_places_down;
	if (number_places_down >= 0) {
		for (int i = 0; i != number_places_down; i++) {
			if (!can_swap_instructions(extensions, instructions[instruction], instructions[instruction + i + 1])) {
				result.number_places_down = i;
				break;
			}
		}
	}
	else {
		for (int i = 0; i != number_places_down; i--) {
			if (!can_swap_instructions(extensions, instructions[instruction], instructions[instruction + i - 1])) {
				result.number_places_down = i;
				break;
			}
		}
	}
	return result_change;
}
void apply_vertical_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, InstructionReorder& change) {
	if (change.number_places_down >= 0) {
		std::rotate(instructions.begin() + change.instruction, instructions.begin() + change.instruction + 1, instructions.begin() + change.instruction + change.number_places_down + 1);
	}
	else {
		std::rotate(instructions.begin() + change.instruction + change.number_places_down, instructions.begin() + change.instruction, instructions.begin() + change.instruction + 1);
	}
}
void undo_last_vertical_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, InstructionReorder& change) {
	if (change.number_places_down >= 0) {
		std::rotate(instructions.begin() + change.instruction, instructions.begin() + change.instruction + change.number_places_down, instructions.begin() + change.instruction + change.number_places_down + 1);
	}
	else {
		std::rotate(instructions.begin() + change.instruction + change.number_places_down, instructions.begin() + change.instruction + change.number_places_down + 1, instructions.begin() + change.instruction + 1);
	}
}


void apply_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, Change* change) {
	switch (change->type) {
	case REGISTER_SWAP: apply_horizontal_change(instructions, extensions, change->horizontal); break;
	case INSTRUCTION_REORDER: apply_vertical_change(instructions, extensions, change->vertical); break;
	default: assert(false);
	}
}
void undo_last_change(std::vector<Instruction>& instructions, std::vector<InstructionSetExtension>& extensions, Change* change) {
	switch (change->type) {
	case REGISTER_SWAP: undo_last_horizontal_change(instructions, extensions, change->horizontal); break;
	case INSTRUCTION_REORDER: undo_last_vertical_change(instructions, extensions, change->vertical); break;
	default: assert(false);
	}
}





// walk upwards from <index>, keeping track which of the two values are still alive