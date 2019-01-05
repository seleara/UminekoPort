import sys
import struct

def main():
	if len(sys.argv) < 2 or (sys.argv[1] != "higu" and sys.argv[1] != "chiru" and sys.argv[1] != "umi"):
		print("Must specify higu or umi or chiru")
		return
	game = ""
	if sys.argv[1] == "higu":
		game = "higurashi"
	elif sys.argv[1] == "chiru":
		game = "chiru"
	elif sys.argv[1] == "umi":
		game = ""
	fontname = game == "higurashi" and "gothic" or "default"

	if len(sys.argv) == 3 and sys.argv[2] == "restore":
		with open(fontname + ".fnt", "rb") as f:
			newdata = list(f.read())
			with open(fontname + "_new.fnt", "wb") as nf:
				nf.write(bytearray(newdata))
			with open(game + "_data/DATA.ROM", "r+b") as rom:
				if game == "higurashi":
					rom.seek(0x453000)
				elif game == "chiru":
					rom.seek(0x138800)
				elif game == "":
					rom.seek(0x149000)
				rom.write(bytearray(newdata))
				return

	with open(fontname + ".fnt", "rb") as f:
		newdata = list(f.read())
		olddata = list(newdata)
		fileSize = struct.unpack('i', ''.join(newdata[4:8]))[0]
		startoff = 16

		def getOffset(index):
			off = startoff + index * 4
			offset = struct.unpack('i', ''.join(olddata[off:off+4]))[0]
			return offset

		def update(index, attr, val):
			offset = getOffset(index)
			newdata[offset + attr] = val

		def setData(index, data):
			offset = getOffset(index)
			newdata[offset + 6:offset + 6 + len(data)] = data

		def replace(src, dst):
			srcOffset = getOffset(src)
			srcBytes = 8 + ((ord(olddata[srcOffset + 6]) & 0xff) | ((ord(olddata[srcOffset + 7]) << 8) & 0xff00))
			srcData = olddata[srcOffset:srcOffset + srcBytes]
			dstOffset = getOffset(dst)
			if dstOffset >= fileSize:
				print("Offset is outside of file.")
				raw_input()
			newdata[dstOffset:dstOffset + srcBytes] = srcData
			#print("Replacing glyph {}[{:08X}] with {}[{:08X}]. Validity = {}".format(dst, dstOffset, src, srcOffset, ''.join("{:02X} ".format(ord(newdata[getOffset(0)+i])) for i in range(10))))
			#raw_input()

		#newdata[getOffset(0):len(newdata) - getOffset(0)] = [0 for i in range(len(newdata) - getOffset(0))]
		for i in range(3000):
			replace(0, i + 1)
		#replace(1, 443)
		#replace(133, 98)
		#replace(0x81fb - 0x8140 + 96, 98)
		#replace(299, 98)
		#update(98, 59, 0x03)
		#update(98, 60, 0x03)
		#update(98, 26, 0x04)
		#update(98, 65, 0x02)
		#update(98, 56, 0x67)
		#update(98, 8, 0xf0)
		#update(96, 7, 0xff)
		#update(96, 8, 0xff)
		#update(96, 9, 0xff)
		#update(96, 10, 0xff)
		#update(98, 8, 0x00)
		#update(98, 15, 0x2c)
		#update(98, 16, 0x01)
		#for i in range(10):
		#	update(98, 6 + i, 0xff)
		#update(3, 0, 100)
		#for i in range(8180):
		#	update(i, 7, i % 255)

		with open(fontname + "_new.fnt", "wb") as nf:
			nf.write(bytearray(newdata))

		with open(game + "_data/DATA.ROM", "r+b") as rom:
			if game == "higurashi":
				rom.seek(0x453000)
			elif game == "chiru":
				rom.seek(0x138800)
			elif game == "":
				rom.seek(0x149000)
			rom.write(bytearray(newdata))

if __name__ == '__main__':
	main()