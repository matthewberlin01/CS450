#include "BoundaryTag.hpp"

/********************************************************************************
*   Function:   BoundaryTag                                                     *
*   Parameters: None                                                            *
*   Return Value: None                                                          *
*   Description: Initializes the freeIdx and iterIdx to the start of memory,    *
*                sets the initial boundary tags of memory, and sets the         *
*                previous and next pointers to null, respectively.              *
********************************************************************************/
BoundaryTag::BoundaryTag()
{
    freeIdx = 0;
    iterIdx = 0;
    //make sure all of memory is clean.
    for(int i = 0; i < SIZE; i++)
    {
        memory[i] = 0;
    }

    memory[0] = SIZE * -1;
    memory[SIZE - 1] = SIZE * -1;
    memory[1] = -1;
    memory[2] = -1;
}

/********************************************************************************
*   Function:   allocate                                                        *
*   Parameters: int numBytes                                                    *
*   Retun Value: void *ptrToMemBlock                                            *
*   Description: Allocates a specified number of bytes to memory. Creates the   *
*                 overhead for the boundary tags within memory and returns the  *
*                 address of the first available position that has been         *
*                 allocated.                                                    *
********************************************************************************/
void* BoundaryTag::allocate(int numBytes)
{
    //initialize the iterator to begin a search through memory.
    internalStart();
    //Create a pointer to a memory block, starting at the first free index.
    void *ptrToMemBlock = &memory[freeIdx];
    int allocatedSpace = (ceil(numBytes / 4.0) + 2) * 4;

    if(DEBUG)
    {
        std::cout << "calling allocate" << std::endl;
        std::cout << "ptrToMemBlock: " << ptrToMemBlock << std::endl;
        std::cout << "numBytes: " << numBytes << std::endl;
    }

    if(allocatedSpace < 16)
    {
        if(DEBUG)
        {
            std::cout << "space to allocate is not enough to store a free space. No allocation will occur. \n Attempted allocated space: " << allocatedSpace << std::endl;
        }
        return nullptr;
    }

    //While there are still free spaces, continue to the next free space.
    do
    {
        //If the current pointer to a free space has a size less than or equal to that of the number of bytes to allocate, then
        //allocate that number of bytes at the pointer to free space.
        if(internalSize(ptrToMemBlock) >= allocatedSpace / 4 && SIZE - allocatedSpace / 4 >= 16)
        {
            allocatedSpace /= 4;

            if(DEBUG)
            {
                std::cout << "iterIdx: " << iterIdx << std::endl;
                std::cout << "memory[" << iterIdx << "]: " << memory[iterIdx] << std::endl;
                std::cout << "ceil(numBytes / 4): " << ceil(numBytes / 4.0) << std::endl;
                std::cout << "allocatedSpace: " << allocatedSpace << std::endl;
                std::cout << "memory[" << abs(memory[iterIdx]) - 1 << "] before updating: " << memory[abs(memory[iterIdx]) - 1] << std::endl;
                std::cout << "memory[" << iterIdx + 1 << "] before updating: " << memory[iterIdx + 1] << std::endl;
                std::cout << "memory[" << iterIdx + 2 << "] before updating: " << memory[iterIdx + 2] << std::endl;
                std::cout << "memory[" << iterIdx << "] before updating: " << memory[iterIdx] << std::endl;
                std::cout << std::endl;
            }

            //Go to the boundary of the current free space and update its value to that of the newly allocated space.
            memory[iterIdx + abs(memory[iterIdx]) - 1] = allocatedSpace;

            //If the value of the available space minus the allocated space is not enough to store a new value, then include
            //the remaining space with the allocated space and upate the freeIdx to the next available position, if there is any.
            if(abs(memory[iterIdx]) - allocatedSpace < 4)
            {
                memory[iterIdx] *= -1;
                memory[memory[iterIdx] - 1] = memory[iterIdx];
                //As long as the current free space has a next pointer, set the freeIdx to that pointer and update
                //that pointer's previous pointer to null since it is now the start of the array.
                if(memory[iterIdx + 2] != -1)
                {
                    freeIdx = memory[iterIdx + 2];
                    memory[freeIdx + 1] = -1;
                }
                memory[iterIdx + 1] = 0;
                memory[iterIdx + 2] = 0;

                return memory + iterIdx + 1;
            }
            else
            {
                //Go to the location of the beginning of the newly allocated space and set that to its allocated space value.
                memory[iterIdx + abs(memory[iterIdx]) - allocatedSpace] = allocatedSpace;
                //Go to the new boundary of the current free space and update that value to the remaining free space.
                memory[iterIdx + abs(memory[iterIdx]) - allocatedSpace - 1] = (abs(memory[iterIdx]) - allocatedSpace) * -1;
                //Update the beginning of the free space to have its new remaining space.
                memory[iterIdx] = (abs(memory[iterIdx]) - allocatedSpace) * -1;
            }
            
            if(DEBUG)
            {
                int allocatedIndex = abs(memory[iterIdx]) + 1;
                std::cout << "memory[" << iterIdx << "] = " << memory[iterIdx] << std::endl;
                std::cout << "memory[" << abs(memory[iterIdx]) - 1 << "] = " << memory[abs(memory[iterIdx]) - 1] << std::endl;
                std::cout << "memory[" << abs(memory[iterIdx]) << "] = " << memory[abs(memory[iterIdx])] << std::endl;
                std::cout << "returned address - memory = " << (iterIdx + abs(memory[iterIdx]) + 1);
                std::cout << std::endl;
            }
            return memory + (iterIdx + abs(memory[iterIdx]) + 1);
        }
    }
    while(ptrToMemBlock = internalNext());
    if(DEBUG)
    {
        std::cout << "allocated nothing, no free spaces." << std::endl;
        std::cout << std::endl;
    }
    return nullptr;
}

