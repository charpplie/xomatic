//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IPROGRESSMONITOR_H__
#define __IPROGRESSMONITOR_H__

class IProgressMonitor
{
public:
	virtual void UpdateProgress(int percent, char *format,  ...) = 0;
};

#endif //__IPROGRESSMONITOR_H__
