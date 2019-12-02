#include "fmp4_muxer.h"
#include <cstring>
#include <malloc.h>

#define BigLittleSwap16(A)  ((((uint16_t)(A) & 0xff00) >> 8) | \
	(((uint16_t)(A) & 0x00ff) << 8))

#define BigLittleSwap32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | \
	(((uint32_t)(A) & 0x00ff0000) >> 8) | \
	(((uint32_t)(A) & 0x0000ff00) << 8) | \
	(((uint32_t)(A) & 0x000000ff) << 24))

#define BigLittleSwap64(A) (((uint64_t)BigLittleSwap32((uint32_t)(A >> 32))) | ((uint64_t)(BigLittleSwap32((uint32_t)(A & 0XFFFFFFFF))) << 32))

int checkCPUendian()
{
	union {
		unsigned long int i;
		unsigned char s[4];
	}c;
	c.i = 0x12345678;
	return (0x12 == c.s[0]);
}

uint16_t hton16(uint16_t h)
{
	return checkCPUendian() ? h : BigLittleSwap16(h);
}

uint32_t hton32(uint32_t h)
{
	return checkCPUendian() ? h : BigLittleSwap32(h);
}

uint64_t hton64(uint64_t h)
{
	return checkCPUendian() ? h : BigLittleSwap64(h);
}

//#pragma comment(lib,"ws2_32.lib")
#pragma pack(push,1)

// Level Base
class BaseBox {
public:
	uint32_t  boxsize = 0;
	uint32_t  boxtype = 0;
};

class FullBaseBox : public BaseBox {
public:
	uint8_t  version = 0;
	uint8_t  flags[3] = { 0, 0, 0 };
};

// Level 8
class AvcCBox : public BaseBox {
public:
	AvcCBox() {
		BaseBox::boxsize = hton32(sizeof(AvcCBox));
		BaseBox::boxtype = hton32(0X61766343);
	};
	uint8_t  configuration_version = 1;
	uint8_t  avc_profile_indication = 0X4d;
	uint8_t  avc_profile_compatibility = 0X00;
	uint8_t  avc_level_indication = 0X2a;
	uint8_t  nal_unit_length_size = 0Xff; //前面6位为reserved，后面2位有效。

	uint8_t sps_pps[48] = { 0 };

	/*
	//sps
	uint8_t  num_sequence_parameter_sets = 0Xe1; //前面3位是reserved,后面5位有效。
	uint16_t sequence_parameter_set_length = hton16(0x0014);
	uint8_t  sequence_parameter_set[0x14] = {
		0X67, 0X4d, 0X00, 0X2a,
		0X95, 0Xa8, 0X1e, 0X00,
		0X89, 0Xf9,	0X61, 0X00,
		0X00, 0X07, 0X08, 0X00,
		0X01, 0X5f, 0X90, 0X04
	};

	//pps
	uint8_t  num_picture_parameter_sets = 1;
	uint16_t picture_parameter_set_length = hton16(0x0004);
	uint8_t  picture_parameter_set[4] = { 0X68, 0Xee, 0X3c, 0X80 };
	*/

};

// Level 7
class Avc1Box : public BaseBox {
public:
	Avc1Box() {
		BaseBox::boxsize = hton32(sizeof(Avc1Box));
		BaseBox::boxtype = hton32(0X61766331);
	};
	uint8_t r0[6] = { 0 };
	uint16_t data_reference_index = hton16(0x0001);
	uint16_t pre_defined0 = 0;
	uint16_t r1 = 0;
	uint32_t pre_defined1 = 0;
	uint32_t pre_defined2 = 0;
	uint32_t pre_defined3 = 0;
	/*
	uint8_t data1[24] = {
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	*/
	uint16_t  width = hton16(0X0780);
	uint16_t  height = hton16(0X0438);
	uint32_t  horiz_resolution = hton32(0X00480000);
	uint32_t  ver_resolution = hton32(0X00480000);
	uint32_t  r2 = 0;
	uint16_t frames_count = hton16(1);
	uint8_t compressr_name[32] = { 0 };
	uint16_t bit_depth = hton16(0x18);
	uint16_t pre_defined4 = hton16(0xFFFF);
	AvcCBox avcC;
};

class UrlBox : public FullBaseBox
{
public:
	UrlBox() {
		FullBaseBox::boxsize = hton32(sizeof(UrlBox));
		FullBaseBox::boxtype = hton32(0X75726C20);
		FullBaseBox::flags[2] = 1;
	}
	//location
};


