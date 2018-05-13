#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>

class BinaryReader;

struct ArchiveEntry {
	ArchiveEntry *parent = nullptr;
	std::vector<ArchiveEntry> children;
	std::map<std::string, int, std::less<>> childrenNames;

	std::string name;
	bool isFolder = false;
	uint64_t offset;
	uint32_t size;
};

struct Bup {
	std::string name;
	uint32_t width, height;
	std::vector<unsigned char> pixels;
	struct SubEntry {
		std::string name;
		uint32_t width, height;
		std::vector<unsigned char> pixels;
	};
	std::vector<SubEntry> subentries;
};

struct Txa {
	std::string name;
	struct SubEntry {
		std::string name;
		uint32_t width, height;
		std::vector<unsigned char> pixels;
	};
	std::vector<SubEntry> subentries;
};

struct Pic {
	std::string name;
	uint32_t width, height;
	std::vector<unsigned char> pixels;
};

struct Msk {
	std::string name;
	uint32_t width, height;
	std::vector<unsigned char> pixels;
};

class Archive {
public:
	void open(const std::string &path);
	void explore();
	std::vector<unsigned char> read(const std::string &path);
	Txa getTxa(const std::string &path);
	Bup getBup(const std::string &path);
	Pic getPic(const std::string &path);
	Msk getMsk(const std::string &path);
private:
	ArchiveEntry &get(const std::string &path);
	void scan(uint64_t startOffset, ArchiveEntry &current, BinaryReader &br);
	void explore(ArchiveEntry &folder);

	void decode(const unsigned char *buffer, size_t bufferSize, unsigned char *output);
	void dpcm(unsigned char *source, unsigned char *destination, int width, int height, int scanline);
	void writeImage(const std::string &path, unsigned char *data, int width, int height, int scanline);
	void extractTxa(ArchiveEntry &txa);
	void extractBup(ArchiveEntry &bup);
	void extractPic(ArchiveEntry &pic);

	ArchiveEntry root_;

	std::ifstream ifs_;
};