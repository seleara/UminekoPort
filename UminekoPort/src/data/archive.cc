#include "archive.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include "compression.h"
#include "../util/binaryreader.h"
#include "../util/string.h"

#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"

void Archive::open(const std::string &path) {
	ifs_.open(path, std::ios_base::binary);
	BinaryReader br(ifs_);
	if (br.readString(4) != "ROM ") {
		throw std::runtime_error("Archive signature does not match the expected 'ROM '.");
	}

	scan(0x10, root_, br);
}

void Archive::explore() {
	explore(root_);

	std::cin.get();
}

void Archive::explore(ArchiveEntry &folder) {
	static void (*cascade)(ArchiveEntry &) = [](ArchiveEntry &current) {
		if (current.parent) {
			cascade(*current.parent);
		} else {
			std::cout << "~/";
		}
		std::cout << current.name << (current.name.size() ? "/" : "");
	};
	cascade(folder);
	std::cout << "\n: ";
	std::string cmd;
	std::getline(std::cin, cmd);
	if (cmd.substr(0, 5) == "exit ") {
		return;
	} else if (cmd.substr(0, 3) == "cd ") {
		auto dir = cmd.substr(3);
		if (dir == ".." && folder.name != "") {
			return;
		}
		auto iter = folder.childrenNames.find(dir);
		if (iter != folder.childrenNames.end()) {
			explore(folder.children[iter->second]);
		}
	} else if (cmd == "ls") {
		for (auto &child : folder.children) {
			if (child.isFolder) {
				std::cout << child.name << "/\n";
			} else {
				std::cout << child.name << "\n";
			}
		}
	} else if (cmd.substr(0, 7) == "export " || cmd.substr(0, 10) == "exportraw ") {
		bool raw = cmd.substr(0, 10) == "exportraw ";
		auto file = raw ? cmd.substr(10) : cmd.substr(7);
		std::transform(file.begin(), file.end(), file.begin(), ::tolower);
		auto iter = folder.childrenNames.find(file);
		if (iter != folder.childrenNames.end()) {
			auto &child = folder.children[iter->second];
			auto ext = child.name.substr(child.name.rfind('.'));
			if (!raw) {
				if (ext == ".txa") {
					extractTxa(child);
				} else if (ext == ".bup") {
					extractBup(child);
				} else if (ext == ".pic") {
					extractPic(child);
				} else if (ext == ".msk") {
					extractMsk(child.path);
				} else {
					std::ofstream ofs(child.name, std::ios_base::binary);
					BinaryReader br(ifs_);
					br.seekg(child.offset);
					char *data = new char[child.size];
					br.read(data, child.size);
					ofs.write(data, child.size);
					ofs.close();
					delete[] data;
				}
			} else {
				std::ofstream ofs(child.name, std::ios_base::binary);
				BinaryReader br(ifs_);
				br.seekg(child.offset);
				char *data = new char[child.size];
				br.read(data, child.size);
				ofs.write(data, child.size);
				ofs.close();
				delete[] data;
			}
		}
	}
	explore(folder);
}

std::vector<unsigned char> Archive::read(const std::string &path) {
	auto &entry = get(path);

	std::lock_guard<std::mutex> lock(mutex_);
	BinaryReader br(ifs_);
	br.seekg(entry.offset);
	std::vector<unsigned char> output(entry.size);
	br.read((char *)&output[0], entry.size);
	return output;
}

ArchiveEntry &Archive::get(const std::string &path) {
	auto lpath = path;
	StringUtil::toLower(lpath);
	auto tokens = StringUtil::splitRef(lpath, { '/' });
	ArchiveEntry *current = &root_;

	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto &token : tokens) {
		auto iter = current->childrenNames.find(token);
		if (iter != current->childrenNames.end()) {
			current = &current->children[iter->second];
		} else {
			throw std::runtime_error("File '" + lpath + "' not found in archive.");
		}
	}
	return *current;
}

