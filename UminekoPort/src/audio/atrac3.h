#pragma once

#include <cstdint>

struct GainInfo {
	int32_t numGainData;
	int32_t levcode[8];
	int32_t loccode[8];
};

struct GainBlock {
	GainInfo block[4];
};

struct TonalComponent {
	int32_t position;
	int32_t numCoefficients;
	float coefficient[8];
};

struct ChannelUnit {
	int32_t bandsCoded;
	int32_t numComponents;
	TonalComponent components[64];
	float previousFrame[1024];
	int32_t gcBlockSwitch;
	GainBlock gainBlock[2];

	float spectrum[1024]; // DECLARE_ALIGNED 32
	float IMDCT_buf[1024]; // DECLARE_ALIGNED 32

	float delayBuf1[46];
	float delayBuf2[46];
	float delayBuf3[46];
};

struct GetBitContext {
	const uint8_t *buffer;
	const uint8_t *bufferEnd;
	int32_t index;
	int32_t sizeInBits;
	int32_t sizeInBitsPlus8;
};

struct Atrac3Context {

};