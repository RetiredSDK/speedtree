///////////////////////////////////////////////////////////////////////  
//  SpeedTree Shaders File
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
//
//  *** Release Version 4.1 ***

#ifdef IDV_OPENGL
    // arbvp1/arbfp1 are chosen as the lowest common denominator for most mainstream cards    
    #define VS_COMPILE_COMMAND VertexProgram = compile arbvp1
    #define PS_COMPILE_COMMAND FragmentProgram = compile arbfp1
#else
    // if using vs_1_1/ps_1_1, see next comment section below
    #define VS_COMPILE_COMMAND VertexShader = compile vs_2_0
    #define PS_COMPILE_COMMAND PixelShader = compile ps_2_0 // targets lower than ps_2_0 will not compile under Dec '06 SDK 
#endif


//  Comment in LOW_COMPILE_TARGET when compiling to lower targets like vs_1_1/ps_1_1 and
//  vp20/fp20.  It will make sure some of the more advanced shader effects are disabled.
//  Also #undef BRANCH_NORMAL_MAPPING in the C++ project.

//#define LOW_COMPILE_TARGET
#ifdef LOW_COMPILE_TARGET
#undef BRANCH_NORMAL_MAPPING
#undef FROND_NORMAL_MAPPING
#undef LEAF_NORMAL_MAPPING
#undef INTERMEDIATE_FRONDBRANCH_FADING
#undef BILLBOARD_NORMAL_MAPPING
#endif