// Level 6

class DrefBox : public FullBaseBox
{
public:
	DrefBox() {
		FullBaseBox::boxsize = hton32(sizeof(DrefBox));
		FullBaseBox::boxtype = hton32(0X64726566);
	};
	uint32_t entry_count = hton32(1);
	UrlBox url;
};

class StsdBox : public FullBaseBox {
public:
	StsdBox() {
		BaseBox::boxsize = hton32(sizeof(StsdBox));
		FullBaseBox::boxtype = hton32(0X73747364);
	}
	uint32_t  entry_count = hton32(1);
	Avc1Box avc1;
};

class SttsEntry {
public:
	uint32_t  sample_count = 0;
	uint32_t  sample_delta = 0;
};

class SttsBox : public FullBaseBox {
public:
	SttsBox() {
		FullBaseBox::boxsize = hton32(sizeof(SttsBox));
		FullBaseBox::boxtype = hton32(0X73747473);
	}
	uint32_t  entry_count = 0;
};

class StssEntry {
public:
	uint32_t  sample_number = 0;
};

class StssBox : public FullBaseBox {
public:
	StssBox() {
		FullBaseBox::boxsize = hton32(sizeof(StssBox));
		FullBaseBox::boxtype = hton32(0X73747373);
	};
	uint32_t  entry_count = 0;
};

class CttsEntry {
public:
	uint32_t  sample_count = 0;
	uint32_t  sample_offset = 0;
};

class CttsBox : public FullBaseBox {
public:
	CttsBox() {
		FullBaseBox::boxsize = hton32(sizeof(CttsBox));
		FullBaseBox::boxtype = hton32(0X63747473);
	};
	uint32_t  entry_count = 0;
};

class StscEntry {
public:
	uint32_t  first_chunk = 0;
	uint32_t  samples_per_chunk = 0;
	uint32_t  samples_description_index = hton32(1);
};

class StscBox : public FullBaseBox {
public:
	StscBox() {
		FullBaseBox::boxsize = hton32(sizeof(StscBox));
		FullBaseBox::boxtype = hton32(0X73747363);
	};
	uint32_t  entry_count = 0;
};

class StszEntry {
public:
	uint32_t  sample_size = 0;
};

class StszBox : public FullBaseBox {
public:
	StszBox() {
		FullBaseBox::boxsize = hton32(sizeof(StszBox));
		FullBaseBox::boxtype = hton32(0X7374737A);
	};
	uint32_t  sample_size = 0;
	uint32_t  sample_count = 0;
};

class StcoEntry {
public:
	uint32_t  chunk_offset = 0;
};

class StcoBox : public FullBaseBox {
public:
	StcoBox() {
		FullBaseBox::boxsize = hton32(sizeof(StcoBox));
		FullBaseBox::boxtype = hton32(0X7374636F);
	};
	uint32_t  entry_count = 0;
};

// Level 5
class StblBox : public BaseBox {

public:
	StblBox() {
		BaseBox::boxsize = hton32(sizeof(StblBox));
		BaseBox::boxtype = hton32(0X7374626C);
	};
	StsdBox stsd;
	SttsBox stts;
	//StssBox stss;
	//CttsBox ctts;
	StscBox stsc;
	StszBox stsz;
	StcoBox stco;
};

class DinfBox :public BaseBox
{
public:
	DinfBox() {
		BaseBox::boxsize = hton32(sizeof(DinfBox));
		BaseBox::boxtype = hton32(0X64696E66);
	};
	DrefBox dref;
};

class VmhdBox : public FullBaseBox
{
public:
	VmhdBox() {
		FullBaseBox::boxsize = hton32(sizeof(VmhdBox));
		FullBaseBox::boxtype = hton32(0X766D6864);
		FullBaseBox::flags[2] = 1;
	};
	uint32_t graphics_mode = 0;
	uint32_t r0 = 0;
	//uint16_t opcolor[3] = { 0, 0, 0 };
};

// Level 4
class MinfBox :public BaseBox
{
public:
	MinfBox() {
		BaseBox::boxsize = hton32(sizeof(MinfBox));
		BaseBox::boxtype = hton32(0X6D696E66);
	};
	VmhdBox vmhd;
	DinfBox dinf;
	StblBox stbl;
};

