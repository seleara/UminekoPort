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
			newData[offset + 6:offset + 6 + len(data)] = data

		#update(96, 7, 0xff)
		#update(96, 8, 0xff)
		#update(96, 9, 0xff)
		#update(96, 10, 0xff)
		update(98, 8, 0x90)
		#update(98, 15, 0x2c)
		#update(98, 16, 0x01)
		#for i in range(10):
		#	update(98, 6 + i, 0xff)
		#update(3, 0, 100)
		#for i in range(8180):
		#	offset = getOffset(i)
		#	update(i, 9, 0x65)

		with open("newfont.fnt", "wb") as nf:
			nf.write(bytearray(newdata))

if __name__ == '__main__':
	main()