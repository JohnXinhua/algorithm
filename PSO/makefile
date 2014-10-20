TARGET=PSO_MPI
SOURCES=main_mpi.cpp 
CC=mpicxx

CFLAGS=-c -Wall
OBJECTS=$(SOURCES:.cpp=.o)
#----------------------------------------------
all: $(SOURCES) $(TARGET) 

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.core *.o $(TARGET)