class ElstBox : public FullBaseBox
{
public:
	ElstBox() {
		FullBaseBox::boxsize = hton32(sizeof(ElstBox));
		FullBaseBox::boxtype = hton32(0X656C7374);
	};
public:
	uint32_t  entry_count = hton32(1);
	//entry []
	uint32_t segment_duration = hton32(0);
	int32_t media_time = 0;
	uint16_t media_rate_integer = hton16(1);
	uint16_t media_rate_fraction = 0;
};

class MdhdBox : public FullBaseBox
{
public:
	MdhdBox() {
		FullBaseBox::boxsize = hton32(sizeof(MdhdBox));
		FullBaseBox::boxtype = hton32(0X6D646864);
	};
public:
	uint32_t  creation_time = 0;
	uint32_t  modification_time = 0;
	uint32_t  time_scale = hton32(0X15F90);
	uint32_t  duration = hton32(0);
	uint8_t Language[4] = { 0x55, 0xc4, 0x00, 0x00 };

};
class HdlrBox : public FullBaseBox
{
public:
	HdlrBox() {
		FullBaseBox::boxsize = hton32(sizeof(HdlrBox));
		FullBaseBox::boxtype = hton32(0X68646C72);
		std::strcpy(name, "VideoHandler");
	};
	uint32_t pre_defined = 0;
	uint32_t handler_type = hton32(0x76696465);
	uint32_t reserved[3] = { 0 };
	char  name[13] = { 0 };
};

// Level 3
class MdiaBox : public BaseBox
{
public:
	MdiaBox() {
		BaseBox::boxsize = hton32(sizeof(MdiaBox));
		BaseBox::boxtype = hton32(0X6D646961);
	};
	MdhdBox mdhd;
	HdlrBox hdlr;
	MinfBox minf;
};

class TkhdBox : public FullBaseBox
{
public:
	TkhdBox() {
		FullBaseBox::boxsize = hton32(sizeof(TkhdBox));
		FullBaseBox::boxtype = hton32(0X746B6864);
		FullBaseBox::flags[2] = 3;
	};
	uint32_t  creation_time = 0;
	uint32_t  modification_time = 0;
	uint32_t  track_id = hton32(1);
	uint8_t  r0[4] = { 0 };
	uint32_t  duration = hton32(0);
	uint8_t  r1[8] = { 0 };
	uint16_t  layer = 0;
	uint16_t  alternate_group = 0;
	uint16_t  volume = 0;
	uint8_t  r2[2] = { 0 };
	uint32_t matrix[9] = { hton32(0x00010000),0,0,0,hton32(0x00010000),0,0,0,hton32(0x40000000) };
	uint32_t  width = hton32(0X07800000);
	uint32_t  height = hton32(0X04380000);
};

class EdtsBox : public BaseBox
{
public:
	EdtsBox() {
		BaseBox::boxsize = hton32(sizeof(EdtsBox));
		BaseBox::boxtype = hton32(0X65647473);
	};
	ElstBox elst;
};

class MehdBox : public FullBaseBox
{
public:
	MehdBox() {
		FullBaseBox::boxsize = hton32(sizeof(MehdBox));
		FullBaseBox::boxtype = hton32(0X6d656864);
	};
public:
	uint32_t fragment_duration = hton32(0);
};

typedef struct {
	uint8_t reserved : 4;
	uint8_t is_leading : 2;
	uint8_t sample_depends_on : 2;
	uint8_t sample_is_depended_on : 2;
	uint8_t sample_has_redundancy : 2;
	uint8_t sample_padding_value : 3;
	uint8_t sample_is_non_sync_sample : 1;
	uint16_t sample_degradation_priority;
}SampleFlags;

class TrexBox : public FullBaseBox
{
public:
	TrexBox() {
		FullBaseBox::boxsize = hton32(sizeof(TrexBox));
		FullBaseBox::boxtype = hton32(0X74726578);
	};
	uint32_t  track_id = hton32(1);
	uint32_t  sample_description_index = hton32(1);
	uint32_t  sample_duration = 0;
	uint32_t  sample_size = 0;
	uint32_t  sample_flags = 0;
};


