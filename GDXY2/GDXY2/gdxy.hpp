#pragma once
#include "common.hpp"
#include <Image.hpp>
#include <Reference.hpp>

class GDXY : public godot::Reference {
	GODOT_CLASS(GDXY, godot::Reference);

	struct RGB {
		uint8_t R;	// ��ɫ
		uint8_t G;	// ��ɫ
		uint8_t B;	// ��ɫ
	};

	struct RGBA : RGB {
		uint8_t A;	// ͨ��
	};

	struct FrameHeader {
		int32_t key_x;			// ͼƬ��ê��X
		int32_t key_y;			// ͼƬ��ê��Y
		uint32_t width;			// ͼƬ�Ŀ�ȣ���λ����
		uint32_t height;		// ͼƬ�ĸ߶ȣ���λ����
	};
public:
	static void _register_methods() {
		godot::register_method("read_mask", &GDXY::read_mask);
		godot::register_method("repair_jpeg", &GDXY::repair_jpeg);
		godot::register_method("read_was", &GDXY::read_was);
		godot::register_method("format_pal", &GDXY::format_pal);
		godot::register_method("string_id", &GDXY::string_id);
	}

	void _init();

	PoolByteArray repair_jpeg(godot::PoolByteArray bytes);

	PoolByteArray read_mask(godot::PoolByteArray bytes, int width, int height);

	PoolByteArray read_was(godot::PoolByteArray bytes, godot::PoolByteArray pal);

	unsigned int string_id(godot::String s);

	PoolByteArray format_pal(godot::PoolByteArray pal);

	size_t decompress_mask(void* in, void* out);

	void jpeg_repair(uint8_t* Buffer, uint32_t inSize, uint8_t* outBuffer, uint32_t* outSize);

	void byte_swap(uint16_t& value);

	uint32_t set_alpha(RGBA Color, uint8_t Alpha);

	RGBA RGB565to888(uint16_t color, uint8_t Alpha = 255);
};

