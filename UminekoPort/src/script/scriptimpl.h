#pragma once

#include "script.h"

//typedef void (ScriptImpl::*CommandFunction)(BinaryReader &, Archive &);
typedef std::function<void(BinaryReader &, Archive &)> CommandFunction;

class Script;

class ScriptImpl {
public:
	ScriptImpl(Script &script);
	virtual void load(BinaryReader &br) = 0;
protected:
	friend class Script;

	Script &script_;

	std::vector<MaskEntry> masks_;
	std::vector<SpriteEntry> sprites_;
	std::vector<CgEntry> cgs_;
	std::vector<BGMEntry> bgms_;
	std::vector<SEEntry> ses_;

	void readMasks(BinaryReader &br, uint32_t offset);
	void readCgs(BinaryReader &br, uint32_t offset);
	void readBgms(BinaryReader &br, uint32_t offset);
	void readSes(BinaryReader &br, uint32_t offset);

	virtual void setupCommands() = 0;

	/**
	 * Commands
	 */
	std::vector<CommandFunction> commands_;

public:

	/**
	 * [41] set_variable(op:u8, var:u16, val:u16, ...)
	 * Perform arithmetic operations and store the result in the specified variable
	 *  op=, ???, op+=, op-=, op*=, op/=
	 */
	void set_variable(BinaryReader &br, Archive &archive);

	/**
	 * [42] ???
	 */
	void command42(BinaryReader &br, Archive &archive);

	/**
	 * [46] jump_if
	 */
	void jump_if(BinaryReader &br, Archive &archive);

	/**
	 * [47] jump(offset:u32)
	 */
	void jump(BinaryReader &br, Archive &archive);

	/**
	 * [48] call(offset:u32)
	 */
	void call(BinaryReader &br, Archive &archive);

	/**
	 * [49] return
	 */
	void return_(BinaryReader &br, Archive &archive);
	
	/**
	 * [4A] branch_on_variable(var:u16, count:u16, offset1:u32[, offset2:u32...])
	 * Checks the value of the first argument and jumps to the corresponding address in the argument list
	 */
	void branch_on_variable(BinaryReader &br, Archive &archive);

	/**
	 * [4D] push(count:u8, val1:u16[, val2:u16...])
	 */
	void push(BinaryReader &br, Archive &archive);

	/**
	 * [4E] pop(count:u8, var1:u16[, var2:u16...])
	 */
	void pop(BinaryReader &br, Archive &archive);
	
	/**
	 * [80] unlock_content(id:u16)
	 */
	void unlock_content(BinaryReader &br, Archive &archive);

	void command81(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}
	
	/**
	 * [83] wait(frames:u16)
	 */
	void wait(BinaryReader &br, Archive &archive);

	void command85(BinaryReader &br, Archive &archive);

	/**
	* [86] display_text(id:u16, unk:u8, pause:u8, text:str16)
	*/
	void display_text(BinaryReader &br, Archive &archive);

	/**
	* [87] wait_msg_advance(segment:u16)
	*/
	void wait_msg_advance(BinaryReader &br, Archive &archive);

	/**
	* [88] return_to_message()
	*/
	void return_to_message(BinaryReader &br, Archive &archive);

	/**
	* [89] hide_text()
	*/
	void hide_text(BinaryReader &br, Archive &archive);

	/**
	* [8C/8D] show_choices(unk:u32, var:u16, unk2:i16, title:str8, choices:splitstr8)
	*/
	void show_choices(BinaryReader &br, Archive &archive);

	/**
	* [8D/??] do_transition(...)
	*/
	void do_transition(BinaryReader &br, Archive &archive);

	/**
	* [9C] play_bgm(id:u16, unk:u16, volume:u32)
	*/
	void play_bgm(BinaryReader &br, Archive &archive);

	/**
	* [9D] stop_bgm(frames:u16)
	*/
	void stop_bgm(BinaryReader &br, Archive &archive);

	void command9E(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}
	/**
	* [A0] play_se(channel:u16, unk:u16, volume:u32)
	*/
	void play_se(BinaryReader &br, Archive &archive);

	/**
	* [A1] stop_se(channel:u16, frames:u16)
	*/
	void stop_se(BinaryReader &br, Archive &archive);

	/**
	* [A2] stop_all_se(frames:u16)
	*/
	void stop_all_se(BinaryReader &br, Archive &archive);

	/**
	* [A3] set_se_volume(channel:u16, volume:u16, frames?:u16)
	*/
	void set_se_volume(BinaryReader &br, Archive &archive);

	void commandA4(BinaryReader &br, Archive &archive) {
		br.skip(7); // Japanese character used in here?
	}

	/**
	* [A6] shake(unk1:u16, unk2:u16)
	*/
	void shake(BinaryReader &br, Archive &archive) {
		auto unk1 = script_.getVariable(br.read<uint16_t>());
		auto unk2 = script_.getVariable(br.read<uint16_t>());
	}

	/**
	* [B0/A0] set_title(unk:u16, title:string)
	*/
	void set_title(BinaryReader &br, Archive &archive);

	/**
	* [B1] play_movie(id:u16)
	*/
	void play_movie(BinaryReader &br, Archive &archive);

	/**
	* [B2] movie_related_B2(unk:u16)
	*/
	void movie_related_B2(BinaryReader &br, Archive &archive);

	/**
	* [B3] movie_related_B3()
	*/
	void movie_related_B3(BinaryReader &br, Archive &archive);

	/**
	* [B4] movie_related_B4()
	*/
	void movie_related_B4(BinaryReader &br, Archive &archive);

	/**
	* [B6] autosave()
	*/
	void autosave(BinaryReader &br, Archive &archive);

	/**
	* [BE] unlock_trophy(id:u16)
	*/
	void unlock_trophy(BinaryReader &br, Archive &archive);

	void commandBF(BinaryReader &br, Archive &archive);

	/**
	* [C1] display_image(...)
	*/
	void display_image(BinaryReader &br, Archive &archive);

	/**
	* [C2] set_layer_property(layer:u16, property:u16, unk:u8, ...)
	*/
	void set_layer_property(BinaryReader &br, Archive &archive);

	void commandC3(BinaryReader &br, Archive &archive);

	void commandC9(BinaryReader &br, Archive &archive);

	void commandCA(BinaryReader &br, Archive &archive);

	void commandCB(BinaryReader &br, Archive &archive);
};