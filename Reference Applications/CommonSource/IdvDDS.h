///////////////////////////////////////////////////////////////////////  
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


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "SpeedTreeMemory.h"
#include "IdvOpenGL.h"


///////////////////////////////////////////////////////////////////////  
//  class CIdvDDS

class CIdvDDS
{
public:
static  GLuint  LoadFromFile(const st_string& strFilename, GLenum eMin, GLenum eMag, bool bFlip = false);
static  GLuint  LoadFromData(unsigned char* pData, int nNumBytes, GLenum eMin, GLenum eMag, bool bFlip = false);
};

