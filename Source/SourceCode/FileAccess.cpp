///////////////////////////////////////////////////////////////////////  
//  FileAccess.cpp
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
//      http://www.idvinc.com

#ifdef WIN32
#pragma warning (disable : 4996) // std::_Copy_opt declared deprecated in VC8
#endif

#include "FileAccess.h"
#include "Endian.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ConvertMemoryToArray definition

unsigned char* CTreeFileAccess::ConvertMemoryToArray(unsigned int& uiNumBytes)
{
    unsigned char* pArray = st_new_array<unsigned char>(m_vMemory.size( ), "CTreeFileAccess::ConvertMemoryToArray, pArray");
    (void) copy(m_vMemory.begin( ), m_vMemory.end( ), pArray);
    uiNumBytes = (unsigned int) m_vMemory.size( );

    return pArray;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ParseFloat definition

float CTreeFileAccess::ParseFloat(void)
{
    st_assert(!EndOfFile( ));
    st_assert(m_nIndex < int(m_vMemory.size( )));

    union
    {
        float           m_fFloat;
        unsigned char   m_ucBytes[4];
    } uFloat;

    // 4-byte floats are written little-endian directly in file
#ifdef BIG_ENDIAN
    uFloat.m_ucBytes[3] = m_vMemory[m_nIndex++];
    uFloat.m_ucBytes[2] = m_vMemory[m_nIndex++];
    uFloat.m_ucBytes[1] = m_vMemory[m_nIndex++];
    uFloat.m_ucBytes[0] = m_vMemory[m_nIndex++];
#else
    uFloat.m_ucBytes[0] = m_vMemory[m_nIndex++];
    uFloat.m_ucBytes[1] = m_vMemory[m_nIndex++];
    uFloat.m_ucBytes[2] = m_vMemory[m_nIndex++];
    uFloat.m_ucBytes[3] = m_vMemory[m_nIndex++];
#endif

    return uFloat.m_fFloat;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ParseInt definition

int CTreeFileAccess::ParseInt(void)
{
    st_assert(!EndOfFile( ));
    st_assert(m_nIndex < int(m_vMemory.size( )));

    union
    {
        int             m_nInt;
        unsigned char   m_ucBytes[4];
    } uInt;

    // 4-byte ints are written little-endian directly in file
#ifdef BIG_ENDIAN
    uInt.m_ucBytes[3] = m_vMemory[m_nIndex++];
    uInt.m_ucBytes[2] = m_vMemory[m_nIndex++];
    uInt.m_ucBytes[1] = m_vMemory[m_nIndex++];
    uInt.m_ucBytes[0] = m_vMemory[m_nIndex++];
#else
    uInt.m_ucBytes[0] = m_vMemory[m_nIndex++];
    uInt.m_ucBytes[1] = m_vMemory[m_nIndex++];
    uInt.m_ucBytes[2] = m_vMemory[m_nIndex++];
    uInt.m_ucBytes[3] = m_vMemory[m_nIndex++];
#endif

    return uInt.m_nInt;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ParseUInt definition

unsigned int CTreeFileAccess::ParseUInt(void)
{
    st_assert(!EndOfFile( ));
    st_assert(m_nIndex < int(m_vMemory.size( )));

    union
    {
        unsigned int    m_uiUnsignedInt;
        unsigned char   m_ucBytes[4];
    } uUnsignedInt;
 
    // 4-byte unsigned ints are written little-endian directly in file
#ifdef BIG_ENDIAN
    uUnsignedInt.m_ucBytes[3] = m_vMemory[m_nIndex++];
    uUnsignedInt.m_ucBytes[2] = m_vMemory[m_nIndex++];
    uUnsignedInt.m_ucBytes[1] = m_vMemory[m_nIndex++];
    uUnsignedInt.m_ucBytes[0] = m_vMemory[m_nIndex++];
#else
    uUnsignedInt.m_ucBytes[0] = m_vMemory[m_nIndex++];
    uUnsignedInt.m_ucBytes[1] = m_vMemory[m_nIndex++];
    uUnsignedInt.m_ucBytes[2] = m_vMemory[m_nIndex++];
    uUnsignedInt.m_ucBytes[3] = m_vMemory[m_nIndex++];
#endif

    // longs are padded with 4 bytes for future expansion
    m_nIndex += 4;

    return uUnsignedInt.m_uiUnsignedInt;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ParseShort definition

short CTreeFileAccess::ParseShort(void)
{
    st_assert(!EndOfFile( ));
    st_assert(m_nIndex < int(m_vMemory.size( )));

    union
    {
        short           m_sShort;
        unsigned char   m_ucBytes[2];
    } uShort;

    // 2-byte shorts are written little-endian directly in file
#ifdef BIG_ENDIAN
    uShort.m_ucBytes[1] = m_vMemory[m_nIndex++];
    uShort.m_ucBytes[0] = m_vMemory[m_nIndex++];
#else
    uShort.m_ucBytes[0] = m_vMemory[m_nIndex++];
    uShort.m_ucBytes[1] = m_vMemory[m_nIndex++];
#endif

    return uShort.m_sShort;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ParseString definition

void CTreeFileAccess::ParseString(st_string& strValue)
{
    st_assert(!EndOfFile( ));
    st_assert(m_nIndex < int(m_vMemory.size( )));

    // a 4-byte string length is stored first, then each character in the string
    strValue = "";
    int nLength = ParseInt( );
    st_assert(m_nIndex + nLength < int(m_vMemory.size( )));

    strValue.resize(nLength);
    (void) copy(m_vMemory.begin( ) + m_nIndex, m_vMemory.begin( ) + m_nIndex + nLength, strValue.begin( ));
    m_nIndex += nLength;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ParseVector definition

stVec CTreeFileAccess::ParseVector(int nSize)
{
    st_assert(!EndOfFile( ));
    st_assert(m_nIndex < int(m_vMemory.size( )));

    stVec cVec(nSize);
    for (int i = 0; i < nSize; ++i)
        cVec[i] = ParseFloat( );

    return cVec;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::ParseVector3 definition

stVec3 CTreeFileAccess::ParseVector3(void)
{
    st_assert(!EndOfFile( ));
    st_assert(m_nIndex < int(m_vMemory.size( )));

    stVec3 cVec;
    for (int i = 0; i < 3; ++i)
        cVec[i] = ParseFloat( );

    return cVec;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::SaveFloat definition

void CTreeFileAccess::SaveFloat(float fFloat)
{
    unsigned char* pFloatBytes = reinterpret_cast<unsigned char*>(&fFloat);

    m_vMemory.push_back(*pFloatBytes++);
    m_vMemory.push_back(*pFloatBytes++);
    m_vMemory.push_back(*pFloatBytes++);
    m_vMemory.push_back(*pFloatBytes++);
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::SaveInt definition

void CTreeFileAccess::SaveInt(int nInt)
{
    unsigned char* pIntBytes = reinterpret_cast<unsigned char*>(&nInt);

    m_vMemory.push_back(*pIntBytes++);
    m_vMemory.push_back(*pIntBytes++);
    m_vMemory.push_back(*pIntBytes++);
    m_vMemory.push_back(*pIntBytes++);
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::SaveLong definition

void CTreeFileAccess::SaveLong(unsigned long ulLong)
{
    unsigned char* pLongBytes = reinterpret_cast<unsigned char*>(&ulLong);

    m_vMemory.push_back(*pLongBytes++);
    m_vMemory.push_back(*pLongBytes++);
    m_vMemory.push_back(*pLongBytes++);
    m_vMemory.push_back(*pLongBytes++);

    // longs are padded with 4 bytes for future expansion
    for (int i = 0; i < 4; ++i)
        m_vMemory.push_back(0);
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::SaveShort definition

void CTreeFileAccess::SaveShort(short sShort)
{
    unsigned char* pShortBytes = reinterpret_cast<unsigned char*>(&sShort);

    m_vMemory.push_back(*pShortBytes++);
    m_vMemory.push_back(*pShortBytes++);
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::SaveString definition

void CTreeFileAccess::SaveString(const st_string& strString)
{
    SaveInt(int(strString.length( )));

    for (unsigned int i = 0; i < strString.length( ); ++i)
        m_vMemory.push_back(strString[i]);
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::SaveVector definition

void CTreeFileAccess::SaveVector(const stVec& cVec, int nSize)
{
    for (int i = 0; i < nSize; ++i)
        SaveFloat(cVec[i]);
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::SaveVector3 definition

void CTreeFileAccess::SaveVector3(const stVec3& cVec)
{
    for (int i = 0; i < 3; ++i)
        SaveFloat(cVec[i]);
}


///////////////////////////////////////////////////////////////////////  
//  CTreeFileAccess::PeekToken definition

EFileToken CTreeFileAccess::PeekToken(void)
{
    st_assert(!EndOfFile( ));

    // 4-byte tokens (ints) are written little-endian directly in file
    int* pReturn = reinterpret_cast<int*>(&m_vMemory[m_nIndex]);

#ifdef BIG_ENDIAN
    int nSwapValue = EndianSwap32(*pReturn);
    pReturn = &nSwapValue;
#endif

    return static_cast<EFileToken>(*pReturn);
}
