#include "scriptdecompiler.h"

#include <fstream>
#include <iomanip>
#include <map>
#include <set>

#include "../util/binaryreader.h"

#include "script.h"
#include "../engine/engine.h"

std::vector<std::string> ScriptDecompiler::functionNames_ = {
	"command_00", "command_01", "command_02", "command_03", "command_04", "command_05", "command_06", "command_07", "command_08", "command_09", "command_0A", "command_0B", "command_0C", "command_0D", "command_0E", "command_0F",
	"command_10", "command_11", "command_12", "command_13", "command_14", "command_15", "command_16", "command_17", "command_18", "command_19", "command_1A", "command_1B", "command_1C", "command_1D", "command_1E", "command_1F",
	"command_20", "command_21", "command_22", "command_23", "command_24", "command_25", "command_26", "command_27", "command_28", "command_29", "command_2A", "command_2B", "command_2C", "command_2D", "command_2E", "command_2F",
	"command_30", "command_31", "command_32", "command_33", "command_34", "command_35", "command_36", "command_37", "command_38", "command_39", "command_3A", "command_3B", "command_3C", "command_3D", "command_3E", "command_3F",
	"command_40", "command_41", "command_42", "command_43", "command_44", "command_45", "jump_if", "jump", "call", "return", "branch_on_variable", "command_4B", "command_4C", "command_4D", "command_4E", "command_4F",
	"command_50", "command_51", "command_52", "command_53", "command_54", "command_55", "command_56", "command_57", "command_58", "command_59", "command_5A", "command_5B", "command_5C", "command_5D", "command_5E", "command_5F",
	"command_60", "command_61", "command_62", "command_63", "command_64", "command_65", "command_66", "command_67", "command_68", "command_69", "command_6A", "command_6B", "command_6C", "command_6D", "command_6E", "command_6F",
	"command_70", "command_71", "command_72", "command_73", "command_74", "command_75", "command_76", "command_77", "command_78", "command_79", "command_7A", "command_7B", "command_7C", "command_7D", "command_7E", "command_7F",
	"command_80", "command_81", "command_82", "command_83", "command_84", "command_85", "display_text", "command_87", "command_88", "command_89", "command_8A", "command_8B", "show_choices", "command_8D", "command_8E", "command_8F",
	"command_90", "command_91", "command_92", "command_93", "command_94", "command_95", "command_96", "command_97", "command_98", "command_99", "command_9A", "command_9B", "command_9C", "command_9D", "command_9E", "command_9F",
	"command_A0", "command_A1", "command_A2", "command_A3", "command_A4", "command_A5", "command_A6", "command_A7", "command_A8", "command_A9", "command_AA", "command_AB", "command_AC", "command_AD", "command_AE", "command_AF",
	"command_B0", "command_B1", "command_B2", "command_B3", "command_B4", "command_B5", "command_B6", "command_B7", "command_B8", "command_B9", "command_BA", "command_BB", "command_BC", "command_BD", "command_BE", "command_BF",
	"command_C0", "display_image", "command_C2", "command_C3", "command_C4", "command_C5", "command_C6", "command_C7", "command_C8", "command_C9", "command_CA", "command_CB", "command_CC", "command_CD", "command_CE", "command_CF",
	"command_D0", "command_D1", "command_D2", "command_D3", "command_D4", "command_D5", "command_D6", "command_D7", "command_D8", "command_D9", "command_DA", "command_DB", "command_DC", "command_DD", "command_DE", "command_DF",
	"command_E0", "command_E1", "command_E2", "command_E3", "command_E4", "command_E5", "command_E6", "command_E7", "command_E8", "command_E9", "command_EA", "command_EB", "command_EC", "command_ED", "command_EE", "command_EF",
	"command_F0", "command_F1", "command_F2", "command_F3", "command_F4", "command_F5", "command_F6", "command_F7", "command_F8", "command_F9", "command_FA", "command_FB", "command_FC", "command_FD", "command_FE", "command_FF"
};

std::vector<SDCommand> ScriptDecompiler::commands_;

ScriptDecompiler::ScriptDecompiler(Script &script) : script_(script) {
	setup();
}