/********************************************************************************
*   Function:   free                                                            *
*   Parameters: void *ptrToMem                                                  *
*   Retun Value: None                                                           *
*   Description: frees ptrToMem from memory. First updates the boundary tags of *
*                the allocated space to denote that it is free space. If the    *
*                freed space can be coalesced with either adjacent space, then  *
*                the newly freed space will attempt to coalesce with them. If   *
*                the newly freed space cannot coalesce with either adjacent     *
*                space, then the newly freed space will set its next and        *
*                previous pointers.                                             *
********************************************************************************/
void BoundaryTag::free(void *ptrToMem)
{
    //create a pointerIndex to the location of the freed memory and set the index to its proper position within memory.
    int* pointerIndex = (int*)ptrToMem;
    int index = pointerIndex - memory - 1;

    if(DEBUG)
    {
        std::cout << "calling free" << std::endl;
        std::cout << "pointerIndex: " << pointerIndex << std::endl;
        std::cout << "index: " << index << std::endl;
        std::cout << "memory[" << index << "] = " << memory[index] << std::endl;
        std::cout << std::endl;
    }
    
    //Update the size overhead at the end of the allocated space to notifiy that it is now free.
    memory[index + memory[index] - 1] *= -1;
    //Update the size overhead at the beginning of the allocated space to notify that it is now free.
    memory[index] *= -1;

    //Run through the entire free pool until you get to the end, all the while keeping track of the previous iterIdx.
    internalStart();
    void *ptrToMemBlock = &freeIdx;
    int previousIndex = freeIdx;
    while(ptrToMemBlock = internalNext())
    {
        if(ptrToMemBlock != nullptr && iterIdx != -1)
        {
            previousIndex = iterIdx;
        }
    }

    //Set the next pointer at the previous last free space to that of the new free space.
    memory[previousIndex + 2] = index;
    //Set the previous pointer at the new free space to that of the previous free space.
    memory[index + 1] = previousIndex;
    //Set the next pointer at the new free space to -1 to indicate that it is the last.
    memory[index + 2] = -1;


    //If the left and right adjacent memory can be coalesced, then coalesce.
    if((index != 0 && index + abs(memory[index]) != SIZE) && memory[index - 1] < 0 && memory[index + abs(memory[index])] < 0){
        leftRightCoalesce(index, index - abs(memory[index - 1]), index + abs(memory[index]));
    }
    //If the left adjacent memory space can be coalesced, then coalsece.
    else if(index != 0 && memory[index - 1] < 0)
    {
        leftCoalesce(index, index - abs(memory[index - 1]));
    }
    //If the right adjacent memory space can be coalesced, then coalesce.
    else if((index + abs(memory[index])) != SIZE && memory[index + abs(memory[index])] < 0)
    {
        rightCoalesce(index, index + abs(memory[index]));
    }
    
    if(DEBUG)
    {
        std::cout << "previousIndex = " << previousIndex << std::endl;
        std::cout << "memory[" << index << "] = " << memory[index] << std::endl;
        std::cout << "memory[" << index + abs(memory[index]) - 1 << "] = " << memory[index + abs(memory[index]) - 1] << std::endl;
        std::cout << "memory[" << index + 1 << "] = " << memory[index + 1] << std::endl;
        std::cout << "memory[" << index + 2 << "] = " << memory[index + 2] << std::endl;
        std::cout << "memory[" << previousIndex + 2 << "] (new space's prev's next pointer) = " << memory[previousIndex + 2] << std::endl;
        std::cout << std::endl;
    }
}

