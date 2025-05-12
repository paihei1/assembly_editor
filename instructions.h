#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <format>

#define CF 128
#define PF 64
#define AF 32
#define ZF 16
#define SF 8
#define DF 4
#define OF 2
#define UNTRACKED_STATE 1

#define NO_JUMP 0
#define UNCONDITIONAL_RELATIVE 1
#define UNCONDITIONAL_ABSOLUTE 2
#define UNCONDITIONAL_FAR 3
#define CONDITIONAL_RELATIVE 4
#define CONDITIONAL_ABSOLUTE 5
#define CONDITIONAL_FAR 6


#define OF_TYPE 0b11000000
// switch (of.head & OF_TYPE) {...}
#define OF_DEREF 0b00000000
#define OF_IN 0b01000000
#define OF_OUT 0b10000000
#define OF_IO 0b11000000

#define OF_DEREF_TYPE 0b11110000
#define OF_IN_DEREF 0b00010000
#define OF_IO_DEREF 0b00100000
#define OF_OUT_DEREF 0b00110000
#define OF_PTR_REGISTER 0b00000111

#define OF_SIMD_REGISTER 0b00100000
// if not SIMD_REGISTER:
#define OF_CAN_BE_REGISTER 0b00010000
#define OF_CAN_BE_HIGH_BYTE 0b00001000
// if SIMD_REGISTER:
#define OF_CAN_BE_LOW_SIMD 0b00110000
#define OF_CAN_BE_HIGH_SIMD 0b00101000

#define OF_IMMEDIATE_SIZE 0b00000111


struct OperandFootprint {
	unsigned char head;
	unsigned char memory_size;
};

struct InstructionFootprint {
	unsigned char always_read_registers;
	unsigned char always_read_flags;
	unsigned char jump_type;
	unsigned char always_written_registers;
	unsigned char always_written_flags;
	OperandFootprint operands[4];
	unsigned int name;
};

struct InstructionSetExtension {
	std::vector<char> names{};
	std::vector<InstructionFootprint> instructions{};
};
struct ParserError : std::exception {
	const std::string* filePath{};
	int linenr{};
	ParserError(const std::string* path, int nr) : filePath(path), linenr(nr) {};
	const char* what() {
		char* result = new char[100 + filePath->size()];
		if (linenr < 0) sprintf_s(result, 100 + filePath->size(), "could not open ISA-Extension file %s.", filePath->c_str());
		else			sprintf_s(result, 100 + filePath->size(), "error parsing ISA-Extension file %s on line %i.", filePath->c_str(), linenr);
		return result;
	}
};
InstructionSetExtension read_instruction_set_extension(const std::string& filePath);

/*
	immediate value	32			00000000 00000000 00000000 00000010 iiiiiiii iiiiiiii iiiiiiii iiiiiiii
	register		4			00000000 00000000 00000000 00000000 00000000 00000000 00000000 0001rrrr
	high byte		2			00000000 00000000 00000000 00000000 00000000 00000000 00000000 000001rr
	SIMD register	5			00000000 00000000 00000000 00000000 00000000 00000000 00000000 001rrrrr
	memory location	5 4 5 32	00000000 00000000 01rrrrri iiiissss oooooooo oooooooo oooooooo oooooooo
	RIP relative	32			00000000 00000000 00000000 00000011 oooooooo oooooooo oooooooo oooooooo
*/
#define OP_IS_IMMEDIATE(x) ((x & 0xffffffff00000000) == 0x0000000200000000)
#define OP_IMMEDIATE(x) (x & 0x00000000ffffffff)
#define OP_IS_REGISTER(x) ((x & 0xfffffffffffffff0) == 0x10)
#define OP_REGISTER(x) (x & 0xf)
#define OP_IS_HIGH_BYTE(x) ((x & 0xfffffffffffffffc) == 4)
#define OP_HIGH_BYTE(x) (x & 3)
#define OP_IS_SIMD_REGISTER(x) ((x & 0xffffffffffffffc0) == 0x20)
#define OP_SIMD_REGISTER(x) (x & 0x1f)
#define OP_IS_RIP_RELATIVE(x) ((x & 0xffffffff00000000) == 0x0000000300000000)
#define OP_RIP_RELATIVE(x) (x & 0x00000000ffffffff)
#define OP_IS_MEMORY_LOCATION(x) ((x & 0x0000c00000000000) == 0x0000400000000000)
#define OP_MEMORY_BASE_REGISTER(x) ((x >> 41) & 0x1f) 
// >15 means no base
#define OP_MEMORY_INDEX_REGISTER(x) ((x >> 36) & 0x1f)
// >15 means no index / scale
#define OP_MEMORY_SCALE(x) ((x >> 32) & 0x0f)
#define OP_MEMORY_OFFSET(x) (x & 0x00000000ffffffff)
constexpr char REGISTER_NAMES[16][4] = { {"RAX"},{"RBX"},{"RCX"},{"RDX"},{"RSI"},{"RDI"},{"RSP"},{"RBP"},{"R8"},{"R9"},{"R10"},{"R11"},{"R12"},{"R13"},{"R14"},{"R15"} };
constexpr char HIGH_BYTE_NAMES[4][3] = { {"AH"},{"BH"},{"CH"},{"DH"} };

struct Instruction {
	unsigned char extension;
	unsigned int index;
	unsigned long long operands[4];
};

std::string print_instruction(const Instruction&, const std::vector<InstructionSetExtension>&);
bool load_instruction(std::string_view, Instruction&, const std::vector<InstructionSetExtension>&);
std::vector<Instruction> load_instructions(std::string_view, const std::vector<InstructionSetExtension>&);
void test_stuff();
std::string print_instruction_footprint(std::vector<InstructionSetExtension>& ises, int ext, int inst);