#include "atrac3.h"

#include <sstream>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

#include "../data/archive.h"

AT3File::AT3File() : avData_(NULL), decoded_frame(NULL) {

}

AT3File::~AT3File() {
	if (decoded_frame) {
		av_frame_free(&decoded_frame);
	}
	avformat_close_input(&format_);
	av_free(avio_->buffer);
	av_free(avio_);
	swr_free(&swr_);

	avcodec_close(context_);
	avcodec_free_context(&context_);
}

void AT3File::load(const std::string &filename, Archive &archive) {
	filename_ = filename;
	data_ = archive.read(filename);

	avData_ = (unsigned char *)av_malloc(data_.size());
	dataOffset_ = 0;
	dataRemaining_ = static_cast<uint32_t>(data_.size());

	format_ = avformat_alloc_context();
	if (!format_) {
		throw std::runtime_error("Cannot allocate format context.");
	}

	avio_ = avio_alloc_context(avData_, static_cast<int>(data_.size()), 0, (void *)this, &AT3File::readBuffer, NULL, NULL);
	if (!avio_) {
		throw std::runtime_error("Cannot allocate AVIO context.");
	}

	format_->pb = avio_;

	auto ret = avformat_open_input(&format_, "dummy", NULL, NULL);
	if (ret < 0) {
		throw std::runtime_error("Cannot open format input.");
	}

	ret = avformat_find_stream_info(format_, NULL);
	if (ret < 0) {
		throw std::runtime_error("Cannot find stream info.");
	}

	int streamIndex = -1;
	for (unsigned int i = 0; i < format_->nb_streams; ++i) {
		if (format_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			streamIndex = static_cast<int>(i);
			break;
		}
	}
	if (streamIndex == -1) {
		throw std::runtime_error("Unable to find audio stream.");
	}
	
	stream_ = format_->streams[streamIndex];

	codec_ = avcodec_find_decoder(stream_->codecpar->codec_id);

	context_ = avcodec_alloc_context3(codec_);
	if (!context_) {
		throw std::runtime_error("Cannot allocate audio codec context.");
	}

	/*context_->channels = stream_->codecpar->channels;
	context_->channel_layout = stream_->codecpar->channel_layout;
	context_->sample_rate = stream_->codecpar->sample_rate;
	context_->block_align = stream_->codecpar->block_align;*/
	avcodec_parameters_to_context(context_, stream_->codecpar);

	AVDictionary *opts = 0;
	auto result = avcodec_open2(context_, codec_, &opts);
	if (result < 0) {
		std::stringstream ss;
		char errbuf[0x100];
		auto errbuf_size = 0x100;
		av_make_error_string(errbuf, errbuf_size, result);
		ss << "Cannot open codec: " << errbuf;
		throw std::runtime_error(ss.str());
	}
	av_free(opts);

	channels_ = context_->channels;
	sampleRate_ = context_->sample_rate;

	//swr_ = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 48000, context_->channel_layout, context_->sample_fmt, context_->sample_rate, 0, NULL);
	swr_ = swr_alloc();
	if (!swr_) {
		throw std::runtime_error("Unable to allocate SWR.");
	}

	av_opt_set_int(swr_, "in_channel_layout", context_->channel_layout, 0);
	av_opt_set_int(swr_, "in_sample_rate", context_->sample_rate, 0);
	av_opt_set_sample_fmt(swr_, "in_sample_fmt", context_->sample_fmt, 0);

	av_opt_set_int(swr_, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_int(swr_, "out_sample_rate", 48000, 0);
	av_opt_set_sample_fmt(swr_, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

	if (swr_init(swr_) < 0) {
		throw std::runtime_error("Unable to initialize SWR.");
	}
	
	av_init_packet(&avpkt);
}


bool AT3File::nextSamples(std::vector<int16_t> &buffer, uint32_t &samplesNeeded) {
	if (!decoded_frame) {
		if (!(decoded_frame = av_frame_alloc())) {
			throw std::runtime_error("Couldn't allocate frame.");
		}

		int ret1 = av_read_frame(format_, &avpkt);
		if (ret1 < 0) return true;

		int got_frame = 0;

		//int len = avcodec_decode_audio4(context_, decoded_frame, &got_frame, &avpkt);
		avcodec_send_packet(context_, &avpkt);
	}

	int ret = avcodec_receive_frame(context_, decoded_frame);
	if (ret < 0) {
		int ret1 = av_read_frame(format_, &avpkt);
		if (ret1 < 0) return true;

		int got_frame = 0;

		//int len = avcodec_decode_audio4(context_, decoded_frame, &got_frame, &avpkt);
		avcodec_send_packet(context_, &avpkt);
		ret = avcodec_receive_frame(context_, decoded_frame);
		if (ret < 0) return true;
	}

	samplesNeeded = 0;

	size_t lastSize = 0;

	int data_size = av_samples_get_buffer_size(NULL, context_->channels, decoded_frame->nb_samples, context_->sample_fmt, 1);

	if (data_size < 0) {
		throw std::runtime_error("Failed to calculate data size.");
	}

	uint8_t **newBuffer;
	int linesize;
	ret = av_samples_alloc_array_and_samples(&newBuffer, &linesize, context_->channels, decoded_frame->nb_samples, context_->sample_fmt, 0);
	if (ret < 0) {
		throw std::runtime_error("Unable to allocate samples.");
	}
	//av_samples_alloc(newBuffer, NULL, context_->channels, decoded_frame->nb_samples, context_->sample_fmt, 0);
	ret = swr_convert(swr_, newBuffer, decoded_frame->nb_samples, (const uint8_t **)decoded_frame->data, decoded_frame->nb_samples);
	if (ret < 0) {
		throw std::runtime_error("Unable to convert samples.");
	}

	auto dstBufSize = av_samples_get_buffer_size(&linesize, context_->channels, ret, context_->sample_fmt, 0);
	//std::vector<unsigned char> dstBuf;
	auto &dstBuf = buffer;
	dstBuf.resize(lastSize + dstBufSize / sizeof(int16_t));
	std::copy(newBuffer[0], newBuffer[0] + dstBufSize, (uint8_t *)(dstBuf.data() + lastSize));
	lastSize = dstBuf.size();
	samplesNeeded += ret; // dstBuf.size() / context_->channels;

	/*for (int i = 0; i < context_->channels; ++i) {
		buffer[i] = *(int16_t *)newBuffer[i];
	}*/

	return false;
}