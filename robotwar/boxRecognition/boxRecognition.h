
// boxRecognition.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CboxRecognitionApp:
// �� Ŭ������ ������ ���ؼ��� boxRecognition.cpp�� �����Ͻʽÿ�.
//

class CboxRecognitionApp : public CWinApp
{
public:
	CboxRecognitionApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CboxRecognitionApp theApp;