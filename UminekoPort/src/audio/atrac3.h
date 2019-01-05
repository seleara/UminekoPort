#pragma once

#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswresample/swresample.h>
}

class Archive;

class AT3File {
public:
	AT3File();
	~AT3File();

	void load(const std::string &filename, Archive &archive);

	void rewind() {

	}

	void seek(double seconds) {

	}

	void setLooping(bool looping) {

	}

	int sampleRate() const {
		return sampleRate_;
	}

	int channels() const {
		return channels_;
	}

	bool nextSamples(std::vector<int16_t> &buffer, uint32_t &samplesNeeded);
private:
	friend class AudioManager;

	static int readBuffer(void *opaque, uint8_t *buf, int bufSize) {
		AT3File *at3 = (AT3File *)opaque;

		bufSize = FFMIN(bufSize, static_cast<int>(at3->dataRemaining_));
		if (!bufSize) {
			return AVERROR_EOF;
		}

		memcpy(buf, at3->data_.data() + at3->dataOffset_, bufSize);
		at3->dataOffset_ += bufSize;
		at3->dataRemaining_ -= bufSize;

		return bufSize;
	}

	std::string filename_;
	unsigned char *avData_;
	uint32_t dataOffset_ = 0, dataRemaining_ = 0;

	std::vector<unsigned char> data_;

	int channels_ = 0;
	int sampleRate_ = 0;

	AudioManager *manager_;

	AVCodec *codec_;
	AVCodecContext *context_;
	AVFormatContext *format_;
	AVIOContext *avio_;
	AVStream *stream_;
	SwrContext *swr_;

	AVPacket avpkt;
	AVFrame *decoded_frame = NULL;
};