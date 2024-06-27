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
#include <string>
#include <map>


///////////////////////////////////////////////////////////////////////  
//  class CDDSLoader

class CDDSLoader
{
public:

static  unsigned int        LoadFromFile(const std::string& strFilename, bool bReport = true);
static  unsigned int        LoadFromData(unsigned char* pchData, int iNumBytes, bool bReport = true);
static  void                ClearCache(bool bDeleteTextures = false);

private:

static  std::map<std::string, unsigned int> m_mapCache;

};