void ScriptDecompiler::setup() {
	commands_.resize(0x100);
	for (auto &cmd : commands_) {
		cmd.opcode = -1;
	}
	commands_[0x01] = { 0x01, { arg(SDType::UInt8) } }; // ???
	commands_[0x40] = { 0x40, { arg(SDType::Bytes, 3) } }; // ???
	commands_[0x41] = { 0x41, {} }; // special case
	commands_[0x42] = { 0x42, { arg(SDType::UInt16), arg(SDType::Bytes, 1), arg(SDType::UInt16), arg(SDType::Bytes, 1), arg(SDType::UInt16), arg(SDType::UInt16), arg(SDType::UInt16), arg(SDType::Bytes, 2) } };
	commands_[0x46] = { 0x46, { arg(SDType::UInt8), arg(SDType::UInt16), arg(SDType::Int16), arg(SDType::JumpAddr) } };
	commands_[0x47] = { 0x47, { arg(SDType::JumpAddr) } };
	commands_[0x48] = { 0x48, { arg(SDType::JumpAddr) } };
	commands_[0x49] = { 0x49, {} };
	commands_[0x4A] = { 0x4A, {} }; // special case
	commands_[0x4D] = { 0x4D, { arg(SDType::UInt8), arg(SDType::Int16) } };
	commands_[0x4E] = { 0x4E, {} }; // special case
	commands_[0x64] = { 0x64, { arg(SDType::Bytes, 1) } }; // ???
	commands_[0x80] = { 0x80, {} }; // special case
	commands_[0x82] = { 0x82, { arg(SDType::UInt16) } };
	commands_[0x83] = { 0x83, { arg(SDType::Bytes, 2) } }; // special case
	commands_[0x85] = { 0x85, { arg(SDType::Bytes, 4) } };
	commands_[0x86] = { 0x86, { arg(SDType::UInt16), arg(SDType::Bytes, 2), arg(SDType::String16) } };
	commands_[0x87] = { 0x87, { arg(SDType::Int16) } };
	commands_[0x88] = { 0x88, {} };
	commands_[0x89] = { 0x89, {} };
	commands_[0x8C] = { 0x8C, { arg(SDType::Bytes, 4), arg(SDType::UInt16), arg(SDType::Bytes, 2), arg(SDType::String8), arg(SDType::String8) } };
	commands_[0x8D] = { 0x8D, {} }; // special case?
	commands_[0x8E] = { 0x8E, {} };
	commands_[0x9C] = { 0x9C, { arg(SDType::UInt32), arg(SDType::UInt32) } };
	commands_[0x9D] = { 0x9D, { arg(SDType::Bytes, 2) } };
	commands_[0xA0] = { 0xA0, { arg(SDType::Bytes, 10) } }; // special case?
	commands_[0xA1] = { 0xA1, { arg(SDType::Bytes, 4) } };
	commands_[0xA2] = { 0xA2, { arg(SDType::Bytes, 2) } };
	commands_[0xA3] = { 0xA3, { arg(SDType::Bytes, 6) } };
	commands_[0xA4] = { 0xA4, { arg(SDType::Bytes, 7) } };
	commands_[0xA6] = { 0xA6, { arg(SDType::Bytes, 4) } };
	commands_[0xB0] = { 0xB0, { arg(SDType::Bytes, 2), arg(SDType::String8) } };
	commands_[0xB1] = { 0xB1, { arg(SDType::Bytes, 3) } };
	commands_[0xB3] = { 0xB3, {} };
	commands_[0xB4] = { 0xB4, {} };
	commands_[0xB6] = { 0xB6, {} };
	commands_[0xB9] = { 0xB9, {} }; // special case
	commands_[0xBD] = { 0xB3, { arg(SDType::Bytes, 3) } };
	commands_[0xBE] = { 0xBE, { arg(SDType::Bytes, 2) } };
	commands_[0xBF] = { 0xBF, { arg(SDType::Bytes, 4) } };
	commands_[0xC1] = { 0xC1, { arg(SDType::UInt16), arg(SDType::UInt16), arg(SDType::UInt8) } }; // special case
	commands_[0xC2] = { 0xC2, {} }; // special case
	commands_[0xC3] = { 0xC3, { arg(SDType::Bytes, 4) } };
	commands_[0xC4] = { 0xC4, { arg(SDType::Bytes, 4) } };
	commands_[0xC5] = { 0xC5, { arg(SDType::Bytes, 2) } };
	commands_[0xC6] = { 0xC6, {} };
	commands_[0xC7] = { 0xC7, { arg(SDType::Bytes, 5) } };
	commands_[0xC9] = { 0xC9, {} };
	commands_[0xCA] = { 0xCA, {} }; // special case
	commands_[0xCB] = { 0xCB, { arg(SDType::Bytes, 2) } };
	commands_[0xD2] = { 0xD2, { arg(SDType::Bytes, 1) } };

	specialCases_.resize(0x100, nullptr);
	specialCases_[0x41] = &ScriptDecompiler::command_41;
	specialCases_[0x4A] = &ScriptDecompiler::command_4A;
	specialCases_[0x4E] = &ScriptDecompiler::command_4E;
	specialCases_[0x80] = &ScriptDecompiler::command_80;
	specialCases_[0x83] = &ScriptDecompiler::command_83;
	specialCases_[0x8D] = &ScriptDecompiler::command_8D;
	specialCases_[0xA0] = &ScriptDecompiler::command_A0;
	specialCases_[0xB9] = &ScriptDecompiler::command_B9;
	specialCases_[0xC1] = &ScriptDecompiler::display_image;
	specialCases_[0xC2] = &ScriptDecompiler::command_C2;
	specialCases_[0xCA] = &ScriptDecompiler::command_CA;
}

