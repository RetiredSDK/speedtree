///////////////////////////////////////////////////////////////////////  
//  IdvFilename.h
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
#include "..\SpeedTreeMemory.h"


///////////////////////////////////////////////////////////////////////  
//  IdvExtension 

inline st_string IdvExtension(const st_string& strFilename, char chExtensionChar = '.')
{
    st_string::size_type uiPos = strFilename.find_last_of(chExtensionChar);
    if (uiPos != st_string::npos)
        return strFilename.substr(uiPos + 1);
    else
        return st_string("");
}


///////////////////////////////////////////////////////////////////////  
//  IdvNoExtension 

inline st_string IdvNoExtension(const st_string& strFilename, char chExtensionChar = '.')
{
    st_string::size_type uiPos = strFilename.find_last_of(chExtensionChar);

    if (uiPos != st_string::npos)
        return strFilename.substr(0, uiPos);
    else
        return strFilename;
}


///////////////////////////////////////////////////////////////////////  
//  IdvPath 

inline st_string IdvPath(const st_string& strFilename, st_string strDelimiters = "/\\")
{
    st_string::size_type uiPos = strFilename.find_last_of(strDelimiters);

    if (uiPos != st_string::npos)
        return strFilename.substr(0, uiPos + 1);
    else
        return st_string("");
}


///////////////////////////////////////////////////////////////////////  
//  IdvNoPath 

inline st_string IdvNoPath(const st_string& strFilename, st_string strDelimiters = "/\\")
{
    st_string::size_type uiPos = strFilename.find_last_of(strDelimiters);
    if (uiPos != st_string::npos)
        return strFilename.substr(uiPos + 1);
    else
        return strFilename;
}

