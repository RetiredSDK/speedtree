///////////////////////////////////////////////////////////////////////  
//  IdvVector.h
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

#pragma once
#define BUILD_VECTOR_SET

#include "IdvFastMath.h"
#include "../Debug.h"
#include <cfloat>

#define VectorSinD(x)       sinf((x) / 57.29578f)
#define VectorCosD(x)       cosf((x) / 57.29578f)
#define VectorSinR(x)       sinf(x)
#define VectorCosR(x)       cosf(x)


///////////////////////////////////////////////////////////////////////  
//  Vector constants

const float c_fPi = 3.1415926535897932384626433832795f;
const float c_fHalfPi = c_fPi * 0.5f;
const float c_fQuarterPi = c_fPi * 0.25f;
const float c_fTwoPi = 2.0f * c_fPi;


///////////////////////////////////////////////////////////////////////  
//  Forward references

class stTransform;
class stRotTransform;
class stRegion;
class stVec;


///////////////////////////////////////////////////////////////////////  
//  class stVec3 declaration
//
//
//  Description:
//
//      Construction:
//      -------------
//      stVec3( )                                    Builds a vector equal to (0.0f, 0.0f, 0.0f).
//      stVec3(bool bSkipInitialization)             Builds a vector within uninitialized member variables
//      stVec3(float x, y, z)                        Builds a vector with values (x, y, z)
//      stVec3(float x, y)                           Builds a vector with values (x, y, 0.0f);
//
//      Variables:
//      ----------
//      m_afData[3]                                 Vector data storage.  Public for OpenGL convenience and fast access.
//
//      Inspectors:
//      -----------
//      AbsoluteValue( )                            Returns a stVec3 where each m_afData[i] = fabs((*this)[i]).
//      AngleBetween(const stVec3& stVec3)            Returns the angle (in radians) between *this and stVec3 (assumes normalized vectors)
//      Distance(const stVec3& cPoint)               Returns the distance between *this and cPoint.
//      GetSize( )                                  Returns the size of the stVec3.
//      GetTolerance( )                             Returns the current tolerance setting (used in RoughlyEqual( )) for all stVec3s.
//      Magnitude( )                                Computes the magnitude of the stVec3.
//      MaxAxis( )                                  Returns the largest value in the stVec3.
//      Midpoint(const stVec3& cVec)                 Computes the point half way between cVec and *this.
//      MinAxis( )                                  Returns the smallest value in the stVec3.
//      RoughlyEqual(const stVec3& stVec3)            Determines if *this stVec3 is equivalent to stVec3 within the specified tolerance.
//
//      Mutators:
//      ---------
//      Set(...)                                    All forms set the specified data points and leave the others unchanged.
//      SetTolerance(float fTolerance)              Sets the RoughlyEqual tolerance for ALL stVec3s.
//      SetSize(int nSize)                          Sets the size (number of data points considered valid) of *this.
//      Normalize( )                                Makes the stVec3 a unit vector.
//
//      Operators:
//      ----------
//      operator float*( )                          Converts a stVec3 to a float*.
//      operator[](int nIndex)                      Returns a reference to the specified data point (const version allows access for other const member functions).
//      operator<(const stVec3 &stVec3)               Returns true if the x coord of *this is less than that of stVec3.  If they are equal, the test moves to the y coord and then the z.
//      operator<=(const stVec3 &stVec3)              Returns true if the x coord of *this is less than or equal to that of stVec3.
//      operator>(const stVec3 &stVec3)               Returns true if the x coord of *this is greater than that of stVec3.  If they are equal, the test moves to the y coord and then the z.
//      operator>=(const stVec3 &stVec3)              Returns true if the x coord of *this is greater than or equal to that of stVec3.
//      operator+(const stVec3& stVec3)               Returns *this + stVec3.
//      operator+=(const stVec3& stVec3)              Adds stVec3 to *this.
//      operator-(const stVec3& stVec3)               Returns *this - stVec3.
//      operator-=(const stVec3& stVec3)              Subtracts stVec3 from *this.
//      operator-( )                                Returns -(*this).
//      operator^(const stVec3& stVec3)               Computes the DOT product of *this and stVec3.
//      operator*(const stVec3& stVec3)               Computes the CROSS product of *this and stVec3.
//      operator*=(const stVec3& stVec3)              *this equals the cross product of *this and stVec3.
//      operator*(float fValue)                     Returns a stVec3 equal to *this with each data point multiplied by fValue.
//      operator*=(float fValue)                    Each data point of *this is multiplied by fValue.
//      operator/(float fValue)                     Returns a stVec3 equal to *this with each data point divided by fValue.
//      operator/=(float fValue)                    Each data point of *this is divided by fValue.
//      operator==(const stVec3& stVec3)              Returns true when *this and stVec3 are identical.
//      operator!=(const stVec3& stVec3)              Returns true when *this and stVec3 are not identical.
//      operator*(const stTransform& CTrans)         Returns a stVec3 that is equal to *this * stTransform3 (the vector is on the left).
//      operator*(const stRotTransform3& CTrans)     Returns a stVec3 that is equal to *this * stTransform3 (the vector is on the left).

class BUILD_VECTOR_SET stVec3
{
public:
        stVec3( );
        stVec3(float fX, float fY, float fZ);
        stVec3(float fX, float fY);
        stVec3(const float* pPos); // assumes three points
        ~stVec3( ) { } // do nothing 

        //  inspectors
        stVec3      AbsoluteValue( ) const;
        float       AngleBetween(const stVec3& cVec) const;
        stVec3      Cross(const stVec3& cVec) const;
        float       Distance(const stVec3& cPoint) const;
        float       Dot(const stVec3& cVec) const;
        float       GetTolerance( ) const                                   { return m_fTolerance; }
        float       Magnitude( ) const;
        float       MaxAxis( ) const;
        stVec3      Midpoint(const stVec3& cVec) const;
        float       MinAxis( ) const;
        bool        RoughlyEqual(const stVec3& cVec) const;
        stVec       ToVec(void) const;

