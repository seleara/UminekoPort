#pragma once

#include <functional>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswresample/swresample.h>
}
#include <soundio/soundio.h>

#include "../math/clock.h"
#include "../data/archive.h"

class AT3File;

class AudioStream {
public:
	~AudioStream();

	void load(std::shared_ptr<AT3File> at3);

	void play();

	void pause();

	// Same as pause, but start over from the first sample upon resuming
	void stop();
	void fadeOut(double seconds);

	void seek(double seconds);

	void destroy();

	void setLooping(bool looping);

	void setCallback(std::function<void()> callback);

	void callback();

	float volume() const;

	void setVolume(float volume);
private:
	static void writeCallback(SoundIoOutStream *outStream, int frameCountMin, int frameCountMax);
	void writeCallback_(SoundIoOutStream *outStream, int frameCountMin, int frameCountMax);

	SoundIoOutStream *outStream;
	std::shared_ptr<AT3File> at3_;

	uint32_t remaining_ = 0, startAt_ = 0;

	bool started_ = false;
	bool looping_ = false;
	float volume_ = 1.0f;
	enum FadeDirection {
		None,
		FadeIn,
		FadeOut
	} fadeDir_;
	float fadeCoeff_ = 1.0f;
	Clock fadeClock_;
	//std::unique_ptr<int16_t> buffer_; // Used in writeCallback_ to store samples. Made into a member variable to avoid malloc/free in the callback
	std::vector<int16_t> buffer_;

	std::function<void()> callback_;

	friend class AudioManager;
	AudioManager *manager_;
};

class AudioManager {
public:
	AudioManager(Archive &archive);
	~AudioManager();

	void drawDebug();

	void playBGM(const std::string &filename, float volume);
	void stopBGM(int frames);
	void playSE(int channel, const std::string &filename, float volume);
	void stopSE(int channel, int frames);
	void stopAllSE(int frames);

private:
	friend class AT3File;
	friend class AudioStream;

	Archive &archive_;

	std::unique_ptr<AudioStream> bgm_;
	std::vector<std::unique_ptr<AudioStream>> ses_;

	SoundIo *soundio_;
	SoundIoDevice *device_;

	//AVCodec *codec_;
	//AVCodecContext *context_;
	//AVFormatContext *format_;
	//SwrContext *swr_;
};