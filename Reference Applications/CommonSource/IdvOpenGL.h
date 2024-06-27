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
#ifdef IDV_OPENGL
#ifdef PS3
#include <PSGL/psgl.h>
#include <PSGL/psglu.h>
#include <Cg/cg.h>
#include "Cg/cgGL.h"
#else
#include "glew.h"
#include "Cg/cgGL.h"
#endif
#endif


///////////////////////////////////////////////////////////////////////  
//  The PS3 libraries don't use ARB suffixes but we need to maintain
//  compatibility with other OpenGL versions

// functions
#ifndef glGenBuffersARB
#define glGenBuffersARB glGenBuffers
#endif
#ifndef glClientActiveTextureARB
#define glClientActiveTextureARB glClientActiveTexture
#endif
#ifndef glActiveTextureARB
#define glActiveTextureARB glActiveTexture
#endif
#ifndef glBindBufferARB
#define glBindBufferARB glBindBuffer
#endif
#ifndef glBufferDataARB
#define glBufferDataARB glBufferData
#endif
#ifndef glBufferSubDataARB
#define glBufferSubDataARB glBufferSubData
#endif
// types
#ifndef GLsizeiptrARB
#define GLsizeiptrARB GLsizeiptr
#endif
#ifndef GLintptrARB
#define GLintptrARB GLintptr
#endif
// enumerations
#ifndef GL_STATIC_DRAW_ARB
#define GL_STATIC_DRAW_ARB GL_STATIC_DRAW
#endif
#ifndef GL_DYNAMIC_DRAW_ARB
#define GL_DYNAMIC_DRAW_ARB GL_DYNAMIC_DRAW
#endif
#ifndef GL_TEXTURE0_ARB
#define GL_TEXTURE0_ARB GL_TEXTURE0
#endif
#ifndef GL_ARRAY_BUFFER_ARB
#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER_ARB
#define GL_ELEMENT_ARRAY_BUFFER_ARB GL_ELEMENT_ARRAY_BUFFER
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT GL_BGRA
#endif
#ifndef GL_BGR_EXT
#define GL_BGR_EXT GL_BGR
#endif

