import serial

FOLDER = "samples"
FILE_PREFIX = "serve"
file_num = 0
sampling = False

csv_temp = []

dev = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
print(f"Connected to {dev.name}")

while True:
	if dev.in_waiting <= 0:
		continue

	line = dev.readline().decode('utf-8').removesuffix("\n")
	#print(line)
	if line == "Num,Acc_X,Acc_Y,Acc_Z":
		print("Start")
		sampling = True
	elif line == "EOF":
		print("Done")
		sampling = False

		with open(f"{FOLDER}/{FILE_PREFIX}{file_num}.csv", "w", encoding="utf-8") as f:
			for string in csv_temp:
				f.write(f"{string}\n")
		file_num += 1
		csv_temp.clear()

	if sampling == True:
		csv_temp.append(line)
