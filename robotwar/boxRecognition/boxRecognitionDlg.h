
// boxRecognitionDlg.h : ��� ����
//

#pragma once
#include "vfw.h"
LRESULT CALLBACK CallbackOnFrame(HWND hWnd, LPVIDEOHDR lpVHdr);

// CboxRecognitionDlg ��ȭ ����
class CboxRecognitionDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CboxRecognitionDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_BOXRECOGNITION_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	HWND m_Cap;
	afx_msg void OnDestroy();
};