        //  mutators
        void        Normalize( );
        void        Set(float fX, float fY);
        void        Set(float fX, float fY, float fZ);
        void        SetTolerance(float fTolerance)                          { m_fTolerance = fTolerance; }

        //  operators
                    operator float*( )                                      { return &m_afData[0]; }
                    operator const float*( ) const                          { return &m_afData[0]; }
        float&      operator[](int nIndex)                                  { return m_afData[nIndex]; }
        bool        operator<(const stVec3& cVec)    const;  
        bool        operator<=(const stVec3& cVec) const;
        bool        operator>(const stVec3& cVec)    const;
        bool        operator>=(const stVec3& cVec) const;
        stVec3      operator+(const stVec3& cVec) const;
        stVec3&     operator+=(const stVec3& cVec);
        stVec3      operator-(const stVec3& cVec) const;
        stVec3&     operator-=(const stVec3& cVec);
        stVec3      operator-( ) const;
        float       operator^(const stVec3& cVec) const;
        stVec3      operator*(const stVec3& cVec) const;
        stVec3&     operator*=(const stVec3& cVec);
        stVec3      operator*(float fValue) const;
        stVec3&     operator*=(float fValue);
        stVec3      operator/(float fValue) const;
        stVec3&     operator/=(float fValue);
        bool        operator==(const stVec3& cVec) const;
        bool        operator!=(const stVec3& cVec) const;
        stVec3      operator*(const stTransform& cTransform) const;
        stVec3      operator*(const stRotTransform& cTransform) const;

        float       m_afData[3];            // store the x, y, & z components of the stVec3

private:
static  float       m_fTolerance;
};



///////////////////////////////////////////////////////////////////////  
//  class stVec declaration
//
//  Description:  Summary of public interface
//
//
//      Construction:
//      -------------
//      stVec( )                                     Builds a vector equal to (0.0f, 0.0f, 0.0f).
//      stVec(int nSize)                             Builds a vector of size nSize with each member set to zero.
//      stVec(float fX, ...)                         All forms build a vector whose size is equal to the number of arguments.  The values are set to the corresponding parameters.
//
//      Variables:
//      ----------
//      m_afData                                    Vector data storage.  Public for OpenGL convenience and fast access.
//
//      Inspectors:
//      -----------
//      GetTolerance( )                             Returns the current tolerance setting (used in RoughlyEqual( )) for all stVecs.
//      GetSize( )                                  Returns the size of the stVec.
//      Magnitude( )                                Computes the magnitude of the stVec.
//      MaxAxis( )                                  Returns the largest value in the stVec.
//      MinAxis( )                                  Returns the smallest value in the stVec.
//      AbsoluteValue( )                            Returns a stVec where each m_afData[i] = fabs((*this)[i]).
//      RoughlyEqual(const stVec& cVec)              Determines if *this stVec is equivalent to cVec within the specified tolerance.
//      Distance(const stVec& cPoint)                Returns the distance between *this and cPoint.
//      AngleBetween(const stVec& cVec)              Returns the angle (in radians) between *this and cVec.
//
//      Mutators:
//      ---------
//      Set(...)                                    All forms set the specified data points and leave the others unchanged.
//      SetTolerance(float fTolerance)              Sets the RoughlyEqual tolerance for ALL stVecs.
//      SetSize(int nSize)                          Sets the size (number of data points considered valid) of *this.
//      Normalize( )                                Makes the stVec a unit vector.
//
//      Operators:
//      ----------
//      operator float*( )                          Converts a stVec to a float*.
//      operator[](int nIndex)                      Returns a reference to the specified data point (const version allows access for other const member functions).
//      operator<(const stVec &cVec)                 Returns true if the x coord of *this is less than that of cVec.  If they are equal, the test moves to the y coord and then the z.
//      operator<=(const stVec &cVec)                Returns true if the x coord of *this is less than or equal to that of cVec.
//      operator>(const stVec &cVec)                 Returns true if the x coord of *this is greater than that of cVec.  If they are equal, the test moves to the y coord and then the z.
//      operator>=(const stVec &cVec)                Returns true if the x coord of *this is greater than or equal to that of cVec.
//      operator+(const stVec& cVec)                 Returns *this + cVec.
//      operator+=(const stVec& cVec)                Adds cVec to *this.
//      operator-(const stVec& cVec)                 Returns *this - cVec.
//      operator-=(const stVec& cVec)                Subtracts cVec from *this.
//      operator-( )                                Returns -(*this).
//      operator^(const stVec& cVec)                 Computes the DOT product of *this and cVec.
//      operator*(const stVec& cVec)                 Computes the CROSS product of *this and cVec.
//      operator*=(const stVec& cVec)                *this equals the cross product of *this and cVec.
//      operator*(float fValue)                     Returns a stVec equal to *this with each data point multiplied by fValue.
//      operator*=(float fValue)                    Each data point of *this is multiplied by fValue.
//      operator/(float fValue)                     Returns a stVec equal to *this with each data point divided by fValue.
//      operator/=(float fValue)                    Each data point of *this is divided by fValue.
//      operator==(const stVec& cVec)                Returns true when *this and cVec are identical.
//      operator!=(const stVec& cVec)                Returns true when *this and cVec are not identical.
//      operator*(const stTransform& cTransform)     Returns a stVec that is equal to *this * cTransform (the vector is on the left).

class BUILD_VECTOR_SET stVec
{
public:
        stVec( );
        stVec(int nSize);
        stVec(float fX, float fY);
        stVec(float fX, float fY, float fZ);
        stVec(float fX, float fY, float fZ, float fW);
        stVec(float fX, float fY, float fZ, float fW, float fV);
        ~stVec( );

        //  inspectors
        float           GetTolerance( ) const                                   { return m_fTolerance; }
        int             GetSize( ) const                                        { return m_nSize; }
        stVec           Midpoint(const stVec& cVec) const                        { return (*this + cVec) * 0.5f; }

        void            SetTolerance(float fTolerance)                          { m_fTolerance = fTolerance; }
        void            SetSize(int nSize)                                      { m_nSize = nSize; }

