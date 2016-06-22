vdi: vdi_reader.h ext2_fs.h boot.h vdi_reader.cpp main.cpp
	g++ -std=c++11 -g vdi_reader.cpp main.cpp -o vdi