/********************************************************************************
*   Function:   leftCoalesce                                                    *
*   Parameters: int currentLeftBoundary, int coalesceLeftBoundary               *
*   Retun Value: None                                                           *
*   Description: coalesces the current space being freed with the left adjacent *
*                space. Removes the current pointer to the left boundary on the *
*                second to last memory, frees the overhead of the current left  *
*                boundary and the coalesced space's right boundary, as well as  *
*                removes the current space's overhead for previous and next     *
*                pointers, as it was the last in the linked list.               *
********************************************************************************/
void BoundaryTag::leftCoalesce(int &currentLeftBoundary, int coalesceLeftBoundary)
{
    if(DEBUG)
    {
        std::cout << "calling leftCoalesce" << std::endl;
        std::cout << "currentLeftBoundary = " << currentLeftBoundary << std::endl;
        std::cout << "coalesceLeftBoundary = " << coalesceLeftBoundary << std::endl;
    }

    //Set newBoundaryValue to the sum of the currentLeftBoundary and the and coalesceLeftBoundary after signifying it is an available space.
    int newBoundaryValue = (abs(memory[currentLeftBoundary]) + abs(memory[coalesceLeftBoundary])) * -1;
    //Set the right boundary of the newly coalesced space to the newBoundaryValue.
    memory[currentLeftBoundary + abs(memory[currentLeftBoundary]) - 1] = newBoundaryValue;
    //Set the left boundary of the newly coalesced space to the newBoundaryValue.
    memory[coalesceLeftBoundary] = newBoundaryValue;
    //Set the next pointer on the currently freed space's previous pointer to null since the memory has been coalesced into another space.
    memory[memory[currentLeftBoundary + 1] + 2] = -1;

    //Remove the extra unnecessary overhead from the coalesced space.
    memory[currentLeftBoundary - 1] = 0;
    memory[currentLeftBoundary] = 0;
    memory[currentLeftBoundary + 1] = 0;
    memory[currentLeftBoundary + 2] = 0;

    if(DEBUG)
    {
        std::cout << "memory[" << coalesceLeftBoundary << "] = " << memory[coalesceLeftBoundary] << std::endl;
        std::cout << "memory[" << coalesceLeftBoundary + 1 << "] = " << memory[coalesceLeftBoundary + 1] << std::endl;
        std::cout << "memory[" << coalesceLeftBoundary + 2 << "] = " << memory[coalesceLeftBoundary + 2] << std::endl;
        std::cout << "memory[" << currentLeftBoundary - 1 << "] = " << memory[currentLeftBoundary] << std::endl;
        std::cout << "memory[" << currentLeftBoundary << "] = " << memory[currentLeftBoundary] << std::endl;
        std::cout << "memory[" << currentLeftBoundary + 1 << "] = " << memory[currentLeftBoundary + 1] << std::endl;
        std::cout << "memory[" << currentLeftBoundary + 2 << "] = " << memory[currentLeftBoundary + 2] << std::endl;
        std::cout << "memory[" << abs(memory[coalesceLeftBoundary]) - 1 << "] = " << memory[abs(memory[coalesceLeftBoundary]) - 1 ] << std::endl;
        std::cout << std::endl;
    }

    //Update the current left boundary to refer to the newly coalesced left boundary position. This is useful when
    //performing a right coalescence, as the index used for currentLeftBoundary would be out of date when performing a
    //right coalescence.
    currentLeftBoundary = coalesceLeftBoundary;
}

