#ifndef FULL_SPEEDTREE_LIB

///////////////////////////////////////////////////////////////////////  
//  SpeedTreeKeyUtility.cpp
//
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization and may
//  not be copied or disclosed except in accordance with the terms of
//  that agreement.
//
//      Copyright (c) 2005 IDV, Inc.
//      All Rights Reserved.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

st_string strFailureCause;
if (!SpeedTreeKeyUtility::KeyIsValid(g_strKey, strFailureCause))
{
    SetError(IdvFormatString("License Error: [%s]", strFailureCause.c_str( )).c_str( ));
    return false;
}

#endif // !FULL_SPEEDTREE_LIB