void Archive::decode(const unsigned char *buffer, size_t bufferSize, unsigned char *output) {
	/*int p = 0;
	int marker = 1;
	char *res = output;

	while (p < bufferSize) {
		if (marker == 1)
			marker = 0x100 | buffer[p++];

		if (marker & 1) {
			uint16_t v = (buffer[p + 0] << 8) | buffer[p + 1];
			int count, offset;
			char *pos;

			if (v & 0x8000) {
				count = ((v >> 5) & 0x3ff) + 3;
				offset = v & 0x1f;
			} else {
				count = (v >> 11) + 3;
				offset = (v & 0x7ff) + 32;
			}

			pos = res - (offset + 1) * 4;

			for (int i = 0; i < count; ++i) {
				*res++ = *pos++;
			}

			p += 2;
		} else {
			*res++ = buffer[p++];
		}

		marker >>= 1;
	}*/
	unsigned char *res = output;
	int p = 0;
	int marker = 1;
	int j;

	while (p<bufferSize) {
		if (marker == 1) marker = 0x100 | buffer[p++];

		if (marker & 1) {
			unsigned int v = (buffer[p + 0] << 8) | buffer[p + 1];
			int count, offset;
			unsigned char *pos;

			if (v & 0x8000) {
				count = ((v >> 5) & 0x3ff) + 3;
				offset = v & 0x1f;
			} else {
				count = (v >> 11) + 3;
				offset = (v & 0x7ff) + 32;
			}

			pos = res - (offset + 1) * 4;

			for (j = 0; j<count; j++)
				*res++ = *pos++;

			p += 2;
		} else {
			*res++ = buffer[p++];
		}

		marker >>= 1;
	}
}

void Archive::dpcm(unsigned char *source, unsigned char *destination, int width, int height, int scanline) {
	for (int i = scanline; i < scanline * height; ++i) {
		destination[i] += source[i - scanline];
	}
}

#pragma pack(push, 1)
struct BMPHeader {
	uint16_t magic = 0x4d42;
	uint32_t size;
	uint32_t reserved = 0;
	uint32_t offset;
	uint32_t infoSize = 40;
	uint32_t width;
	uint32_t height;
	uint16_t planes = 1;
	uint16_t bpp = 32;
	uint32_t compression = 0;//3;
	uint32_t imageSize;
	uint32_t ppmx = 2835;
	uint32_t ppmy = 2835;
	uint32_t colors = 0;
	uint32_t importantColors = 0;
};
#pragma pack(pop)

void Archive::writeImage(const std::string &path, unsigned char *data, int width, int height, int scanline, int bpp) {
	/*std::ofstream ofs(path, std::ios_base::binary);
	BMPHeader header;

	unsigned char bitmasks[] = {
		0x00, 0x00,	0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x56, 0xB8, 0x1E, 0xFC, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66,
		0x66, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x63, 0xF8, 0x28, 0xFF, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00
	};

	header.compression = 0;
	header.offset = sizeof(BMPHeader);
	header.size = sizeof(BMPHeader) + width * height * 4;
	header.width = width;
	header.height = height;
	header.imageSize = width * height * 4;

	ofs.write((char *)&header, sizeof(header));
	//ofs.write((char *)bitmasks, sizeof(bitmasks));

	for (int j = height - 1; j >= 0; --j) {
		for (int i = 0; i < width; ++i) {
			ofs.write((char *)&data[i * 4 + j * scanline], 4);
		}
	}*/

	if (bpp == 4) {
		unsigned char *transformed = new unsigned char[scanline * height];
		for (int i = 0; i < width * height; ++i) {
			transformed[i * 4 + 0] = data[i * 4 + 2];
			transformed[i * 4 + 1] = data[i * 4 + 1];
			transformed[i * 4 + 2] = data[i * 4 + 0];
			transformed[i * 4 + 3] = data[i * 4 + 3];
		}

		stbi_write_png(path.c_str(), width, height, 4, transformed, scanline);

		delete[] transformed;
	} else if (bpp == 1) {
		stbi_write_png(path.c_str(), width, height, 1, data, scanline);
	}
}

struct TxaHeader {
	uint32_t magic;
	uint32_t filesize;
	uint32_t offset;
	uint32_t encodedSize;
	uint32_t decodedSize;
	uint32_t chunks;

