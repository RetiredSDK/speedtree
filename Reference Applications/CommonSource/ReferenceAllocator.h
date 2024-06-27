///////////////////////////////////////////////////////////////////////  
//  ReferenceAllocator.h
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#pragma once

// define to override global new, new[], delete, and delete[] to track all allocations, both
// inside and outside of the SpeedTree allocator mechanism
#define OVERRIDE_NEW 
                    
// define to pass a customer allocator object in SpeedTreeRT to trap all internal allocations
#define USE_CUSTOM_ALLOCATOR


///////////////////////////////////////////////////////////////////////  
//  Overloaded global new

#ifdef OVERRIDE_NEW
void* operator new(size_t size)
{
    void *p = malloc(size); 
    if (p == 0) // did malloc fail?
        throw std::bad_alloc( ); // ANSI/ISO compliant behavior

    return p;
}


///////////////////////////////////////////////////////////////////////  
//  Overloaded global new[]

void* operator new[](size_t size)
{
    void *p = malloc(size);
    if (p == 0) // did malloc fail?
        throw std::bad_alloc(); // ANSI/ISO compliant behavior

    return p;
}


///////////////////////////////////////////////////////////////////////  
//  Overloaded global delete

void operator delete(void* p)
{
    if (p)
    {
        free(p);
    }
}


///////////////////////////////////////////////////////////////////////  
//  Overloaded global delete[]

void operator delete[](void* p)
{
    if (p)
    {
        free(p);
    }
}
#endif


///////////////////////////////////////////////////////////////////////  
//  class CReferenceAllocator

#ifdef USE_CUSTOM_ALLOCATOR

class CReferenceAllocator : public CSpeedTreeAllocator
{
public:
    void* Alloc(size_t BlockSize, size_t Alignment = 16)
    {
        UNREFERENCED_PARAMETER(Alignment);

        void *p = malloc(BlockSize);
        if (p == 0) // did malloc fail?
            throw std::bad_alloc( ); // ANSI/ISO compliant behavior

        return p;
    }

    void Free(void* pBlock)
    {
        if (pBlock)
            free(pBlock);
    }
};

#endif // USE_CUSTOM_ALLOCATOR

