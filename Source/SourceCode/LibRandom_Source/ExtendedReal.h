///////////////////////////////////////////////////////////////////////  
//  ExtendedReal.h
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com
//
//  Original random library code was supplied by Robert Davies of Statistics
//  Research Associates, Ltd. of Wellington, New Zealand.  Their corporate
//  URL is http://www.statsresearch.co.nz/
//
//  All modifications to their software is supplied under the terms of a
//  license agreement or nondisclosure agreement with Interactive Data
//  Visualization and may not be copied or disclosed except in accordance with
//  the terms of that agreement.

#pragma once
#include "../Debug.h"

typedef float Real;
enum EXT_REAL_CODE
{
    Finite, PlusInfinity, MinusInfinity, Indefinite, Missing
};


///////////////////////////////////////////////////////////////////////  
//  class ExtReal declaration
//

class ExtReal
{
public:
        ExtReal(Real v)                     { c = Finite; value = v; }
        ExtReal(const EXT_REAL_CODE& cx)    { c = cx; value = 0.0f; }
        ExtReal( )                          { c = Missing; value = 0.0f; }

        ExtReal         operator+(const ExtReal&) const;
        ExtReal         operator-(const ExtReal&) const;
        ExtReal         operator*(const ExtReal&) const;
        ExtReal         operator-() const;
        Real            Value( ) const      { return value; }
        bool            IsReal( ) const     { return c == Finite; }
        EXT_REAL_CODE   Code( ) const       { return c; }

private:
        Real            value;
        EXT_REAL_CODE   c;
};