	uint32_t unknown, unknown2;
};

struct TxaChunk {
	uint16_t length;
	uint16_t index;
	uint16_t width;
	uint16_t height;
	uint16_t scanline;
	uint16_t unknown;
	uint32_t offset;

	//char name[0];
};

void Archive::extractTxa(ArchiveEntry &txa) {
	BinaryReader br(ifs_);
	br.seekg(txa.offset);
	auto header = br.read<TxaHeader>();
	std::cout << "Magic = " << std::hex << header.magic << std::dec << "\n";
	//char *metadata = new char[header.offset - sizeof(header)];
	std::vector<TxaChunk> chunks; // (header.chunks);
	std::vector<std::string> names;
	//br.read(metadata, header.offset - sizeof(header));

	for (uint32_t i = 0; i < header.chunks; ++i) {
		uint64_t chunkStart = br.tellg();
		auto chunk = br.read<TxaChunk>();
		auto name = br.readString();
		br.seekg(chunkStart + chunk.length);
		std::cout << "W = " << chunk.width << ", H = " << chunk.height << "\n";
		chunks.push_back(std::move(chunk));
		names.push_back(std::move(name));
	}

	unsigned char *data = new unsigned char[header.decodedSize];
	unsigned char *buffer = new unsigned char[header.encodedSize];
	br.seekg(txa.offset + header.offset);
	br.read((char *)buffer, header.encodedSize);
	decode(buffer, header.encodedSize, data);

	std::stringstream bmpName;
	for (uint32_t i = 0; i < header.chunks; ++i) {
		bmpName << txa.name << "_" << names[i] << ".png";
		std::cout << "PNG: " << bmpName.str() << "\n";
		writeImage(bmpName.str(), data + chunks[i].offset, chunks[i].width, chunks[i].height, chunks[i].scanline);
		bmpName.clear();
		bmpName.str("");
	}

	delete[] data;
	delete[] buffer;
}

struct BupHeader {
	uint32_t magic;
	uint32_t fileSize;
	uint32_t unknown;
	uint16_t left;
	uint16_t top;
	uint16_t width;
	uint16_t height;
	uint32_t offset;
	uint32_t size;
	uint32_t chunks;
};

struct BupChunk {
	char title[16];
	uint32_t unknown;
	struct {
		uint16_t left;
		uint16_t top;
		uint16_t width;
		uint16_t height;
		uint32_t offset;
		uint32_t size;
	} picture[2];

	uint32_t unknown2;
	uint32_t unknown3;
	uint32_t unknown4;
	uint32_t unknown5;
};

struct PicHeader {
	uint32_t magic;
	uint32_t fileSize;
	uint16_t eWidth;
	uint16_t eHeight;
	uint16_t width;
	uint16_t height;
	uint32_t unknown;
	uint32_t chunks;
};

struct PicChunk {
	uint32_t version;
	uint16_t left;
	uint16_t top;
	uint16_t width;
	uint16_t height;
	uint32_t offset;
	uint32_t size;
};

struct MskHeader {
	uint32_t magic; // 0x334B534D MSK3
	uint32_t fileSize;
	uint16_t width;
	uint16_t height;
	uint32_t unknown;
};