/********************************************************************************
*   Function:   rightCoalesce                                                   *
*   Parameters: int currentLeftBoundary, int coalesceLeftBoundary               *
*   Retun Value: None                                                           *
*   Description: coalesces the current space being freed with the right adjacent*
*                space. Removes the current pointer to the left boundary on the *
*                second to last memory, frees the overhead of the coalesced     *
*                boundary and the current boundary's right boundary.            *
********************************************************************************/
void BoundaryTag::rightCoalesce(int currentLeftBoundary, int coalesceLeftBoundary)
{
    //Set newBoundaryValue to the sum of the currentLeftBoundary and the and coalesceLeftBoundary after signifying it is an available space.
    int newBoundaryValue = (abs(memory[currentLeftBoundary]) + abs(memory[coalesceLeftBoundary])) * -1;

    if(DEBUG)
    {
        std::cout << "calling rightCoalesce" << std::endl;
        std::cout << "currentLeftBoundary = " << currentLeftBoundary << std::endl;
        std::cout << "coalesceLeftBoundary = " << coalesceLeftBoundary << std::endl;
        std::cout << "newBoundaryValue = " << newBoundaryValue << std::endl;
    }

    //Set the right boundary of the newly coalesced space to the newBoundaryValue.
    memory[coalesceLeftBoundary + abs(memory[coalesceLeftBoundary]) - 1] = newBoundaryValue;
    //Set the left boundary of the newly coalesced space to the newBoundaryValue.
    memory[currentLeftBoundary] = newBoundaryValue;

    //Set the next pointer of the coalesced spaces's previous pointer to that of the coalesced space's next pointer.
    memory[memory[coalesceLeftBoundary + 1] + 2] = currentLeftBoundary;
    //Set the previous pointer of the coalesced space's next pointer to that of the current space.
    memory[memory[coalesceLeftBoundary + 2] + 1] = currentLeftBoundary;
    //Set the next pointer of the current space's previous pointer to null, since it should no longer point to the coalesced space.
    memory[memory[currentLeftBoundary + 1] + 2] = -1;

    //Set the previous pointer of the newly coalesced memory to that of the coalesced space's previous pointer.
    memory[currentLeftBoundary + 1] = memory[coalesceLeftBoundary + 1];
    //Set the next pointer of the newly coalesced memory to that of the coalesced space's next pointer.
    memory[currentLeftBoundary + 2] = memory[coalesceLeftBoundary + 2];

    //If the next pionter of the newly coalesced memory points to any of the given boundaries, set the
    //next pointer to null since it was pointing to itself.
    if(memory[currentLeftBoundary + 2] == currentLeftBoundary || memory[currentLeftBoundary + 2] == coalesceLeftBoundary)
    {
        memory[currentLeftBoundary + 2] = -1;
    }
    
    //If the freeIdx currently points to the coalesced space, then set the freeIdx to the newly coalesced space.
    if(freeIdx == coalesceLeftBoundary)
    {
        freeIdx = currentLeftBoundary;
    }

    //Remove the extra unnecessary overhead from the coalesced space.
    memory[coalesceLeftBoundary - 1] = 0;
    memory[coalesceLeftBoundary] = 0;
    memory[coalesceLeftBoundary + 1] = 0;
    memory[coalesceLeftBoundary + 2] = 0;

    if(DEBUG)
    {
        std::cout << "memory[" << currentLeftBoundary << "] = " << memory[currentLeftBoundary] << std::endl;
        std::cout << "memory[" << currentLeftBoundary + 1 << "] = " << memory[currentLeftBoundary + 1] << std::endl;
        std::cout << "memory[" << currentLeftBoundary + 2 << "] = " << memory[currentLeftBoundary + 2] << std::endl;
        std::cout << "memory[" << coalesceLeftBoundary - 1 << "] = " << memory[coalesceLeftBoundary - 1 ] << std::endl;
        std::cout << "memory[" << coalesceLeftBoundary << "] = " << memory[coalesceLeftBoundary] << std::endl;
        std::cout << "memory[" << coalesceLeftBoundary + 1 << "] = " << memory[coalesceLeftBoundary + 1] << std::endl;
        std::cout << "memory[" << coalesceLeftBoundary + 2 << "] = " << memory[coalesceLeftBoundary + 2] << std::endl;
        std::cout << "memory[" << currentLeftBoundary + abs(memory[currentLeftBoundary]) - 1 << "] = " << memory[currentLeftBoundary + abs(memory[currentLeftBoundary]) - 1] << std::endl;
        std::cout << "memory[" << abs(memory[currentLeftBoundary + 1]) + 2 << "] = " << memory[abs(memory[currentLeftBoundary + 1]) + 2] << std::endl;
        std::cout << std::endl;
    }
}

