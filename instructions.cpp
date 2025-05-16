#include "instructions.h"
#include "ctre.hpp"
#include <cassert>
#include <format>

#define RX_REGISTER "r[a-d]x|r[sd]i|r[sb]p|r[89]|r1[0-5]|R[A-D]X|R[SD]I|R[SB]P|R[89]|R1[0-5]"
#define RX_SIMD_REGISTER "[xyz]mm[12][0-9]|[xyz]mm3[01]|[xyz]mm[0-9]|[XYZ]MM[12][0-9]|[XYZ]MM3[01]|[XYZ]MM[0-9]"

#define DBG(msg) (std::cout << msg << "\n")

template<typename T, int base>
T read_number(std::string_view n) {
	T result = 0;
	for (char digit : n) result = result * base + (digit - '0');
	return result;
}
unsigned char read_register(auto name) {
	if (name == "rax" || name == "RAX") return 0;
	if (name == "rbx" || name == "RBX") return 1;
	if (name == "rcx" || name == "RCX") return 2;
	if (name == "rdx" || name == "RDX") return 3;
	if (name == "rsi" || name == "RSI") return 4;
	if (name == "rdi" || name == "RDI") return 5;
	if (name == "rsp" || name == "RSP") return 6;
	if (name == "rbp" || name == "RBP") return 7;
	if (name == "r8" || name == "R8") return 8;
	if (name == "r9" || name == "R9") return 9;
	if (name == "r10" || name == "R10") return 10;
	if (name == "r11" || name == "R11") return 11;
	if (name == "r12" || name == "R12") return 12;
	if (name == "r13" || name == "R13") return 13;
	if (name == "r14" || name == "R14") return 14;
	if (name == "r15" || name == "R15") return 15;
	assert(false);
}
unsigned char read_simd_register(auto name) {
	if (name.size() == 4) return name[3] - '0';
	return 10 * (name[3] - '0') + (name[4] - '0');
}

OperandFootprint read_operand_footprint(std::string_view& source) {
	auto m = ctre::match<"\\s+(in|io|out|in[*]|io[*]|out[*])\\s+(.*)">(source);
	constexpr OperandFootprint err{ 0,0 };
	if (!m) { return err; }
	auto keyword = m.get<1>().to_view(); // in/io/out/in*/io*/out*
	auto rest = m.get<2>().to_view();
	if (keyword[keyword.size() - 1] != '*') {
		unsigned char head = (keyword=="in") ? OF_IN : (keyword=="io") ? OF_IO : OF_OUT;
		unsigned char size = 0;
		while (auto caps = ctre::match<"\\s*([rhxy]|i[124]|m[1-9][0-9]*)(.*)">(rest)) {
			rest = caps.get<2>().to_view();
			auto arg = caps.get<1>().to_view();
			switch (arg[0]) {
			case 'r': if (head & OF_SIMD_REGISTER) return err; head |= OF_CAN_BE_REGISTER; break;
			case 'h': if (head & OF_SIMD_REGISTER) return err; head |= OF_CAN_BE_HIGH_BYTE; break;
			case 'x': if (!(head & OF_SIMD_REGISTER) && (head & 0x18)) return err; head |= OF_CAN_BE_LOW_SIMD; break;
			case 'y': if (!(head & OF_SIMD_REGISTER) && (head & 0x18)) return err; head |= OF_CAN_BE_HIGH_SIMD; break;
			case 'i': if (head & OF_IMMEDIATE_SIZE) return err; head |= arg[1] - '0'; break;
			case 'm': if (size) return err; arg.remove_prefix(1); size = read_number<unsigned char,10>(arg); break;
			default: return err;
			}
		}
		source = rest;
		return OperandFootprint{ head, size };
	}
	else {
		unsigned char head = (keyword == "in*") ? OF_IN_DEREF : (keyword == "io*") ? OF_IO_DEREF : OF_OUT_DEREF;
		constexpr ctll::fixed_string regex = "(" RX_REGISTER ")\\s+([1-9][0-9]*)(.*)";
		auto m = ctre::match<regex>(rest);
		if (!m) return err;
		source = m.get<3>().to_view();
		head |= read_register(m.get<1>().to_view());
		return OperandFootprint{ head , read_number<unsigned char,10>(m.get<2>().to_view()) };
	}
	return OperandFootprint{};
}
InstructionSetExtension read_instruction_set_extension(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file) throw ParserError{ &filePath, -1 };
	std::string line;
	int linenr = 0;
	InstructionSetExtension ise{};
	while (std::getline(file, line)) {
		linenr++;
		if (ctre::match<"\\s*(#.*)?">(line)) continue;
		auto m = ctre::match<"\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s+"
			"([01]{8})\\s*0{8}\\s*([01]{7})\\s*([0-7])\\s*([01])\\s*"
			"([01]{8})\\s*0{8}\\s*([01]{7})\\s*([0-7])\\s*([01])(.*)">(line);
		if (!m) throw ParserError{ &filePath, linenr };
		if (m.get<4>().to_view() != m.get<8>().to_view()) throw ParserError{ &filePath, linenr };

		InstructionFootprint inst{ 
			.always_read_registers = read_number<unsigned char, 2>(m.get<2>().to_view()),
			.always_read_flags = unsigned char((read_number<unsigned char, 2>(m.get<3>().to_view()) << 1) + (m.get<5>().to_view()[0] - '0')) ,
			.jump_type = unsigned char(m.get<4>().to_view()[0] - '0'),
			.always_written_registers = read_number<unsigned char, 2>(m.get<6>().to_view()),
			.always_written_flags = unsigned char((read_number<unsigned char, 2>(m.get<7>().to_view()) << 1) + (m.get<9>().to_view()[0] - '0')) ,
			.operands = {0,0,0,0},
			.name = unsigned int(ise.names.size())};
		for (auto c : m.get<1>().to_view()) ise.names.push_back(c);
		ise.names.push_back('\0');
		auto rest = m.get<10>().to_view();
		for (int i = 0; i != 4; i++) {
			auto op = read_operand_footprint(rest);
			if (op.head) inst.operands[i] = op; else break;
		}
		if (!ctre::match<"\\s*(#.*)?">(rest)) throw ParserError{ &filePath, linenr };
		ise.instructions.push_back(inst);
	}
	return ise;
}

