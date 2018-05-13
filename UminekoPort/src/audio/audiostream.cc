#include "audio.h"

#include <iostream>

#include "atrac3.h"

AudioStream::~AudioStream() {
	//if (buffer_)
	//	delete buffer_;
	if (outStream)
		soundio_outstream_destroy(outStream);
}

void AudioStream::load(std::shared_ptr<AT3File> at3) {
	int err;
	outStream = soundio_outstream_create(manager_->device_);
	if (!outStream) {
		std::cerr << "Out of memory." << std::endl;
		return;
	}

	//outStream->format = SoundIoFormatS16BE;
	outStream->format = SoundIoFormatS16LE;
	outStream->write_callback = &AudioStream::writeCallback;
	outStream->userdata = this;
	outStream->sample_rate = at3->sampleRate();

	at3_ = at3;
	//looping_ = adx_[0]->looping();

	if ((err = soundio_outstream_open(outStream))) {
		std::cerr << "Unable to open device: " << soundio_strerror(err) << std::endl;
		return;
	}

	if (outStream->layout_error) {
		std::cerr << "Unable to set channel layout: " << soundio_strerror(outStream->layout_error) << std::endl;
		return;
	}

	/*if (buffer_)
	delete[] buffer_;*/
	//buffer_.resize(outStream->layout.channel_count);
	buffer_.resize(at3->channels() * 4096);
}

void AudioStream::play() {
	if (!started_) {
		int err;
		if ((err = soundio_outstream_start(outStream))) {
			std::cerr << "Unable to start stream: " << soundio_strerror(err) << std::endl;
			return;
		}
		started_ = true;
		return;
	}
	soundio_outstream_pause(outStream, false);
}

void AudioStream::pause() {
	soundio_outstream_pause(outStream, true);
}

// Same as pause, but start over from the first sample upon resuming
void AudioStream::stop() {
	pause();
	soundio_outstream_clear_buffer(outStream);
	at3_->rewind();
}

void AudioStream::seek(double seconds) {
	pause();
	at3_->seek(seconds);
	soundio_outstream_clear_buffer(outStream);
	play();
}

void AudioStream::destroy() {
	soundio_outstream_destroy(outStream);
}

void AudioStream::setLooping(bool looping) {
	//looping_ = looping;
	at3_->setLooping(looping_);
}

void AudioStream::setCallback(std::function<void()> callback) {
	//callback_ = callback;
}

void AudioStream::callback() {
	//callback_();
}

float AudioStream::volume() const {
	return volume_;
}

void AudioStream::setVolume(float volume) {
	volume_ = volume;
	if (started_)
		soundio_outstream_clear_buffer(outStream);
}

void AudioStream::writeCallback(SoundIoOutStream *outStream, int frameCountMin, int frameCountMax) {
	AudioStream *stream = static_cast<AudioStream *>(outStream->userdata);
	stream->writeCallback_(outStream, frameCountMin, frameCountMax);
}

void AudioStream::writeCallback_(SoundIoOutStream *outStream, int frameCountMin, int frameCountMax) {
	SoundIoChannelLayout *layout = &outStream->layout;
	float sampleRate = static_cast<float>(outStream->sample_rate);
	float secondsPerFrame = 1.0f / sampleRate;
	SoundIoChannelArea *areas;
	int framesLeft = frameCountMax;
	int err;

	bool done = false;
	while (framesLeft > 0) {
		int frameCount = framesLeft;

		//if (remaining_ == 0) {
		if (remaining_ == 0) {
			uint32_t samplesNeeded = 1;
			done = at3_->nextSamples(buffer_, samplesNeeded);
			frameCount = samplesNeeded;
		} else {
			frameCount = remaining_;
			remaining_ = 0;
		}

		if (frameCount > framesLeft) {
			remaining_ = frameCount;
			break;
		}
		/*} else {
			frameCount = remaining_;
		}

		if (frameCount > framesLeft) {
			remaining_ = frameCount - framesLeft;
			startAt_ = framesLeft;
			frameCount = framesLeft;
		} else {
			remaining_ = 0;
			startAt_ = 0;
		}*/

		if ((err = soundio_outstream_begin_write(outStream, &areas, &frameCount))) {
			std::cerr << "SoundIO Error: " << soundio_strerror(err) << std::endl;
			return;
		}

		if (!frameCount) {
			break;
		}

		//for (int frame = 0; frame < frameCount; ++frame) {
		for (int frame = 0; frame < frameCount; ++frame) {
			for (int channel = 0; channel < layout->channel_count; ++channel) {
				int16_t *ptr = reinterpret_cast<int16_t *>(areas[channel].ptr + areas[channel].step * frame);
				*ptr = static_cast<int16_t>(buffer_[frame * layout->channel_count + channel] * volume()); // Should rework this to only use the first two channels for stereo, etc. and make an exception for mono ADX files
			}

			if (done) {
				// ...
			}
		}

		if ((err = soundio_outstream_end_write(outStream))) {
			std::cerr << "SoundIO Error: " << soundio_strerror(err) << std::endl;
			return;
		}

		framesLeft -= frameCount;
	}

	//if (done) {
	//	stream->callback();
	//}
}