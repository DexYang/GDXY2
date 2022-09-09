#include "gdxy2.hpp"
#include <vector>
#include <stdio.h>

#define MEM_READ_WITH_OFF(off,dst,src,len) if(off+len<=src.size()){  memcpy((uint8_t*)dst,(uint8_t*)(src.data()+off),len);off+=len;   }

void GDXY2::_init() {
	Godot::print("_init");
}

PoolByteArray GDXY2::read_mapx(godot::PoolByteArray bytes) {
	Godot::print("read_mapx");
	const uint8_t* in = bytes.read().ptr();

	godot::PoolByteArray pba = godot::PoolByteArray();
	bool result = m_ujpeg.decode(in, bytes.size(), true);
	if (!result)
		return pba;
	if (!m_ujpeg.isValid())
		return pba;

	pba.resize(230400);
	m_ujpeg.getImage(pba.write().ptr());

	return pba;
}

PoolByteArray GDXY2::read_jpeg(godot::PoolByteArray bytes) {
	Godot::print("read_jpeg");
	const uint8_t* in = bytes.read().ptr();

	std::vector<uint8_t> out(bytes.size() * 2, 0);
	uint32_t tmpSize = 0;
	jpeg_repair((uint8_t*)in, bytes.size(), out.data(), &tmpSize);

	godot::PoolByteArray pba = godot::PoolByteArray();
	bool result = m_ujpeg.decode(out.data(), tmpSize, false);
	if (!result)
		return pba;
	if (!m_ujpeg.isValid())
		return pba;

	pba.resize(230400);
	m_ujpeg.getImage(pba.write().ptr());

	return pba;
}


PoolByteArray GDXY2::read_mask(godot::PoolByteArray bytes, int width, int height) {
	Godot::print("_read");
	const uint8_t* in = bytes.read().ptr();
	std::vector<uint8_t> out((width + 3) / 4 * height, 0);

	decompress_mask((uint8_t * )in, out.data());

	godot::PoolByteArray pba = godot::PoolByteArray();
	pba.resize(width * height * 4);

	int maskIndex = 0;
	for (int i = 0; i < height; i++) {
		int counter4 = 0;
		for (int j = 0; j < width; j++) {
			int pos = i * width + j;
			uint8_t byte = out[maskIndex];

			uint8_t flag = (byte >> counter4 * 2) & 3;
			counter4++;
			if (counter4 >= 4) {
				counter4 = 0;
				maskIndex++;
			}
			pba.set(pos * 4 + 0, 0);
			pba.set(pos * 4 + 1, 0);
			pba.set(pos * 4 + 2, 0);
			pba.set(pos * 4 + 3, 0);

			if (flag > 0) {
				pba.set(pos * 4 + 3, flag == 3 ? 128 : 1);
			}
		}
		if (counter4 != 0) {
			counter4 = 0;
			maskIndex++;
		}
	}

	return pba;
}


