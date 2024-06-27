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

#include "IdvDDS.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Endian Issues

#ifdef BIG_ENDIAN
    #define EndianSwap32(a) (((((a) & (unsigned int) 0xff000000) >> 24) | ((a) & (unsigned int) 0x00ff0000) >> 8 | ((a) & (unsigned int) 0x0000ff00) << 8 | ((a) & (unsigned int) 0x000000ff) << 24))
    #define EndianSwap16(a) (((((a) & 0xff00) >> 8) | (((a) & 0x00ff) << 8)))
#else
    #define EndianSwap32
    #define EndianSwap16
#endif


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
// FlipDXT1Block

static void FlipDXT1Block(SDXTColBlock* pLine, int iNumBlocks)
{
    SDXTColBlock* pCurrentBlock = pLine;
    unsigned char ucTemp;

    for (int i = 0; i < iNumBlocks; ++i)
    {
        ucTemp = pCurrentBlock->m_aucRow[0];
        pCurrentBlock->m_aucRow[0] = pCurrentBlock->m_aucRow[3];
        pCurrentBlock->m_aucRow[3] = ucTemp;

        ucTemp = pCurrentBlock->m_aucRow[1];
        pCurrentBlock->m_aucRow[1] = pCurrentBlock->m_aucRow[2];
        pCurrentBlock->m_aucRow[2] = ucTemp;

        pCurrentBlock++;
    }
}


///////////////////////////////////////////////////////////////////////
// FlipDXT3Block

static void FlipDXT3Block(SDXTColBlock* pLine, int iNumBlocks)
{
    SDXTColBlock* pCurrentBlock = pLine;
    SDXT3AlphaBlock* pAlphaBlock = NULL;

    unsigned char ucTemp;
    unsigned short usTemp;

    for (int i = 0; i < iNumBlocks; ++i)
    {
        // alpha
        pAlphaBlock = (SDXT3AlphaBlock*)(pCurrentBlock);

        usTemp = pAlphaBlock->m_ausRow[0];
        pAlphaBlock->m_ausRow[0] = pAlphaBlock->m_ausRow[3];
        pAlphaBlock->m_ausRow[3] = usTemp;

        usTemp = pAlphaBlock->m_ausRow[1];
        pAlphaBlock->m_ausRow[1] = pAlphaBlock->m_ausRow[2];
        pAlphaBlock->m_ausRow[2] = usTemp;

        pCurrentBlock++;


        // color
        ucTemp = pCurrentBlock->m_aucRow[0];
        pCurrentBlock->m_aucRow[0] = pCurrentBlock->m_aucRow[3];
        pCurrentBlock->m_aucRow[3] = ucTemp;

        ucTemp = pCurrentBlock->m_aucRow[1];
        pCurrentBlock->m_aucRow[1] = pCurrentBlock->m_aucRow[2];
        pCurrentBlock->m_aucRow[2] = ucTemp;

        pCurrentBlock++;
    }
}


///////////////////////////////////////////////////////////////////////
// FlipDXT5Block

static void FlipDXT5Block(SDXTColBlock* pLine, int iNumBlocks)
{
    SDXTColBlock* pCurrentBlock = pLine;
    SDXT5AlphaBlock *pAlphaBlock = NULL;

    unsigned char ucTemp;
    const unsigned int c_ulMask = 0x00000007; // ulBits = 00 00 01 11
    unsigned int ulBits = 0;
    unsigned char aucBits[4][4];
    
    for (int i = 0; i < iNumBlocks; ++i)
    {
        // alpha
        pAlphaBlock = (SDXT5AlphaBlock*)(pCurrentBlock);
        
        memcpy(&ulBits, &pAlphaBlock->m_aucRow[0], sizeof(unsigned char) * 3);
        
        aucBits[0][0] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[0][1] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[0][2] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[0][3] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][0] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][1] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][2] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][3] = (unsigned char)(ulBits & c_ulMask);

        ulBits = 0;
        memcpy(&ulBits, &pAlphaBlock->m_aucRow[3], sizeof(unsigned char) * 3);

        aucBits[2][0] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[2][1] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[2][2] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[2][3] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][0] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][1] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][2] = (unsigned char)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][3] = (unsigned char)(ulBits & c_ulMask);

        unsigned int *pBits = ((unsigned int*) &(pAlphaBlock->m_aucRow[0]));
        *pBits &= 0xff000000;

        *pBits = *pBits | (aucBits[3][0] << 0);
        *pBits = *pBits | (aucBits[3][1] << 3);
        *pBits = *pBits | (aucBits[3][2] << 6);
        *pBits = *pBits | (aucBits[3][3] << 9);

        *pBits = *pBits | (aucBits[2][0] << 12);
        *pBits = *pBits | (aucBits[2][1] << 15);
        *pBits = *pBits | (aucBits[2][2] << 18);
        *pBits = *pBits | (aucBits[2][3] << 21);

        pBits = ((unsigned int*)(&(pAlphaBlock->m_aucRow[3])));
        *pBits &= 0xff000000;

        *pBits = *pBits | (aucBits[1][0] << 0);
        *pBits = *pBits | (aucBits[1][1] << 3);
        *pBits = *pBits | (aucBits[1][2] << 6);
        *pBits = *pBits | (aucBits[1][3] << 9);

        *pBits = *pBits | (aucBits[0][0] << 12);
        *pBits = *pBits | (aucBits[0][1] << 15);
        *pBits = *pBits | (aucBits[0][2] << 18);
        *pBits = *pBits | (aucBits[0][3] << 21);

        pCurrentBlock++;


        // color
        ucTemp = pCurrentBlock->m_aucRow[0];
        pCurrentBlock->m_aucRow[0] = pCurrentBlock->m_aucRow[3];
        pCurrentBlock->m_aucRow[3] = ucTemp;

        ucTemp = pCurrentBlock->m_aucRow[1];
        pCurrentBlock->m_aucRow[1] = pCurrentBlock->m_aucRow[2];
        pCurrentBlock->m_aucRow[2] = ucTemp;

        pCurrentBlock++;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvDDS::LoadFromFile