/********************************************************************************
*   Function:   leftRightCoalesce                                               *
*   Parameters: int currentLeftBoundary, int leftCoalesceLeftBoundary,          *
*               int rightCoalesceRightBoundary                                  *
*   Retun Value: None                                                           *
*   Description: coalesces the current space being freed with both the left and *
*                right adjacent memory spaces. If the left coalesced space's    *
*                next pointer points to the current space, the newly coalesced  *
*                memory will use the right coalesced space's and vice versa.    *
*                Otherwise, the newly coalesced space will use the left         *
*                coalesced space's previous pointer, the right coalesced space's*
*                next pointer, set the current space's previous pointer to null,*
*                and free the unnecessary overhead from the coalesced spaces.   *
********************************************************************************/
void BoundaryTag::leftRightCoalesce(int &currentLeftBoundary, int leftCoalesceLeftBoundary, int rightCoalesceLeftBoundary)
{
    

    //Set newBoundaryValue to the sum of the currentLeftBoundary and the and coalesceLeftBoundary after signifying it is an available space.
    int newBoundaryValue = (abs(memory[currentLeftBoundary]) + abs(memory[leftCoalesceLeftBoundary]) + abs(memory[rightCoalesceLeftBoundary])) * -1;

    if(DEBUG)
    {
        std::cout << "calling leftRightCoalesce" << std::endl;
        std::cout << "currentLeftBoundary = " << currentLeftBoundary << std::endl;
        std::cout << "leftCoalesceLeftBoundary = " << leftCoalesceLeftBoundary << std::endl;
        std::cout << "rightCoalesceLeftBoundary = " << rightCoalesceLeftBoundary << std::endl;
        std::cout << "newBoundaryValue = " << newBoundaryValue << std::endl;
        std::cout << std::endl;
    }

    //Set the right boundary of the newly coalesced space to that of the newBoundaryValue.
    memory[rightCoalesceLeftBoundary + abs(memory[rightCoalesceLeftBoundary]) - 1] = newBoundaryValue;
    //Set the left boundary of the newly coalesced space to that of the newBoundaryValue.
    memory[leftCoalesceLeftBoundary] = newBoundaryValue;

    //If the left coalesced space's next pointer is the currentLeftBoundary, use the right coalesced space's pointers.
    if(memory[leftCoalesceLeftBoundary + 2] == currentLeftBoundary)
    {
        if(DEBUG)
        {
            std::cout << "left coalesce's next == current" << std::endl;
        }
        
        //First update the next pointer of the left coalesced space's previous pointer to null.
        memory[memory[leftCoalesceLeftBoundary + 1] + 2] = -1;
        memory[leftCoalesceLeftBoundary + 1] = memory[rightCoalesceLeftBoundary + 1];
        memory[leftCoalesceLeftBoundary + 2] = memory[rightCoalesceLeftBoundary + 2];
        
        //Update the previous pointer of the newly coalesced memory's next pointer to the newly coalesced space,
        //as long as leftCoalesceLeftBoundary's next pointer is not null, as this can cause issues when coalescing
        //the memory back to being completely free.
        if(memory[leftCoalesceLeftBoundary + 2] != -1)
        {
            memory[memory[leftCoalesceLeftBoundary + 2] + 1] = leftCoalesceLeftBoundary;
        }

        //Update the next pointer of the newly coalesced memory's previous pointer to the newly coalesced space,
        //as long as leftCoalesceLeftBoundary's previous pointer is not null, as this can cause issues when coalescing
        //the memory back to being completely free.
        if(memory[leftCoalesceLeftBoundary + 1] != -1)
        {
            memory[memory[leftCoalesceLeftBoundary + 1] + 2] = leftCoalesceLeftBoundary; 
        }
    }
    //If the right coalesced space's next pointer is currentLeftBoundary, use the left coalesced space's pointers.
    //We only need to update the next pointer of the right coalesced space's previous pointer to null since the newly
    //coalesced memory already uses the left coalesced space's pointers.
    else if(memory[rightCoalesceLeftBoundary + 2] == currentLeftBoundary)
    {
        if(DEBUG)
        {
            std::cout << "right coalesce's next == current" << std::endl;
        }
        memory[memory[rightCoalesceLeftBoundary + 1] + 2] = -1;
    }
    //Otherwise, use the left coalseced space's previous pointer and the right coalesced space's next pointer.
    else
    {
        if(DEBUG)
        {
            std::cout << "none of the above, normal leftRightCoalesce" << std::endl;
        }

        //If the previous pointer of leftCoalesceLeftBoundary points to rightCoalesceLeftBoundary, use rightCoalesceLeftBoundary's
        //previous pointer and update the next pointer of the newly coalesced space's previous pointer to the proper position.
        if(memory[leftCoalesceLeftBoundary + 1] == rightCoalesceLeftBoundary)
        {
            memory[leftCoalesceLeftBoundary + 1] = memory[rightCoalesceLeftBoundary + 1];
            memory[memory[leftCoalesceLeftBoundary + 1] + 2] = leftCoalesceLeftBoundary;
        }

        //As long as the next pointer of rightCoalesceLeftBoundary doesn't point to leftCoalesceLeftBoundary, set the
        //next pointer of the newly coalesced space to the next pointer of the rightCoalesceLeftBoundary and update the
        //previous pinter of the newly coalesced space to point to its current position.
        if(memory[rightCoalesceLeftBoundary + 2] != leftCoalesceLeftBoundary)
        {
            memory[leftCoalesceLeftBoundary + 2] = memory[rightCoalesceLeftBoundary + 2];
            memory[memory[leftCoalesceLeftBoundary + 2] + 1] = leftCoalesceLeftBoundary;
        }
        
        //Update the next pointer of the current space's previous pointer to null, since it is now the last free space.
        memory[memory[currentLeftBoundary + 1] + 2] = -1;
    }

    if(DEBUG)
    {
        std::cout << "memory[" << leftCoalesceLeftBoundary << "] = " << memory[leftCoalesceLeftBoundary] << std::endl;
        std::cout << "memory[" << rightCoalesceLeftBoundary + abs(memory[rightCoalesceLeftBoundary]) - 1 << "] = " << memory[rightCoalesceLeftBoundary + abs(memory[rightCoalesceLeftBoundary]) - 1] << std::endl;
        std::cout << std::endl;
    }


    //If the newly coalesced memory's previous pointer points to any of the given left boundaries, then
    //set it to null as it is now pointing to itself.
    if(memory[leftCoalesceLeftBoundary + 1] == currentLeftBoundary || memory[leftCoalesceLeftBoundary + 1] == leftCoalesceLeftBoundary || memory[leftCoalesceLeftBoundary + 1] == rightCoalesceLeftBoundary)
    {
        memory[leftCoalesceLeftBoundary + 1] = -1;
    }
    //If the newly coalesced memory's next pointer points to any of the given left boundaries, then
    //set it to null as it is now pointing to itself.
    if(memory[leftCoalesceLeftBoundary + 2] == currentLeftBoundary || memory[leftCoalesceLeftBoundary + 2] == leftCoalesceLeftBoundary || memory[leftCoalesceLeftBoundary + 2] == rightCoalesceLeftBoundary)
    {
        memory[leftCoalesceLeftBoundary + 2] = -1;
    }


    //If the freeIdx was pointing to the right coalesced space, we need to change it to point to the new beginning
    //of the newly coalesced space. We don't need to check the leftCoalesced space as that is already the beginning
    //of the newly coalesced space.
    if(freeIdx == rightCoalesceLeftBoundary)
    {
        freeIdx = leftCoalesceLeftBoundary;
    }


    //Remove the extra unnecessary overhead from the coalesced space.
    memory[currentLeftBoundary - 1] = 0;
    memory[currentLeftBoundary] = 0;
    memory[currentLeftBoundary + 1] = 0;
    memory[currentLeftBoundary + 2] = 0;
    memory[rightCoalesceLeftBoundary - 1] = 0;
    memory[rightCoalesceLeftBoundary] = 0;
    memory[rightCoalesceLeftBoundary + 1] = 0;
    memory[rightCoalesceLeftBoundary + 2] = 0;

    if(DEBUG)
    {
        std::cout << "memory[" << leftCoalesceLeftBoundary << "] = " << memory[leftCoalesceLeftBoundary] << std::endl;
        std::cout << "memory[" << leftCoalesceLeftBoundary + 1 << "] = " << memory[leftCoalesceLeftBoundary + 1] << std::endl;
        std::cout << "memory[" << leftCoalesceLeftBoundary + 2 << "] = " << memory[leftCoalesceLeftBoundary + 2] << std::endl;
        std::cout << "memory[" << currentLeftBoundary - 1 << "] = " << memory[currentLeftBoundary - 1] << std::endl;
        std::cout << "memory[" << currentLeftBoundary << "] = " << memory[currentLeftBoundary] << std::endl;
        std::cout << "memory[" << currentLeftBoundary + 1 << "] = " << memory[currentLeftBoundary + 1] << std::endl;
        std::cout << "memory[" << currentLeftBoundary + 2 << "] = " << memory[currentLeftBoundary + 2] << std::endl;
        std::cout << "memory[" << rightCoalesceLeftBoundary - 1 << "] = " << memory[rightCoalesceLeftBoundary - 1] << std::endl;
        std::cout << "memory[" << rightCoalesceLeftBoundary << "] = " << memory[rightCoalesceLeftBoundary] << std::endl;
        std::cout << "memory[" << rightCoalesceLeftBoundary + 1 << "] = " << memory[rightCoalesceLeftBoundary + 1] << std::endl;
        std::cout << "memory[" << rightCoalesceLeftBoundary + 2 << "] = " << memory[rightCoalesceLeftBoundary + 2] << std::endl;
        std::cout << "memory[" << leftCoalesceLeftBoundary + abs(memory[leftCoalesceLeftBoundary]) - 1 << "] = " << memory[leftCoalesceLeftBoundary + abs(memory[leftCoalesceLeftBoundary]) - 1] << std::endl;
        std::cout << "memory[" << memory[leftCoalesceLeftBoundary + 1] + 2 << "] = " << memory[memory[leftCoalesceLeftBoundary + 1] + 2] << std::endl;
        std::cout << "memory[" << memory[leftCoalesceLeftBoundary + 2] + 1 << "] = " << memory[memory[leftCoalesceLeftBoundary + 2] + 1] << std::endl;
        std::cout << "memory[" << memory[leftCoalesceLeftBoundary + 2] + 2 << "] = " << memory[memory[leftCoalesceLeftBoundary + 2] + 2] << std::endl;
        std::cout << "memory[" << memory[rightCoalesceLeftBoundary + 1] + 2 << "] = " << memory[memory[rightCoalesceLeftBoundary + 1] + 2] << std::endl;
        std::cout << "memory[" << memory[rightCoalesceLeftBoundary + 2] + 1 << "] = " << memory[memory[rightCoalesceLeftBoundary + 2] + 1] << std::endl;
        std::cout << "memory[" << memory[rightCoalesceLeftBoundary + 2] + 2 << "] = " << memory[memory[rightCoalesceLeftBoundary + 2] + 2] << std::endl;
        std::cout << "memory[" << memory[currentLeftBoundary + 1] << "] = " << memory[memory[currentLeftBoundary + 1]] << std::endl;
        std::cout << "freeIdx = " << freeIdx << std::endl;
        std::cout << std::endl;
    }
}

