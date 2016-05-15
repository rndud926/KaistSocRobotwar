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

//EDGE - �ܰ��� ����
#define EDGE_ROBERTS 0
#define EDGE_PREWITT 1
#define EDGE_SOBEL 2
//Filtering - ������ �ε巴�� ����� ���͸�
#define FILTERING_3ARG 0
#define FILTERING_5ARG 1
#define FILTERING_3weight 2
#define FILTERING_5weight 3
#define FILTERING_Gaussian 4 

//Filtering - ���� ��ī�Ӱ� �����
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
 * [�Լ���]
 *		SOCV_draw_line
 * [�Լ� ����]
 *		����, ����, �밢���� �׸��ϴ�. 
 * [�Ķ����]
 *		img	: 24��Ʈ bitmap�̹��� 
 *      p1, p2 : p1 ���� p2 ���� ������ �׸��ϴ�.
 *
 * [���� Ÿ��]
 *		IMAGE_D* Ÿ���� 24��Ʈ Ʈ���÷� �̹��� ����ü.
 *
 * [��� ����]
 *		img = SOCV_draw_line(img, p1, p2 , color);
 * [�߰��ҳ���]
 *      8��Ʈ������ �׸���
 */
extern "C" MYDLLTYPE IMAGE_D* SOCV_draw_line(IMAGE_D *img, POINT_D p1, POINT_D p2, Color_D color);
/*
 * [�Լ���]
 *		SOCV_img_Draw
 * [�Լ� ����]
 *		�簢���� �׸��ϴ�. 
 * [�Ķ����]
 *		img : 24��Ʈ Ȥ�� 8��Ʈ �̹���
 *		rect : �������� rect
 *		color : 24��Ʈ Ȥ�� 8��Ʈ
 *		thichness : �β� . �ٱ������� Ȯ����
 * [���� Ÿ��]
 *		IMAGE_D* Ÿ���� 24��Ʈ Ȥ�� 8��Ʈ �̹��� 
 *
 * [��� ����]
 *		img = SOCV_img_Draw(img, rect, Color, 3);
 * [�߰��� ����]
 *      �������� rect�׸��� 
 */

extern "C" MYDLLTYPE IMAGE_D* SOCV_img_Draw(IMAGE_D* img, RECT_D* rect, Color_D Color, int thickness);
/*
 * [�Լ���]
 *		BOOL _rect_check
 * [�Լ� ����]
 *		SOCV_img_Draw ���ο��� �̹��� �������� �������� �Ǵ��ϴ� �Լ�. 
 * [�Ķ����]
 *		w = width
 *		h = height
 *		rect = SOCV�ȿ��� �޾ƿ� rect 
 * [���� Ÿ��]
 *		TRUE OR FALSE 
 *
 * [��� ����]
 *		_rect_check(w,h,rect);

 */
extern "C" MYDLLTYPE BOOL _rect_check(int w , int h ,RECT_D* rect);
extern "C" MYDLLTYPE IMAGE_D* SOCV_basic_hough(IMAGE_D* img);
extern "C" MYDLLTYPE void SOCV_save_raw(IMAGE_D *img);
extern "C" MYDLLTYPE int SOCV_OTS_binary(IMAGE_D* img);

extern "C" MYDLLTYPE IMAGE_D* SOCV_gy_algorithm(IMAGE_D *img);
/*
*  [���ϰ�] 
*		�Ӱ谪
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

/*********************** �ڷᱸ�� *****************************/
void BubbleSort(PeakPoint *peak, int length);
NODE *insert_node(NODE *plist, NODE *pprev, POINT_D point, int color_select);
NODE *delete_node(NODE *plist, NODE *pprev, NODE *pcurr);
void print_list(NODE *plist);
void destroy_list(NODE *plist);
BOOL _check_linkdupilcation(NODE *plist, POINT_D point);
void judge_rectangle(TrackingInfo *trackinginfo);
void dinocv_draw_line(IMAGE_D *img, POINT_D p1, POINT_D p2, Color_D *clr, int color_select);

#endif