GLuint CIdvDDS::LoadFromFile(const st_string& strFilename, GLenum eMin, GLenum eMag, bool bFlip)
{
    unsigned int uiReturn = 0;

    if (!strFilename.empty( ))
    {
        FILE* pFile = fopen(strFilename.c_str( ), "rb");
        if (pFile != NULL)
        {   
            fseek(pFile, 0L, SEEK_END);
            int iNumBytes = ftell(pFile);
            int iErrorCode = fseek(pFile, 0, SEEK_SET);
            
            if (iNumBytes > 0 && iErrorCode >= 0)
            {
                unsigned char* pData = st_new_array<unsigned char>(iNumBytes, "CDDSLoader::LoadFromFile, pData");
                int iNumBytesRead = int(fread(pData, 1, iNumBytes, pFile));
                if (iNumBytesRead == iNumBytes)
                    uiReturn = LoadFromData(pData, iNumBytes, eMin, eMag, bFlip);

                st_delete_array<unsigned char>(pData, "CDDSLoader::LoadFromFile, pData");
            }

            fclose(pFile);
        }
    }

    return uiReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvDDS::LoadFromData

GLuint CIdvDDS::LoadFromData(unsigned char* pchData, int iNumBytes, GLenum eMin, GLenum eMag, bool bFlip)
{
    unsigned int uiReturn = 0;
    if (iNumBytes > 0 && pchData != NULL)
    {
        if (pchData[0] == 'D' && pchData[1] == 'D' && pchData[2] == 'S' && pchData[3] == ' ')
        {
            SDDSHeader sHeader;
            memcpy(&sHeader, &(pchData[4]), sizeof(SDDSHeader));
            
            int iFormat = 0;
            int iBlockSize = 0;
            void (*fpFlipBlocks)(SDXTColBlock*, int) = NULL;

            if (EndianSwap32(sHeader.m_sPixelFormat.m_uiFlags) & DDS_FOURCC)
            {
                switch(EndianSwap32(sHeader.m_sPixelFormat.m_uiFourCC))
                {
                case FOURCC_DXT1:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                    iBlockSize = 8;
                    fpFlipBlocks = &FlipDXT1Block;
                    break;
                case FOURCC_DXT3:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                    iBlockSize = 16;
                    fpFlipBlocks = &FlipDXT3Block;
                    break;
                case FOURCC_DXT5:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                    iBlockSize = 16;
                    fpFlipBlocks = &FlipDXT5Block;
                    break;
                default:
                    break;
                }
            }

            if (iFormat != 0)
            {
                int iWidth = EndianSwap32(sHeader.m_uiWidth);
                int iHeight = EndianSwap32(sHeader.m_uiHeight);
                int iNumMipmaps = EndianSwap32(sHeader.m_uiMipMapCount);
                unsigned int uiCurrentIndex = sizeof(SDDSHeader) + 4;

                glGenTextures(1, &uiReturn);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, uiReturn);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, eMin);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, eMag);                

                for (int i = 0; i < iNumMipmaps && (iWidth || iHeight); i++)
                {
                    int iSize = ((iWidth + 3) / 4) * ((iHeight + 3) / 4) * (iFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
                    unsigned char* pData = &pchData[uiCurrentIndex];

                    if (bFlip)
                    {
                        int iXBlocks = iWidth / 4;
                        int iYBlocks = iHeight / 4;
                        int iLineSize = iXBlocks * iBlockSize;

                        unsigned char* pTempBlock = st_new_array<unsigned char>(iLineSize, "CDDSLoader::LoadFromData, pTempBlock");

                        SDXTColBlock* pTop = NULL;
                        SDXTColBlock* pBottom = NULL;
                    
                        for (int j = 0; j < (iYBlocks >> 1); ++j)
                        {
                            pTop = (SDXTColBlock*)(pData + j * iLineSize);
                            pBottom = (SDXTColBlock*)(pData + (((iYBlocks - j) - 1) * iLineSize));

                            (*fpFlipBlocks)(pTop, iXBlocks);
                            (*fpFlipBlocks)(pBottom, iXBlocks);

                            memcpy(pTempBlock, pBottom, iLineSize);
                            memcpy(pBottom, pTop, iLineSize);
                            memcpy(pTop, pTempBlock, iLineSize);
                        }

                        st_delete_array<unsigned char>(pTempBlock, "CDDSLoader::LoadFromData, pTempBlock");
                    }

                    glCompressedTexImage2D(GL_TEXTURE_2D, 
                                            i, 
                                            iFormat, 
                                            iWidth, 
                                            iHeight, 
                                            0, 
                                            iSize, 
                                            pData);
                    uiCurrentIndex += iSize;
                    if ((int)uiCurrentIndex > iNumBytes)
                    {
                        glDeleteTextures(1, &uiReturn);
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

    return uiReturn;
}