void ScriptDecompiler::decompile(const std::string &path, const std::vector<unsigned char> &data, uint32_t scriptOffset) {
	std::ofstream ofs;
	ofs.open(Engine::game + "_" + path + ".src");
	BinaryReader br((char *)data.data(), data.size());

	br.seekg(scriptOffset);

	std::set<uint32_t> jumps;
	std::map<uint32_t, std::string> lines;
	
	while (br.tellg() < data.size()) {
		auto offset = br.tellg();
		auto opcode = br.read<uint8_t>();
		const auto &cmd = commands_[opcode];
		FuncInfo funcInfo;
		try {
			if (specialCases_[opcode]) {
				auto func = specialCases_[opcode];
				funcInfo = (this->*func)(cmd, br);
			} else {
				funcInfo = buildFunction(cmd, br);
			}
		} catch (UnimplementedOpcodeError &) {
			std::stringstream ss;
			ss << "// ERROR: Undefined opcode 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)opcode << std::dec << ", aborting.\n\n";
			br.skip(-1);
			for (int i = 0; i < 6; ++i) {
				for (int j = 0; j < 0x10; ++j) {
					ss << std::hex << std::setw(2) << std::setfill('0') << (int)br.read<uint8_t>() << (j == 0xf ? "\n" : " ") << std::dec;
				}
			}
			lines.insert({ offset, ss.str() });
			break;
		}
		lines.insert({ offset, funcInfo.line });
		for (auto &jump : funcInfo.jumps) {
			jumps.insert(jump);
		}
	}

	for (const auto &line : lines) {
		auto jump = jumps.find(line.first);
		if (jump != jumps.cend()) {
			ofs << std::hex << std::setw(8) << std::setfill('0') << *jump << std::dec << ":\n";
		}
		ofs << "  " << line.second << '\n';
	}
	ofs.close();
}

std::string ScriptDecompiler::getFunctionLine(BinaryReader &br) const {
	auto curPos = br.tellg();
	auto opcode = br.read<uint8_t>();
	const auto &cmd = commands_[opcode];
	FuncInfo funcInfo;
	try {
		if (specialCases_[opcode]) {
			auto func = specialCases_[opcode];
			funcInfo = (this->*func)(cmd, br);
		} else {
			funcInfo = buildFunction(cmd, br);
		}
	}
	catch (UnimplementedOpcodeError &) {
		std::stringstream ss;
		ss << "// ERROR: Undefined opcode 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)opcode << std::dec << ", aborting.\n\n";
		br.skip(-1);
		for (int i = 0; i < 6; ++i) {
			for (int j = 0; j < 0x10; ++j) {
				ss << std::hex << std::setw(2) << std::setfill('0') << (int)br.read<uint8_t>() << (j == 0xf ? "\n" : " ") << std::dec;
			}
		}
		br.seekg(curPos);
		return ss.str();
	}
	br.seekg(curPos);
	return funcInfo.line;
}

FuncInfo ScriptDecompiler::buildFunction(const SDCommand &cmd, BinaryReader &br) const {
	if (cmd.opcode == -1) {
		throw UnimplementedOpcodeError();
	}
	FuncInfo fi;

	const auto &functionName = getName(cmd.opcode);
	std::stringstream ss;
	ss << functionName << '(';
	for (int i = 0; i < cmd.arguments.size(); ++i) {
		const auto &arg = cmd.arguments[i];
		if (arg.type == SDType::JumpAddr) {
			auto offset = br.read<uint32_t>();
			br.skip(-4);
			fi.jumps.push_back(offset);
		}
		ss << parseArgument(arg, br);
		if (i < cmd.arguments.size() - 1)
			ss << ", ";
	}
	ss << ')';
	fi.line = ss.str();
	return fi;
}