class TfhdBox : public FullBaseBox
{
public:
	TfhdBox() {
		FullBaseBox::boxsize = hton32(sizeof(TfhdBox));
		FullBaseBox::boxtype = hton32(0X74666864);
		FullBaseBox::flags[0] = 0x02;
		FullBaseBox::flags[2] = 0x28;
	};
	uint32_t track_id = hton32(1);
	//uint64_t base_data_offset = 0;// hton32(0X001D27B);
	//uint32_t sample_description_index = hton32(1);
	uint32_t default_sample_duration = hton32(0XE10);
	//uint32_t default_sample_size = 0;
	uint32_t default_sample_flags = hton32(0X01010000);
};

class TfdtBox : public FullBaseBox
{
public:
	TfdtBox() {
		FullBaseBox::boxsize = hton32(sizeof(TfdtBox));
		FullBaseBox::boxtype = hton32(0X74666474);
		FullBaseBox::version = 0x01;
	};
	// if version= 0 , decode time use uint32_t type.
	uint64_t base_media_decode_time = hton64(0);
};


class TrunSample {
public:
	TrunSample() {
		//sample_duration = hton32(0x8E10);
	};
public:
	//uint32_t sample_duration = 0;
	uint32_t sample_size = 0;
	uint32_t sample_flags = hton32(0X01010000);
	//uint32_t composition_time_offset = hton32(0x0400);
};


class TrunBox : public FullBaseBox
{
public:
	TrunBox() {
		FullBaseBox::boxsize = hton32(sizeof(TrunBox));
		FullBaseBox::boxtype = hton32(0X7472756E);
		FullBaseBox::flags[1] = 0X06;// 0X02;
		FullBaseBox::flags[2] = 0X01;// 0X05;
		//ts[0].duration = hton32(0XE10);
	};
	uint32_t sample_count = hton32(1);
	// all fields in the following array are optional
	int32_t data_offset = hton32(0);
	//uint32_t first_sample_flags = hton32(0X01010000);
	TrunSample ts[1];
};

// Level 2

class TrafBox :public BaseBox
{
public:
	TrafBox() {
		BaseBox::boxsize = hton32(sizeof(TrafBox));
		BaseBox::boxtype = hton32(0X74726166);
	};
	TfhdBox tfhd;
	TfdtBox tfdt;
	TrunBox trun;
};


class MvhdBox : public FullBaseBox
{
public:
	MvhdBox() {
		FullBaseBox::boxsize = hton32(sizeof(MvhdBox));
		FullBaseBox::boxtype = hton32(0X6D766864);
	};
	uint32_t  creation_time = 0;
	uint32_t  modification_time = 0;
	uint32_t  time_scale = hton32(0X00003E8);
	uint32_t  duration = hton32(0);
	uint32_t  rate = hton32(0X10000);
	uint16_t  volume = hton16(0X100);
	uint8_t r0[10] = { 0 };
	uint32_t matrix[9] = { hton32(0x00010000),0,0,0,hton32(0x00010000),0,0,0,hton32(0x40000000) };
	uint32_t pre_defined[6] = { 0,0,0,0,0,0 };
	uint32_t next_track_id = hton32(0X02);
};
class TrakBox : public BaseBox
{
public:
	TrakBox() {
		BaseBox::boxsize = hton32(sizeof(TrakBox));
		BaseBox::boxtype = hton32(0X7472616B);
	};
	TkhdBox tkhd;
	//EdtsBox edts;
	MdiaBox mdia;
};

class MvexBox : public BaseBox
{
public:
	MvexBox() {
		BaseBox::boxsize = hton32(sizeof(MvexBox));
		BaseBox::boxtype = hton32(0X6D766578);
	};
	MehdBox mehd;
	TrexBox trex;
};

uint8_t UDTA[217] = {
0x00, 0x00, 0x00, 0xD9, 0x75, 0x64, 0x74, 0x61, 0x00, 0x00, 0x00, 0xD1, 0x6D, 0x65, 0x74, 0x61,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x68, 0x64, 0x6C, 0x72, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x6D, 0x64, 0x69, 0x72, 0x61, 0x70, 0x70, 0x6C, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA4, 0x69, 0x6C, 0x73, 0x74, 0x00, 0x00, 0x00,
0x30, 0xA9, 0x6E, 0x61, 0x6D, 0x00, 0x00, 0x00, 0x28, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00,
0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0x49, 0x4B, 0x20, 0x4D, 0x65, 0x64, 0x69, 0x61, 0x20, 0x53,
0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x56, 0x33, 0x2E, 0x30, 0x2E, 0x32, 0x30, 0x00, 0x00, 0x00,
0x25, 0xA9, 0x74, 0x6F, 0x6F, 0x00, 0x00, 0x00, 0x1D, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00,
0x01, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x61, 0x76, 0x66, 0x35, 0x38, 0x2E, 0x32, 0x36, 0x2E, 0x31,
0x30, 0x31, 0x00, 0x00, 0x00, 0x47, 0xA9, 0x63, 0x6D, 0x74, 0x00, 0x00, 0x00, 0x3F, 0x64, 0x61,
0x74, 0x61, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0x49, 0x4B, 0x20, 0x4D, 0x65,
0x64, 0x69, 0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x53, 0x65, 0x73, 0x73, 0x69,
0x6F, 0x6E, 0x20, 0x44, 0x65, 0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x3A,
0x20, 0x73, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x72, 0x64
};

