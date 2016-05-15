#ifndef _IMGCORE_H_
#define _IMGCORE_H_

#ifdef MYDLLTYPE
#define MYDLLTYPE _declspec(dllexport)
#else
#define MYDLLTYPE _declspec(dllexport)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Alias
#define TRUE					1
#define FALSE					0
#define PI 3.14159
typedef unsigned long			DWORD;
typedef int						BOOL;
typedef unsigned char			BYTE;
typedef unsigned short			WORD;
typedef unsigned int			UINT;
typedef long					LONG;

typedef struct{
	int x;
	int y;
}POINT_D;

typedef struct NODE{
	POINT_D point;
	struct NODE *link;
}NODE;

typedef struct{
	int top;
	int left;
	int bottom;
	int right;
}RECT_D;

typedef struct{
	BYTE **source;
	UINT width;
	UINT height;
	RECT_D *roi;	// if not roi mode, it must be NULL.
	WORD bpp;		// bit per pixel
}IMAGE_D;

typedef struct{
	BYTE blue;
	BYTE green;
	BYTE red;
	BYTE reserved;
}RGB;

#pragma pack(push,1)

typedef struct _tagBITMAPFILEHEADER{
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
}BITMAPFILEHEADER_ROBOT;


typedef struct _tagBITMAPINFOHEADER{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
}BITMAPINFOHEADER_ROBOT;

#pragma pack(pop)

typedef struct{
	double rho;
	double ang;
}LineParam;

typedef struct{
	BYTE red;
	BYTE green;
	BYTE blue;
}Color_D;

typedef struct _ContourPoints{
	int num;
	int x[100000];
	int y[100000];
	
}ContourPoints;

typedef struct RGB_COLOR_D RGB_COLOR_D;
struct RGB_COLOR_D{
    unsigned char r, g, b;    /* Channel intensities between 0 and 255 */
};

typedef struct HSV_COLOR_D HSV_COLOR_D;
struct HSV_COLOR_D{
    unsigned char hue;        /* Hue degree between 0 and 255 */
    unsigned char sat;        /* Saturation between 0 (gray) and 255 */
    unsigned char val;        /* Value between 0 (black) and 255 */
};

typedef struct _TrackingInfo{
	RECT_D *roi;
	int variance;
	int averageH;
	POINT_D middlePoint;
	NODE *plist;
}TrackingInfo;

typedef struct _ColorObjectInfo{
	NODE *plist;
	POINT_D middlePoint;
	int LastTime;
}ColorObjectInfo;

typedef struct _obj{
	ColorObjectInfo *redRectangle;
	ColorObjectInfo *blueRectangle;
	ColorObjectInfo *yellowRectangle;
	ColorObjectInfo *greenRectangle;
}ObjectInfo;

typedef struct _PeakPoin{
	int ang;
	int rho;
	int value;
}PeakPoint;

//EDGE - 외곽선 검출
#define EDGE_ROBERTS 0
#define EDGE_PREWITT 1
#define EDGE_SOBEL 2
//Filtering - 영상을 부드럽게 만드는 필터링
#define FILTERING_3ARG 0
#define FILTERING_5ARG 1
#define FILTERING_3weight 2
#define FILTERING_5weight 3
#define FILTERING_Gaussian 4 

//Filtering - 영상 날카롭게 만들기
#define FILTERING_unsharp 5
#define FILTERING_HIGHBOOST 6


#define d_max(x,y) ((x)>(y) ? x : y)
#define d_min(x,y) ((x)<(y) ? x : y)
#define d_min3(x,y,z) (d_min(d_min((x),(y)), (z)))
#define d_max3(x,y,z) (d_max(d_max((x),(y)), (z)))



#define min(a,b)    (((a) < (b)) ? (a) : (b))
//
//extern "C" MYDLLTYPE double max(double a, double b);
extern "C" MYDLLTYPE double limit(double num);


extern "C" MYDLLTYPE IMAGE_D* SOCV_make_Image(int width, int height, int bpp);

extern "C" MYDLLTYPE void SOCV_release_RECT(RECT_D *rect);
extern "C" MYDLLTYPE void SOCV_release_Image(IMAGE_D *img);

extern "C" MYDLLTYPE void SOCV_set_ROI(IMAGE_D *img,RECT_D* rect);

//imgtrans
extern "C" MYDLLTYPE IMAGE_D* SOCV_Gray_24to8(IMAGE_D *img);

extern "C" MYDLLTYPE BOOL SOCV_Edge_Image(IMAGE_D *img, int mode);
extern "C" MYDLLTYPE IMAGE_D* SOCV_BinaryImage(IMAGE_D *img, int threshold);

extern "C" MYDLLTYPE void SOCV_swap(int *a, int *b);
extern "C" MYDLLTYPE void SOCV_morphologh_bin_Erosion(IMAGE_D *img);
extern "C" MYDLLTYPE void SOCV_morphologh_bin_Dilation(IMAGE_D *img);
extern "C" MYDLLTYPE RECT_D* SOCV_make_ROI(int top, int bottom, int left, int right);
extern "C" MYDLLTYPE IMAGE_D* SOCV_Filtering(IMAGE_D *img, int mode, double Gausiansigma);
extern "C" MYDLLTYPE IMAGE_D* SOCV_Edge_canny(IMAGE_D *img);

extern "C" MYDLLTYPE void linetracking(IMAGE_D *img);
extern "C" MYDLLTYPE RECT_D* SOCV_Fchang_G(IMAGE_D *img);
extern "C" MYDLLTYPE RECT_D* Fchang_G(IMAGE_D *img);

