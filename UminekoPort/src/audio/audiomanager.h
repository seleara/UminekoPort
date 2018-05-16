#pragma once

#include <memory>
#include <vector>

struct SoundIo;
struct SoundIoDevice;

class Archive;

class AudioManager {
public:
	AudioManager(Archive &archive);
	~AudioManager();

	void drawDebug();

	void setBGMVolume(float volume);

	void playBGM(const std::string &filename, float volume);
	void stopBGM(int frames);

	void playSE(int channel, const std::string &filename, float volume);
	void stopSE(int channel, int frames);
	void stopAllSE(int frames);

	void playVoice(const std::string &filename);
private:
	friend class AT3File;
	friend class AudioStream;

	Archive &archive_;

	std::unique_ptr<AudioStream> bgm_;
	std::unique_ptr<AudioStream> voice_;
	std::vector<std::unique_ptr<AudioStream>> ses_;

	SoundIo *soundio_;
	SoundIoDevice *device_;

	//AVCodec *codec_;
	//AVCodecContext *context_;
	//AVFormatContext *format_;
	//SwrContext *swr_;
};