        float           Magnitude( ) const;
        float           MaxAxis( ) const;
        float           MinAxis( ) const;
        bool            RoughlyEqual(const stVec& cVec) const;
        stVec           AbsoluteValue( ) const;
        float           Distance(const stVec& cPoint) const;
        float           AngleBetween(const stVec& cVec) const;

        //  mutators
        void            Normalize( );
        void            Set(float fX, float fY)                                 { m_afData[0] = fX; m_afData[1] = fY; }
        void            Set(float fX, float fY, float fZ)                       { m_afData[0] = fX; m_afData[1] = fY; m_afData[2] = fZ; }
        void            Set(float fX, float fY, float fZ, float fW)             { m_afData[0] = fX; m_afData[1] = fY; m_afData[2] = fZ; m_afData[3] = fW; }
        void            Set(float fX, float fY, float fZ, float fW, float fV)   { m_afData[0] = fX; m_afData[1] = fY; m_afData[2] = fZ; m_afData[3] = fW; m_afData[4] = fV; }

        //  operators
                        operator float*( )                                      { return &m_afData[0]; }
                        operator const float*( ) const                          { return &m_afData[0]; }
        
        float&          operator[](int nIndex)                                  { return m_afData[nIndex]; }

        bool            operator<(const stVec& cVec) const;  
        bool            operator<=(const stVec& cVec) const;
        bool            operator>(const stVec& cVec) const;
        bool            operator>=(const stVec& cVec) const;
        stVec           operator+(const stVec& cVec) const;
        stVec&          operator+=(const stVec& cVec);
        stVec           operator-(const stVec& cVec) const;
        stVec&          operator-=(const stVec& cVec);
        stVec           operator-( ) const;
        float           operator^(const stVec& cVec) const;
        stVec           operator*(const stVec& cVec) const;
        stVec&          operator*=(const stVec& cVec);
        stVec           operator*(float fValue) const;
        stVec&          operator*=(float fValue);
        stVec           operator/(float fValue) const;
        stVec&          operator/=(float fValue);
        bool            operator==(const stVec& cVec) const;
        bool            operator!=(const stVec& cVec) const;
        stVec           operator*(const stTransform& cTrans) const;
        bool            operator<=(const stRegion& cRegion) const;
        bool            operator<(const stRegion& cRegion) const;

        float           m_afData[5];            // store each component of the vector
private:
        int             m_nSize;    
        static float    m_fTolerance;
};



///////////////////////////////////////////////////////////////////////  
//  class stRotTransform declaration
//
//
//  Description:  Optimized class used for matrix rotations & scales only (3 x 3 instead of 4 x 4)
//
//      Construction:
//      -------------
//      stRotTransform( )                            Builds a transformation matrix.  LoadIdentity( ) is called.
//      stRotTransform(bool)                         Builds a matrix without initialization.
//
//      Variables:
//      ----------
//      m_afData[3][3]                              Matrix data storage.  Public for OpenGL convenience and fast access.
//
//      Mutators:
//      ---------
//      Set(int nRow, int nCol, float fValue)       Sets the element at [nRow][nCol] to fValue.
//      Set(float afData[3][3])                     Sets the entire matrix.
//      LoadIdentity( )                             Makes *this the identity matrix.
//      Scale(float fFactor)                        Scales *this on the x, y, and z axes by fFactor.
//      Scale(float fX, float fY, float fZ)         Scales *this.
//      RotateX(float fAngle)                       Rotates *this fAngle degrees around the X axis (faster than general rotate)
//      RotateY(float fAngle)                       Rotates *this fAngle degrees around the Y axis (faster than general rotate)
//      RotateZ(float fAngle)                       Rotates *this fAngle degrees around the Z axis (faster than general rotate)
//      RotateYZ(float fAngle)                      Rotates *this fAngle degrees around the Y then Z axes (faster than separate calls)
//      RotateAxis(float fAngle, stVec3 cAxis)       Rotates *this fAngle degrees around the axis represented by cAxis.  *cAxis is assumed normalized.
//      RotateAxis(float fAngle, stVec cAxis)        Rotates *this fAngle degrees around the axis represented by cAxis.  *cAxis is assumed normalized.
//
//      Operators:
//      ----------
//      operator float*( )                          Converts a stTransform3 to a float*.
//      operator*(const stRotTransform3& )           Returns a stTransform3 equal to *this * stTransform3.
//      operator*(const stVec3& stVec3)               Returns a stVec3 equal to *this * stVec3.
//      operator*(const stVec& stVec)                 Returns a stVec equal to *this * stVec.
//      operator+(const stRotTransform3& )           Returns a stTransform3 equal to *this + stTransform3.
//      operator-(const stRotTransform3& )           Returns a stTransform3 equal to *this - stTransform3.

class BUILD_VECTOR_SET stRotTransform
{
public:
        // constructors
        stRotTransform( );

        // data gathering/setting
        void                    Set(int nRow, int nCol, float fValue)       { m_afData[nRow][nCol] = fValue; }
        void                    Set(float afData[3][3]);
        void                    SetRow(int nRow, const float* pRow)         { st_assert(pRow); m_afData[nRow][0] = pRow[0]; m_afData[nRow][1] = pRow[1]; m_afData[nRow][2] = pRow[2]; }
        void                    LoadIdentity( );

        // standard affine transform functions
        void                    RotateX(float fAngle);
        void                    RotateY(float fAngle);
        void                    RotateZ(float fAngle);
        void                    RotateYZ(float fYangle, float fZangle);
        void                    RotateAxis(float fAngle, stVec3 cAxis);
        void                    RotateAxisFromIdentity(float fAngle, stVec3 cAxis);

        // C++ operator overloads
                                operator float*( )                          { return &m_afData[0][0]; }
                                operator const float*( ) const              { return &m_afData[0][0]; }
        stRotTransform          operator*(const stRotTransform& cTrans) const;
        stVec3                  operator*(const stVec3& cVec) const;

        float                   m_afData[3][3];             // 3 x 3 rotation matrix
};



