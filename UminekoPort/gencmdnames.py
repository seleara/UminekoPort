if __name__ == '__main__':
	names = [("\"command_" + "{:02X}".format(i) + "\"") for i in range(0, 256)]
	strs = ", ".join(names)
	print(strs)