std::string print_instruction_footprint(std::vector<InstructionSetExtension>& ises, int ext, int inst) {
	InstructionFootprint footprint = ises[ext].instructions[inst];
	char* name = &ises[ext].names[footprint.name];
	std::string result = name;
	result += " ";
	for (int i = 0; i != 8; i++) result += (footprint.always_read_registers & (1 << i)) ? "1" : "0";
	result += "00000000 ";
	for (int i = 7; i != 0; i--) result += (footprint.always_read_flags & (1 << i)) ? "1" : "0";
	result += " ";
	result += footprint.jump_type;
	result += " ";
	result += (footprint.always_read_flags & 1) ? "1" : "0";
	result += " ";
	for (int i = 0; i != 8; i++) result += (footprint.always_written_registers & (1 << i)) ? "1" : "0";
	result += "00000000 ";
	for (int i = 7; i != 0; i--) result += (footprint.always_written_flags & (1 << i)) ? "1" : "0";
	result += " ";
	result += footprint.jump_type;
	result += " ";
	result += (footprint.always_written_flags & 1) ? "1" : "0";
	result += " ...";
	return result;
}

constexpr auto HEX = "0123456789ABCDEF";
std::string print_instruction(const Instruction& inst, const std::vector<InstructionSetExtension>& ises) {
	auto& footprint = ises[inst.extension].instructions[inst.index];
	std::string result { &ises[inst.extension].names[footprint.name] };
	for (int i = 0; i != 4; i++) if (auto op = inst.operands[i]) {
		if (OP_IS_HIGH_BYTE(op)) result += HIGH_BYTE_NAMES[OP_HIGH_BYTE(op)];
		else if (OP_IS_REGISTER(op)) { result += " "; result += REGISTER_NAMES[OP_REGISTER(op)]; }
		else if (OP_IS_IMMEDIATE(op)) {
			result += " 0x";
			for (int j = 28; j >= 0; j -= 4) result += HEX[(op >> j) & 0xf];
		}
		else if (OP_IS_RIP_RELATIVE(op)) {
			result += std::format(" *RIP[{}]", int(OP_RIP_RELATIVE(op)));
			for (int j = 28; j >= 0; j -= 4) result += HEX[(op >> j) & 0xf];
			result += "]";
		}
		else if (OP_IS_SIMD_REGISTER(op)) {
			result += " XMM";
			if (OP_SIMD_REGISTER(op) > 9) result += '0' + OP_SIMD_REGISTER(op) / 10;
			result += '0' + (OP_SIMD_REGISTER(op) % 10);
		}
		else if (OP_IS_MEMORY_LOCATION(op))
			if (OP_MEMORY_SCALE(op)) result += std::format(" *{}[{}{}{:+}]", REGISTER_NAMES[OP_MEMORY_BASE_REGISTER(op)], OP_MEMORY_SCALE(op), REGISTER_NAMES[OP_MEMORY_INDEX_REGISTER(op)], OP_MEMORY_OFFSET(op));
			else result += std::format(" *{}[{}]", REGISTER_NAMES[OP_MEMORY_BASE_REGISTER(op)], OP_MEMORY_OFFSET(op));
		else {
			DBG("op: " << std::hex << op << std::dec);
			assert(false);
		}
	}
	return result + std::format(" ({},{})", inst.extension, inst.index);
}