///////////////////////////////////////////////////////////////////////  
//  class stTransform declaration
//
//
//  Description:  Summary of public interface
//
//      Construction:
//      -------------
//      stTransform( )                               Builds a transformation matrix.  LoadIdentity( ) is called.
//
//      Variables:
//      ----------
//      m_afData                                    Matrix data storage.  Public for OpenGL convenience and fast access.
//
//      Inspectors:
//      -----------
//      IsIdentity( )                               Returns true if *this represents the identity matrix.
//
//      Mutators:
//      ---------
//      Set(int nRow, int nCol, float fValue)       Sets the element at [nRow][nCol] to fValue.
//      Set(float afData[4][4])                     Sets the entire matrix.
//      LoadIdentity( )                             Makes *this the identity matrix.
//      Scale(float fFactor)                        Scales *this on the x, y, and z axes by fFactor.
//      Scale(float fX, float fY, float fZ)         Scales *this.
//      Translate(float fX, float fY, float fZ)     Translates *this.
//      Rotate(float fAngle, char chAxis)           Rotates *this fAngle degrees around chAxis ('x', 'X', 'y', etc.)
//      RotateAxis(float fAngle, stVec cAxis)        Rotates *this fAngle degrees around the axis represented by cAxis.  cAxis is normalized.
//      LookAt(const stVec& cEye, const stVec& ...)   Mimics the gluLookAt routine.
//
//      Operators:
//      ----------
//      operator float*( )                          Converts a stTransform to a float*.
//      operator*(const stTransform& cTransform)     Returns a stTransform equal to *this * cTransform.
//      operator*(const stVec& cVec)                 Returns a stVec equal to *this * cVec.
//      operator*(const stRegion& cRegion)           Returns a stRegion where cRegion.m_cMin/m_cMax are multiplied by *this.
//      operator+(const stTransform& cTransform)     Returns a stTransform equal to *this + cTransform.
//      operator-(const stTransform& cTransform)     Returns a stTransform equal to *this - cTransform.

class BUILD_VECTOR_SET stTransform
{
public:
        enum EInversionCodeType
        {
            OK, SINGULAR
        };

        stTransform( );
        ~stTransform( );

        // direct access
        void                    Set(int nRow, int nCol, float fValue)       { m_afData[nRow][nCol] = fValue; }
        void                    Set(float afData[4][4]);

        // inspectors
        bool                    IsIdentity( ) const;

        // mutators
        void                    LoadIdentity( );
        void                    Scale(float fFactor);
        void                    Scale(float fX, float fY, float fZ);
        void                    Translate(float fX, float fY, float fZ);
        void                    Rotate(float fAngle, char chAxis);
        void                    RotateAxis(float fAngle, stVec cAxis);
        void                    LookAt(const stVec& cEye, const stVec& cCenter, const stVec& cUp);

        // operators
                                operator float*( )                          { return &m_afData[0][0]; }
                                operator const float*( ) const              { return &m_afData[0][0]; }
        stTransform             operator*(const stTransform& cTransform) const;
        stVec                   operator*(const stVec& cVec) const;
        stVec3                  operator*(const stVec3& cVec3) const;
        stRegion                operator*(const stRegion& cRegion) const;
        stTransform             operator+(const stTransform& cTransform) const;
        stTransform             operator-(const stTransform& cTransform) const;

        float                   m_afData[4][4];             // standard 4 x 4 transform matrix
};



///////////////////////////////////////////////////////////////////////  
//  class stRegion declaration
//
//
//  Description:  Summary of public interface
//
//
//      Construction:
//      -------------
//      stRegion( )                              Builds a CBox and calls Initialize( ).
//
//      Variables:
//      ----------
//      m_cMin                                  Minimum extents vector.  Public for OpenGL convenience and fast access.
//      m_cMax                                  Maximum extents vector.  Public for OpenGL convenience and fast access.
//
//      Inspectors:
//      -----------
//      Dimension(int nAxis)                    Returns the length of the specified axis (0 = x, 1 = y, 2 = z).
//      MidValue(int nAxis)                     Returns the middle of the specified axis.
//      Midpoint( )                             Returns the midpoint of the extents.
//      LongestExtent( )                        Returns the length of the longest axis.
//      ShortestExtent( )                       Returns the length of the shortest axis.
//      AverageExtent( )                        Returns the average length of all axes.
//      IsSet( )                                Returns true when any member of *this does not match the initialized state.
//
//
//      Mutators:
//      ---------
//      Initialize( )                           Sets m_cMin to (FLT_MAX, FLT_MAX, FLT_MAX) and m_cMax to -m_cMin.  Called automatically by the constructor.
//      Scale(float fScale)                     Scales *this by fScale relative to the origin (NOT the midpoint).
//
//      Operators:
//      ----------
//      operator+(const stRegion& cRegion)       Returns a stRegion exactly large enough to include *this and cRegion.
//      operator+(const stVec& cVec)             Returns a stRegion where cVec is added to m_cMin/m_cMax.
//      operator^(const stVec3& cVec)            Returns a stRegion where min and max include those of cVec
//      operator*(float fScale)                 Returns a stRegion scaled by fScale relative to the midpoint of *this (NOT the origin).
//      operator*(const stTransform& cTransform) Returns a stRegion where m_cMin/m_cMax of *this are multiplied by cTransform.
//      operator<(const stRegion& cRegion)       Returns true when *this is contained within cRegion (using <= and >=).

class BUILD_VECTOR_SET stRegion
{
public:
        stRegion( );

        // inspectors
        float                   Dimension(int nAxis) const          { return m_cMax.m_afData[nAxis] - m_cMin.m_afData[nAxis]; }
        float                   MidValue(int nAxis) const           { return (m_cMax.m_afData[nAxis] + m_cMin.m_afData[nAxis]) * 0.5f; }
        stVec                   Midpoint( ) const                   { return m_cMin.Midpoint(m_cMax); }
        float                   LongestExtent( ) const              { return (((m_cMax - m_cMin).AbsoluteValue( )).MaxAxis( )); }
        float                   ShortestExtent( ) const             { return (((m_cMax - m_cMin).AbsoluteValue( )).MinAxis( )); }
        float                   AverageExtent( ) const;
        bool                    IsSet( ) const;

