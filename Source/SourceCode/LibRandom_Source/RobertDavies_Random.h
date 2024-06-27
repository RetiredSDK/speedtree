///////////////////////////////////////////////////////////////////////  
//  RobertDavies_Random.h
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

#pragma once
#include "ExtendedReal.h"

// forward references

class RepeatedRandom;
class SelectedRandom;


///////////////////////////////////////////////////////////////////////  
//  class Random declaration

class Random
{
public:
friend  class           RandomPermutation;

        Random( ) { }                           // do nothing
virtual ~Random( ) { }                          // make destructors virtual

static  void            Set(double s);          // set seed (0 < seed < 1)
static  double          Get( );                 // get seed
static  void            SetLong(long s);        // set seed (0 < seed < 1)
static  long            GetSeed( );             // get seed
virtual Real            Next( );                // get new value
virtual char*           Name( );                // identification
virtual Real            Density(Real) const;    // used by PosGen & Asymgen
virtual ExtReal         Mean( ) const       { return 0.5f; }            // mean of distribution
virtual ExtReal         Variance( ) const   { return 1.0f / 12.0f; }    // variance of distribution
virtual int             nelems( ) const     { return 1; }
virtual void            tDelete( ) { }          // delete components of sum
virtual void            load(int*, Real*, Random**);

private:
static  double          seed;                   // seed
static  Real            Buffer[128];            // for mixing random numbers

                        inline static Real Random::Raw( )
                        {
                            long iseed = (long) seed;
                            long hi = iseed / 127773L;                 // integer division
                            long lo = iseed - hi * 127773L;            // modulo
                            iseed = 16807 * lo - 2836 * hi;
                            if (iseed <= 0)
                                iseed += 2147483647L;
                            seed = (double)iseed;

                            return float(seed) * 4.656612875e-10f;
                        }
};


///////////////////////////////////////////////////////////////////////  
//  class Uniform declaration

class Uniform : public Random
{
public:
        Uniform( ) 
        {
        }

        char*           Name( );                
        Real            Next( )                 { return Random::Next( ); }
        ExtReal         Mean( ) const           { return 0.5f; }
        ExtReal         Variance( ) const       { return 1.0f / 12.0f; }
        Real            Density(Real x) const   { return (x < 0.0f || x > 1.0f) ? 0.0f : 1.0f; }

private:
};


///////////////////////////////////////////////////////////////////////  
//  class PosGen declaration

class PosGen : public Random
{
public:
        PosGen( );                          // constructor
        ~PosGen( );                         // destructor

        char*           Name( );            // identification
        Real            Next( );            // to get a single new value
        ExtReal         Mean( ) const           { return (ExtReal)Missing; }
        ExtReal         Variance( ) const       { return (ExtReal)Missing; }

private:

protected:
        void            Build(bool);        // called on first call to Next
        Real            xi, *sx, *sfx;
        bool            NotReady;
};


///////////////////////////////////////////////////////////////////////  
//  class SymGen declaration

class SymGen : public PosGen
{
public:
        char*           Name( );            // identification
        Real            Next( );            // to get a single new value
};


///////////////////////////////////////////////////////////////////////  
//  class Normal declaration
//

class Normal : public SymGen
{
public:
        Normal( );
        ~Normal( );

        char*           Name( );                // identification
        Real            Density(Real) const;    // normal density function
        ExtReal         Mean( ) const           { return 0.0f; }
        ExtReal         Variance( ) const       { return 1.0f; }

private:
static  Real            Nxi, *Nsx, *Nsfx;       // so we need initialise only once
static  long            count;                  // assume initialised to 0
};