size_t GDXY2::decompress_mask(void* in, void* out) {
	uint8_t* op;
	uint8_t* ip;
	unsigned t;
	uint8_t* m_pos;

	op = (uint8_t*)out;
	ip = (uint8_t*)in;

	if (*ip > 17) {
		t = *ip++ - 17;
		if (t < 4)
			goto match_next;
		do
			*op++ = *ip++;
		while (--t > 0);
		goto first_literal_run;
	}

	while (1) {
		t = *ip++;
		if (t >= 16)
			goto match;
		if (t == 0) {
			while (*ip == 0) {
				t += 255;
				ip++;
			}
			t += 15 + *ip++;
		}

		*(unsigned*)op = *(unsigned*)ip;
		op += 4;
		ip += 4;
		if (--t > 0) {
			if (t >= 4) {
				do {
					*(unsigned*)op = *(unsigned*)ip;
					op += 4;
					ip += 4;
					t -= 4;
				} while (t >= 4);
				if (t > 0)
					do
						*op++ = *ip++;
				while (--t > 0);
			}
			else
				do
					*op++ = *ip++;
			while (--t > 0);
		}

	first_literal_run:

		t = *ip++;
		if (t >= 16)
			goto match;

		m_pos = op - 0x0801;
		m_pos -= t >> 2;
		m_pos -= *ip++ << 2;

		*op++ = *m_pos++;
		*op++ = *m_pos++;
		*op++ = *m_pos;

		goto match_done;

		while (1) {
		match:
			if (t >= 64) {
				m_pos = op - 1;
				m_pos -= (t >> 2) & 7;
				m_pos -= *ip++ << 3;
				t = (t >> 5) - 1;

				goto copy_match;

			}
			else if (t >= 32) {
				t &= 31;
				if (t == 0) {
					while (*ip == 0) {
						t += 255;
						ip++;
					}
					t += 31 + *ip++;
				}

				m_pos = op - 1;
				m_pos -= (*(unsigned short*)ip) >> 2;
				ip += 2;
			}
			else if (t >= 16) {
				m_pos = op;
				m_pos -= (t & 8) << 11;
				t &= 7;
				if (t == 0) {
					while (*ip == 0) {
						t += 255;
						ip++;
					}
					t += 7 + *ip++;
				}
				m_pos -= (*(unsigned short*)ip) >> 2;
				ip += 2;
				if (m_pos == op)
					goto eof_found;
				m_pos -= 0x4000;
			}
			else {
				m_pos = op - 1;
				m_pos -= t >> 2;
				m_pos -= *ip++ << 2;
				*op++ = *m_pos++;
				*op++ = *m_pos;
				goto match_done;
			}

			if (t >= 6 && (op - m_pos) >= 4) {
				*(unsigned*)op = *(unsigned*)m_pos;
				op += 4;
				m_pos += 4;
				t -= 2;
				do {
					*(unsigned*)op = *(unsigned*)m_pos;
					op += 4;
					m_pos += 4;
					t -= 4;
				} while (t >= 4);
				if (t > 0)
					do
						*op++ = *m_pos++;
				while (--t > 0);
			}
			else {
			copy_match:
				*op++ = *m_pos++;
				*op++ = *m_pos++;
				do
					*op++ = *m_pos++;
				while (--t > 0);
			}

		match_done:

			t = ip[-2] & 3;
			if (t == 0)
				break;

		match_next:
			do
				*op++ = *ip++;
			while (--t > 0);
			t = *ip++;
		}
	}

eof_found:
	return (op - (uint8_t*)out);
}

void GDXY2::jpeg_repair(uint8_t* Buffer, uint32_t inSize, uint8_t* outBuffer, uint32_t* outSize) {
	// JPEG数据处理原理
	// 1、复制D8到D9的数据到缓冲区中
	// 2、删除第3、4个字节 FFA0
	// 3、修改FFDA的长度00 09 为 00 0C
	// 4、在FFDA数据的最后添加00 3F 00
	// 5、替换FFDA到FF D9之间的FF数据为FF 00

	uint32_t TempNum = 0; // 临时变量，表示已读取的长度
	uint16_t TempTimes = 0; // 临时变量，表示循环的次数
	uint32_t Temp = 0;
	bool break_while = false;

	// 当已读取数据的长度小于总长度时继续
	while (!break_while && TempNum < inSize && *Buffer++ == 0xFF) {
		*outBuffer++ = 0xFF;
		TempNum++;
		switch (*Buffer) {
		case 0xD8:
			*outBuffer++ = 0xD8;
			Buffer++;
			TempNum++;
			break;
		case 0xA0:
			Buffer++;
			outBuffer--;
			TempNum++;
			break;
		case 0xC0:
			*outBuffer++ = 0xC0;
			Buffer++;
			TempNum++;

			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			byte_swap(TempTimes); // 将长度转换为Intel顺序

			for (int i = 0; i < TempTimes; i++) {
				*outBuffer++ = *Buffer++;
				TempNum++;
			}

			break;
		case 0xC4:
			*outBuffer++ = 0xC4;
			Buffer++;
			TempNum++;
			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			byte_swap(TempTimes); // 将长度转换为Intel顺序

			for (int i = 0; i < TempTimes; i++) {
				*outBuffer++ = *Buffer++;
				TempNum++;
			}
			break;
		case 0xDB:
			*outBuffer++ = 0xDB;
			Buffer++;
			TempNum++;

			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			byte_swap(TempTimes); // 将长度转换为Intel顺序

			for (int i = 0; i < TempTimes; i++) {
				*outBuffer++ = *Buffer++;
				TempNum++;
			}
			break;
		case 0xDA:
			*outBuffer++ = 0xDA;
			*outBuffer++ = 0x00;
			*outBuffer++ = 0x0C;
			Buffer++;
			TempNum++;

			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			byte_swap(TempTimes); // 将长度转换为Intel顺序
			Buffer++;
			TempNum++;
			Buffer++;

			for (int i = 2; i < TempTimes; i++) {
				*outBuffer++ = *Buffer++;
				TempNum++;
			}
			*outBuffer++ = 0x00;
			*outBuffer++ = 0x3F;
			*outBuffer++ = 0x00;
			Temp += 1; // 这里应该是+3的，因为前面的0xFFA0没有-2，所以这里只+1。

			// 循环处理0xFFDA到0xFFD9之间所有的0xFF替换为0xFF00
			for (; TempNum < inSize - 2;) {
				if (*Buffer == 0xFF) {
					*outBuffer++ = 0xFF;
					*outBuffer++ = 0x00;
					Buffer++;
					TempNum++;
					Temp++;
				}
				else {
					*outBuffer++ = *Buffer++;
					TempNum++;
				}
			}
			// 直接在这里写上了0xFFD9结束Jpeg图片.
			Temp--; // 这里多了一个字节，所以减去。
			outBuffer--;
			*outBuffer-- = 0xD9;
			break;
		case 0xD9:
			// 算法问题，这里不会被执行，但结果一样。
			*outBuffer++ = 0xD9;
			TempNum++;
			break;
		case 0xE0:
			break_while = true; // 如果碰到E0,则说明后面的数据不需要修复
			while (TempNum < inSize) {
				*outBuffer++ = *Buffer++;
				TempNum++;
			}
			break;
		default:
			break;
		}
	}
	Temp += inSize;
	*outSize = Temp;
}