/********************************************************************************
*   Function:   start                                                           *
*   Parameters: None                                                            *
*   Retun Value: None                                                           *
*   Description: resets iterIdx to 0, which is the first space the driver will  *
*                check when asserting memory size.                              *
********************************************************************************/
void BoundaryTag::start()
{
    if(DEBUG)
    {
        std::cout << "calling start" << std::endl;
        std::cout << "freeIdx: " << freeIdx << std::endl;
        std::cout << "memory[freeIdx]: " << memory[freeIdx] << std::endl;
        std::cout <<"memory[0]: " << memory[0] << std::endl;
        std::cout << std::endl;
    }
    iterIdx = 0;
}

/********************************************************************************
*   Function:   next                                                            *
*   Parameters: None                                                            *
*   Retun Value: void*                                                          *
*   Description: the external use of next for the driver. If the iterIdx and    *
*                freeIdx are at the same position, then return the first free   *
*                space. If the next position is at the end of memory, return    *
*                nullptr. Otherwise, return the next free space. Traverses      *
*                memory based on distance to next adjacent space instead of only*
*                traversing through free spaces.                                *
********************************************************************************/
void* BoundaryTag::next()
{
if(DEBUG)
    {
        std::cout << "calling next" << std::endl;
    }

    if(iterIdx == freeIdx && freeIdx == 0)
    {
        if(DEBUG)
        {
            std::cout << "iterIdx == freeIdx && freeIdx == 0" << std::endl;
            std::cout << "iterIdx before = " << iterIdx << std::endl;
            std::cout << "iterIdx after = " << abs(memory[iterIdx]) << std::endl;
            std::cout << "freeIdx = " << freeIdx << std::endl;
            std::cout << "memory[" << freeIdx << "] = " << memory[freeIdx] << std::endl;
            std::cout << std::endl;
        }
        
        iterIdx = abs(memory[iterIdx]);
        return &memory[freeIdx];
    }
    else if(iterIdx >= SIZE || abs(memory[iterIdx]) == 0)
    {
        if(DEBUG)
       {
        std::cout << "about to return nullptr." << std::endl;
        std::cout << "iterIdx = " << iterIdx << std::endl;
        std::cout << "memory[" << iterIdx << "] = " << memory[iterIdx] << std::endl;
        std::cout << std::endl;
       }
        return nullptr;
    }
    else
    {
        int returnedValue = iterIdx;
        iterIdx += abs(memory[iterIdx]);
        return &memory[returnedValue];
    }    
}