Txa Archive::getTxa(const std::string &path) {
	auto &entry = get(path);

	std::lock_guard<std::mutex> lock(mutex_);

	BinaryReader br(ifs_);
	br.seekg(entry.offset);
	auto header = br.read<TxaHeader>();
	std::cout << "Magic = " << std::hex << header.magic << std::dec << "\n";
	//char *metadata = new char[header.offset - sizeof(header)];
	std::vector<TxaChunk> chunks; // (header.chunks);
	std::vector<std::string> names;
	//br.read(metadata, header.offset - sizeof(header));

	for (uint32_t i = 0; i < header.chunks; ++i) {
		uint64_t chunkStart = br.tellg();
		auto chunk = br.read<TxaChunk>();
		auto name = br.readString();
		br.seekg(chunkStart + chunk.length);
		std::cout << "W = " << chunk.width << ", H = " << chunk.height << "\n";
		chunks.push_back(chunk);
		names.push_back(name);
	}

	unsigned char *data = new unsigned char[header.decodedSize];
	unsigned char *buffer = new unsigned char[header.encodedSize];
	br.seekg(entry.offset + header.offset);
	br.read((char *)buffer, header.encodedSize);
	decode(buffer, header.encodedSize, data);

	Txa txa;
	txa.name = entry.name;
	txa.subentries.reserve(header.chunks);
	for (uint32_t i = 0; i < header.chunks; ++i) {
		//std::stringstream bmpName;
		//bmpName << txa.name << "_" << names[i] << ".png";
		//std::cout << "PNG: " << bmpName.str() << "\n";
		//writeImage(bmpName.str(), data + chunks[i].offset, chunks[i].width, chunks[i].height, chunks[i].scanline);
		auto &subEntry = txa.subentries.emplace_back();
		subEntry.name = names[i];
		subEntry.width = chunks[i].width;
		subEntry.height = chunks[i].height;
		subEntry.pixels.resize(subEntry.width * subEntry.height * 4);
		memcpy(subEntry.pixels.data(), data + chunks[i].offset, subEntry.width * subEntry.height * 4);
	}

	delete[] data;
	delete[] buffer;

	return txa;
}

Pic Archive::getPic(const std::string &path) {
	auto &entry = get(path);

	std::lock_guard<std::mutex> lock(mutex_);

	BinaryReader br(ifs_);
	br.seekg(entry.offset);

	auto header = br.read<PicHeader>();

	std::vector<PicChunk> chunks(header.chunks);
	br.read((char *)chunks.data(), header.chunks * sizeof(PicChunk));

	Pic pic;
	pic.name = path;
	pic.pixels.resize(4 * header.width * header.height);
	pic.width = header.width;
	pic.height = header.height;

	for (uint32_t i = 0; i < header.chunks; ++i) {
		PicChunk &chunk = chunks[i];

		auto size = chunk.size;
		unsigned char *buffer = new unsigned char[size];
		auto chunkStride = 4 * ((chunk.width + 3) & 0xfffc);
		unsigned char *result = new unsigned char[chunk.height * chunkStride];

		br.seekg(entry.offset + chunk.offset);
		br.read((char *)buffer, size);

		decode(buffer, size, result);
		dpcm(result, result, chunk.width, chunk.height, chunkStride);
		for (int y = 0; y < chunk.height; ++y) {
			/*for (int x = 0; x < chunk.width * 4; ++x) {
				pic.pixels[chunk.left * 4 + x + (chunk.top + y) * header.width * 4] = result[x + y * chunkStride];
			}*/
			memcpy(&pic.pixels[chunk.left * 4 + (chunk.top + y) * header.width * 4], result + y * chunkStride, chunkStride);
		}
		delete[] buffer;
		delete[] result;
	}

	return pic;
}

Msk Archive::getMsk(const std::string &path) {
	auto &entry = get(path);

	std::lock_guard<std::mutex> lock(mutex_);

	BinaryReader br(ifs_);
	br.seekg(entry.offset);

	auto header = br.read<MskHeader>();

	Msk msk;
	msk.name = entry.name;
	msk.width = header.width;
	msk.height = header.height;
	std::vector<uint8_t> data;
	data.resize(entry.size);
	br.read((char *)data.data(), data.size());
	msk.pixels = DataCompression::decompress12_4(data.data(), data.size(), msk.width * msk.height);

	return msk;
}

void Archive::extractMsk(const std::string &path) {
	auto msk = getMsk(path);
	std::vector<unsigned char> expandedPixels(msk.width * msk.height * 4);
	for (uint32_t i = 0; i < msk.height; ++i) {
		for (uint32_t j = 0; j < msk.width; ++j) {
			expandedPixels[i * msk.width * 4 + j * 4 + 0] = msk.pixels[i * msk.width + j];
			expandedPixels[i * msk.width * 4 + j * 4 + 1] = msk.pixels[i * msk.width + j];
			expandedPixels[i * msk.width * 4 + j * 4 + 2] = msk.pixels[i * msk.width + j];
			expandedPixels[i * msk.width * 4 + j * 4 + 3] = 0xff;
		}
	}
	writeImage(msk.name + "_test.png", expandedPixels.data(), msk.width, msk.height, 4 * msk.width);
}