        // mutators
        void                    Initialize( );
        void                    Scale(float fScale);

        // operators
        stRegion                operator+(const stRegion& cRegion) const;
        stRegion                operator+(const stVec& cVec) const;
        stRegion                operator^(const stVec3& cVec) const;
        stRegion                operator*(float fScale) const;
        stRegion                operator*(const stTransform& cTransform) const;
        bool                    operator<(const stRegion& cRegion) const;

        stVec                   m_cMin;             // minimum corner of the region (x, y, z)
        stVec                   m_cMax;             // maximum corner of the region (x, y, z)
};


///////////////////////////////////////////////////////////////////////  
//  class stVec3 inlined function definitions
//

// stVec3( )
inline stVec3::stVec3( )
{
    m_afData[0] = m_afData[1] = m_afData[2] = 0.0f;
}

// stVec3(float fX, float fY, float fZ)
inline stVec3::stVec3(float fX, float fY, float fZ)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
    m_afData[2] = fZ;
}

// stVec3(float fX, float fY)
inline stVec3::stVec3(float fX, float fY)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
    m_afData[2] = 0.0f;
}

// stVec3(const float* pPos)
inline stVec3::stVec3(const float* pPos)
{
    st_assert(pPos);
    m_afData[0] = pPos[0];
    m_afData[1] = pPos[1];
    m_afData[2] = pPos[2];
}

// AbsoluteValue
inline stVec3 stVec3::AbsoluteValue(void) const
{
    return stVec3(static_cast<float>(fabs(m_afData[0])),
                  static_cast<float>(fabs(m_afData[1])),
                  static_cast<float>(fabs(m_afData[2])));
}

// AngleBetween
inline float stVec3::AngleBetween(const stVec3& cVec) const
{
    float fDot = *this ^ cVec;
    if (fDot < -1.0f)
        fDot = -1.0f;
    if (fDot > 1.0f)
        fDot = 1.0f;
    return acosf(fDot);
}

// Dot
inline float stVec3::Dot(const stVec3& cVec) const
{
    return m_afData[0] * cVec.m_afData[0] +
           m_afData[1] * cVec.m_afData[1] +
           m_afData[2] * cVec.m_afData[2];
}

// Cross
inline stVec3 stVec3::Cross(const stVec3& cVec) const
{
    return stVec3(m_afData[1] * cVec.m_afData[2] - m_afData[2] * cVec.m_afData[1],
                  m_afData[2] * cVec.m_afData[0] - m_afData[0] * cVec.m_afData[2],
                  m_afData[0] * cVec.m_afData[1] - m_afData[1] * cVec.m_afData[0]);
}

// Distance
inline float stVec3::Distance(const stVec3& cVec) const
{
    return _idv_sqrt1(
                ((cVec.m_afData[0] - m_afData[0]) * (cVec.m_afData[0] - m_afData[0])) +
                ((cVec.m_afData[1] - m_afData[1]) * (cVec.m_afData[1] - m_afData[1])) +
                ((cVec.m_afData[2] - m_afData[2]) * (cVec.m_afData[2] - m_afData[2]))
                );
}

// Magnitude
inline float stVec3::Magnitude( ) const
{
    return _idv_sqrt1(m_afData[0] * m_afData[0] + 
                      m_afData[1] * m_afData[1] + 
                      m_afData[2] * m_afData[2]);
}

// MaxAxis
inline float stVec3::MaxAxis( ) const
{
    float fMax(m_afData[0]);

    if (m_afData[1] > fMax)
        fMax = m_afData[1];
    if (m_afData[2] > fMax)
        fMax = m_afData[2];

    return fMax;
}

// Midpoint
inline stVec3 stVec3::Midpoint(const stVec3& cVec) const
{
    return stVec3(0.5f * (cVec.m_afData[0] + m_afData[0]),
                  0.5f * (cVec.m_afData[1] + m_afData[1]),
                  0.5f * (cVec.m_afData[2] + m_afData[2]));
}

// MinAxis
inline float stVec3::MinAxis( ) const
{
    float fMin(m_afData[0]);

    if (m_afData[1] < fMin)
        fMin = m_afData[1];
    if (m_afData[2] < fMin)
        fMin = m_afData[2];

    return fMin;
}

// RoughlyEqual
inline bool stVec3::RoughlyEqual(const stVec3& cVec) const
{
    return ((fabs(m_afData[0] - cVec.m_afData[0]) < m_fTolerance) &&
            (fabs(m_afData[1] - cVec.m_afData[1]) < m_fTolerance) &&
            (fabs(m_afData[2] - cVec.m_afData[2]) < m_fTolerance));
}

// ToVec
inline stVec stVec3::ToVec(void) const
{
    return stVec(m_afData[0], m_afData[1], m_afData[2]);
}

// Normalize
inline void stVec3::Normalize( )
{
    float fSumOfSquares = (m_afData[0] * m_afData[0]) +
                          (m_afData[1] * m_afData[1]) +
                          (m_afData[2] * m_afData[2]);
    float fInvSquareRoot = 1.0f / ((fSumOfSquares > 100.0f * FLT_EPSILON) ? sqrtf(fSumOfSquares) : 0.01f);

    m_afData[0] *= fInvSquareRoot;
    m_afData[1] *= fInvSquareRoot;
    m_afData[2] *= fInvSquareRoot;
}

// Set(float fX, float fY)
inline void stVec3::Set(float fX, float fY)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
}

// Set(float fX, float fY. float fZ)
inline void stVec3::Set(float fX, float fY, float fZ)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
    m_afData[2] = fZ;
}

