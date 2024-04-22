CC = g++
CCFLAGS = -g -std=c++17
INCLUDES =
# LIBRARIES = -lboost_system -lboost_thread -lpthread -lrt 
LIBRARIES = -lpthread
EXECUTABLES = pipgrep

pipgrep: pipgrep.o
	$(CC) $(CCFLAGS) $(INCLUDES) -o pipgrep pipgrep.o $(LIBRARIES)

# Rule for generating .o file from .cpp file
%.o: %.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $^ 

# All files to be generated
all: producer-consumer

# Clean the directory
clean: 
	rm -f $(EXECUTABLES)  *.o
