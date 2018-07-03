#include "chiruscript.h"

ChiruScript::ChiruScript(Script &script) : UmiScript(script) {}

void ChiruScript::load(BinaryReader &br) {
	script_.scriptOffset_ = br.read<uint32_t>();

	// Resource offsets
	auto maskOffset = br.read<uint32_t>();
	auto cgOffset = br.read<uint32_t>();
	auto spriteOffset = br.read<uint32_t>();
	auto animeOffset = br.read<uint32_t>();
	auto bgmOffset = br.read<uint32_t>();
	auto seOffset = br.read<uint32_t>();
	auto movieOffset = br.read<uint32_t>();
	auto voiceOffset = br.read<uint32_t>();
	auto unknown1Offset = br.read<uint32_t>();
	auto unknown2Offset = br.read<uint32_t>();
	auto textOffset = br.read<uint32_t>();
	auto unknown3Offset = br.read<uint32_t>();
	auto characterProfileOffset = br.read<uint32_t>();

	readMasks(br, maskOffset);

	readSprites(br, spriteOffset);

	readCgs(br, cgOffset);

	readAnims(br, animeOffset);

	readBgms(br, bgmOffset);

	readSes(br, seOffset);
}