class MfhdBox : public FullBaseBox
{
public:
	MfhdBox() {
		FullBaseBox::boxsize = hton32(sizeof(MfhdBox));
		FullBaseBox::boxtype = hton32(0X6D666864);
	};
	uint32_t  sequence_number = hton32(1);
};

class TfraBox : public FullBaseBox
{
public:
	TfraBox() {
		FullBaseBox::boxsize = hton32(sizeof(TfraBox));
		FullBaseBox::boxtype = hton32(0X74667261);
	};
	uint32_t r0[3] = { 0 };
};

class MfroBox : public FullBaseBox
{
public:
	MfroBox() {
		FullBaseBox::boxsize = hton32(sizeof(MfroBox));
		FullBaseBox::boxtype = hton32(0X6D66726F);
	};
	uint32_t size = 0;
};

// Level 1
class FtypBox : public BaseBox
{
public:
	FtypBox() {
		BaseBox::boxsize = hton32(0X00000024);
		BaseBox::boxtype = hton32(0X66747970);
	};
	uint32_t  major_brand = hton32(0X69736F6D);
	uint32_t  minor_version = hton32(0X00000200);
	uint32_t compatible_brands[5] = { hton32(0X69736F6D), hton32(0X69736F32), hton32(0X61766331), hton32(0X69736F36), hton32(0X6D703431) };//"isomiso2avc1iso6mp41";
};

class MoovBox : public BaseBox {
public:
	MoovBox() {
		BaseBox::boxsize = hton32(sizeof(MoovBox));
		BaseBox::boxtype = hton32(0X6D6F6F76);
		memcpy(udta, UDTA, 217);
	};
	MvhdBox mvhd;
	TrakBox trak;
	MvexBox mvex;
	uint8_t udta[217];
};

class MoofBox : public BaseBox {
public:
	MoofBox() {
		BaseBox::boxsize = hton32(sizeof(MoofBox));
		BaseBox::boxtype = hton32(0X6D6F6F66);
	};
	MfhdBox mfhd;
	TrafBox traf;
};

class MdatBox : public BaseBox
{
public:
	MdatBox() {
		BaseBox::boxsize = hton32(sizeof(MdatBox));
		BaseBox::boxtype = hton32(0X6D646174);
	};

};

class MfraBox : public BaseBox
{
public:
	MfraBox() {
		BaseBox::boxsize = hton32(sizeof(MfraBox));
		BaseBox::boxtype = hton32(0X6D667261);
	};
	TfraBox tfra;
	MfroBox mfro;
};

class StypBox : public BaseBox
{
public:
	StypBox() {
		BaseBox::boxsize = hton32(0X00000024);
		BaseBox::boxtype = hton32(0X73747970);
	};
	uint32_t  major_brand = hton32(0X69736F6D);
	uint32_t  minor_version = hton32(0X00000200);
	uint32_t compatible_brands[5] = { hton32(0X69736F6D), hton32(0X69736F32), hton32(0X61766331), hton32(0X69736F36), hton32(0X6D703432) };//"isomiso2avc1iso6mp42";
};


//FILE* fp;


FMp4Muxer::FMp4Muxer()
{
	frame_number = 0;

	// fp = fopen("D:\\tmp\\test.mp4", "wb");
};


FMp4Muxer::~FMp4Muxer()
{
	//fclose(fp);
};