void GDXY2::byte_swap(uint16_t& value) {
	uint16_t tempvalue = value >> 8;
	value = (value << 8) | tempvalue;
}

PoolByteArray GDXY2::read_was(godot::PoolByteArray bytes, godot::PoolByteArray palette) {
	uint8_t* data = (uint8_t* )bytes.read().ptr();
	RGBA* pal = (RGBA* )palette.read().ptr();

	uint32_t offset = 0;

	FrameHeader frameHeader{ };
	memcpy((uint8_t*)&frameHeader, (uint8_t*)(data + offset), sizeof(FrameHeader));
	offset += sizeof(FrameHeader);

	std::vector<uint32_t> frameLineOffset;
	frameLineOffset.resize(frameHeader.height);
	memcpy((uint8_t*)frameLineOffset.data(), (uint8_t*)(data + offset), frameHeader.height * 4);
	offset += frameHeader.height * 4;

	std::vector<uint32_t> rgba(frameHeader.height * frameHeader.width, 0);
	uint32_t pos = 0;

	for (uint32_t h = 0; h < frameHeader.height; h ++) {
		uint32_t linePixels = 0;
		bool lineNotOver = true;
		uint8_t* pData = data + frameLineOffset[h];

		while (*pData != 0 && lineNotOver) {

			uint8_t level = 0;  // Alpha
			uint8_t repeat = 0; // 重复次数
			uint32_t color = 0;	//重复颜色
			uint8_t style = (*pData & 0xc0) >> 6;   // 取字节的前两个比特
			switch (style) {
			case 0:   // {00******}
				if (*pData & 0x20) {  // {001*****} 表示带有Alpha通道的单个像素
					// {001 +5bit Alpha}+{1Byte Index}, 表示带有Alpha通道的单个像素。
					// {001 +0~31层Alpha通道}+{1~255个调色板引索}
					level = (*pData) & 0x1f;  // 0x1f=(11111) 获得Alpha通道的值
					if (*(pData - 1) == 0xc0) {  //特殊处理
						//Level = 31;
						//Pixels--;
						//pos--;
						if (linePixels <= frameHeader.width) {
							rgba[pos] = rgba[pos - 1];
							linePixels++;
							pos++;
							pData += 2;
							break;
						}
						else {
							lineNotOver = false;
						}
					}
					pData++;  // 下一个字节
					if (linePixels <= frameHeader.width) {
						rgba[pos] = set_alpha(pal[*pData], (level << 3) | 7 - 1);
						linePixels++;
						pos++;
						pData++;
					}
					else {
						lineNotOver = false;
					}
				}
				else {   // {000*****} 表示重复n次带有Alpha通道的像素
					// {000 +5bit Times}+{1Byte Alpha}+{1Byte Index}, 表示重复n次带有Alpha通道的像素。
					// {000 +重复1~31次}+{0~255层Alpha通道}+{1~255个调色板引索}
					// 注: 这里的{00000000} 保留给像素行结束使用，所以只可以重复1~31次。
					repeat = (*pData) & 0x1f; // 获得重复的次数
					pData++;
					level = *pData; // 获得Alpha通道值
					pData++;
					color = set_alpha(pal[*pData], (level << 3) | 7 - 1);
					for (int i = 1; i <= repeat; i++) {
						if (linePixels <= frameHeader.width) {
							rgba[pos] = color;
							pos++;
							linePixels++;
						}
						else {
							lineNotOver = false;
						}
					}
					pData++;
				}
				break;
			case 1: // {01******} 表示不带Alpha通道不重复的n个像素组成的数据段
				// {01  +6bit Times}+{nByte Datas},表示不带Alpha通道不重复的n个像素组成的数据段。
				// {01  +1~63个长度}+{n个字节的数据},{01000000}保留。
				repeat = (*pData) & 0x3f; // 获得数据组中的长度
				pData++;
				for (int i = 1; i <= repeat; i++) {
					if (linePixels <= frameHeader.width) {
						rgba[pos] = *(uint32_t*)&pal[*pData];
						pos++;
						linePixels++;
						pData++;
					}
					else {
						lineNotOver = false;
					}
				}
				break;
			case 2: // {10******} 表示重复n次像素
				// {10  +6bit Times}+{1Byte Index}, 表示重复n次像素。
				// {10  +重复1~63次}+{0~255个调色板引索},{10000000}保留。
				repeat = (*pData) & 0x3f; // 获得重复的次数
				pData++;
				color = *(uint32_t*)&pal[*pData];
				for (int i = 1; i <= repeat; i++) {
					if (linePixels <= frameHeader.width) {
						rgba[pos] = color;
						pos++;
						linePixels++;
					}
					else {
						lineNotOver = false;
					}
				}
				pData++;
				break;
			case 3: // {11******} 表示跳过n个像素，跳过的像素用透明色代替
				// {11  +6bit Times}, 表示跳过n个像素，跳过的像素用透明色代替。
				// {11  +跳过1~63个像素},{11000000}保留。
				repeat = (*pData) & 0x3f; // 获得重复次数
				if (repeat == 0) {
					if (linePixels <= frameHeader.width) { //特殊处理
						pos--;
						linePixels--;
					}
					else {
						lineNotOver = false;
					}
				}
				else {
					for (int i = 1; i <= repeat; i++) {
						if (linePixels <= frameHeader.width) {
							pos++;
							linePixels++;
						}
						else {
							lineNotOver = false;
						}
					}
				}
				pData++;
				break;
			default: // 一般不存在这种情况
				printf("WAS ERROR\n");
				break;
			}
		}
		if (*pData == 0 || !lineNotOver)
		{
			uint32_t repeat = frameHeader.width - linePixels;
			if (h > 0 && !linePixels) {//法术处理
				uint8_t* last = data + frameLineOffset[h - 1];
				if (*last != 0) {
					memcpy(rgba.data() + pos, rgba.data() + pos - frameHeader.width, frameHeader.width * 4);
					pos += frameHeader.width;
				}
			}
			else if (repeat > 0) {
				pos += repeat;
			}
		}
	}
	std::vector<uint32_t>().swap(frameLineOffset);

	godot::PoolByteArray pba = godot::PoolByteArray();

	pba.resize(frameHeader.height * frameHeader.width * 4);
	m_ujpeg.getImage(pba.write().ptr());
	memcpy(pba.write().ptr(), (uint8_t*)rgba.data(), frameHeader.height* frameHeader.width * 4);
	return pba;
}