void Archive::extractPic(ArchiveEntry &entry) {
	BinaryReader br(ifs_);
	br.seekg(entry.offset);

	auto header = br.read<PicHeader>();

	std::vector<PicChunk> chunks(header.chunks);
	br.read((char *)chunks.data(), header.chunks * sizeof(PicChunk));

	Pic pic;
	pic.name = entry.name;
	pic.pixels.resize(4 * header.width * header.height);
	pic.width = header.width;
	pic.height = header.height;

	for (uint32_t i = 0; i < header.chunks; ++i) {
		PicChunk &chunk = chunks[i];

		auto size = chunk.size;
		unsigned char *buffer = new unsigned char[size];
		auto chunkStride = 4 * ((chunk.width + 3) & 0xfffc);
		unsigned char *result = new unsigned char[chunk.height * chunkStride];

		br.seekg(entry.offset + chunk.offset);
		br.read((char *)buffer, size);

		decode(buffer, size, result);
		dpcm(result, result, chunk.width, chunk.height, chunkStride);
		for (int y = 0; y < chunk.height; ++y) {
			/*for (int x = 0; x < chunk.width * 4; ++x) {
			pic.pixels[chunk.left * 4 + x + (chunk.top + y) * header.width * 4] = result[x + y * chunkStride];
			}*/
			memcpy(&pic.pixels[chunk.left * 4 + (chunk.top + y) * header.width * 4], result + y * chunkStride, chunkStride);
		}
		delete[] buffer;
		delete[] result;
	}
	writeImage(pic.name + "_test.png", pic.pixels.data(), header.width, header.height, 4 * header.width);
}

Bup Archive::getBup(const std::string &path) {
	auto &entry = get(path);

	std::lock_guard<std::mutex> lock(mutex_);

	BinaryReader br(ifs_);
	br.seekg(entry.offset);
	auto header = br.read<BupHeader>();
	std::cout << "Magic = " << std::hex << header.magic << std::dec << "\n";
	std::vector<BupChunk> chunks;
	chunks.reserve(header.chunks);

	for (uint32_t i = 0; i < header.chunks; ++i) {
		auto chunk = br.read<BupChunk>();
		chunks.push_back(chunk);
	}

	auto stride = 4 * ((header.width + 3) & 0xfffc);

	//unsigned char *data = new unsigned char[stride * header.height * 4];
	Bup bup;
	bup.pixels.resize(stride * header.height);

	unsigned char *buffer = new unsigned char[header.size];
	br.seekg(entry.offset + header.offset);
	br.read((char *)buffer, header.size);
	decode(buffer, header.size, bup.pixels.data());
	dpcm(bup.pixels.data(), bup.pixels.data(), header.width, header.height, stride);
	/*for (int i = stride; i < stride * header.height; ++i) {
		bup.pixels[i] += bup.pixels[i - stride];
	}*/
	//writeImage(bup.name + "_test.png", data, header.width, header.height, stride);

	delete[] buffer;

	bup.subentries.resize(header.chunks);
	for (uint32_t i = 0; i < header.chunks; ++i) {
		BupChunk &chunk = chunks[i];

		auto &subentry = bup.subentries[i];
		subentry.width = stride / 4;
		subentry.height = header.height;
		subentry.name = chunk.title;
		subentry.pixels.resize(stride * header.height);
		std::copy(bup.pixels.begin(), bup.pixels.end(), subentry.pixels.begin());

		if (chunk.picture[0].width == 0) continue;

		auto stride0 = 4 * ((chunk.picture[0].width + 3) & 0xfffc);
		unsigned char *xdata = new unsigned char[stride0 * chunk.picture[0].height];
		buffer = new unsigned char[chunk.picture[0].size];
		br.seekg(entry.offset + chunk.picture[0].offset);
		br.read((char *)buffer, chunk.picture[0].size);
		decode(buffer, chunk.picture[0].size, xdata);
		dpcm(xdata, xdata, chunk.picture[0].width, chunk.picture[0].height, stride0);

		auto w = chunk.picture[0].width;
		auto h = chunk.picture[0].height;
		auto dx = chunk.picture[0].left;
		auto dy = chunk.picture[0].top;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				int d = (x + dx) * 4 + (y + dy) * stride;
				int s = x * 4 + y * stride0;

				int da = subentry.pixels[d + 3];
				int sa = xdata[s + 3];

				if (sa != 0) {
					for (int j = 0; j < 4; ++j) {
						subentry.pixels[d + j] = xdata[s + j];
					}
				}
			}
		}
	}

	bup.width = stride / 4;
	bup.height = header.height;
	return bup;
}

