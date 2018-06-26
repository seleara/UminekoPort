#include "audiomanager.h"

#include <iostream>
#include <sstream>

#include <imgui/imgui.h>
#include <soundio/soundio.h>

#include "audiostream.h"
#include "atrac3.h"

AudioManager::AudioManager(Archive &archive) : archive_(archive) {
	bgm_ = std::make_unique<AudioStream>();
	bgm_->manager_ = this;
	voice_ = std::make_unique<AudioStream>();
	voice_->manager_ = this;
	ses_.resize(0x20);
	for (auto &se : ses_) {
		se = std::make_unique<AudioStream>();
		se->manager_ = this;
	}

	try {
		soundio_ = soundio_create();
		if (!soundio_) {
			throw std::runtime_error("Unable to initialize SoundIO: Out of memory");
		}
		int err;
		if ((err = soundio_connect(soundio_))) {
			throw std::runtime_error("Error connecting: " + std::string(soundio_strerror(err)) + '\n');
		}

		std::cout << "Connected." << std::endl;

		soundio_flush_events(soundio_);

		int defaultOutDeviceIndex = soundio_default_output_device_index(soundio_);
		if (defaultOutDeviceIndex < 0) {
			throw std::runtime_error("No output device found.");
		}

		device_ = soundio_get_output_device(soundio_, defaultOutDeviceIndex);
		if (!device_) {
			throw std::runtime_error("Out of memory.");
		}

		std::cout << "Output device: " << device_->name << std::endl;
	}
	catch (...) {
		throw std::runtime_error("Unable to initialize audio manager.");
	}

	av_log_set_level(AV_LOG_QUIET);
}

AudioManager::~AudioManager() {
	/*if (context_) {
		avcodec_close(context_);
		av_free(context_);
	}*/
	ses_.clear();
	bgm_.reset();
	voice_.reset();
	soundio_device_unref(device_);
	soundio_destroy(soundio_);
}

void AudioManager::drawDebug() {
	static bool windowOpen = true;
	ImGui::Begin("Audio Manager", &windowOpen);

	ImGui::Text("BGM Fade = %f", bgm_->fadeCoeff_);
	float masterVol = bgm_->masterVolume();
	float origVol = masterVol;
	ImGui::SliderFloat("BGM Volume", &masterVol, 0.0f, 1.0f);
	if (masterVol != origVol) {
		bgm_->setMasterVolume(masterVol);
	}

	ImGui::End();
}

void AudioManager::setBGMVolume(float volume) {
	bgm_->setMasterVolume(volume);
}

void setSEVolume(int channel, float volume) {
	// TODO: Implement
}

void AudioManager::playBGM(const std::string &filename, float volume) {
	auto at3file = std::make_shared<AT3File>();
	at3file->manager_ = this;

	at3file->load(filename, archive_);
	bgm_->load(at3file);

	bgm_->setVolume(volume);
	bgm_->play();
}

void AudioManager::stopBGM(int frames) {
	bgm_->fadeOut(frames / 60.0);
}

void AudioManager::playSE(int channel, const std::string &filename, float volume) {
	auto at3file = std::make_shared<AT3File>();
	at3file->manager_ = this;

	at3file->load(filename, archive_);
	ses_[channel]->load(at3file);

	ses_[channel]->setVolume(volume);
	ses_[channel]->play();
}

void AudioManager::stopSE(int channel, int frames) {
	ses_[channel]->fadeOut(frames / 60.0);
}

void AudioManager::stopAllSE(int frames) {
	double time = frames / 60.0;
	for (auto &se : ses_) {
		se->fadeOut(time);
	}
}

void AudioManager::playVoice(const std::string &filename) {
	auto at3file = std::make_shared<AT3File>();
	at3file->manager_ = this;

	at3file->load(filename, archive_);
	voice_->load(at3file);
	voice_->setVolume(1.0f);
	voice_->play();
}