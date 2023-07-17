
#ifndef _BoundaryTag_hpp
#define _BoundaryTag_hpp
#define DEBUG false
#include <math.h>
#include <iostream>

class BoundaryTag {
    enum { SIZE = 4096, BYTES_PER_WORD = sizeof(int), FREE_OVERHEAD = 4 }; 
public:
    BoundaryTag();
    void* allocate(int numBytes); // allocate a block of memory with "numBytes" bytes
    void free(void *ptrToMem);    // recycle the memory that "ptrToMem" points to.
    void start();               
    void* next();
    bool isFree(void *ptrToMem);
    int size(void *ptr); 

private:
    int memory[SIZE];
    int freeIdx;
    int iterIdx;


    void leftCoalesce(int &currentLeftBoundary, int coalesceLeftBoundary);
    void rightCoalesce(int currentLeftBoundary, int coalesceLeftBoundary);
    void leftRightCoalesce(int &currentLeftBoundary, int leftCoalesceLeftBoundary, int rightCoalesceLeftBoundary);
    int internalSize(void *ptrToMem);
    void* internalNext();
    void internalStart();
};

#endif