//imageio 
extern "C" MYDLLTYPE IMAGE_D* SOCV_Load_Image(char *file_Name);
extern "C" MYDLLTYPE void SOCV_save_Bitmap(char *file_name,IMAGE_D *img);

/*
 * [함수명]
 *		SOCV_draw_line
 * [함수 설명]
 *		가로, 세로, 대각선을 그립니다. 
 * [파라미터]
 *		img	: 24비트 bitmap이미지 
 *      p1, p2 : p1 부터 p2 까지 직선을 그립니다.
 *
 * [리턴 타입]
 *		IMAGE_D* 타입의 24비트 트루컬러 이미지 구조체.
 *
 * [사용 예시]
 *		img = SOCV_draw_line(img, p1, p2 , color);
 * [추가할내용]
 *      8비트에서의 그리기
 */
extern "C" MYDLLTYPE IMAGE_D* SOCV_draw_line(IMAGE_D *img, POINT_D p1, POINT_D p2, Color_D color);
/*
 * [함수명]
 *		SOCV_img_Draw
 * [함수 설명]
 *		사각형을 그립니다. 
 * [파라미터]
 *		img : 24비트 혹은 8비트 이미지
 *		rect : 포인터형 rect
 *		color : 24비트 혹은 8비트
 *		thichness : 두께 . 바깥쪽으로 확장함
 * [리턴 타입]
 *		IMAGE_D* 타입의 24비트 혹은 8비트 이미지 
 *
 * [사용 예시]
 *		img = SOCV_img_Draw(img, rect, Color, 3);
 * [추가할 내용]
 *      여러개의 rect그리기 
 */

extern "C" MYDLLTYPE IMAGE_D* SOCV_img_Draw(IMAGE_D* img, RECT_D* rect, Color_D Color, int thickness);
/*
 * [함수명]
 *		BOOL _rect_check
 * [함수 설명]
 *		SOCV_img_Draw 내부에서 이미지 범위안의 도형인지 판단하는 함수. 
 * [파라미터]
 *		w = width
 *		h = height
 *		rect = SOCV안에서 받아온 rect 
 * [리턴 타입]
 *		TRUE OR FALSE 
 *
 * [사용 예시]
 *		_rect_check(w,h,rect);

 */
extern "C" MYDLLTYPE BOOL _rect_check(int w , int h ,RECT_D* rect);
extern "C" MYDLLTYPE IMAGE_D* SOCV_basic_hough(IMAGE_D* img);
extern "C" MYDLLTYPE void SOCV_save_raw(IMAGE_D *img);
extern "C" MYDLLTYPE int SOCV_OTS_binary(IMAGE_D* img);

extern "C" MYDLLTYPE IMAGE_D* SOCV_gy_algorithm(IMAGE_D *img);
/*
*  [리턴값] 
*		임계값
*/

/*********************** HOG TEST **********************************/

//extern "C" MYDLLTYPE void SOCV_Normalization_image(IMAGE_D *img);

extern "C" MYDLLTYPE HSV_COLOR_D dinocv_conv_rgb2hsv(RGB_COLOR_D rgb);
extern "C" MYDLLTYPE void SOCV_NormalizationRGB_image(IMAGE_D *img);

/*********************** Soc robotWar *****************************/
IMAGE_D *SOCV_int_hue(IMAGE_D *_24img);
IMAGE_D* SOCV_block_variance(IMAGE_D * _8hue);
IMAGE_D *SOCV_hue_hough(IMAGE_D* _8hue);
void _houghLine(IMAGE_D *_8img, LineParam line, BYTE c);
void _houghDrawline(IMAGE_D * _img, int x1, int x2, int y1, int y2, Color_D *color, int color_select);
BOOL _8x8filtering(IMAGE_D * _8img);
IMAGE_D *draw_hue(IMAGE_D * _8img);
PeakPoint* find4PeakPoint(IMAGE_D* _houghed_img);
PeakPoint* SearchPeakPoint(PeakPoint *peak, int length);
POINT_D find_vertices(PeakPoint *peak, TrackingInfo *trackinginfo,IMAGE_D *img);
BOOL Tracking_info(TrackingInfo *trackingInfo, IMAGE_D *_img);
void Tracking(TrackingInfo *trackinginfo, IMAGE_D *resize);
void middle_view(IMAGE_D *_img, ColorObjectInfo *cbj, Color_D *color, int Color_select);
TrackingInfo* make_trackingInfo(TrackingInfo* trackinginfo);
BOOL checkDupilcation(TrackingInfo *trackinginfo, POINT_D *midpoint);
void SOCV_GammaCorrection(IMAGE_D *img, double gamma);
void dinocv_canny_edge(IMAGE_D *image, int th_high, int th_low);
BOOL object_distinguish(TrackingInfo *trackinginfo, IMAGE_D *img, Color_D *color, ObjectInfo* obj);
ColorObjectInfo* make_ColorObjectInfo(ColorObjectInfo* objectInfo);

/*********************** 자료구조 *****************************/
void BubbleSort(PeakPoint *peak, int length);
NODE *insert_node(NODE *plist, NODE *pprev, POINT_D point, int color_select);
NODE *delete_node(NODE *plist, NODE *pprev, NODE *pcurr);
void print_list(NODE *plist);
void destroy_list(NODE *plist);
BOOL _check_linkdupilcation(NODE *plist, POINT_D point);
void judge_rectangle(TrackingInfo *trackinginfo);
void dinocv_draw_line(IMAGE_D *img, POINT_D p1, POINT_D p2, Color_D *clr, int color_select);

#endif