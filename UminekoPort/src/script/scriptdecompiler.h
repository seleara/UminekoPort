#pragma once

#include <string>
#include <vector>

enum class SDType {
	Bytes,
	UInt8,
	Int8,
	UInt16,
	Int16,
	UInt32,
	Int32,
	UInt64,
	Int64,
	Float,
	Double,
	String8,
	String16,
	SplitString8,
	Addr,
	JumpAddr, // Separating just in case
	Sprite,
	Picture,
};

struct SDArgument {
	SDType type;
	uint32_t count;
};

inline SDArgument arg(SDType type) {
	return SDArgument { type, 0 };
}

inline SDArgument arg(SDType type, uint32_t count) {
	return SDArgument { type, count };
}

struct SDCommand {
	int32_t opcode;
	std::vector<SDArgument> arguments;
};

struct UnimplementedOpcodeError {
};

struct FuncInfo {
	std::string line;
	std::vector<uint32_t> jumps;
};

class BinaryReader;
class Script;

class ScriptDecompiler {
public:
	ScriptDecompiler(Script &script);
	void setup();
	void decompile(const std::string &path, const std::vector<unsigned char> &data, uint32_t scriptOffset);

	std::string getFunctionLine(BinaryReader &br) const;
private:
	FuncInfo buildFunction(const SDCommand &cmd, BinaryReader &br) const;
	const std::string &getName(uint8_t opcode) const;
	std::string parseArgument(const SDArgument &arg, BinaryReader &br) const;

	bool isVariable(uint16_t value) const {
		return ((value >> 0xC) & 0xF) == 0x8;
	}

	static std::vector<std::string> functionNamesUmi_;
	static std::vector<std::string> functionNamesHigu_;
	static std::vector<SDCommand> commands_;

	typedef FuncInfo(ScriptDecompiler::*SDCommandFunc)(const SDCommand &, BinaryReader &) const;


	// Special cases
	std::vector<SDCommandFunc> specialCases_;

	FuncInfo command_41(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_46(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_4A(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_4D(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_4E(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_80(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_80_higu(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_83(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_8D(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_9C(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_A0(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_B0(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_B9(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_BD(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo display_image(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_C2(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_C7(const SDCommand &cmd, BinaryReader &br) const;
	FuncInfo command_CA(const SDCommand &cmd, BinaryReader &br) const;


	Script &script_;
};