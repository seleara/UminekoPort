#pragma once

#include <functional>
#include <memory>

#include "../math/clock.h"

struct SoundIoOutStream;

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
	float masterVolume() const;

	void setVolume(float volume);
	void setMasterVolume(float volume);
private:
	static void writeCallback(SoundIoOutStream *outStream, int frameCountMin, int frameCountMax);
	void writeCallback_(SoundIoOutStream *outStream, int frameCountMin, int frameCountMax);

	SoundIoOutStream *outStream;
	std::shared_ptr<AT3File> at3_;

	uint32_t remaining_ = 0, startAt_ = 0;

	bool started_ = false;
	bool looping_ = false;
	float volume_ = 1.0f;
	float masterVolume_ = 1.0f;
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