// operator <
inline bool stVec3::operator<(const stVec3& cVec) const
{
    if (m_afData[0] < cVec.m_afData[0])
    {
        return true;
    }
    else if (m_afData[0] > cVec.m_afData[0])
    {
        return false;
    }
    else
    {
        if (m_afData[1] < cVec.m_afData[1])
            return true;
        else if (m_afData[1] > cVec.m_afData[1])
            return false;
        else
            return m_afData[2] < cVec.m_afData[2];
    }
}

// operator <=
inline bool stVec3::operator<=(const stVec3& cVec) const
{
    return (m_afData[0] <= cVec.m_afData[0]);
}

// operator >
inline bool stVec3::operator>(const stVec3& cVec) const
{
    if (m_afData[0] > cVec.m_afData[0])
    {
        return true;
    }
    else if (m_afData[0] < cVec.m_afData[0])
    {
        return false;
    }
    else
    {
        if (m_afData[1] > cVec.m_afData[1])
            return true;
        else if (m_afData[1] < cVec.m_afData[1])
            return false;
        else
            return m_afData[2] > cVec.m_afData[2];
    }
}

// operator >=
inline bool stVec3::operator>=(const stVec3& cVec) const
{
    return (m_afData[0] >= cVec.m_afData[0]);
}

// operator +
inline stVec3 stVec3::operator+(const stVec3& cVec) const
{
    return stVec3(m_afData[0] + cVec.m_afData[0],
                  m_afData[1] + cVec.m_afData[1],
                  m_afData[2] + cVec.m_afData[2]);
}

// operator +=
inline stVec3& stVec3::operator+=(const stVec3& cVec) 
{
    m_afData[0] += cVec.m_afData[0];
    m_afData[1] += cVec.m_afData[1];
    m_afData[2] += cVec.m_afData[2];

    return *this;
}

// operator -
inline stVec3 stVec3::operator-(const stVec3& cVec) const
{
    return stVec3(m_afData[0] - cVec.m_afData[0],
                  m_afData[1] - cVec.m_afData[1],
                  m_afData[2] - cVec.m_afData[2]);
}

// operator +=
inline stVec3& stVec3::operator-=(const stVec3& cVec) 
{
    m_afData[0] -= cVec.m_afData[0];
    m_afData[1] -= cVec.m_afData[1];
    m_afData[2] -= cVec.m_afData[2];

    return *this;
}

// operator-
inline stVec3 stVec3::operator-( ) const
{
    return stVec3(-m_afData[0],
                 -m_afData[1],
                 -m_afData[2]);
}

// operator^
inline float stVec3::operator^(const stVec3& cVec) const
{
    return m_afData[0] * cVec.m_afData[0] +
           m_afData[1] * cVec.m_afData[1] +
           m_afData[2] * cVec.m_afData[2];
}

// operator*
inline stVec3 stVec3::operator*(const stVec3& cVec) const
{
    return stVec3(m_afData[1] * cVec.m_afData[2] - m_afData[2] * cVec.m_afData[1],
                  m_afData[2] * cVec.m_afData[0] - m_afData[0] * cVec.m_afData[2],
                  m_afData[0] * cVec.m_afData[1] - m_afData[1] * cVec.m_afData[0]);
}

// operator*=
inline stVec3& stVec3::operator*=(const stVec3& cVec)
{
    float x(m_afData[0]), y(m_afData[1]), z(m_afData[2]);

    m_afData[0] = y * cVec.m_afData[2] - z * cVec.m_afData[1];
    m_afData[1] = z * cVec.m_afData[0] - x * cVec.m_afData[2];
    m_afData[2] = x * cVec.m_afData[1] - y * cVec.m_afData[0];

    return *this;
}

// operator*
inline stVec3 stVec3::operator*(float fScale) const
{
    return stVec3(m_afData[0] * fScale,
                 m_afData[1] * fScale,
                 m_afData[2] * fScale);
}

// operator*=
inline stVec3& stVec3::operator*=(float fScale) 
{
    m_afData[0] *= fScale;
    m_afData[1] *= fScale;
    m_afData[2] *= fScale;

    return *this;
}

// operator/
inline stVec3 stVec3::operator/(float fScale) const
{
    return stVec3(m_afData[0] / fScale,
                  m_afData[1] / fScale,
                  m_afData[2] / fScale);
}

// operator/=
inline stVec3& stVec3::operator/=(float fScale) 
{
    m_afData[0] /= fScale;
    m_afData[1] /= fScale;
    m_afData[2] /= fScale;

    return *this;
}

// operator==
inline bool stVec3::operator==(const stVec3& cVec) const
{
    return m_afData[0] == cVec.m_afData[0] &&
           m_afData[1] == cVec.m_afData[1] &&
           m_afData[2] == cVec.m_afData[2];
}

// operator!=
inline bool stVec3::operator!=(const stVec3& cVec) const
{
    return m_afData[0] != cVec.m_afData[0] ||
           m_afData[1] != cVec.m_afData[1] ||
           m_afData[2] != cVec.m_afData[2];
}

// operator*
inline stVec3 stVec3::operator*(const stTransform& cTransform) const
{
    return stVec3(m_afData[0] * cTransform.m_afData[0][0] +
                  m_afData[1] * cTransform.m_afData[1][0] +
                  m_afData[2] * cTransform.m_afData[2][0] +
                                cTransform.m_afData[3][0],
                  m_afData[0] * cTransform.m_afData[0][1] +
                  m_afData[1] * cTransform.m_afData[1][1] +
                  m_afData[2] * cTransform.m_afData[2][1] +
                                cTransform.m_afData[3][1],
                  m_afData[0] * cTransform.m_afData[0][2] +
                  m_afData[1] * cTransform.m_afData[1][2] +
                  m_afData[2] * cTransform.m_afData[2][2] +
                                cTransform.m_afData[3][2]);
}

// operator*
inline stVec3 stVec3::operator*(const stRotTransform& cTransform) const
{
    return stVec3(m_afData[0] * cTransform.m_afData[0][0] +
                  m_afData[1] * cTransform.m_afData[1][0] +
                  m_afData[2] * cTransform.m_afData[2][0],
                  m_afData[0] * cTransform.m_afData[0][1] +
                  m_afData[1] * cTransform.m_afData[1][1] +
                  m_afData[2] * cTransform.m_afData[2][1],
                  m_afData[0] * cTransform.m_afData[0][2] +
                  m_afData[1] * cTransform.m_afData[1][2] +
                  m_afData[2] * cTransform.m_afData[2][2]);
}


