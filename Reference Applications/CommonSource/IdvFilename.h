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
#ifdef _XBOX
#include <Xtl.h>
#elif WIN32
#ifndef _WINDOWS_
#include <windows.h>
#endif
#endif
#ifdef PS3
#include <dirent.h>
#endif


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


///////////////////////////////////////////////////////////////////////  
//  IdvStrcmpi 

inline bool IdvStrcmpi(const st_string& strA, const st_string& strB)
{
#ifdef PS3
    return strcasecmp(strA.c_str( ), strB.c_str( )) == 0;
#else
    return _strcmpi(strA.c_str( ), strB.c_str( )) == 0;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  IdvScanFolder

inline void IdvScanFolder(const st_string& strFolder, const st_string& strExtension, bool bRecurse, st_vector_string& vFiles)
{
#if WIN32 | _XBOX
    WIN32_FIND_DATAA sDirEntry;
    st_string strSearch = strFolder + "\\*";

    HANDLE hDirHandle = FindFirstFileA(strSearch.c_str( ), &sDirEntry);
    if (hDirHandle != INVALID_HANDLE_VALUE)
    {
        // check "../data" for any folder
        do
        {
            st_string strFilename(sDirEntry.cFileName);
            if (IdvStrcmpi(IdvExtension(strFilename), strExtension))
                vFiles.push_back(strFolder + "\\" + strFilename);
            else if (bRecurse &&
                     sDirEntry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                     strcmp(sDirEntry.cFileName, ".") && 
                     strcmp(sDirEntry.cFileName, ".."))
            {
                IdvScanFolder(strFolder + "\\" + st_string(sDirEntry.cFileName), strExtension, bRecurse, vFiles);
            }

        } while (FindNextFileA(hDirHandle, &sDirEntry));

        (void) FindClose(hDirHandle);
    }
    else
        printf("Failed to scan folder [%s] with wildcard [%s]\n", strFolder.c_str( ), strSearch.c_str( ));
#endif

#ifdef PS3
    DIR* pDir = opendir(strFolder.c_str( ));
    if (pDir)
    {
        struct dirent* pEntry = readdir(pDir);
        while (pEntry)
        {
            st_string strFilename(pEntry->d_name);
            if (IdvStrcmpi(IdvExtension(strFilename), strExtension))
            {
                strFilename = strFolder + "/" + strFilename;
                vFiles.push_back(strFilename);
            }
            else if (bRecurse &&
                     pEntry->d_type == DT_DIR &&
                     strcmp(pEntry->d_name, ".") && 
                     strcmp(pEntry->d_name, ".."))
            {
                IdvScanFolder(pEntry->d_name, strExtension, bRecurse, vFiles);
            }

            pEntry = readdir(pDir);
        }
        closedir(pDir);
    }
    else
        printf("Failed to scan folder [%s] - %s\n", strFolder.c_str( ), strerror(errno));
#endif
}

