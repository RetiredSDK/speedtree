///////////////////////////////////////////////////////////////////////  
//  ExtendedReal.cpp
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All rights reserved in all media.
//
//  Original random library code was supplied by Robert Davies of Statistics
//  Research Associates, Ltd. of Wellington, New Zealand.  Their corporate
//  URL is http://www.statsresearch.co.nz/
//
//  All modifications to their software is supplied under the terms of a
//  license agreement or nondisclosure agreement with Interactive Data
//  Visualization and may not be copied or disclosed except in accordance with
//  the terms of that agreement.

#include "ExtendedReal.h"


///////////////////////////////////////////////////////////////////////  
//  ExtReal::operator+ definition
//

ExtReal ExtReal::operator+(const ExtReal& er) const
{
   if (c == Finite && er.c == Finite)
       return ExtReal(value+er.value);
   if (c == Missing || er.c == Missing)
       return ExtReal(Missing);
   if (c == Indefinite || er.c == Indefinite)
       return ExtReal(Indefinite);
   if (c == PlusInfinity)
   {
      if (er.c == MinusInfinity)
          return ExtReal(Indefinite);
      return *this;
   }
   if (c == MinusInfinity)
   {
      if (er.c == PlusInfinity)
          return ExtReal(Indefinite);
      return *this;
   }

   return er;
}


///////////////////////////////////////////////////////////////////////  
//  ExtReal::operator- definition
//

ExtReal ExtReal::operator-(const ExtReal& er) const
{
   if (c == Finite && er.c == Finite)
       return ExtReal(value-er.value);
   if (c == Missing || er.c == Missing)
       return ExtReal(Missing);
   if (c == Indefinite || er.c == Indefinite)
       return ExtReal(Indefinite);
   if (c == PlusInfinity)
   {
      if (er.c == PlusInfinity)
          return ExtReal(Indefinite);
      return *this;
   }
   if (c == MinusInfinity)
   {
      if (er.c == MinusInfinity)
          return ExtReal(Indefinite);
      return *this;
   }

   return er;
}


///////////////////////////////////////////////////////////////////////  
//  ExtReal::operator* definition
//
 
ExtReal ExtReal::operator*(const ExtReal& er) const
{
   if (c == Finite && er.c == Finite)
       return ExtReal(value*er.value);
   if (c == Missing || er.c == Missing)
       return ExtReal(Missing);
   if (c == Indefinite || er.c == Indefinite)
       return ExtReal(Indefinite);
   if (c == Finite)
   {
      if (value == 0.0)
          return ExtReal(Indefinite);
      if (value>0.0)
          return er;

      return (-er);
   }
   if (er.c == Finite)
   {
      if (er.value == 0.0)
          return ExtReal(Indefinite);
      if (er.value>0.0)
          return *this;

      return -(*this);
   }
   if (c == PlusInfinity)
       return er;

   return (-er);
}


///////////////////////////////////////////////////////////////////////  
//  ExtReal::operator- definition
//

ExtReal ExtReal::operator-() const
{
   switch (c)
   {
      case Finite:        return ExtReal(-value);
      case PlusInfinity:  return ExtReal(MinusInfinity);
      case MinusInfinity: return ExtReal(PlusInfinity);
      case Indefinite:    return ExtReal(Indefinite);
      case Missing:       return ExtReal(Missing);
   }

   return 0.0;
}