///////////////////////////////////////////////////////////////////////  
//  class stRotTransform inlined function definitions

// stRotTransform
inline stRotTransform::stRotTransform( )
{
    m_afData[0][0] = 1.0f;
    m_afData[0][1] = 0.0f;
    m_afData[0][2] = 0.0f;
    m_afData[1][0] = 0.0f;
    m_afData[1][1] = 1.0f;
    m_afData[1][2] = 0.0f;
    m_afData[2][0] = 0.0f;
    m_afData[2][1] = 0.0f;
    m_afData[2][2] = 1.0f;
}


// Set
inline void stRotTransform::Set(float afData[3][3])
{
    m_afData[0][0] = afData[0][0]; 
    m_afData[0][1] = afData[0][1];
    m_afData[0][2] = afData[0][2];
    m_afData[1][0] = afData[1][0];
    m_afData[1][1] = afData[1][1];
    m_afData[1][2] = afData[1][2];
    m_afData[2][0] = afData[2][0];
    m_afData[2][1] = afData[2][1];
    m_afData[2][2] = afData[2][2];
}

// LoadIdentity
inline void stRotTransform::LoadIdentity( )
{
    m_afData[0][0] = 1.0f;
    m_afData[0][1] = 0.0f;
    m_afData[0][2] = 0.0f;
    m_afData[1][0] = 0.0f;
    m_afData[1][1] = 1.0f;
    m_afData[1][2] = 0.0f;
    m_afData[2][0] = 0.0f;
    m_afData[2][1] = 0.0f;
    m_afData[2][2] = 1.0f;
}
    

// operator*
inline stRotTransform stRotTransform::operator*(const stRotTransform& cTrans) const
{
    stRotTransform cTemp;

    cTemp.m_afData[0][0] = m_afData[0][0] * cTrans.m_afData[0][0] +
                           m_afData[0][1] * cTrans.m_afData[1][0] +
                           m_afData[0][2] * cTrans.m_afData[2][0];
    cTemp.m_afData[0][1] = m_afData[0][0] * cTrans.m_afData[0][1] +
                           m_afData[0][1] * cTrans.m_afData[1][1] +
                           m_afData[0][2] * cTrans.m_afData[2][1];
    cTemp.m_afData[0][2] = m_afData[0][0] * cTrans.m_afData[0][2] +
                           m_afData[0][1] * cTrans.m_afData[1][2] +
                           m_afData[0][2] * cTrans.m_afData[2][2];
    cTemp.m_afData[1][0] = m_afData[1][0] * cTrans.m_afData[0][0] +
                           m_afData[1][1] * cTrans.m_afData[1][0] +
                           m_afData[1][2] * cTrans.m_afData[2][0];
    cTemp.m_afData[1][1] = m_afData[1][0] * cTrans.m_afData[0][1] +
                           m_afData[1][1] * cTrans.m_afData[1][1] +
                           m_afData[1][2] * cTrans.m_afData[2][1];
    cTemp.m_afData[1][2] = m_afData[1][0] * cTrans.m_afData[0][2] +
                           m_afData[1][1] * cTrans.m_afData[1][2] +
                           m_afData[1][2] * cTrans.m_afData[2][2];
    cTemp.m_afData[2][0] = m_afData[2][0] * cTrans.m_afData[0][0] +
                           m_afData[2][1] * cTrans.m_afData[1][0] +
                           m_afData[2][2] * cTrans.m_afData[2][0];
    cTemp.m_afData[2][1] = m_afData[2][0] * cTrans.m_afData[0][1] +
                           m_afData[2][1] * cTrans.m_afData[1][1] +
                           m_afData[2][2] * cTrans.m_afData[2][1];
    cTemp.m_afData[2][2] = m_afData[2][0] * cTrans.m_afData[0][2] +
                           m_afData[2][1] * cTrans.m_afData[1][2] +
                           m_afData[2][2] * cTrans.m_afData[2][2];

    return cTemp;
}



// operator*
inline stVec3 stRotTransform::operator*(const stVec3& cVec3) const
{
    return stVec3(m_afData[0][0] * cVec3.m_afData[0] +
                  m_afData[0][1] * cVec3.m_afData[1] +
                  m_afData[0][2] * cVec3.m_afData[2],
                  m_afData[1][0] * cVec3.m_afData[0] +
                  m_afData[1][1] * cVec3.m_afData[1] +
                  m_afData[1][2] * cVec3.m_afData[2],
                  m_afData[2][0] * cVec3.m_afData[0] +
                  m_afData[2][1] * cVec3.m_afData[1] +
                  m_afData[2][2] * cVec3.m_afData[2]);
}


// RotateX
inline void stRotTransform::RotateX(float fAngle)
{
    stRotTransform cRotMatrix;

    float fCosine = VectorCosD(fAngle);
    float fSine = VectorSinD(fAngle);

    cRotMatrix.m_afData[0][0] = 1.0f;
    cRotMatrix.m_afData[0][1] = 0.0f;
    cRotMatrix.m_afData[0][2] = 0.0f;
    cRotMatrix.m_afData[1][0] = 0.0f;
    cRotMatrix.m_afData[1][1] = fCosine;
    cRotMatrix.m_afData[1][2] = fSine;
    cRotMatrix.m_afData[2][0] = 0.0f;
    cRotMatrix.m_afData[2][1] = -fSine;
    cRotMatrix.m_afData[2][2] = fCosine;

    // this function can be further optimized by hardcoding
    // the multiplication here and removing terms with 0.0 multipliers.

    *this = cRotMatrix * *this;
}