uint32_t  ret;
/*
bool get_h264_data(uint32_t * buf, uint32_t  in_len, uint32_t * out_buf, uint32_t  &out_len)
{
	char nalu[4] = { 0X00, 0X00, 0X00, 0X01 };
	memcpy(out_buf, nalu, 4);
	out_buf += 4;
	memcpy(out_buf, buf, in_len);
	out_len = in_len + 4;
	//	cout << "out_len = " <<out_len<< endl;
	return true;
}
*/

uint32_t FMp4Muxer::generate_ftyp_moov(uint8_t*& fmp4_header, FMp4Info fi)
{
	FtypBox ftyp;

	MoovBox moov;

	//sps & pps

	uint8_t* t = moov.trak.mdia.minf.stbl.stsd.avc1.avcC.sps_pps;

	uint8_t  num_sequence_parameter_sets = 0Xe1;
	uint16_t sequence_parameter_set_length = hton16(fi.sps_size);

	uint8_t  num_picture_parameter_sets = 1;
	uint16_t picture_parameter_set_length = hton16(fi.pps_size);

	*t = num_sequence_parameter_sets;
	t += sizeof(num_sequence_parameter_sets);
	memcpy(t, &sequence_parameter_set_length, sizeof(sequence_parameter_set_length));
	t += sizeof(sequence_parameter_set_length);
	memcpy(t, fi.sps, fi.sps_size);
	t += fi.sps_size;

	*t = num_picture_parameter_sets;
	t += sizeof(num_picture_parameter_sets);
	memcpy(t, &picture_parameter_set_length, sizeof(picture_parameter_set_length));
	t += sizeof(picture_parameter_set_length);
	memcpy(t, fi.pps, fi.pps_size);
	//t += fi.pps_size;

	//

	fmp4_header = (uint8_t*)malloc(sizeof(ftyp) + sizeof(moov));
	memcpy(fmp4_header, &ftyp, sizeof(ftyp));
	memcpy(fmp4_header + sizeof(ftyp), &moov, sizeof(moov));

	uint32_t fmp4_header_size = sizeof(ftyp) + sizeof(moov);

	return fmp4_header_size;

	//StypBox styp;
	//fwrite(&styp, sizeof(StypBox), 1, fp);

	//MfraBox mfra;
	//fwrite(&mfra, sizeof(MfraBox), 1, fp);


}



uint32_t FMp4Muxer::generate_moof_mdat(uint8_t*& frame, uint32_t frameSize)
{
	MoofBox moof;

	if (frame[0] == 0x65)
	{
		moof.traf.trun.ts[0].sample_flags = hton32(0X02000000);
	}

	moof.traf.trun.ts[0].sample_size = hton32(frameSize + 4);
	//moof.traf.trun.ts[0].composition_time_offset = hton16(1024);
	//LONGLONG p = _ftelli64(fp);
	//moof.traf.tfhd.base_data_offset = hton64(p);
	// moof.traf.tfhd.default_sample_size = hton32(frameSize + 4);
	moof.traf.trun.data_offset = hton32(sizeof(moof) + sizeof(MdatBox));
	//moof.traf.trun.data_offset = hton32(sizeof(moof));
	moof.traf.tfdt.base_media_decode_time = hton64((uint64_t)((uint64_t)0x0E10 * frame_number));

	frame_number++;
	moof.mfhd.sequence_number = hton32(frame_number);
	//fwrite(&moof, sizeof(moof), 1, fp);


	MdatBox mdat;
	mdat.boxsize = hton32(sizeof(mdat) + frameSize + 4);
	//fwrite(&mdat, sizeof(mdat), 1, fp);

	uint32_t fs = hton32(frameSize);
	//fwrite(&fs, 4, 1, fp);

	//fwrite(frame, frameSize, 1, fp);

	//fflush(fp);
	size_t bufSize = sizeof(moof) + sizeof(mdat) + sizeof(uint32_t) + frameSize;
	uint8_t* buf = (uint8_t*)malloc(bufSize);
	memcpy(buf, &moof, sizeof(moof));
	memcpy(buf + sizeof(moof), &mdat, sizeof(mdat));
	memcpy(buf + sizeof(moof) + sizeof(mdat), &fs, sizeof(uint32_t));
	memcpy(buf + sizeof(moof) + sizeof(mdat) + sizeof(uint32_t), frame, frameSize);

	//fmp4_server_batch_send(buf, sizeof(moof) + sizeof(mdat) + frameSize + 4);

	//free(buf);
	frame = buf;

	return sizeof(moof) + sizeof(mdat) + frameSize + 4;
}

