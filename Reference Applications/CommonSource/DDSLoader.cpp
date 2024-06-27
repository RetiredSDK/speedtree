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

// Some of this code is taken from nv_dds.h/.cpp available from NVidia


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#include "DDSLoader.h"
#include <PSGL/psgl.h>
#include <PSGL/psglu.h>
#include <stdio.h>
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Endian Issues

//#if defined (__PPU__)
//  #define BIG_ENDIAN
//#endif

#ifdef BIG_ENDIAN
    #define EndianSwap32(a) (((((a) & 0xff000000) >> 24) | ((a) & 0x00ff0000) >> 8 | ((a) & 0x0000ff00) << 8 | ((a) & 0x000000ff) << 24))
    #define EndianSwap16(a) (((((a) & 0xff00) >> 8) | (((a) & 0x00ff) << 8)))
#else
    #define EndianSwap32
    #define EndianSwap16
#endif


///////////////////////////////////////////////////////////////////////  
//  Static members

map<string, unsigned int> CDDSLoader::m_mapCache;


///////////////////////////////////////////////////////////////////////  
//  DDS Info

#define DDS_FOURCC  0x00000004
#define DDS_RGB     0x00000040
#define DDS_RGBA    0x00000041
#define DDS_DEPTH   0x00800000

#define DDS_COMPLEX 0x00000008
#define DDS_CUBEMAP 0x00000200
#define DDS_VOLUME  0x00200000

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

struct SDDSPixelFormat
{
    unsigned int    m_uiSize;
    unsigned int    m_uiFlags;
    unsigned int    m_uiFourCC;
    unsigned int    m_uiRGBBitCount;
    unsigned int    m_uiRBitMask;
    unsigned int    m_uiGBitMask;
    unsigned int    m_uiBBitMask;
    unsigned int    m_uiABitMask;
};

struct SDXTColBlock
{
    unsigned short  m_usCol0;
    unsigned short  m_usCol1;
    unsigned char   m_aucRow[4];
};

struct SDXT3AlphaBlock
{
    unsigned short  m_ausRow[4];
};

struct SDXT5AlphaBlock
{
    unsigned char   m_ucAlpha0;
    unsigned char   m_ucAlpha1;
    unsigned char   m_aucRow[6];
};

struct SDDSHeader
{
    unsigned int    m_uiSize;
    unsigned int    m_uiFlags;
    unsigned int    m_uiHeight;
    unsigned int    m_uiWidth;
    unsigned int    m_uiPitchOrLinearSize;
    unsigned int    m_uiDepth;
    unsigned int    m_uiMipMapCount;
    unsigned int    m_auiReserved1[11];
    SDDSPixelFormat m_sPixelFormat;
    unsigned int    m_uiCaps1;
    unsigned int    m_uiCaps2;
    unsigned int    m_auiReserved2[3];
};


///////////////////////////////////////////////////////////////////////  
//  CDDSLoader::LoadFromFile

unsigned int CDDSLoader::LoadFromFile(const string& strFilename, bool bReport)
{
    unsigned int uiReturn = 0;

    if (!strFilename.empty( ))
    {
        map<string, unsigned int>::iterator iter = m_mapCache.find(strFilename);
        if (iter != m_mapCache.end( ))
        {
            uiReturn = iter->second;
            if (bReport)
                printf("Using cached DDS texture [%s]\n", strFilename.c_str( ));
        }
        else
        {
            FILE* pFile = fopen(strFilename.c_str( ), "rb");
            if (pFile != NULL)
            {   
                fseek(pFile, 0L, SEEK_END);
                int iNumBytes = ftell(pFile);
                int iErrorCode = fseek(pFile, 0, SEEK_SET);
                
                if (iNumBytes > 0 && iErrorCode >= 0)
                {
                    unsigned char* pData = new unsigned char[iNumBytes];
                    int iNumBytesRead = fread(pData, 1, iNumBytes, pFile);
                    if (iNumBytesRead == iNumBytes)
                        uiReturn = LoadFromData(pData, iNumBytes, false);

                    delete [] pData;
                    pData = NULL;
                }

                fclose(pFile);
            }

            if (uiReturn != 0)
            {
                m_mapCache[strFilename] = uiReturn;
                if (bReport)
                    printf("Loaded DDS texture [%s]\n", strFilename.c_str( ));
            }
            else if (bReport)
            {
                printf("Failed to load DDS texture [%s]\n", strFilename.c_str( ));
            }
        }
    }

    return uiReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CDDSLoader::LoadFromData

unsigned int CDDSLoader::LoadFromData(unsigned char* pchData, int iNumBytes, bool bReport)
{
    unsigned int uiReturn = 0;
    if (iNumBytes > 0 && pchData != NULL)
    {
        if (pchData[0] == 'D' && pchData[1] == 'D' && pchData[2] == 'S' && pchData[3] == ' ')
        {
            SDDSHeader sHeader;
            memcpy(&sHeader, &(pchData[4]), sizeof(SDDSHeader));
            
            int iFormat = 0;
            if (EndianSwap32(sHeader.m_sPixelFormat.m_uiFlags) & DDS_FOURCC)
            {
                switch(EndianSwap32(sHeader.m_sPixelFormat.m_uiFourCC))
                {
                case FOURCC_DXT1:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                    break;
                case FOURCC_DXT3:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                    break;
                case FOURCC_DXT5:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                    break;
                default:
                    break;
                }
            }

            if (iFormat != 0)
            {
                glGenTextures(1, &uiReturn);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, uiReturn);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                int iWidth = EndianSwap32(sHeader.m_uiWidth);
                int iHeight = EndianSwap32(sHeader.m_uiHeight);
                int iNumMipmaps = EndianSwap32(sHeader.m_uiMipMapCount);
                unsigned int uiCurrentIndex = sizeof(SDDSHeader) + 4;

                for (int i = 0; i < iNumMipmaps && (iWidth || iHeight); i++)
                {
                    int iSize = ((iWidth + 3) / 4) * ((iHeight + 3) / 4) * (iFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);

                    glCompressedTexImage2D(GL_TEXTURE_2D, 
                                            i, 
                                            iFormat, 
                                            iWidth, 
                                            iHeight, 
                                            0, 
                                            iSize, 
                                            &pchData[uiCurrentIndex]);
                    uiCurrentIndex += iSize;
                    if ((int)uiCurrentIndex > iNumBytes)
                    {
                        glDeleteTextures(1, &uiReturn);
                        if (bReport)
                            printf("Failed to load DDS texture\n");
                        return 0;
                    }

                    iWidth = iWidth >> 1;
                    if (iWidth <= 0)
                        iWidth = 1;
                    iHeight = iHeight >> 1;
                    if (iHeight <= 0)
                        iHeight = 1; 
                }               
            }
        }
    }

    if (bReport)
    {
        if (uiReturn != 0)
            printf("Failed to load DDS texture\n");
        else
            printf("Loaded DDS texture.\n");
    }           
                
    return uiReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CDDSLoader::ClearCache

void CDDSLoader::ClearCache(bool bDeleteTextures)
{
    if (bDeleteTextures)
    {
        for (map<string, unsigned int>::iterator iter = m_mapCache.begin( ); iter != m_mapCache.end( ); ++iter)
            glDeleteTextures(1, &iter->second); 
    }
    m_mapCache.clear( );
}