const std::string &ScriptDecompiler::getName(uint8_t opcode) const {
	return functionNames_[opcode];
}

std::string ScriptDecompiler::parseArgument(const SDArgument &arg, BinaryReader &br) const {
	std::stringstream ss;
	switch (arg.type) {
	case SDType::Bytes:
		for (int i = 0; i < arg.count; ++i) {
			auto b = br.read<uint8_t>();
			ss << std::hex << std::setw(2) << std::setfill('0') << (int)b << (i < arg.count - 1 ? " " : "");
		}
		break;
	case SDType::UInt8:
		ss << (int)br.read<uint8_t>();
		break;
	case SDType::Int8:
		ss << (int)br.read<int8_t>();
		break;
	case SDType::UInt16: {
		auto val = br.read<uint16_t>();
		if (val & 0x8000) {
			ss << "var[" << (val & ~0x8000) << "]";
		} else {
			ss << val;
		}
		break;
	}
	case SDType::Int16: {
		auto val = br.read<uint16_t>();
		if (val & 0x8000) {
			ss << "var[" << (val & ~0x8000) << "]";
		} else {
			ss << (int16_t)val;
		}
		break;
	}
	case SDType::UInt32:
		ss << br.read<uint32_t>();
		break;
	case SDType::Int32:
		ss << br.read<int32_t>();
		break;
	case SDType::UInt64:
		ss << br.read<uint64_t>();
		break;
	case SDType::Int64:
		ss << br.read<int64_t>();
		break;
	case SDType::Float:
		ss << br.read<float>();
		break;
	case SDType::Double:
		ss << br.read<double>();
		break;
	case SDType::String8: {
		auto strSize = br.read<uint8_t>();
		auto str = br.readString(strSize - 1);
		br.skip(1);
		ss << "\"" << str << "\"";
		break;
	}
	case SDType::String16: {
		auto strSize = br.read<uint16_t>();
		auto str = br.readString(strSize - 1);
		br.skip(1);
		ss << "\"" << str << "\"";
		break;
	}
	case SDType::Addr:
	case SDType::JumpAddr:
		ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << br.read<uint32_t>() << std::dec;
		break;
	case SDType::Sprite:
		
		break;
	}
	return ss.str();
}

