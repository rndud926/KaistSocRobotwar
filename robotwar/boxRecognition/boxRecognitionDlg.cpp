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


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.
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
		//블럭단위 분산수행
		fprintf(stderr,"block_variance error\n");
		exit(1);
	}

	
	if( (error_code = _8x8filtering(resize) ) == NULL)
	{
		//필터링
		fprintf(stderr,"filtering error\n");
		exit(1);
	}

	SOCV_morphologh_bin_Erosion(resize);
	SOCV_morphologh_bin_Erosion(resize);
	
	
	if( ( error_code = SOCV_Edge_Image(resize, EDGE_SOBEL)) == NULL)
	{
		//엣지 계산
		fprintf(stderr,"Edge error\n");
		exit(1);
	}
	
	//dinocv_canny_edge(resize,0,90);
	
	if( ( hough = SOCV_hue_hough(resize) ) == NULL)
	{
		//블럭단위 허프 후보군
		fprintf(stderr,"hue_hough error\n");
		exit(1);
	}

	if( ( peak = find4PeakPoint(hough) ) == NULL)
	{
		//블럭단위 허프 후보군 고름(버블정렬 퀵소트로 대체해야함)
		fprintf(stderr,"peak error\n");
		exit(1);
	}

	if( (candidate_peak = SearchPeakPoint(peak,30) ) == NULL)
	{
		//허프 룩업테이블에서 4가지 후보 색출 
		//fprintf(stderr,"no candidate\n");
	}
	else // 객체를 찾음
	{
		//추적 정보 저장 
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

	//Display 시간, 카운트, 선긋기
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

	// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	// 구현입니다.
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


// CboxRecognitionDlg 대화 상자




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


// CboxRecognitionDlg 메시지 처리기

BOOL CboxRecognitionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
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

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	m_Cap = capCreateCaptureWindow(TEXT("SoC_RobotWar"), WS_CHILD | WS_VISIBLE, 0, 0, 640, 480, this->m_hWnd, NULL);
	if(capSetCallbackOnFrame(m_Cap, CallbackOnFrame) == FALSE) return FALSE;
	if(capDriverConnect(m_Cap, 0) == FALSE) return FALSE;

	capPreviewRate(m_Cap, 30);
	capOverlay(m_Cap, false);
	capPreview(m_Cap, true);


	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CboxRecognitionDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CboxRecognitionDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CboxRecognitionDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	capDriverDisconnect(m_Cap);
	if(pImgBuffer)
		delete []pImgBuffer;
}
