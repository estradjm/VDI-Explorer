# Compiler
CC=g++

# Compiler flags
CFLAGS=-c -Wall -g -std=c++11

# Linker flags
LDFLAGS=

# Source files
SOURCES=main.cpp vdi_reader.cpp ext2.cpp

# Object files
OBJECTS=$(SOURCES:.cpp=.o)

# Executable
EXECUTABLE=vdi

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
