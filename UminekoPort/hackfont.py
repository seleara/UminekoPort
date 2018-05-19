import sys
import struct

def main():
	with open("default.fnt", "rb") as f:
		newdata = list(f.read())
		fileSize = struct.unpack('i', ''.join(newdata[4:8]))[0]
		startoff = 16

		def getOffset(index):
			off = startoff + index * 4
			offset = struct.unpack('i', ''.join(newdata[off:off+4]))[0]
			return offset

		def update(index, attr, val):
			offset = getOffset(index)
			newdata[offset + attr] = val

		def setData(index, data):
			offset = getOffset(index)
			newdata[offset + 6:offset + 6 + len(data)] = data

		def replace(src, dst):
			srcOffset = getOffset(src)
			srcBytes = 8 + ((ord(newdata[srcOffset + 6]) & 0xff) | ((ord(newdata[srcOffset + 7]) << 8) & 0xff00))
			srcData = newdata[srcOffset:srcOffset + srcBytes]
			dstOffset = getOffset(dst)
			newdata[dstOffset:dstOffset + srcBytes] = srcData

		#replace(133, 98)
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

		with open("newfont.fnt", "wb") as nf:
			nf.write(bytearray(newdata))

		with open("data/DATA.ROM", "r+b") as rom:
			rom.seek(0x149000)
			rom.write(bytearray(newdata))

if __name__ == '__main__':
	main()