/********************************************************************************
*   Function:   isFree                                                          *
*   Parameters: void *ptrToMem                                                  *
*   Retun Value: Bool                                                           *
*   Description: checks to see whether pointer to memory contains a space that  *
*                is free or not.                                                *
********************************************************************************/
bool BoundaryTag::isFree(void *ptrToMem)
{
    //create a pointerIndex to the location of the freed memory and set the index to its proper position within memory.
    int *pointerIndex = (int*)ptrToMem;
    int index = pointerIndex - memory - 1;
    //If the address of ptrToMem in memory is positive, return true, else false.
    if(memory[index] < 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/********************************************************************************
*   Function:   size                                                            *
*   Parameters: void *ptr                                                       *
*   Retun Value: int                                                            *
*   Description: returns the size in bytes of the given pointer in memory.      *
********************************************************************************/
int BoundaryTag::size(void *ptr)
{
    int *pointerIndex = (int*)ptr;
    int index = pointerIndex - memory;
    
    if(DEBUG)
    {
        std::cout << "calling size" << std::endl;
        std::cout << "index: " << index << std::endl;
        std::cout << "size: " << abs(memory[index]) * BYTES_PER_WORD << std::endl;
        std::cout << std::endl;
    }
    return abs(memory[index]) * BYTES_PER_WORD;
}

/********************************************************************************
*   Function:   internalSize                                                    *
*   Parameters: void *ptr                                                       *
*   Retun Value: int                                                            *
*   Description: returns the size of the given pointer in memory. These pointers*
*                refer to the location of the left boundary tag and indicate    *
*                the available number of index spaces wihtin memory.            *
********************************************************************************/
int BoundaryTag::internalSize(void *ptrToMem)
{
    int *pointerIndex = (int*)ptrToMem;
    int index = pointerIndex - memory;
    if(DEBUG)
    {
        std::cout << "calling internal size" << std::endl;
        std::cout << "index: " << index << std::endl;
        std::cout << "internal size: " << abs(memory[index]) << std::endl;
        std::cout << std::endl;
    }
    return abs(memory[index]);
}

/********************************************************************************
*   Function:   interalNext                                                     *
*   Parameters: None                                                            *
*   Retun Value: void*                                                          *
*   Description: the internal use of next for keeping track of free spaces. If  *
*                the freeIdx and the iterIdx are equal, then return the first   *
*                space. If the next free space is null, then return nullptr.    *
*                Otherwise returns the address of the next free space in        *
*                memory.                                                        *
********************************************************************************/
void *BoundaryTag::internalNext()
{
    if(DEBUG)
    {
        std::cout << "calling internalNext" << std::endl;
        std::cout << "iterIdx before updating: " << iterIdx << std::endl;
    }
    
    //If the freeIdx is pointing to a nullptr, then we have exhausted our available free spaces and there is no need
    //to check any further.
    if(freeIdx == -1)
    {
        if(DEBUG)
        {
            std::cout << "returning nullptr, no more free spaces." << std::endl;
        }
        return nullptr;
    }

    //If the freeIdx is equal to iterIdx, then update iterIdx and return freeIdx.
    //Else if the next free space is null, then return nullptr. Otherwise, increment iterIdx and return iterIdx.
    if(freeIdx == iterIdx)
    {
        iterIdx = memory[iterIdx + 2];
        
        if(DEBUG)
        {
            std::cout << "freeIdx == iterIdx" << std::endl;
            std::cout << "iterIdx after updating: " << iterIdx << std::endl;
            std::cout << "memory[freeIdx address] - memory : " << &memory[freeIdx] - memory << std::endl;
            std::cout << std::endl;
        }
        return &memory[freeIdx];
    }
    else if(memory[iterIdx + 2] == -1 || iterIdx < 0 || iterIdx > SIZE)
    {
        
        if(DEBUG)
        {
            std::cout << "returning nullptr, no more free spaces." << std::endl;
            std::cout << std::endl;
        }
        
        return nullptr;
    }
    else
    {
        iterIdx = memory[iterIdx + 2];
        
        if(DEBUG)
        {
            std::cout << "iterIdx after updating: " << iterIdx << std::endl;
            std::cout << "iterIdx as a void*: " << &iterIdx << std::endl;
            std::cout << "iterIdx as a void* - memory: " << &iterIdx - memory << std::endl;
            std::cout << "memory[iterIdx + 2]: " << memory[iterIdx + 2] << std::endl;
            std::cout << std::endl;
        }
        return &memory[iterIdx];
    }
}

/********************************************************************************
*   Function:   interalStart                                                    *
*   Parameters: None                                                            *
*   Retun Value: None                                                           *
*   Description: the internal use of start for setting iterIdx to the proper    *
*                position before iterating through the free spaces. Freeidx may *
*                not always be 0, and as such iterIdx shouldn't start at the    *
*                beginning of the array when searching for a free space.        *
********************************************************************************/
void BoundaryTag::internalStart()
{
    if(DEBUG)
    {
        std::cout << "calling internal start" << std::endl;
        std::cout << "iterIdx = " << iterIdx << std::endl;
        std::cout << "freeIdx = " << freeIdx << std::endl;
        
    }
    iterIdx = freeIdx;
}