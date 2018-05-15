import sys

def restore():
	with open("data/DATA.ROM", "r+b") as f:
		f.seek(0x532800)

		with open("export/original.snr", "rb") as snr:
			data = snr.read()

			f.write(data)

def dump():
	f = open("data/DATA.ROM", "rb")
	f.seek(0x532800)

	orig = open("export/original.snr", "wb")

def patch():
	with open("data/DATA.ROM", "r+b") as f:
		f.seek(0x532800)

		with open("patch.snr", "rb") as snr:
			data = snr.read()

			f.write(data)

def main():
	if len(sys.argv) < 2:
		return
	if sys.argv[1] == "p":
		patch()
	elif sys.argv[1] == "d":
		dump()
	elif sys.argv[1] == "r":
		restore()

if __name__ == '__main__':
	main()