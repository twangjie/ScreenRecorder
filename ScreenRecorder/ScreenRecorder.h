
// ScreenRecorder.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CScreenRecorderApp: 
// �йش����ʵ�֣������ ScreenRecorder.cpp
//

class CScreenRecorderApp : public CWinApp
{
public:
	CScreenRecorderApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CScreenRecorderApp theApp;