// RotateY
inline void stRotTransform::RotateY(float fAngle)
{
    stRotTransform cRotMatrix;

    float fCosine = VectorCosD(fAngle);
    float fSine = VectorSinD(fAngle);

    cRotMatrix.m_afData[0][0] = fCosine;
    cRotMatrix.m_afData[0][1] = 0.0f;
    cRotMatrix.m_afData[0][2] = -fSine;
    cRotMatrix.m_afData[1][0] = 0.0f;
    cRotMatrix.m_afData[1][1] = 1.0f;
    cRotMatrix.m_afData[1][2] = 0.0f;
    cRotMatrix.m_afData[2][0] = fSine;
    cRotMatrix.m_afData[2][1] = 0.0f;
    cRotMatrix.m_afData[2][2] = fCosine;

    // this function can be further optimized by hardcoding
    // the multiplication here and removing terms with 0.0 multipliers.

    *this = cRotMatrix * *this;
}



// RotateZ
inline void stRotTransform::RotateZ(float fAngle)
{
    stRotTransform cRotMatrix;

    float fCosine = VectorCosD(fAngle);
    float fSine = VectorSinD(fAngle);

    cRotMatrix.m_afData[0][0] = fCosine;
    cRotMatrix.m_afData[0][1] = fSine;
    cRotMatrix.m_afData[0][2] = 0.0f;
    cRotMatrix.m_afData[1][0] = -fSine;
    cRotMatrix.m_afData[1][1] = fCosine;
    cRotMatrix.m_afData[1][2] = 0.0f;
    cRotMatrix.m_afData[2][0] = 0.0f;
    cRotMatrix.m_afData[2][1] = 0.0f;
    cRotMatrix.m_afData[2][2] = 1.0f;

    // this function can be further optimized by hardcoding
    // the multiplication here and removing terms with 0.0 multipliers.

    *this = cRotMatrix * *this;
}



// RotateYZ
inline void stRotTransform::RotateYZ(float fYangle, float fZangle)
{
    stRotTransform cRotMatrix;

    float fCosineY = VectorCosD(fYangle);
    float fSineY = VectorSinD(fYangle);
    float fCosineZ = VectorCosD(fZangle);
    float fSineZ = VectorSinD(fZangle);

    cRotMatrix.m_afData[0][0] = fCosineY * fCosineZ;
    cRotMatrix.m_afData[0][1] = fCosineY * fSineZ;
    cRotMatrix.m_afData[0][2] = -fSineY;
    cRotMatrix.m_afData[1][0] = -fSineZ;
    cRotMatrix.m_afData[1][1] = fCosineZ;
    cRotMatrix.m_afData[1][2] = 0.0f;
    cRotMatrix.m_afData[2][0] = fSineY * fCosineZ;
    cRotMatrix.m_afData[2][1] = fSineY * fSineZ;
    cRotMatrix.m_afData[2][2] = fCosineY;

    *this = cRotMatrix * *this;
}


// RotateAxis
inline void stRotTransform::RotateAxis(float fAngle, stVec3 cAxis)
{
    stRotTransform cRotMatrix;

    float s = VectorSinD(fAngle);
    float c = VectorCosD(fAngle);
    float t = 1.0f - c;

    float x = cAxis.m_afData[0];
    float y = cAxis.m_afData[1];
    float z = cAxis.m_afData[2];

    cRotMatrix.m_afData[0][0] = t * x * x + c;
    cRotMatrix.m_afData[0][1] = t * x * y + s * z;
    cRotMatrix.m_afData[0][2] = t * x * z - s * y;
    cRotMatrix.m_afData[1][0] = t * x * y - s * z;
    cRotMatrix.m_afData[1][1] = t * y * y + c;
    cRotMatrix.m_afData[1][2] = t * y * z + s * x;
    cRotMatrix.m_afData[2][0] = t * x * z + s * y;
    cRotMatrix.m_afData[2][1] = t * y * z - s * x;
    cRotMatrix.m_afData[2][2] = t * z * z + c;
        
    *this = cRotMatrix * *this;
}   

// RotateAxisFromIdentity
inline void stRotTransform::RotateAxisFromIdentity(float fAngle, stVec3 cAxis)
{
    float s = VectorSinD(fAngle);
    float c = VectorCosD(fAngle);
    float t = 1.0f - c;

    float x = cAxis.m_afData[0];
    float y = cAxis.m_afData[1];
    float z = cAxis.m_afData[2];

    m_afData[0][0] = t * x * x + c;
    m_afData[0][1] = t * x * y + s * z;
    m_afData[0][2] = t * x * z - s * y;
    m_afData[1][0] = t * x * y - s * z;
    m_afData[1][1] = t * y * y + c;
    m_afData[1][2] = t * y * z + s * x;
    m_afData[2][0] = t * x * z + s * y;
    m_afData[2][1] = t * y * z - s * x;
    m_afData[2][2] = t * z * z + c;
}


///////////////////////////////////////////////////////////////////////  
//  Global utility function definitions

inline float VecRad2Deg(float fRad)
{
    return fRad * 57.295779513082320876798154814105f;
}

inline float VecDeg2Rad(float fDeg)
{
    return fDeg / 57.295779513082320876798154814105f;
}

inline float VecInterpolate(float fStart, float fEnd, float fPercent)
{
    return fStart + (fEnd - fStart) * fPercent;
}

inline stVec VecInterpolate(const stVec& cStart, const stVec& cEnd, float fPercent)
{
    return cStart + (cEnd - cStart) * fPercent;
}

inline stVec3 VecInterpolate(const stVec3& cStart, const stVec3& cEnd, float fPercent)
{
    return cStart + (cEnd - cStart) * fPercent;
}

inline bool VecRouglyEqual(float fA, float fB, float fTolerance)
{
    return fabs(fA - fB) < fTolerance;
}


///////////////////////////////////////////////////////////////////////  
//  typedefs to help tie STL allocations into SpeedTree allocator

DefineAllocator(st_allocator_stVec);
typedef std::vector<stVec, st_allocator_stVec<stVec> > st_vector_stVec;

DefineAllocator(st_allocator_stVec3);
typedef std::vector<stVec3, st_allocator_stVec3<stVec3> > st_vector_stVec3;
