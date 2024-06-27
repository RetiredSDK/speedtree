///////////////////////////////////////////////////////////////////////  
//  LeafOffsetTable.h
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


///////////////////////////////////////////////////////////////////////  
// Leaf Offset Table
//
// In an effort to keep all of the leaves from facing the same direction,
// small rotations are added to the billboarding matrix in an order to 
// fight depth buffer conflict.  This table contains a series of values
// used as the rotation angles (in degrees)

static  const   int     c_nLeafOffsetCount = 30;
static  const   float   c_fLeafOffsetScalar = 1.0f;
static  const   float   c_afLeafOffsets[c_nLeafOffsetCount] =
{
    c_fLeafOffsetScalar * 6.0f,     c_fLeafOffsetScalar * -10.0f,   c_fLeafOffsetScalar * 16.0f,
    c_fLeafOffsetScalar * -8.0f,    c_fLeafOffsetScalar * 2.0f,     c_fLeafOffsetScalar * -18.0f,
    c_fLeafOffsetScalar * -14.0f,   c_fLeafOffsetScalar * 0.0f,     c_fLeafOffsetScalar * -4.0f,
    c_fLeafOffsetScalar * 10.0f,    c_fLeafOffsetScalar * -6.0f,    c_fLeafOffsetScalar * 8.0f,
    c_fLeafOffsetScalar * 4.0f,     c_fLeafOffsetScalar * -2.0f,    c_fLeafOffsetScalar * 14.0f,
    c_fLeafOffsetScalar * -12.0f,   c_fLeafOffsetScalar * 14.0f,    c_fLeafOffsetScalar * -16.0f,
    c_fLeafOffsetScalar * 4.0f,     c_fLeafOffsetScalar * -18.0f,   c_fLeafOffsetScalar * 13.0f,
    c_fLeafOffsetScalar * 9.0f,     c_fLeafOffsetScalar * 17.0f,    c_fLeafOffsetScalar * -9.0f,
    c_fLeafOffsetScalar * 3.0f,     c_fLeafOffsetScalar * -7.0f,    c_fLeafOffsetScalar * -15.0f,
    c_fLeafOffsetScalar * 7.0f,     c_fLeafOffsetScalar * -5.0f,    c_fLeafOffsetScalar * 15.0f
};


