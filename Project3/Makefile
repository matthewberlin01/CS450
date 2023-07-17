
CFLAG=-std=c++20
boundaryTagApp.x: BoundaryTag.o driver.o
	g++ $(CFLAGS)  BoundaryTag.o driver.o -o boundaryTagApp.x

BoundaryTag.o: BoundaryTag.cpp BoundaryTag.hpp
	g++  $(CFLAGS) -c BoundaryTag.cpp -o BoundaryTag.o

driver.o: driver.cpp BoundaryTag.hpp
	g++  $(CFLAGS) -c driver.cpp -o driver.o

driver2.o :driver2.cpp BoundaryTag.hpp
	g++  $(CFLAGS) -c driver2.cpp -o driver2.o

clean:
	rm -f BoundaryTag.o driver.o boundaryTagApp.x core *~