FuncInfo ScriptDecompiler::command_41(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	auto unk = br.read<uint8_t>();
	ss << getName(cmd.opcode) << "(" << (int)unk << ", ";
	auto test = unk & 0x80;
	if (test == 0) {
		ss << parseArgument(arg(SDType::UInt16), br) << ", ";
		ss << parseArgument(arg(SDType::Int16), br) << ")";
	} else {
		ss << parseArgument(arg(SDType::Bytes, 6), br) << ")";
	}
	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_4A(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	ss << getName(cmd.opcode) << "(" << parseArgument(arg(SDType::UInt16), br) << ", ";
	auto count = br.read<uint16_t>();
	ss << count;
	for (int i = 0; i < count; ++i) {
		ss << ", " << parseArgument(arg(SDType::JumpAddr), br);
	}
	ss << ")";
	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_4E(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	auto unk = br.read<uint8_t>();
	ss << getName(cmd.opcode) << "(" << (int)unk;
	if (unk == 1)
		ss << ", " << parseArgument(arg(SDType::UInt16), br) << ")";
	else if (unk == 4)
		ss << ", " << parseArgument(arg(SDType::Bytes, 3), br) << ")";
	else
		ss << ")";
	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_80(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	ss << getName(cmd.opcode) << "(" << parseArgument(arg(SDType::Bytes, 3), br) << ", ";
	auto count = br.read<uint8_t>();
	ss << (int)count;
	count &= ~0x80;
	for (int i = 0; i < count; ++i) {
		ss << ", " << parseArgument(arg(SDType::UInt16), br);
	}
	ss << ")";

	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_83(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	auto unk = br.read<uint16_t>();
	br.skip(-2);
	ss << getName(cmd.opcode) << "(" << parseArgument(arg(SDType::UInt16), br);
	if (unk == 3 || unk == 4)
		ss << ", " << parseArgument(arg(SDType::UInt16), br);
	else if (unk == 6) {
		ss << ", " << parseArgument(arg(SDType::UInt16), br);
		ss << ", " << parseArgument(arg(SDType::UInt16), br);
	}
	//ss << parseArgument(arg(SDType::UInt16), br);
	//count &= ~0x80;
	/*if (unk & 0x8000)
		ss << ", " << parseArgument(arg(SDType::Bytes, 1), br);*/
	//else if (unk < 0xa) // ???
	//	ss << ", " << parseArgument(arg(SDType::Bytes, 2), br);
	ss << ")";

	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_8D(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	auto next = br.read<uint8_t>();
	std::stringstream ss;
	ss << getName(cmd.opcode) << "(" << (int)next << ", ";
	next &= ~0x80;
	if (next == 0x02)
		ss << parseArgument(arg(SDType::UInt16), br) << ")";
	else if (next == 0x03) {
		ss << parseArgument(arg(SDType::UInt16), br);
		ss << ", " << parseArgument(arg(SDType::UInt16), br) << ")";
	} else if (next == 0x0E)
		ss << parseArgument(arg(SDType::Bytes, 6), br) << ")";
	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_A0(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	auto first = br.read<uint16_t>();
	ss << getName(cmd.opcode) << "(" << first << ", ";
	auto count = br.read<uint8_t>();
	ss << (int)count;
	if (script_.version() != 1) {
		for (int i = 0; i < count; ++i) {
			ss << ", " << parseArgument(arg(SDType::Bytes, 1), br);
		}
	} else {
		ss << ", " << parseArgument(arg(SDType::Bytes, 7), br);
	}
	ss << ")";
	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_B9(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	auto count = br.read<uint8_t>();
	ss << getName(cmd.opcode) << "(" << (int)count;
	for (int i = 0; i < count; ++i) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 1), br);
	}
	ss << ")";
	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::display_image(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	auto layer = br.read<uint16_t>();
	br.skip(-2);
	ss << getName(cmd.opcode) << "(";
	ss << parseArgument(arg(SDType::UInt16), br) << ", ";
	auto type = br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();

	ss << type << ", " << (int)unk3;

	uint16_t spriteId = 0;

	if (unk3 == 0) { // clear layer
		if (script_.version() != 0x01) {
			spriteId = br.read<uint16_t>();
			ss << ", " << spriteId;
		}
		ss << ") // clear?";
		fi.line = ss.str();
		return fi;
	}

	if (unk3 == 0x2D) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 8), br);
	}
	if (unk3 == 1) {
		spriteId = br.read<uint16_t>();
		if (type == 3) {
			ss << ", \"bustup/" << std::string(script_.sprites_[spriteId].name) << ".bup\", \"" << std::string(script_.sprites_[spriteId].pose) << "\"";
		} else if (type == 2) {
			ss << ", \"picture/" << std::string(script_.cgs_[spriteId].name) << ".pic\"";
		} else {
			ss << ", UNKNOWN";
		}
		ss << ") // spriteId = " << spriteId;
	} else {
		ss << "UNKNOWN)";
	}

	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_C2(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	ss << getName(cmd.opcode) << "(";
	auto layer = br.read<uint16_t>();
	br.skip(-2);
	ss << parseArgument(arg(SDType::UInt16), br) << ", ";
	auto unk2 = br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	ss << unk2 << ", " << (int)unk3;
	if (unk3 == 0x0) {
		// ...
	} else if (unk3 == 0x06) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 4), br);
	} else if (unk3 == 0x07) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 6), br);
	} else {
		ss << ", " << parseArgument(arg(SDType::UInt16), br);
	}
	ss << ")";
	fi.line = ss.str();
	return fi;
}

FuncInfo ScriptDecompiler::command_CA(const SDCommand &cmd, BinaryReader &br) const {
	FuncInfo fi;
	std::stringstream ss;
	ss << getName(cmd.opcode) << "(" << parseArgument(arg(SDType::UInt16), br);
	auto unk = br.read<uint8_t>();
	ss << ", " << (int)unk;
	if (unk == 0x00) {
		// ...
	} else if (unk == 0x01) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 2), br);
	} else if (unk == 0x02) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 2), br);
	} else if (unk == 0x03) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 4), br);
	} else if (unk == 0x06) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 4), br);
	} else if (unk == 0x07) {
		ss << ", " << parseArgument(arg(SDType::Bytes, 6), br);
	} else {
		//br.skip(0);
	}
	ss << ')';
	fi.line = ss.str();
	return fi;
}