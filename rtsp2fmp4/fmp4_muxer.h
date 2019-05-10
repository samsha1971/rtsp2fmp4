#pragma once
#pragma warning(disable : 4996)
#pragma warning(disable : 4267)

#include <stdint.h>

class FMp4Info {
public:
	uint8_t* sps;
	uint16_t sps_size;
	//
	uint8_t* pps;
	uint16_t pps_size;
	//
	uint32_t w;
	uint32_t h;
	uint32_t fps;
};

class FMp4Muxer
{
public:
	FMp4Muxer();
	~FMp4Muxer();
public:
	uint32_t generate_ftyp_moov(uint8_t * & fmp4_header, FMp4Info fi);
	uint32_t generate_moof_mdat(uint8_t * &frame, uint32_t frame_size);
	
//private:
	int frame_number;
};

