#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include "stdafx.h"
#include "boxRecognition.h"
#include "boxRecognitionDlg.h"
#include "afxdialogex.h"
#include "imgcore.h"
#include "dinotime.h"
#include <cmath>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WIDTH 640
#define HEIGHT 480

TrackingInfo *trackingInfo = NULL;

ObjectInfo *obj = NULL;

IMAGE_D *resize = NULL;
IMAGE_D *hough = NULL;
IMAGE_D *filtering = NULL;
IMAGE_D *img = NULL;
IMAGE_D *hue = NULL;

PeakPoint *peak = NULL;
PeakPoint *candidate_peak = NULL;

BITMAPINFO bitInfo;
LPBYTE pImgBuffer;


// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.
LRESULT CALLBACK CallbackOnFrame(HWND hWnd, LPVIDEOHDR lpVHdr){

	register int i, j;
	int error_code = 0;
	
	DWatch watch;

	HSV_COLOR_D hsv;
	RGB_COLOR_D rgb;

	int block_width = 10;
	int block_height = 10;

	watch.Start();

	if(trackingInfo == NULL)
	{
		trackingInfo = make_trackingInfo(trackingInfo);
	}

	if(obj == NULL)
	{
		obj = (ObjectInfo *)malloc(sizeof(ObjectInfo));
		obj->blueRectangle = make_ColorObjectInfo(obj->blueRectangle);
		obj->redRectangle = make_ColorObjectInfo(obj->redRectangle);
		obj->greenRectangle = make_ColorObjectInfo(obj->blueRectangle);
		obj->yellowRectangle = make_ColorObjectInfo(obj->yellowRectangle);
	}

	img = SOCV_make_Image(WIDTH,HEIGHT,24);

	Color_D color[5]= { {255, 0, 0} , {0, 0, 255}, { 0, 255, 0}, { 255, 255, 0}, { 255, 0, 255}} ;
	
	for(i = 0 ; i < HEIGHT ; i++){
		memcpy(img->source[i], lpVHdr->lpData+WIDTH*i*3, sizeof(BYTE) *WIDTH*3);
	}
	
	resize = NULL;
	hough = NULL;
	peak = NULL;
	candidate_peak = NULL;
	
	if(  ( hue = SOCV_int_hue(img) ) == NULL)
	{
		//rgb24 -> hsi ( return -> hue)
		fprintf(stderr,"trans hue error\n");
		exit(1);
	}


	if( ( filtering = SOCV_Filtering(hue,FILTERING_unsharp,NULL) ) == NULL)
	{
		//filtering
		fprintf(stderr,"Filtering error\n");
		exit(1);
	}


	if( ( resize = SOCV_block_variance(filtering) ) == NULL)
	{
		//������ �л����
		fprintf(stderr,"block_variance error\n");
		exit(1);
	}

	
	if( (error_code = _8x8filtering(resize) ) == NULL)
	{
		//���͸�
		fprintf(stderr,"filtering error\n");
		exit(1);
	}

	SOCV_morphologh_bin_Erosion(resize);
	SOCV_morphologh_bin_Erosion(resize);
	
	
	if( ( error_code = SOCV_Edge_Image(resize, EDGE_SOBEL)) == NULL)
	{
		//���� ���
		fprintf(stderr,"Edge error\n");
		exit(1);
	}
	
	//dinocv_canny_edge(resize,0,90);
	
	if( ( hough = SOCV_hue_hough(resize) ) == NULL)
	{
		//������ ���� �ĺ���
		fprintf(stderr,"hue_hough error\n");
		exit(1);
	}

	if( ( peak = find4PeakPoint(hough) ) == NULL)
	{
		//������ ���� �ĺ��� ��(�������� ����Ʈ�� ��ü�ؾ���)
		fprintf(stderr,"peak error\n");
		exit(1);
	}

	if( (candidate_peak = SearchPeakPoint(peak,30) ) == NULL)
	{
		//���� ������̺��� 4���� �ĺ� ���� 
		//fprintf(stderr,"no candidate\n");
	}
	else // ��ü�� ã��
	{
		//���� ���� ���� 
		POINT_D midpoint;
		midpoint = find_vertices(candidate_peak,trackingInfo,img);

		if(trackingInfo != NULL) 
		{
			if(Tracking_info(trackingInfo,hue))
			{
				object_distinguish(trackingInfo,img,color,obj);
			}	
		}
	}

	//Display �ð�, ī��Ʈ, ���߱�
	middle_view(img,obj->redRectangle,color,1);
	middle_view(img,obj->blueRectangle,color,2);
	middle_view(img,obj->greenRectangle,color,3);
	middle_view(img,obj->yellowRectangle,color,4);

	/*
	POINT_D p1 = { 130, 130};
	POINT_D p2 = { 130, 200};

	_houghDrawline(img,p1.x,p2.x,p1.y,p2.y,128);
	*/

	for(i = 0 ; i < resize->height; i++)
	{
		for(j = 0 ; j < resize->width ; j++)
		{
			img->source[i][j*3] = resize->source[i][j];
			img->source[i][j*3+1] = resize->source[i][j];
			img->source[i][j*3+2] = resize->source[i][j];
		}
	}

	watch.End();
	double fps = (double)watch.GetDurationMilliSecond();
	printf("fps : %3.1lf \n", 1000/fps);

	/*
	for(i = 0 ; i < filtering->height; i++)
	{
		for(j = 0 ; j < filtering->width ; j++)
		{
			img->source[i][j*3] = filtering->source[i][j];
			img->source[i][j*3+1] = filtering->source[i][j];
			img->source[i][j*3+2] = filtering->source[i][j];
		}
	}
	*/

	
	for(i = 0 ; i < HEIGHT ; i++){
		memcpy(lpVHdr->lpData+WIDTH*i*3, img->source[i], sizeof(BYTE) *WIDTH*3);
	}
	
	free(peak);
	if(candidate_peak != NULL)
		free(candidate_peak);
	SOCV_release_Image(hough);
	SOCV_release_Image(filtering);
	SOCV_release_Image(resize);
	SOCV_release_Image(img);
	SOCV_release_Image(hue);
	return (LRESULT)true;
}
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

	// �����Դϴ�.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CboxRecognitionDlg ��ȭ ����




CboxRecognitionDlg::CboxRecognitionDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CboxRecognitionDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CboxRecognitionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CboxRecognitionDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CboxRecognitionDlg �޽��� ó����

BOOL CboxRecognitionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

	m_Cap = capCreateCaptureWindow(TEXT("SoC_RobotWar"), WS_CHILD | WS_VISIBLE, 0, 0, 640, 480, this->m_hWnd, NULL);
	if(capSetCallbackOnFrame(m_Cap, CallbackOnFrame) == FALSE) return FALSE;
	if(capDriverConnect(m_Cap, 0) == FALSE) return FALSE;

	capPreviewRate(m_Cap, 30);
	capOverlay(m_Cap, false);
	capPreview(m_Cap, true);


	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

void CboxRecognitionDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CboxRecognitionDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CboxRecognitionDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CboxRecognitionDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	capDriverDisconnect(m_Cap);
	if(pImgBuffer)
		delete []pImgBuffer;
}