void Archive::extractBup(ArchiveEntry &bup) {
	BinaryReader br(ifs_);
	br.seekg(bup.offset);
	auto header = br.read<BupHeader>();
	std::cout << "Magic = " << std::hex << header.magic << std::dec << "\n";
	std::vector<BupChunk> chunks;
	chunks.reserve(header.chunks);

	for (uint32_t i = 0; i < header.chunks; ++i) {
		auto chunk = br.read<BupChunk>();
		chunks.push_back(chunk);
	}

	auto stride = 4 * ((header.width + 3) & 0xfffc);

	unsigned char *data = new unsigned char[stride * header.height * 4];

	unsigned char *buffer = new unsigned char[header.size];
	br.seekg(bup.offset + header.offset);
	br.read((char *)buffer, header.size);
	decode(buffer, header.size, data);
	for (int i = stride; i < stride * header.height; ++i) {
		data[i] += data[i - stride];
	}
	writeImage(bup.name + "_test.png", data, header.width, header.height, stride);

	delete[] buffer;
	delete[] data;
}

Png Archive::getPng(const std::string &path) {
	auto pngData = read(path);
	Png png;
	png.name = path;
	auto *data = stbi_load_from_memory(pngData.data(), static_cast<int>(pngData.size()), (int *)&png.width, (int *)&png.height, 0, 4);
	png.pixels.resize(png.width * png.height * 4);
	std::copy(data, data + png.pixels.size(), png.pixels.data());
	stbi_image_free(data);
	return png;
}

struct ArchiveChunk {
	uint32_t nameOffset;
	uint32_t offset;
	uint32_t size;
};

void Archive::scan(uint64_t startOffset, ArchiveEntry &current, BinaryReader &br) {
	br.seekg(startOffset);
	auto count = br.read<uint32_t>();
	std::vector<ArchiveChunk> chunks(count);
	br.read((char *)chunks.data(), count * sizeof(ArchiveChunk));

	current.children.reserve(count);
	for (uint32_t i = 0; i < count; ++i) {
		auto &chunk = chunks[i];
		int isFolder = chunk.nameOffset & 0x80000000;
		chunk.nameOffset &= ~0x80000000;
		uint64_t offset = chunk.offset;
		offset <<= isFolder ? 4 : 11;

		br.seekg(startOffset + chunk.nameOffset);
		auto name = br.readString();

		if (name == "." || name == "..")
			continue;

		if (isFolder) {
			std::cout << "Reading folder '" << name << "' (parent = '" << current.name << "')...\n";
			auto &entry = current.children.emplace_back();
			entry.parent = &current;
			entry.isFolder = true;
			entry.name = std::move(name);
			entry.path = current.path + entry.name + "/";
			current.childrenNames.insert({ entry.name, current.children.size() - 1 });
			scan(offset, entry, br);
		} else {
			auto &entry = current.children.emplace_back();
			entry.parent = &current;
			entry.offset = offset;
			entry.size = chunk.size;
			entry.name = std::move(name);
			entry.path = current.path + entry.name;
			current.childrenNames.insert({ entry.name, current.children.size() - 1 });
		}
	}
}