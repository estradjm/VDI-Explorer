# Compiler
#CC=g++
CC=gcc

# Compiler flags
CFLAGS=-Wall -g -std=c++11 -fprofile-arcs -ftest-coverage
#CFLAGS=-c -Wall -g -O0 -std=c++11 


# Linker flags
LDFLAGS=

# Source files
SOURCES=src/main.cpp src/ext2.cpp src/interface.cpp src/utility.cpp src/vdi_reader.cpp
#SOURCES=main.cpp exceptions.cpp ext2.cpp interface.cpp utility.cpp vdi_reader.cpp

# Object files
OBJECTS=$(SOURCES:.cpp=.o)

# Miscellaneous debug files
MISCDEBUG=*.cpp.*

# Executable
EXECUTABLE=vdi

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm build/$(EXECUTABLE)

clean_debug:
	rm $(OBJECTS) $(EXECUTABLE) $(MISCDEBUG)