bool instruction_is_valid(const Instruction& inst, const std::vector<InstructionSetExtension>& ises) {
	auto& footprint = ises[inst.extension].instructions[inst.index];
	for (int i = 0; i != 4; i++) {
		auto ofp = footprint.operands[i];
		auto op = inst.operands[i];
		if ((op == 0) != ((ofp.head & OF_TYPE) == OF_DEREF || ofp.head == 0)) return false;
		if (op == 0) continue;
		if (OP_IS_HIGH_BYTE(op) && ofp.head & OF_SIMD_REGISTER
		|| (OP_IS_HIGH_BYTE(op) && !(ofp.head & OF_CAN_BE_HIGH_BYTE))
		|| (OP_IS_IMMEDIATE(op) && (ofp.head & OF_IMMEDIATE_SIZE) == 0)
		|| (OP_IS_IMMEDIATE(op) && (OP_IMMEDIATE(op) >> ((ofp.head & OF_IMMEDIATE_SIZE) * 8)))
		|| (OP_IS_MEMORY_LOCATION(op) && ofp.memory_size == 0)
		|| (OP_IS_RIP_RELATIVE(op) && ofp.memory_size == 0)
		|| (OP_IS_REGISTER(op) && ofp.head & OF_SIMD_REGISTER)
		|| (OP_IS_REGISTER(op) && !(ofp.head & OF_CAN_BE_REGISTER))
		|| (OP_IS_SIMD_REGISTER(op) && OP_SIMD_REGISTER(op) < 16 && (ofp.head & OF_CAN_BE_LOW_SIMD) != OF_CAN_BE_LOW_SIMD)
		|| (OP_IS_SIMD_REGISTER(op) && OP_SIMD_REGISTER(op) > 15 && (ofp.head & OF_CAN_BE_HIGH_SIMD) != OF_CAN_BE_HIGH_SIMD))
			return false;
	}
	return true;

}
unsigned long long read_operand(std::string_view& source) {
	// " (0,0)"
	auto match = ctre::match<"\\s+(.+)">(source);
	if (!match) return 0;
	auto rest = match.get<1>().to_view();
	switch (rest[0]) {
	case 'X': case 'x': case 'Y': case 'y': case 'Z': case 'z': {
		auto m = ctre::match<"(" RX_SIMD_REGISTER ")(.*)">(rest);
		if (!m) return 0;
		source = m.get<2>().to_view();
		return read_simd_register(m.get<1>().to_view()) | 0x20;
	}
	case 'r': case 'R': {
		auto m = ctre::match<"(" RX_REGISTER ")(.*)">(rest);
		if (!m) return 0;
		source = m.get<2>().to_view();
		return read_register(m.get<1>().to_view()) | 0x10;
	}
	case '*': {
		auto m = ctre::match<
			"\\*("
			RX_REGISTER
			"|rip|RIP)?\\[\\s*(([124])\\s*\\*?\\s*("
			RX_REGISTER
			"))?\\s*\\+?\\s*(\\-)?\\s*([0-9]+)?\\s*\\](.*)">(rest);
		if (!m) return 0;
		source = m.get<7>().to_view();
		if (m.get<1>().to_view() == "rip" || m.get<1>().to_view() == "RIP") {
			if (m.get<2>().to_view().size()) return 0;
			auto x = read_number<unsigned int, 10>(m.get<6>().to_view());
			return 0x300000000 | ((m.get<5>()) ? ~x : x);
		}
		unsigned long long result = 0x400000000000;
		if (m.get<1>()) result |= unsigned long long(read_register(m.get<1>().to_view())) << 41;
		else result |= 0x10ull << 41;
		if (m.get<2>()) {
			result |= unsigned long long((m.get<3>().to_view()[0] - '0')) << 32;
			result |= unsigned long long(read_register(m.get<4>().to_view())) << 36;
		}
		else result |= 0x10ull << 36;
		auto offset = read_number<unsigned int, 10>(m.get<6>().to_view());
		result |= (m.get<5>()) ? ~offset : offset;
		return result;
	}
	case '0': if (auto m = ctre::match<"0[xX]([0-9a-fA-F]{1,8})(.*)">(rest)) {
		unsigned long long result = 0;
		for (char d : m.get<1>().to_view())
			if ('0' <= d && d <= '9') result = (result << 4) | (d - '0');
			else if ('a' <= d && d <= 'f') result = (result << 4) | (d - 'a' + 10);
			else result = (result << 4) | (d - 'A' + 10);
		source = m.get<2>().to_view();
		return result | 0x200000000;
	} [[fallthrough]];
	default:
		auto m = ctre::match<"([\\+\\-])?([0-9]+)(.*)">(rest);
		if (!m) return 0;
		source = m.get<3>().to_view();
		auto x = read_number<unsigned int, 10>(m.get<2>().to_view());
		return 0x200000000 | ((m.get<1>().to_view() == "-") ? ~x : x);
	}
}
bool load_instruction(std::string_view source, Instruction& inst, const std::vector<InstructionSetExtension>& ises) {
	auto m = ctre::match<"\\s*([[:alpha:]_][[:alpha:]0-9_]*)(\\s+.*)">(source);
	if (!m) { 
		DBG("did not match initial regex ( identifier )");
		return false;
	}
	auto name = m.get<1>().to_view();
	auto rest = m.get<2>().to_view();
	unsigned long long ops[4] = { 0,0,0,0 };
	for (int i = 0; i != 4; i++) {
		// DBG("left: '" << rest << "'\n");
		if (auto op = read_operand(rest)) {
			ops[i] = op;
		}
		else break;
	}
	auto m1 = ctre::match<"(\\s+\\(\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*\\))?\\s*(#.*)?">(rest);
	if (!m1) {
		DBG("did not match rest of line regex ( (int,int) #...)");
		DBG("'" << rest << "'");
		return false;
	}
	unsigned char ext;
	unsigned int idx;
	if (m1.get<1>()) {
		ext = read_number<unsigned char, 10>(m1.get<2>().to_view());
		idx = read_number<unsigned int, 10>(m1.get<3>().to_view());
		if (&ises[ext].names[ises[ext].instructions[idx].name] != name) return false;
	}
	else {
		bool found = false;
		for (int e = 0; e != ises.size(); e++) 
			for (int i = 0; i != ises[e].instructions.size(); i++) if (name == &ises[e].names[ises[e].instructions[i].name]) {
				DBG("possible hit: " << (&ises[e].names[ises[e].instructions[i].name]) << " (" << e << "," << i << ")");
				int j = 0;
				bool hit = true;
				for (auto op : ises[e].instructions[i].operands) if (op.head != 0 && (op.head & OF_TYPE) != OF_DEREF) {
					if (!ops[j]) hit = false;
					if (OP_IS_HIGH_BYTE(ops[j]) && op.head & OF_SIMD_REGISTER
						|| (OP_IS_HIGH_BYTE(ops[j]) && !(op.head & OF_CAN_BE_HIGH_BYTE))
						|| (OP_IS_IMMEDIATE(ops[j]) && (op.head & OF_IMMEDIATE_SIZE) == 0)
						|| (OP_IS_IMMEDIATE(ops[j]) && (OP_IMMEDIATE(ops[j]) >> ((op.head & OF_IMMEDIATE_SIZE) * 8)))
						|| (OP_IS_MEMORY_LOCATION(ops[j]) && op.memory_size == 0)
						|| (OP_IS_RIP_RELATIVE(ops[j]) && op.memory_size == 0)
						|| (OP_IS_REGISTER(ops[j]) && op.head & OF_SIMD_REGISTER)
						|| (OP_IS_REGISTER(ops[j]) && !(op.head & OF_CAN_BE_REGISTER))
						|| (OP_IS_SIMD_REGISTER(ops[j]) && OP_SIMD_REGISTER(ops[j]) < 16 && (op.head & OF_CAN_BE_LOW_SIMD) != OF_CAN_BE_LOW_SIMD)
						|| (OP_IS_SIMD_REGISTER(ops[j]) && OP_SIMD_REGISTER(ops[j]) > 15 && (op.head & OF_CAN_BE_HIGH_SIMD) != OF_CAN_BE_HIGH_SIMD))
						hit = false;
					j++;
				}
				if (ops[j]) hit = false;
				if (hit) {
					if (!found) { ext = e; idx = i; }
					else return false;
				}
			}
	}
	inst.extension = ext;
	inst.index = idx;
	auto& footprint = ises[ext].instructions[idx];
	for (int i = 0, j = 0; i != 4; i++)
		if ((footprint.operands[i].head & OF_TYPE) != OF_DEREF) inst.operands[i] = ops[j++];
		else inst.operands[i] = 0;
	if (!instruction_is_valid(inst, ises)) {
		DBG("produced invalid instruction");
		DBG(print_instruction(inst, ises));
		return false;
	}
	return true;
}

std::vector<Instruction> load_instructions(std::string_view source, const std::vector<InstructionSetExtension>& extensions) {
	DBG("loading instructions from: \"" << source << "\"");
	std::vector<Instruction> result{};
	while (auto match = ctre::match<"(\\s*(#[^\n]*)?\n)*([^\n]*)(.*)">(source)) {
		source = match.get<4>().to_view();
		DBG("line: \"" << match.get<3>().to_view() << "\"");
		std::string_view line = match.get<3>().to_view();
		if (!line.size()) return result;
		result.push_back({});
		if (!load_instruction(line, result[result.size() - 1], extensions)) return {};
	}
	return {};
}

void test_stuff() {
	
}