CC = g++
CCFLAGS = -g -std=c++17
INCLUDES =
# LIBRARIES = -lboost_system -lboost_thread -lpthread -lrt 
LIBRARIES = -lpthread
EXECUTABLES = pipegrep

pipegrep: pipegrep.o
	$(CC) $(CCFLAGS) $(INCLUDES) -o pipegrep pipegrep.o $(LIBRARIES)

# Rule for generating .o file from .cpp file
%.o: %.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $^ 

# All files to be generated
all: pipegrep

# Clean the directory
clean: 
	rm -f $(EXECUTABLES)  *.o