uint32_t GDXY2::set_alpha(RGBA Color, uint8_t Alpha) {
	Color.A = Alpha;
	return *(uint32_t*)&Color;
};



unsigned int GDXY2::string_id(godot::String s) {
	const char* str = s.alloc_c_string();

	int i;
	unsigned int v;
	static unsigned m[70];
	strncpy((char*)m, str, 256);
	for (i = 0; i < 256 / 4 && m[i]; i++);
	m[i++] = 0x9BE74448, m[i++] = 0x66F42C48;
	v = 0xF4FA8928;

	unsigned int cf = 0;
	unsigned int esi = 0x37A8470E;
	unsigned int edi = 0x7758B42B;
	unsigned int eax = 0;
	unsigned int ebx = 0;
	unsigned int ecx = 0;
	unsigned int edx = 0;
	unsigned long long temp = 0;
	while (true) {
		// mov ebx, 0x267B0B11
		ebx = 0x267B0B11;
		// rol v, 1
		cf = (v & 0x80000000) > 0 ? 1 : 0;
		v = ((v << 1) & 0xFFFFFFFF) + cf;
		// xor ebx, v
		ebx = ebx ^ v;
		// mov eax, [eax + ecx * 4]
		eax = m[ecx];
		// mov edx, ebx
		edx = ebx;
		// xor esi, eax
		esi = esi ^ eax;
		// xor edi, eax
		edi = edi ^ eax;
		// add edx, edi
		temp = (unsigned long long)edx + (unsigned long long)edi;
		cf = (temp & 0x100000000) > 0 ? 1 : 0;
		edx = temp & 0xFFFFFFFF;
		// or edx, 0x2040801
		edx = edx | 0x2040801;
		// and edx, 0xBFEF7FDF
		edx = edx & 0xBFEF7FDF;
		// mov eax, esi
		eax = esi;
		// mul edx
		temp = (unsigned long long)eax * (unsigned long long)edx;
		eax = temp & 0xffffffff;
		edx = (temp >> 32) & 0xffffffff;
		cf = edx > 0 ? 1 : 0;
		// adc eax, edx
		temp = (unsigned long long)eax + (unsigned long long)edx + (unsigned long long)cf;
		eax = temp & 0xffffffff;
		cf = (temp & 0x100000000) > 0 ? 1 : 0;
		// mov edx, ebx
		edx = ebx;
		// adc eax, 0
		temp = (unsigned long long)eax + (unsigned long long)cf;
		eax = temp & 0xffffffff;
		cf = (temp & 0x100000000) > 0 ? 1 : 0;
		// add edx, esi
		temp = (unsigned long long)edx + (unsigned long long)esi;
		cf = (temp & 0x100000000) > 0 ? 1 : 0;
		edx = temp & 0xFFFFFFFF;
		// or edx, 0x804021
		edx = edx | 0x804021;
		// and edx, 0x7DFEFBFF
		edx = edx & 0x7DFEFBFF;
		// mov esi, eax
		esi = eax;
		// mov eax, edi
		eax = edi;
		// mul edx
		temp = (unsigned long long)eax * (unsigned long long)edx;
		eax = temp & 0xffffffff;
		edx = (temp >> 32) & 0xffffffff;
		cf = edx > 0 ? 1 : 0;
		// add edx, edx
		temp = (unsigned long long)edx + (unsigned long long)edx;
		cf = (temp & 0x100000000) > 0 ? 1 : 0;
		edx = temp & 0xFFFFFFFF;
		// adc eax, edx
		temp = (unsigned long long)eax + (unsigned long long)edx + (unsigned long long)cf;
		eax = temp & 0xffffffff;
		cf = (temp & 0x100000000) > 0 ? 1 : 0;
		// jnc _skip
		if (cf != 0) {
			// add eax, 2
			temp = (unsigned long long)eax + 2;
			cf = (temp & 0x100000000) > 0 ? 1 : 0;
			eax = temp & 0xFFFFFFFF;
		}
		// inc ecx;
		ecx += 1;
		// mov edi, eax
		edi = eax;
		// cmp ecx, i  jnz _loop
		if (ecx - i == 0) break;
	}
	// xor esi, edi
	esi = esi ^ edi;
	// mov v, esi
	v = esi;
	return v;
}

PoolByteArray GDXY2::format_pal(godot::PoolByteArray pal) {
	const uint16_t* in = (uint16_t * )pal.read().ptr();
	godot::PoolByteArray pba = godot::PoolByteArray();
	pba.resize(256 * 4);
	RGBA rgba;
	for (int k = 0; k < 256; k++) {
		rgba = RGB565to888(in[k]); // 16to32调色板转换
		pba.set(k * 4 + 0, rgba.R);
		pba.set(k * 4 + 1, rgba.G);
		pba.set(k * 4 + 2, rgba.B);
		pba.set(k * 4 + 3, rgba.A);
	}
	return pba;
}

GDXY2::RGBA GDXY2::RGB565to888(uint16_t color, uint8_t Alpha)
{
	GDXY2::RGBA pixel;

	uint8_t r = (color >> 11) & 0x1f;
	uint8_t g = (color >> 5) & 0x3f;
	uint8_t b = (color) & 0x1f;

	pixel.A = Alpha;
	pixel.R = (r << 3) | (r >> 2);
	pixel.G = (g << 2) | (g >> 4);
	pixel.B = (b << 3) | (b >> 2);

	return pixel;
};