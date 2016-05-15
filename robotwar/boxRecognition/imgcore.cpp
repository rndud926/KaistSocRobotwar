#define DLLEXPORT
#include "imgcore.h"

double max(double a, double b)
{
	double num;
	if(a>b)
		num = a;
	else
		num = b;
	return num;
}
double limit(double num){
	if( num > 255)
		num = 255.0;
	else if(num < 0.0 )
		num = 0.0;
	return num;
}
void SOCV_swap(int *a, int *b){
	int temp;
	temp = *a;
	*a = *b;
	*b = temp;
}




IMAGE_D* SOCV_make_Image(int width, int height, int bpp){
	IMAGE_D *img = (IMAGE_D *)malloc(sizeof(IMAGE_D));
	img->height = height;
	img->width = width;
	img->bpp = bpp;
	img->roi = NULL;
	BYTE *arr;

	arr = (BYTE *)malloc(sizeof(BYTE)*width*height*(bpp/8));
	img->source = (BYTE **)malloc(sizeof(BYTE *)*height);
	for(int i = 0 ; i < height ; i++)
	{
		img->source[i] = arr + i*img->width*(bpp/8);
	}
	memset(arr,0,sizeof(BYTE)*width*height*(bpp/8));

	return img;
}


void SOCV_release_Image(IMAGE_D *img){
	if(img->roi != NULL)
	{
		free(img->roi);
	}
	free(img->source[0]);
	free(img->source);
	free(img);
}





void SOCV_set_ROI(IMAGE_D *img,RECT_D *rect)
{
	printf("%d %d %d %d\n",rect->bottom, rect->top, rect->right , rect->left);
	if( rect->top >= 0 || rect->bottom >= 0 ||rect->right < img->width || rect->left < img->height)
	{
		img->roi = (RECT_D *)malloc(sizeof(RECT_D));
		img->roi->bottom = rect->bottom;
		img->roi->left = rect->left;
		img->roi->right = rect->right;
		img->roi->top = rect->top;
	}
	SOCV_release_RECT(rect);
}

void SOCV_release_ROI(IMAGE_D *img)
{
	free(img->roi);
}

IMAGE_D* SOCV_Gray_24to8(IMAGE_D *img){
	int width = img->width;
	int height = img->height;
	
	
	IMAGE_D *tmp = SOCV_make_Image(width, height, 8);

	for(int i = 0 ; i < height ; i++)
	{
		for(int j = 0 ; j < width ; j++)
		{
			tmp->source[i][j] = (img->source[i][j*3]+ img->source[i][j*3+1]+ img->source[i][j*3+2])/3;
			//fprintf(stderr, "[%d %d %d %d]\n", img->source[i][j*3], img->source[i][j*3+1], img->source[i][j*3+2], tmp->source[i][j]);
		}
	}

	//SOCV_save_Bitmap("TEST_GRAY.bmp",tmp);
	return tmp;

}

BOOL SOCV_Edge_Image(IMAGE_D *img, int mode)
{
	int w = img->width;
	int h = img->height;
	int h1, h2;
	double hval;

	IMAGE_D* Copy = SOCV_make_Image(img->width , img->height , 8);


	for(int i = 0 ; i < h ; i++)
	{
		for(int j = 0 ; j < w ; j++)
		{
			Copy->source[i][j] = img->source[i][j];
		}
	}

	if( mode == EDGE_ROBERTS)
	{
		for(int i = 1 ; i < h-1 ; i++)
		{
			for(int j = 1 ; j <w-1 ; j++)
			{
				h1 = Copy->source[i][j] - Copy->source[i-1][j-1];
				h2 = Copy->source[i][j] - Copy->source[i-1][j-1];

				hval = sqrt( (double)h1*h1 + h2*h2 );

				img->source[i][j] = (BYTE)limit(hval);
			}
		}


	}

	else if( mode == EDGE_PREWITT)
	{
		for(int i = 1 ; i < h-1 ; i++)
		{
			for(int j = 1 ; j <w-1 ; j++)
			{
				h1 = -Copy->source[i-1][j-1] - Copy->source[i-1][j] -Copy->source[i-1][j+1]
				+ Copy->source[i+1][j-1] + Copy->source[i+1][j] +Copy->source[i+1][j+1];

				h2 = -Copy->source[i-1][j-1] - Copy->source[i][j-1] -Copy->source[i+1][j-1]
				+ Copy->source[i-1][j+1] + Copy->source[i][j+1] + Copy->source[i+1][j+1];

				hval = sqrt( (double)h1*h1 + h2*h2 );

				img->source[i][j] = (BYTE)limit(hval);
			}
		}

	}
	else if( mode == EDGE_SOBEL)
	{
		for(int i = 1 ; i < h-1 ; i++)
		{
			for(int j = 1 ; j <w-1 ; j++)
			{
				h1 = -Copy->source[i-1][j-1] - 2*Copy->source[i-1][j] -Copy->source[i-1][j+1]
				+ Copy->source[i+1][j-1] + 2*Copy->source[i+1][j] +Copy->source[i+1][j+1];

				h2 = -Copy->source[i-1][j-1] - 2*Copy->source[i][j-1] -Copy->source[i+1][j-1]
				+ Copy->source[i-1][j+1] + 2*Copy->source[i][j+1] + Copy->source[i+1][j+1];

				hval = sqrt( (double)h1*h1 + h2*h2 );

				img->source[i][j] = (BYTE)limit(hval);
			}
		}

	}
	//SOCV_save_Bitmap("TEST_EDGE.bmp",img);
	SOCV_release_Image(Copy);
	return TRUE;

}

LineParam SOCV_HoughLine(IMAGE_D *img){
	int w = img->width;
	int h = img->height;

	IMAGE_D * Copy = SOCV_make_Image(img->width , img->height , 8);

	for(int i = 0 ; i < h ; i++)
	{
		for(int j = 0 ; j < w ; j++)
		{
			Copy->source[i][j] = img->source[i][j];
		}
	}

	int num_rho = (int)(sqrt( (double)w*w + h*h) *2);
	int num_ang = 360;

	//룩업테이블 만들기

	double *tsin = (double *)malloc(sizeof(double)*num_ang);
	double *tcos = (double *)malloc(sizeof(double)*num_ang);

	for(int i = 0 ; i < num_ang ; i++)
	{
		tsin[i] = (double)sin(i*PI/num_ang);
		tcos[i] = (double)cos(i*PI/num_ang);
	}

	//축적배열 생성
	int **arr = (int **)malloc(sizeof(int *)*num_rho);
	int *arry = (int *)malloc(sizeof(int)*num_rho*num_ang);
	memset(arry,0,sizeof(arry)*num_rho*num_ang);

	printf("%dx%d\n",num_rho,num_ang);
	for(int i = 0 ; i < num_rho ; i++)
	{
		arr[i] = arry + i*num_ang;
	}

	/*
	for(int i = 0 ; i < img->height ; i++)
	{
	for(int j = 0 ; j < img->width ; j++)
	{
	arr[i][j]++;
	printf("%d\n",arr[i][j]);
	}
	}
	*/

	int m,n;
	for( int i = 0 ; i < h ; i++)
	{
		for( int j = 0 ; j < w; j++)
		{
			if(img->source[i][j] > 128)
			{
				for( n = 0 ; n < num_ang ; n++)
				{
					m = (int)floor( j * tsin[n] + i * tcos[n] + 0.5);
					m += (num_rho/2);
					//printf("%d %d\n",m,n);
					//printf("%d\n",arr[m][n]);
					arr[m][n]++; 
					//printf("%d\n",arr[m][n]);
				}
			}
		}
	}

	//축적 배열에서 최대값 찾기

	LineParam line;
	line.rho = line.ang  = 0 ; 

	int arr_max = 0 ; 
	for( m = 0 ; m < num_rho ; m++)
	{
		for( n = 0 ; n < num_ang ; n++)
		{
			line.rho = m - (num_rho /2 ) ;
			line.ang = n*180.0/num_ang;
		}
	}



	free(arry);
	free(arr);
	free(tsin);
	free(tcos);
	SOCV_release_Image(Copy);

	return line;
}
void SOCV_DrawLine_LineParam(IMAGE_D *img, LineParam line, BYTE c){
	int x, y;
	int w = img->width;
	int h = img->height;

	IMAGE_D* Copy = SOCV_make_Image(w,h,8);

	/*
	for(int i = 0 ; i < img->height ; i++)
	{
	for(int j = 0 ; j < img->width ; j++)
	{
	Copy->source[i][j] = img->source[i][j];
	}
	}
	*/
	// 수직선인 경우

	if( line.ang == 90)
	{
		x = (int)(line.rho + 0.5);

		for( y= 0 ; y <h ; y++)
			Copy->source[y][x] =c ;

		return ;
	}

	// (rho, ang) 파라미터를 이용하여 직선의 시작 좌표와 끝 좌표를 계산


	int x1 = 0;
	int y1 = (int)floor(line.rho / cos(line.ang*PI/180) + 0.5);
	int x2 = img->width - 1;
	int y2 = (int)floor((line.rho - x2*sin(line.ang*PI/180)) / cos(line.ang*PI/180) + 0.5);

	printf("(%d,%d), (%d,%d)\n",x1,y1,x2,y2);
	//SOCV_DrawLine(img,x1,y1,x2,y2,c);

	SOCV_release_Image(Copy);
}

void SOCV_DrawLine(IMAGE_D* img, int x1, int y1, int x2, int y2, BYTE c)
{
	int x,y;
	double m;

	int w = img->width ;
	int h = img->height;

	IMAGE_D* Copy = SOCV_make_Image(w,h,8);

	//수직선인 경우

	if( x1 == x2)
	{
		if( y1 > y2)
			SOCV_swap(&y1,&y2);

		for( y = y1; y < y2 ; y++)
			Copy->source[y][x1] =c;

		return ;
	}

	// x1,y1 에서 x2,y2까지 직선 그리기

	m = (double)(y2-y1)/(x2-x1);
	if( (m > -1) && ( x2- x1))
	{
		if( x1 > x2)
		{
			SOCV_swap(&x1,&x2);
			SOCV_swap(&y1,&y2);
		}

		for( x = x1 ; x <= x2 ; x++)
		{
			y = (int)floor(m * (x - x1) + y1 + 0.5);
			if( y >= 0 && y<h )
				Copy->source[y][x] = c;
		}

	}
	else 
	{
		if( y1>y2)
		{
			SOCV_swap(&x1,&x2);
			SOCV_swap(&y1,&y2);
		}
		for( y = y1 ; y <= y2; y++)
		{
			x = (int)floor( (y-y1) / m + x1 + 0.5);
			if( y >= 0 && y < h)
				Copy->source[y][x] = c;
		}
	}


	SOCV_release_Image(Copy);
}

IMAGE_D* SOCV_BinaryImage(IMAGE_D *img, int threshold)
{

	int right = img->width;
	int left = img->height;
	int rowAverage=0,rowcount=0;
	int highAverage=0,highcount=0;
	int top=0, bottom=0;
	int counter=0;
	int afterThreshold=-1;
	int basicthreshodl = 100;

	if( img->roi != NULL)
	{
		top = img->roi->top;
		bottom = img->roi->bottom;
		left = img->roi->left;
		right = img->roi->right;
	}

	if( threshold == NULL)
	{
		while(1)
		{
			for(int i=top ; i<left ; i++)
			{
				for(int j = bottom ; j<right ; j++)
				{
					//나누기
					if(img->source[i][j]>=basicthreshodl)
					{
						rowAverage += img->source[i][j];
						//printf("rowAverage : [%d][%d]%d\n",i,j,rowAverage);
						rowcount++;
					}
					else 
					{
						highAverage += img->source[i][j];
						//printf("highAverage : [%d][%d]%d\n",i,j,highAverage);
						highcount++;
					}
				}
			}

			rowAverage /= rowcount;
			highAverage /= highcount;
			basicthreshodl = (rowAverage+highAverage)/2;
			printf("threshold : %d \n",basicthreshodl);
			if(counter==10)
				break;
			counter++;
			highAverage=0;
			rowAverage=0;
			rowcount=0;
			highcount=0;

			if(afterThreshold == basicthreshodl) break;
			else afterThreshold = basicthreshodl;
		}

		//이진화
		for(int i=top;i<left;i++)
		{
			for(int j=bottom;j<right ; j++)
			{
				img->source[i][j] = ((img->source[i][j] > basicthreshodl) ? 255 : 0);
			}		
		}
		//SOCV_save_Bitmap("TEST_BINARY_basic.bmp",img);
	}

	if(threshold !=NULL)
	{
		//이진화
		for(int i=top;i<left;i++)
		{
			for(int j=bottom;j<right ; j++)
			{
				img->source[i][j] = ((img->source[i][j] > threshold) ? 255 : 0);
			}		
		}
		//SOCV_save_Bitmap("TEST_BINARY_OTSU.bmp",img);
	}
	return img;

}
void SOCV_morphologh_bin_Erosion(IMAGE_D *img)
{
	int i , j;
	int w = img->width;
	int h = img->height;

	IMAGE_D *tmp = SOCV_make_Image(w,h,8);
	//IMAGE_D *returnTmp = SOCV_make_Image(w,h,8);
	memcpy(tmp->source[0],img->source[0],sizeof(BYTE)*img->width*img->height);


	for( i = 1 ; i < h - 1 ; i++)
	{
		for( j = 1 ; j < w - 1 ; j++)
		{

			if(tmp->source[i][j] != 0)
			{
				if( tmp->source[i-1][j-1] == 0 || 
					tmp->source[i-1][j  ] == 0 ||
					tmp->source[i-1][j+1] == 0 ||
					tmp->source[i  ][j-1] == 0 ||
					tmp->source[i  ][j+1] == 0 ||
					tmp->source[i+1][j-1] == 0 ||
					tmp->source[i+1][j  ] == 0 ||
					tmp->source[i+1][j+1] == 0)
				{
					img->source[i][j] = 0;
				}
			}
		}
	}

	for( i = 0 ; i < img->height ; i++)
	{
		memcpy(img->source[i],tmp->source[i],sizeof(img->width));
	}

	SOCV_release_Image(tmp);

}

void SOCV_morphologh_bin_Dilation(IMAGE_D *img)
{
	int i , j;
	int w = img->width;
	int h = img->height;

	IMAGE_D *tmp = SOCV_make_Image(w,h,8);
	//IMAGE_D *returnTmp = SOCV_make_Image(w,h,8);
	memcpy(tmp->source[0],img->source[0],sizeof(BYTE)*img->width*img->height);


	for( i = 1 ; i < h - 1 ; i++)
	{
		for( j = 1 ; j < w - 1 ; j++)
		{

			if(tmp->source[i][j] != 0)
			{
				if( tmp->source[i-1][j-1] == 0 || 
					tmp->source[i-1][j  ] == 0 ||
					tmp->source[i-1][j+1] == 0 ||
					tmp->source[i  ][j-1] == 0 ||
					tmp->source[i  ][j+1] == 0 ||
					tmp->source[i+1][j-1] == 0 ||
					tmp->source[i+1][j  ] == 0 ||
					tmp->source[i+1][j+1] == 0)
				{
					img->source[i][j] = 255;
				}
			}
		}
	}

	for( i = 0 ; i < img->height ; i++)
	{
		memcpy(img->source[i],tmp->source[i],sizeof(img->width));
	}

	SOCV_release_Image(tmp);

}

RECT_D* SOCV_make_ROI(int top, int bottom, int left, int right){
	RECT_D* rect = (RECT_D *)malloc(sizeof(RECT_D));
	rect->top = top;
	rect->bottom =bottom;
	rect->right = right;
	rect->left = left;

	return rect;
}
void SOCV_release_RECT(RECT_D *rect)
{
	free(rect);
}
IMAGE_D* SOCV_Filtering(IMAGE_D *img, int mode, double Gausiansigma)
{
	int i, j;
	int w = img->width;
	int h = img->height;
	int temp;

	IMAGE_D* tmp1 =	SOCV_make_Image(w,h,8);
	memcpy(tmp1->source[0],img->source[0],sizeof(BYTE)*w*h);


	if( mode == FILTERING_3ARG)
	{
		IMAGE_D* tmp1 =	SOCV_make_Image(w,h,8);
		memcpy(tmp1->source[0],img->source[0],sizeof(BYTE)*w*h);
		IMAGE_D* tmp2 = SOCV_make_Image(w,h,8);
		memcpy(tmp2->source[0],img->source[0],sizeof(BYTE)*w*h);

		for(i = 2 ; i<h-2 ; i++)
		{
			for(j = 2 ; j<w-2 ; j++)
			{
				temp = tmp1->source[i-1][j-1] + tmp1->source[i-1][j] + tmp1->source[i-1][j-1] + 
					tmp1->source[i][j-1] + tmp1->source[i][j] + tmp1->source[i][j+1] +
					tmp1->source[i+1][j-1] + tmp1->source[i+1][j+1] + tmp1->source[i+1][j+1];
				tmp2->source[i][j] = (BYTE)limit(temp/9. + 0.5);		

			}
		}

	}
	else if( mode == FILTERING_5ARG)
	{
		IMAGE_D* tmp1 =	SOCV_make_Image(w,h,8);
		memcpy(tmp1->source[0],img->source[0],sizeof(BYTE)*w*h);
		IMAGE_D* tmp2 = SOCV_make_Image(w,h,8);
		memcpy(tmp2->source[0],img->source[0],sizeof(BYTE)*w*h);

		for(i = 2 ; i<h-2 ; i++)
		{
			for(j = 2 ; j<w-2 ; j++)
			{
				temp = tmp1->source[i-2][j-2] + tmp1->source[i-2][j-1] + tmp1->source[i-2][j] + tmp1->source[i-2][j+1] + tmp1->source[i-2][j+2] +
					tmp1->source[i-1][j-2] + tmp1->source[i-1][j-1] + tmp1->source[i-1][j] + tmp1->source[i-1][j+1] + tmp1->source[i-1][j+2] +
					tmp1->source[i][j-2] + tmp1->source[i][j-1] + tmp1->source[i][j] + tmp1->source[i][j+1] + tmp1->source[i][j+2] +
					tmp1->source[i+1][j-2] + tmp1->source[i+1][j-1] + tmp1->source[i+1][j] + tmp1->source[i+1][j+1] + tmp1->source[i+1][j+2] +
					tmp1->source[i+2][j-2] + tmp1->source[i+2][j-1] + tmp1->source[i+2][j] + tmp1->source[i+2][j+1] + tmp1->source[i+2][j+2];

				tmp2->source[i][j] = (BYTE)limit(temp/25. + 0.5);		
			}
		}
	}
	else if( mode == FILTERING_3weight)
	{
		IMAGE_D* tmp1 =	SOCV_make_Image(w,h,8);
		memcpy(tmp1->source[0],img->source[0],sizeof(BYTE)*w*h);
		IMAGE_D* tmp2 = SOCV_make_Image(w,h,8);
		memcpy(tmp2->source[0],img->source[0],sizeof(BYTE)*w*h);

		for(i = 1 ; i<h-1; i++)
		{
			for(j = 1 ; j<w-1 ; j++)
			{
				temp = tmp1->source[i-1][j-1] + 2*tmp1->source[i-1][j] + tmp1->source[i-1][j-1] + 
					2*tmp1->source[i][j-1] + 4*tmp1->source[i][j] + 2*tmp1->source[i][j+1] +
					tmp1->source[i+1][j-1] + 2*tmp1->source[i+1][j+1] + tmp1->source[i+1][j+1];
				tmp2->source[i][j] = (BYTE)limit(temp/16. + 0.5);	
			}
		}

	}
	else if( mode == FILTERING_5weight)
	{
		IMAGE_D* tmp1 =	SOCV_make_Image(w,h,8);
		memcpy(tmp1->source[0],img->source[0],sizeof(BYTE)*w*h);
		IMAGE_D* tmp2 = SOCV_make_Image(w,h,8);
		memcpy(tmp2->source[0],img->source[0],sizeof(BYTE)*w*h);

		for(i = 2 ; i<h-2 ; i++)
		{
			for(j = 2 ; j<w-2 ; j++)
			{
				temp = tmp1->source[i-2][j-2] + 4*tmp1->source[i-2][j-1] +6*tmp1->source[i-2][j] + 4*tmp1->source[i-2][j+1] + tmp1->source[i-2][j+2] +
					4*tmp1->source[i-1][j-2] + 16*tmp1->source[i-1][j-1] + 24*tmp1->source[i-1][j] + 16*tmp1->source[i-1][j+1] + 4*tmp1->source[i-1][j+2] +
					6*tmp1->source[i][j-2] + 24*tmp1->source[i][j-1] + 36*tmp1->source[i][j] + 24*tmp1->source[i][j+1] + 6*tmp1->source[i][j+2] +
					4*tmp1->source[i+1][j-2] + 16*tmp1->source[i+1][j-1] + 24*tmp1->source[i+1][j] + 16*tmp1->source[i+1][j+1] + 4*tmp1->source[i+1][j+2] +
					tmp1->source[i+2][j-2] + 4*tmp1->source[i+2][j-1] + 6*tmp1->source[i+2][j] + 4*tmp1->source[i+2][j+1] + tmp1->source[i+2][j+2];

				tmp2->source[i][j] = (BYTE)limit(temp/256. + 0.5);		

			}
		}
	}
	else if( mode == FILTERING_Gaussian)
	{
		int i, j, k, x;

		int w = img->width;
		int h = img->height;

		IMAGE_D * tmp = SOCV_make_Image(w,h,8);
		memcpy(tmp->source[0],img->source[0],sizeof(BYTE)*w*h);

		//////////////////////////////////////////////////////////////////////////
		// 1차원 가우시안 마스크 생성
		//////////////////////////////////////////////////////////////////////////

		int dim = (int)max(3.0, 2*4*Gausiansigma + 1.0); // 3x3 or sigma값에 따른 마스크크기
		if (dim % 2 == 0) dim++; // 1차원 가우시안 마스크를 홀수 개로 만든다.
		int dim2 = (int)dim/2;

		double* pMask = new double[dim];
		for (i=0; i<dim; i++)
		{
			x = i - dim2;
			pMask[i] = exp(-(x*x)/(2*Gausiansigma*Gausiansigma)) / (sqrt(2*PI)*Gausiansigma);
		}

		//////////////////////////////////////////////////////////////////////////
		// 임시 버퍼 메모리 공간 할당
		//////////////////////////////////////////////////////////////////////////

		double** buf = new double*[h];
		for (i=0; i<h; i++)
		{
			buf[i] = new double[w];
			memset(buf[i], 0, sizeof(double)*w);
		}

		//////////////////////////////////////////////////////////////////////////
		// 세로 방향 컨벌루션
		//////////////////////////////////////////////////////////////////////////

		double sum1, sum2;

		for (i=0; i<w; i++)
			for (j=0; j<h; j++)
			{
				sum1 = sum2 = 0.0;

				for (k=0; k<dim; k++)
				{
					x = k - dim2 + j;

					if (x >= 0 && x < h)
					{
						sum1 += pMask[k];
						sum2 += (pMask[k] * tmp->source[x][i]);
					}
				}

				buf[j][i] = (sum2/sum1);
			}

			//////////////////////////////////////////////////////////////////////////
			// 가로 방향 컨벌루션
			//////////////////////////////////////////////////////////////////////////

			for (j=0; j<h; j++)
				for (i=0; i<w; i++)
				{
					sum1 = sum2 = 0.0;

					for (k=0; k<dim; k++)
					{
						x = k - dim2 + i;

						if (x >= 0 && x < w)
						{
							sum1 += pMask[k];
							sum2 += (pMask[k] * buf[j][x]);
						}
					}

					tmp->source[j][i] = (BYTE)limit(sum2/sum1);
				}
				SOCV_save_Bitmap("TEST_FILTERING.bmp",tmp);
				//////////////////////////////////////////////////////////////////////////
				// 동적 할당했던 메모리 공간 해제
				//////////////////////////////////////////////////////////////////////////

				delete [] pMask;

				for (i=0; i<h; i++)
				{
					delete [] buf[i];
				}

				delete [] buf;

				return tmp;

	}
	else if( mode == FILTERING_unsharp)
	{
		
		for( j = 1 ; j < h-1 ; j++)
		{
			for( i = 1 ; i< w-1 ; i++)
			{
				temp = (BYTE)limit(4*img->source[j][i]
						- img->source[j-1][i] 
						- img->source[j][i-1] 
						- img->source[j+1][i] 
						- img->source[j][i+1]);
				//img->source[j][i] = temp;
				tmp1->source[j][i] = temp;
			}
		}

		//SOCV_release_Image(tmp1);
	}

	return tmp1;
}
IMAGE_D* SOCV_Edge_canny(IMAGE_D *img)
{
	int h = img->height;
	int w = img->width;

	int h1, h2;
	double hval;
	double direction;
	double temp;
	double radian = 180 / PI;

	IMAGE_D* Copy = SOCV_make_Image(w,h,8);
	memcpy(Copy->source[0],img->source[0],sizeof(BYTE)*w*h);

	if(img->bpp == 24)
	{
		img = SOCV_Gray_24to8(img);
	}

	img = SOCV_Filtering(img,FILTERING_Gaussian,1.0);
	//sobel edge
	for(int i = 1 ; i < h-1 ; i++)
	{
		for(int j = 1 ; j <w-1 ; j++)
		{
			h1 = -Copy->source[i-1][j-1] - 2*Copy->source[i-1][j] -Copy->source[i-1][j+1]
			+ Copy->source[i+1][j-1] + 2*Copy->source[i+1][j] +Copy->source[i+1][j+1]; // y 

			h2 = -Copy->source[i-1][j-1] - 2*Copy->source[i][j-1] -Copy->source[i+1][j-1]
			+ Copy->source[i-1][j+1] + 2*Copy->source[i][j+1] + Copy->source[i+1][j+1]; // x 

			hval = abs(h1) + abs(h2); //강도

			img->source[i][j] = (BYTE)limit(hval);
		}
	}

	//Non -maximum suppression 

	for( int i = 1 ; i < h-1 ; i++)
	{
		for( int j = 1 ; j < w-1 ; j++)
		{
			//printf("%d %d \n",i,j);
			h1 = -Copy->source[i-1][j-1] - 2*Copy->source[i-1][j] -Copy->source[i-1][j+1]
			+ Copy->source[i+1][j-1] + 2*Copy->source[i+1][j] +Copy->source[i+1][j+1]; // y 

			h2 = -Copy->source[i-1][j-1] - 2*Copy->source[i][j-1] -Copy->source[i+1][j-1]
			+ Copy->source[i-1][j+1] + 2*Copy->source[i][j+1] + Copy->source[i+1][j+1]; // x 
			if(h2 != 0)
			{
				hval = abs(h1) + abs(h2); //강도
				temp = h1/h2;
				direction = atan(temp) * radian;

				if( 0.0 < direction && direction < 22.5)
				{
					//printf("a");
					if( img->source[i][j] < img->source[i][j-1] || img->source[i][j] < img->source[i][j+1])
						img->source[i][j] = 0;
				}
				else if( 22.5 < direction && direction < 67.5)
				{
					//printf("b");
					if( img->source[i][j] < img->source[i-1][j+1] || img->source[i][j] < img->source[i+1][j-1])
						img->source[i][j] = 0;
				}
				else if( 67.5 < direction && direction < 112.5)
				{
					//printf("c");
					if( img->source[i][j] < img->source[i-1][j-1] || img->source[i][j] < img->source[i+1][j+1])
						img->source[i][j] = 0;
				}
				else if( 157.5 < direction && direction < 180.0)
				{
					//printf("d");
					if( img->source[i][j] < img->source[i][j-1] || img->source[i][j] < img->source[i][j+1])
						img->source[i][j] = 0;
				}
			}
		}
	}

	return img;
}



RECT_D* Fchang_G(IMAGE_D *img)
{
	printf("img->bpp: %d\n",img->bpp);
	IMAGE_D* tmp = SOCV_make_Image(img->width, img->height , 8);
	tmp = SOCV_Gray_24to8(img);


	RECT_D* rect=(RECT_D *)malloc(sizeof(RECT_D *));
	printf("전처리끝\n");
	
	int height=img->height;
	int width=img->width;
	int i,j;
	int d; //방향 조절 변수 
	int count; //8방향 획수 샐 변수
	int nx, ny; //다음 픽셀값 저장
	//int xPoint, yPoint; //외곽선 시작 픽셀값 저장
	//int num; //외곽선 픽셀 갯수에 사용될 변수
	int x,y; //처음으로 찾은 픽셀값
	int counter=0;
	int LabelCounter =0 ; 
	int dold; 
	//	int minmaxCounter; //레이블카운터용
	int Xmin=height,Xmax=-1,Ymin=height,Ymax=-1; //창을 만들기위한 x,y 최소값 최대값
	int min,max;
	int minX[10],maxX[10],minY[10],maxY[10]; //저장소
	int savePointX=0,savePointY=0;
	int FrontPoint, EndPoint;
	int dir[8][2]={ //외곽선 경우
		{1,0},
		{1,1},
		{0,1},
		{-1,1},
		{-1,0},
		{-1,-1},
		{0,-1},
		{1,-1}
	};
	ContourPoints cp;
	cp.num  = d = count = 0 ; 
	int ** tmpArray;
	tmpArray = (int **)malloc(sizeof(int *)*height-1);
	tmpArray[0] = (int *)malloc(sizeof(int)*height*width);
	for(i=1;i<height;i++)
		tmpArray[i] = tmpArray[i-1] +width;
	memset(*tmpArray,0,sizeof(int)*height*width);


	//copy pixel
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			tmpArray[i][j] = (int)tmp->source[i][j];
			printf("%4d",tmpArray[i][j]);
		}
		printf("\n");
	}

	for(j=1;j<height;j++)
	{
		for(i=1;i<width;i++)
		{
			if(tmpArray[j][i] == 255 && tmpArray[j][i-1] != 256 && tmpArray[j-1][i] != 256 && tmpArray[j-1][i-1] != 256 ) 
			{
				printf("최초 픽셀 -> tmpArray[%d][%d]\n",j,i);
				y = j;
				x = i;
				dold  = count= d = 0 ;
				LabelCounter++;
				//여기서부터 external contour tracing
				FrontPoint = cp.num;
				while(1)
				{
					nx = x + dir[d][0];
					ny = y + dir[d][1];

					if( nx< 0 || nx>=width || ny<0||ny>=height)
						break;


					if(tmpArray[ny][nx] ==0||tmpArray[ny][nx] ==-1 || tmpArray[ny][nx] == 255)
					{
						//진행 방향에 있는 픽셀이 객체가 아닌경우,
						//시계 방향으로 진행 방향을 바꾸고 다시 시도한다.
						tmpArray[ny][nx] =-1;

						if(++d>7) d =0;
						count++;
						// 8방향 모두 배경(background)인 경우
						if(count >=8)
						{
							printf("8방향\n");
							break;
						}
					}
					else
					{
						//진행 방향의 픽셀이 객체일 경우,
						//현재 점을 외곽선 정보에 저장
						cp.x[cp.num] =x;
						cp.y[cp.num] =y;
						cp.num++;
						//진행 방향으로 이동
						//printf("%d %d \n",x,y);
						/*
						int numX = x;
						int numY = y;

						if( Xmin > x)
						Xmin = x;
						else if ( Xmax < x)
						Xmax = x;

						if( Ymin > y)
						Ymin = y;
						else if ( Ymax < y)
						Ymax = y;
						*/
						tmpArray[ny][nx] = 256;
						//printf(" (%d %d)\n",ny,nx);
						x=nx;
						y=ny;
						//방향정보 초기화

						count = 0;
						dold=d;
						d = (dold +6) % 8; // d = dold -2 와 같은 형태
					}
					// 시작점으로 돌아왔고, 진행 방향이 초기화된 경우 외곽선 추적을 끝낸다.
					if(x==i && y==j && d ==0 )
					{

						EndPoint = cp.num;
						printf("external cp.num -> %d\n",cp.num);
						//(x,y)좌표값계산
						printf("FrontPoint : %d\nEndPoint : %d\n",FrontPoint,EndPoint);
						for(savePointX = FrontPoint ; savePointX<cp.num; savePointX++){
							min = cp.x[savePointX];
							if(Xmin >= min){
								Xmin = min;
							}
							else if(Xmax <= min){
								Xmax = min;
							}
						}
						for (savePointY=FrontPoint ; savePointY<cp.num; savePointY++){
							min = cp.y[savePointY];
							if(Ymin >= min){
								Ymin = min;
							}
							else if(Ymax <= min){
								Ymax = min;
							}
						}
						minX[LabelCounter] = Xmin;
						maxX[LabelCounter] = Xmax;
						minY[LabelCounter] = Ymin;
						maxY[LabelCounter] = Ymax;
						savePointY=savePointX=cp.num;
						//printf("LabelCounter -> %d\n",LabelCounter);
						printf("(%d,%d) - (%d,%d)\n(%d,%d) - (%d,%d) \n",minX[LabelCounter],minY[LabelCounter],maxX[LabelCounter],minY[LabelCounter],minX[LabelCounter],
							maxY[LabelCounter],maxX[LabelCounter],maxY[LabelCounter]);
						rect->left = Xmin;
						rect->top = Ymin;
						rect->right = Xmax;
						rect->bottom = Ymax;
						printf("%d %d %d %d \n",Xmin , Ymin , Xmax, Ymax);
						//초기화
						Xmin=height,Xmax=-1,Ymin=height,Ymax=-1;

						break;
					}

				}
			}

			if(tmpArray[j][i] == 0 && tmpArray[j][i-1] > 0)
			{
				y = j;
				x = i;
				dold  = count= 0;
				d = 3;
				tmpArray[j][i] =-1;
				counter++;
				//여기서 부터 internal contour tracing
				while(1)
				{
					nx = x + dir[d][0];
					ny = y + dir[d][1];

					//범위밖이면 종료 
					if( nx< 0 || nx>=width || ny<0||ny>=height )
						break;

					if(tmpArray[ny][nx] > 0 ){
						//printf(" d : - > %d 서치 : -> tmpArray[%d][%d]\n",d,ny,nx);
						//반시계 방향으로 진행 방향을 바꾸고 다시 시도한다.
						tmpArray[ny][nx] = 256;
						if(d == 0 ){
							//printf("d<=0\n");
							d =7;
						}
						d = d-1;
						count++;
						// 8방향 모두 배경(background)인 경우
						if(count >=8)
							break;


					}
					else
					{
						//진행방향 내부점 찾음
						//진행 방향으로 이동
						tmpArray[ny][nx] = -1;
						x=nx;
						y=ny;
						//방향정보 초기화
						count = 0;
						d += 2 ; 
						if(d >7 ) d = 1;
					}
					//printf("d ->%d\n",d);
					if(x==i && y==j && d==3)
						break;
				}
			}
		}
	}

	//모든 픽셀을 0으로 만들고 외곽선 부분만 픽셀로 남기는 부분.
	/*
	for(int j=0;j<img->height;j++){
	for(int i=0;i<img->width;i++){
	tmpArray[j][i] = 0;
	tmp->source[j][i] = 0;
	}
	}
	//외곽선 추출 부분
	for(int i=0;i<cp.num;i++)
	tmp->source[cp.y[i]][cp.x[i]] = 255;

	SOCV_save_Bitmap("Test_Fchang.bmp",tmp);
	*/

	for(int i = height-1; i > 0; i--)
	{
		for(int j = 0 ; j < width; j++)
		{
			printf(" %4d ",tmpArray[i][j]);
		}
		printf("\n");
	}


	printf("객체 갯수 : %d\n",LabelCounter);
	return rect;
}

IMAGE_D* SOCV_Load_Image(char *file_Name){
	FILE *fp;
	IMAGE_D *img = { 0 };
	BITMAPFILEHEADER_ROBOT bitmapHEADER;
	BITMAPINFOHEADER_ROBOT bitmapINFO;

	if(( fp = fopen(file_Name,"rb"))==NULL){
		fprintf(stderr,"파일 로드 실패 \n");
		exit(1);
	}
	//printf("파일로드성공******************************\n");

	memset(&bitmapHEADER,0,sizeof(bitmapHEADER));
	memset(&bitmapINFO,0,sizeof(bitmapINFO));

	fread(&bitmapHEADER,sizeof(bitmapHEADER),1,fp);
	fread(&bitmapINFO,sizeof(bitmapINFO),1,fp);
	img = (IMAGE_D *)malloc(sizeof(IMAGE_D));


	img->bpp = bitmapINFO.biBitCount;
	img->width  = bitmapINFO.biWidth;
	img->height = bitmapINFO.biHeight;
	img->roi = NULL;

	/*
	printf(" bfSize = %d\n",bitmapHEADER.bfSize);
	printf(" width = %d\n",bitmapINFO.biWidth);

	printf(" bfType = %x\n",bitmapHEADER.bfType); 
	printf(" bfOffBIts = %x\n",bitmapHEADER.bfOffBits);
	printf(" bfReserved1 %x\n",bitmapHEADER.bfReserved1);
	printf(" bfReserved2 %x\n",bitmapHEADER.bfReserved2);
	printf(" bfsizeMAKE = %x\n",img->width * img->height * (img->bpp/8) + 54);
	printf("-----------------------\n");
	printf("bitmapINFO.biSize : %x\n",bitmapINFO.biSize);
	printf("bitmapINFO.biClrImportant : %x\n",bitmapINFO.biClrImportant);
	printf("bitmapINFO.biClrUsed : %x\n",bitmapINFO.biClrUsed);
	printf("bitmapINFO.biCompression : %x\n",bitmapINFO.biCompression);
	printf("bitmapINFO.biPlanes : %x\n",bitmapINFO.biPlanes);
	printf("bitmapINFO.biSizeImage : %x\n",bitmapINFO.biSizeImage);
	printf("bitmapINFO.biXPelsPerMeter : %x\n",bitmapINFO.biXPelsPerMeter);
	printf("bitmapINFO.biYPelsPerMeter : %x\n",bitmapINFO.biYPelsPerMeter);
	printf("bpp : %x\n",img->bpp);
	printf("width : %x\n",img->width);
	printf("height : %x\n",img->height);
	printf("-----------------------\n");
	*/

	BYTE *arr = (BYTE *)malloc(sizeof(BYTE) * img->width *img->height *3);
	fread(arr,sizeof(BYTE),img->width*img->height*3,fp);
	img->source = (BYTE **)malloc(sizeof(BYTE *) *img->height);

	for(int i = 0 ; i < img->height ; i++)
	{
		img->source[i] = arr + i*img->width*3;
	}
	fclose(fp);
	return img;
}

void SOCV_save_Bitmap(char *file_name,IMAGE_D *img)
{
	BITMAPINFOHEADER_ROBOT bitmapINFO;
	BITMAPFILEHEADER_ROBOT bitmapHEADER;
	memset(&bitmapINFO,0,sizeof(bitmapINFO));
	memset(&bitmapHEADER,0,sizeof(BITMAPFILEHEADER_ROBOT));

	if(img->bpp == 24){
		bitmapHEADER.bfSize = img->width * img->height * (img->bpp/8) + 54 + (img->width)%4 * img->width;	
		bitmapHEADER.bfOffBits = 54;
		/*
		printf("img->width : %d \n", img->width);
		printf("img->height : %d \n",img->height);
		printf(" %d \n",((img->width*img->bpp/8 + 3 )&~3));
		printf("bfSize : %d\n",bitmapHEADER.bfSize);
		*/
	}
	else
	{
		bitmapHEADER.bfSize = img->width * img->height * (img->bpp/8) + 54 + 1024 ;	
		bitmapHEADER.bfOffBits = 54 + 1024;
		//printf("bfSize : %x\n",bitmapHEADER.bfSize);
		//printf("bfOffBIts = %x\n",bitmapHEADER.bfOffBits);
	}


	bitmapINFO.biPlanes = 1;
	bitmapINFO.biSize = 40;
	bitmapINFO.biWidth = img->width;
	bitmapINFO.biHeight = img->height;
	bitmapINFO.biBitCount = img->bpp;
	bitmapINFO.biSizeImage = ((img->width*img->bpp/8 + 3 )&~3) * img->height * (img->bpp/8);
	bitmapHEADER.bfType = 19778; //BM 
	
	FILE *fp = fopen(file_name , "wb");

	fwrite(&bitmapHEADER,sizeof(BITMAPFILEHEADER_ROBOT),1,fp);

	fseek(fp,14L,SEEK_SET);
	fwrite(&bitmapINFO,sizeof(bitmapINFO),1,fp);
	if(img->bpp == 8){
		RGB* rgb = (RGB *)malloc(sizeof(RGB) * 256);
		for(int i = 0 ; i<=256 ; i++)
		{
			rgb[i].reserved = 0;
			rgb[i].blue = i;
			rgb[i].green = i;
			rgb[i].red= i;
		}
		fwrite(rgb,sizeof(RGB),256,fp);
		//free(rgb); //이거언제 할당해야되는거지

	}

	fwrite(img->source[0],sizeof(BYTE)*img->height * img->width * (img->bpp/8),1,fp);
	fclose(fp);
}
RECT_D* SOCV_Fchang_G(IMAGE_D *img)
{
	
	printf("img->bpp: %d\n",img->bpp);
	IMAGE_D* tmp = SOCV_make_Image(img->width, img->height , 8);
	tmp = SOCV_Gray_24to8(img);
	printf("전처리끝\n");
	

	int height=img->height;
	int width=img->width;
	int i,j;
	int d; //방향 조절 변수 
	int count; //8방향 획수 샐 변수
	int nx, ny; //다음 픽셀값 저장
	//int xPoint, yPoint; //외곽선 시작 픽셀값 저장
	//	int num; //외곽선 픽셀 갯수에 사용될 변수
	int x,y; //처음으로 찾은 픽셀값
	int counter=0;
	int LabelCounter =0 ; 
	int dold; 
	//	int minmaxCounter; //레이블카운터용
	int Xmin=height,Xmax=-1,Ymin=height,Ymax=-1; //창을 만들기위한 x,y 최소값 최대값
	int min,max;
	int minX[10],maxX[10],minY[10],maxY[10]; //저장소
	int savePointX=0,savePointY=0;
	int FrontPoint, EndPoint;
	int dir[8][2]={ //외곽선 경우
		{1,0},
		{1,1},
		{0,1},
		{-1,1},
		{-1,0},
		{-1,-1},
		{0,-1},
		{1,-1}
	};
	ContourPoints cp;
	cp.num  = d = count = 0 ; 


	int ** tmpArray;
	tmpArray = (int **)malloc(sizeof(int *)*height-1);
	tmpArray[0] = (int *)malloc(sizeof(int)*height*width);
	for(i=1;i<height;i++)
		tmpArray[i] = tmpArray[i-1] +width;
	memset(*tmpArray,0,sizeof(int)*height*width);


	//copy pixel
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			tmpArray[i][j] = (int)tmp->source[i][j];

		}
	}

	for(j=1;j<height;j++)
	{
		for(i=1;i<width;i++)
		{
			if(tmpArray[j][i] == 255 && tmpArray[j][i-1] == 0 && tmpArray[j-1][i] == 0 && tmpArray[j-1][i-1] == 0  ) 
			{
				printf("최초 픽셀 -> tmpArray[%d][%d]\n",j,i);
				//printf("tmpArray[%d][%d] -> %d \ntmpArray[%d][%d] -> %d  \ntmpArray[%d][%d] -> %d  \ntmpArray[%d][%d] -> %d \n", 
				//	j,i,tmpArray[j][i],j,i-1,tmpArray[j][i-1],j-1,i,tmpArray[j-1][i],j-1,i-1,tmpArray[j-1][i-1]);
				y = j;
				x = i;
				dold  = count= d = 0 ;
				LabelCounter++;
				//여기서부터 external contour tracing
				FrontPoint = cp.num;
				while(1)
				{
					nx = x + dir[d][0];
					ny = y + dir[d][1];
					if( nx< 0 || nx>=width || ny<0||ny>=height ||tmpArray[ny][nx] ==0||tmpArray[ny][nx] ==-1)
					{
						//진행 방향에 있는 픽셀이 객체가 아닌경우,
						//시계 방향으로 진행 방향을 바꾸고 다시 시도한다.
						tmpArray[ny][nx] =-1;

						if(++d>7) d =0;
						count++;
						// 8방향 모두 배경(background)인 경우
						if(count >=8)
						{
							cp.x[cp.num] =x;
							cp.y[cp.num] =y;
							cp.num++;
							//printf("8방향 모두 배경! \n");

							break;
						}
					}
					else
					{
						//진행 방향의 픽셀이 객체일 경우,
						//현재 점을 외곽선 정보에 저장
						cp.x[cp.num] =x;
						cp.y[cp.num] =y;
						cp.num++;
						//진행 방향으로 이동
						tmpArray[ny][nx] = 256;
						x=nx;
						y=ny;
						//방향정보 초기화

						count = 0;
						dold=d;
						d = (dold +6) % 8; // d = dold -2 와 같은 형태
					}
					// 시작점으로 돌아왔고, 진행 방향이 초기화된 경우 외곽선 추적을 끝낸다.
					if(x==i && y==j && d ==0 )
					{

						EndPoint = cp.num;
						printf("external cp.num -> %d\n",cp.num);
						//(x,y)좌표값계산
						printf("FrontPoint : %d\nEndPoint : %d\n",FrontPoint,EndPoint);
						for(savePointX = FrontPoint ; savePointX<cp.num; savePointX++){
							min = cp.x[savePointX];
							if(Xmin >= min){
								Xmin = min;
							}
							else if(Xmax <= min){
								Xmax = min;
							}
						}
						for (savePointY=FrontPoint ; savePointY<cp.num; savePointY++){
							min = cp.y[savePointY];
							if(Ymin >= min){
								Ymin = min;
							}
							else if(Ymax <= min){
								Ymax = min;
							}
						}
						minX[LabelCounter] = Xmin;
						maxX[LabelCounter] = Xmax;
						minY[LabelCounter] = Ymin;
						maxY[LabelCounter] = Ymax;
						savePointY=savePointX=cp.num;
						//printf("LabelCounter -> %d\n",LabelCounter);
						printf("(%d,%d) - (%d,%d)\n(%d,%d) - (%d,%d) \n",minX[LabelCounter],minY[LabelCounter],maxX[LabelCounter],minY[LabelCounter],minX[LabelCounter],
							maxY[LabelCounter],maxX[LabelCounter],maxY[LabelCounter]);

						//초기화
						Xmin=height,Xmax=-1,Ymin=height,Ymax=-1;

						break;
					}
				}

			}

			if(tmpArray[j][i] == 0 && tmpArray[j][i-1] > 0)
			{
				//printf("tmpArray[%d][%d]->%d\ntmpArray[%d][%d]->%d\ntmpArray[%d][%d]->%d\n",j,i,tmpArray[j][i],j-1,i,tmpArray[j-1][i],j,i+1,tmpArray[j][i+1]);
				//printf("내부픽셀발견! ->tmpArray[%d][%d]\n",j,i);
				y = j;
				x = i;
				dold  = count= 0;
				d = 3;
				tmpArray[j][i] =-1;
				counter++;
				//여기서 부터 internal contour tracing
				while(1){
					nx = x + dir[d][0];
					ny = y + dir[d][1];

					//범위밖이면 종료 
					if( nx< 0 || nx>=width || ny<0||ny>=height )
						break;

					if(tmpArray[ny][nx] > 0 ){
						//printf(" d : - > %d 서치 : -> tmpArray[%d][%d]\n",d,ny,nx);
						//반시계 방향으로 진행 방향을 바꾸고 다시 시도한다.
						tmpArray[ny][nx] = 256;
						if(d == 0 ){
							//printf("d<=0\n");
							d =7;
						}
						d = d-1;
						count++;
						// 8방향 모두 배경(background)인 경우
						if(count >=8)
						{
							cp.x[cp.num] =nx;
							cp.y[cp.num] =ny;
							cp.num++;
							printf("8방향 모두 배경! -> (%d, %d)\n" , nx,ny);
							break;
						}
						//마킹
						cp.x[cp.num] =nx;
						cp.y[cp.num] =ny;
						cp.num++;

					}
					else{
						//printf(" d : - > %d 서치 : -> tmpArray[%d][%d]\n",d,ny,nx);
						//printf("0을 찾음 \n");
						//진행방향 내부점 찾음
						//진행 방향으로 이동
						tmpArray[ny][nx] = -1;
						x=nx;
						y=ny;
						//방향정보 초기화
						count = 0;
						d += 2 ; 
						if(d >7 ) d = 1;
					}
					//printf("d ->%d\n",d);
					if(x==i && y==j && d==3){
						printf("완료\n");
						printf("internal cp.num -> %d\n",cp.num);
						break;
					}

				}

			}

		}
	}
	/*
	for(j=0;j<height;j++){
	for(i=0;i<width;i++){
	printf("%4d", tmpArray[j][i]);	
	}
	printf("\n");
	}
	*/

	printf("%d\n",cp.num);
	for(j=0;j<height;j++){
		for(i=0;i<width;i++){
			tmpArray[j][i] = 0;
			tmp->source[j][i] = 0;
		}
	}

	printf("그린다\n");
	for(i=0;i<cp.num;i++)
		tmp->source[cp.y[i]][cp.x[i]] = 255;

	for(i=1;i<=LabelCounter;i++){
		for(j = minX[i] ; j<maxX[i] ; j++){
			tmp->source[minY[i]][j] = 255;
			tmp->source[maxY[i]][j] = 255;
		}

		for(int k = minY[i] ; k<maxY[i] ; k++){
			tmp->source[k][minX[i]] = 255;
			tmp->source[k][maxX[i]] = 255;
		}
	}

	RECT_D* rect=(RECT_D *)malloc(sizeof(RECT_D *)*LabelCounter+1);

	for( i=1; i<=LabelCounter ;i++)
	{
		rect[i].left = minX[i];
		rect[i].top = minY[i];
		rect[i].right = maxX[i];
		rect[i].bottom = maxY[i];
		printf("%d %d %d %d \n",minX[i] , minY[i] , minX[i], maxY[i]);

	}
	printf("객체 갯수 - > : %d\n",LabelCounter);

	return rect;
}

IMAGE_D* SOCV_draw_line(IMAGE_D * img, POINT_D p1, POINT_D p2, Color_D color)
{

	//The Bresenham Line 알고리즘 사용

	double MidPoint = 0, value= 0;
	int j ; 
	int start, end;
	//시작위치 조정 

	double dx = p2.x - p1.x;
	double dy = p2.y - p1.y;
	double m = dy/dx; //기울기

	int x1 = p1.x;
	int y1 = p1.y;
	int x2 = p2.x;
	int y2 = p2.y;


	//printf("(%d, %d) -> (%d,%d) %lf \n",p1.x, p1.y, p2.x, p2.y,m);
	printf("m : %lf \n",m);


	if(x1 > x2) { start = x2; end = x1; }
	else		{ start = x1; end = x2; }



	if( m>=0) //증가
	{
		printf("증가그래프 \n");

		if(y1 > y2) MidPoint = y2 +0.5;
		else		MidPoint = y1 + 0.5;
		if(MidPoint < 0 ) MidPoint = 0.5;
		//픽셀그리기
		
		if( start < 0) return NULL;
		//if( end > img->width) end = img->width;
		printf("start :%d end : %d\n",start , end);

		for(int i = start ; i < end ; i++)
		{
			j = m * ( i - p1.x) + p1.y ;//y값을 구한다
			//printf(" (%lf, %d )  j ; %lf \n", MidPoint , i , j);
			//getchar();
			if( MidPoint > 0 && MidPoint < img->height -1)
			{
				if( j <= MidPoint)
				{
					if(img->bpp == 24)
					{
						img->source[i][p1.x*3] = color.blue;
						img->source[i][p1.x*3+1] = color.green;
						img->source[i][p1.x*3+2] = color.red;
					}
					else
					{
						img->source[i][j] = (BYTE)(color.red + color.green + color.red)/3;
					}
				}
				else if( j > MidPoint)
				{
					if(img->bpp == 24)
					{
						img->source[i][p1.x*3] = color.blue;
						img->source[i][p1.x*3+1] = color.green;
						img->source[i][p1.x*3+2] = color.red;
					}
					else
					{
						img->source[i][j] = (BYTE)(color.red + color.green + color.red)/3;
					}
				}

				//printf(" j : %lf ",j);
				//printf(" MidPoint : %lf \n",MidPoint);
			}
		}
	}
	else if ( m < 0) //감소
	{
		printf("감소그래프\n");
		if(y1 > y2) MidPoint = y1 - 0.5;
		else		MidPoint = y2 - 0.5;

		if( start < 0) return NULL; 

		for(int i = start ; i < end ; i++)
		{
			if( i > img->height)
					continue;

			j = m * ( i - p1.x) + p1.y ;//y값을 구한다

			//printf(" j = %d \n",j);
			if( MidPoint > 0 && MidPoint < img->height -1)
			{
				if( j >= MidPoint)
				{
					if(img->bpp == 24)
					{
						img->source[i][p1.x*3] = color.blue;
						img->source[i][p1.x*3+1] = color.green;
						img->source[i][p1.x*3+2] = color.red;
					}
					else
					{
						img->source[i][j] = (BYTE)(color.red + color.green + color.red)/3;
					}
				}
				else if( j < MidPoint)
				{
					MidPoint--;
					if(img->bpp == 24)
					{
						img->source[i][p1.x*3] = color.blue;
						img->source[i][p1.x*3+1] = color.green;
						img->source[i][p1.x*3+2] = color.red;
					}
					else
					{
						img->source[i][j] = (BYTE)(color.red + color.green + color.red)/3;
					}
				}
				
			}

		}
	}

	return img;
}

IMAGE_D* SOCV_img_Draw(IMAGE_D* img, RECT_D* rect, Color_D Color, int thickness)
{
	if(img->bpp == 24)
	{

		for(int j = 0 ; j<thickness ; j++)
		{

			if(!(_rect_check(img->width,img->height,rect)))
			{
				printf("으앙죽었당께!\n");
				return NULL;
			}

			//가로그리기
			for(int i = rect->left ; i<=rect->right ; i++)
			{
	
				img->source[rect->top][i*3] = Color.blue;
				img->source[rect->top][i*3+1] = Color.green;
				img->source[rect->top][i*3+2] = Color.red;

				img->source[rect->bottom][i*3] = Color.blue;
				img->source[rect->bottom][i*3+1] = Color.green;
				img->source[rect->bottom][i*3+2] = Color.red;
			}

			//세로그리기
			for(int i = rect->top ; i<rect->bottom ; i++)
			{
				img->source[i][rect->left*3] = Color.blue;
				img->source[i][rect->left*3+1] = Color.green;
				img->source[i][rect->left*3+2] = Color.red;

				img->source[i][rect->right*3] = Color.blue;
				img->source[i][rect->right*3+1] = Color.green;
				img->source[i][rect->right*3+2] = Color.red ;
			}

			rect->left  -= 1;
			rect->right += 1;
			rect->top   -= 1;
			rect->bottom += 1;
		}
	}
	else if( img->bpp == 8)
	{
		int color = Color.blue + Color.green + Color.red ; 
		for(int j = 1 ; j <= thickness ; j++)
		{

			if(!(_rect_check(img->width,img->height,rect)))
			{
				break;
			}

			//가로그리기
			for(int i = rect->left ; i<=rect->right ; i++)
			{
				img->source[rect->top][i] = color;
				img->source[rect->top][i] = color;
				img->source[rect->top][i] = color;

				img->source[rect->bottom][i] = color;
				img->source[rect->bottom][i] = color;
				img->source[rect->bottom][i] = color;
			}

			//세로그리기
			for(int i = rect->top ; i<rect->bottom ; i++)
			{
				img->source[i][rect->left] = color;
				img->source[i][rect->left] = color;
				img->source[i][rect->left] = color;

				img->source[i][rect->right] = color;
				img->source[i][rect->right] = color;
				img->source[i][rect->right] = color;
			}

			rect->left  -= 1;
			rect->right += 1;
			rect->top   -= 1;
			rect->bottom += 1;
			//printf("%d %d %d %d \n",rect->top,rect->left,rect->bottom,rect->right);
		}
	}
}

BOOL _rect_check(int w , int h ,RECT_D* rect)
{
	//printf("(%d, %d) -> (%d , %d)\n",rect->left, rect->top , rect->right , rect->bottom);
	if( rect->left  <= 0 ||
		rect->top   <= 0 ||
		rect->right >= w ||
		rect->top   >= h )
	{
		//printf("Rect가 범위를 초과하였습니다.\n");
		return false;
	}

	return true;
}
IMAGE_D* SOCV_basic_hough(IMAGE_D* img)
{

	if(img->bpp == 24)
	{
		img = SOCV_Gray_24to8(img);
		//img = SOCV_Edge_Image(img,EDGE_SOBEL);
		//int threshold = SOCV_OTS_binary(img);
		img = SOCV_BinaryImage(img,NULL);
	}

	int w = img->width;
	int h = img->height;
	int maxinum=0;
	int num_rho = (int)sqrt( (double)  w*w + h*h);
	int num_ang = 180;

	int block_width = 30 ; 
	int block_height = num_rho;

	printf("num_rho : %d \n",num_rho);
	double* tsin = (double *)malloc(sizeof(double)*num_ang);
	double* tcos = (double *)malloc(sizeof(double)*num_ang);

	for(int i = 0 ; i < num_ang ; i++)
	{
		tsin[i] = (double)sin(i*PI/num_ang);
		tcos[i] = (double)cos(i*PI/num_ang);
	}

	IMAGE_D* tmp = SOCV_make_Image(num_ang,num_rho*2,8);
	int count=0;

	//좌표변환
	for(int i = 0 ; i<img->height ; i ++)
	{
		for(int j = 0 ; j < img->width ; j++)
		{
			if(img->source[i][j] == 255)
			{
				for(int ang = 0 ; ang<num_ang ; ang++)
				{
					int b = (int)floor(j * tsin[ang] + i * tcos[ang] + 0.5) + num_rho;
					tmp->source[b][ang]++;
					if(tmp->source[b][ang] > maxinum)
						maxinum++;
				}
			}
		}
	}

	if(maxinum >= 255)
		maxinum = 254;
	int threshold = maxinum;
	printf("임계값 : %d\n", threshold);
	int arr_max = 0 ; 

	int line_rho, line_ang;
	int counter = 0 ; 
	line_rho = 0 ; line_ang = 0 ; 

	ContourPoints cp;
	cp.num=0;
	//printf("%d %d \n",num_rho/block_height*2, num_ang/block_width);

	//국지적 최대값

	for(int i = 1 ; i < num_rho*2-1 ; i++)
	{
		for(int j = 1 ; j <num_ang-1 ; j++)
		{
			if(tmp->source[i][j] >= threshold)
			{
				if(tmp->source[i][j] > tmp->source[i-1][j] &&
					tmp->source[i][j] > tmp->source[i-1][j+1] &&
					tmp->source[i][j] > tmp->source[i][j+1] &&
					tmp->source[i][j] > tmp->source[i+1][j+1] &&
					tmp->source[i][j] > tmp->source[i+1][j] &&
					tmp->source[i][j] > tmp->source[i+1][j-1] &&
					tmp->source[i][j] > tmp->source[i][j-1] &&
					tmp->source[i][j] > tmp->source[i-1][j-1])
				{
					cp.x[count] = i;
					cp.y[count] = j;
					count++;
				}
			}
		}
		//printf(" count : %d \n",count);
	}



	/*
	for(int i = 0 ; i <num_rho / block_height*2 ; i++)
	{
	for(int j = 0 ; j <num_ang/ block_width ; j++)
	{
	printf(" (%d, %d) ", i*block_height , j*block_width);
	for(int row = 0 ; row < block_height ; row++)
	{
	for(int colum = 0 ; colum < block_width; colum++)
	{
	if(arr_max <= tmp->source[i*block_height+row][j*block_width+colum])
	{
	arr_max = tmp->source[i*block_height+row][j*block_width+colum];
	cp.x[count] = i*block_height+row;
	cp.y[count] = j*block_width+colum;
	}
	else if ( arr_max == 0)
	{
	cp.x[count] = 0 ;
	cp.y[count] = 0 ;
	}
	}
	}
	printf(" 검출 ( %d, %d) \n",cp.x[count], cp.y[count]);
	count++;
	}
	}
	*/

	//SOCV_save_Bitmap("TEST_hugh.bmp",tmp);

	/*
	for(int i = 0 ; i < count ; i++)
	{
	printf(" num_rho : %d num_ang : %d \n",cp.x[i],cp.y[i]);
	}
	*/
	////이미지 그리기 테스트 

	POINT_D p1,p2;
	Color_D color;
	color.blue = 0;
	color.green = 0;
	color.red = 255 ; 

	IMAGE_D *SOC = SOCV_Load_Image("simple4.bmp");



	for(int i = 0 ; i < count ; i++)
	{
		if( cp.x[i] > 0 && cp.y[i] > 0)
		{
			//printf(" 잡힌 좌표 : ( %d , %d ) \n", cp.x[i], cp.y[i]);
			line_rho = (cp.x[i]-num_rho);
			line_ang = cp.y[i];
			printf(" line_rho : %d , line_ang : %d ", line_rho, line_ang);

			if( line_ang == 90)
			{
				p1.x = (int)(line_rho + 0.5);
				p2.x = (int)(line_rho + 0.5);
				p1.y = 0;
				p2.y = img->height-1;
			}
			else
			{
				p1.x = 0;
				p1.y = (int)floor(line_rho / tcos[line_ang] +0.5);
				p2.x = img->width -1; 
				p2.y = (int)floor((line_rho - p2.x * tsin[line_ang]) / tcos[line_ang] +0.5);
			}

			//printf(" (%d, %d) -> (%d,%d)\n",p1.x,p1.y,p2.x,p2.y);
			//getchar();
			/*
			if(p2.y <0 )
			p2.y *=-1;
			if(p1.y <0)
			p1.y *=-1;
			*/
			//printf(" (%d, %d) -> (%d,%d)\n",p1.x,p1.y,p2.x,p2.y);
			SOC = SOCV_draw_line(SOC,p1,p2,color);
			//getchar();
			printf( " count : %d1 \n", i);
			SOCV_save_Bitmap("TEST_TTT.bmp",SOC);
		}
	}

	//printf("*arr_max : %d\n",arr_max);
	//printf("*line_rho : %d , line_ang : %d \n",line_rho, line_ang);

	printf(" 직선수 : %d \n",count);

	free(tsin);
	free(tcos);


	return tmp;
}

void SOCV_save_raw(IMAGE_D *img)
{
	FILE *fp = fopen("TEST_raw.raw","wb");
	if(img->bpp == 8)
	{
		RGB* rgb = (RGB *)malloc(sizeof(RGB) * 256);
		for(int i = 0 ; i<=256 ; i++)
		{
			rgb[i].reserved = 0;
			rgb[i].blue = i;
			rgb[i].green = i;
			rgb[i].red= i;
		}
		fwrite(rgb,sizeof(RGB),256,fp);
	}

	fwrite(img->source[0],sizeof(BYTE)*img->height * img->width * (img->bpp/8),1,fp);
	fclose(fp);
}

int SOCV_OTS_binary(IMAGE_D* img)
{
	if(img->bpp == 24)
	{
		img = SOCV_Gray_24to8(img);
	}

	int size_num = 256; 

	int *histogram = (int *)malloc(sizeof(int)*size_num);
	double *Wb = (double *)malloc(sizeof(double)*size_num);
	double *Mb = (double *)malloc(sizeof(double)*size_num);
	double *Wf = (double *)malloc(sizeof(double)*size_num);
	double *Mf = (double *)malloc(sizeof(double)*size_num);
	double *Between_Class_Variance = (double *)malloc(sizeof(double)*size_num); //분산

	//초기화
	memset(histogram,0,sizeof(int)*size_num);
	memset(Wb,0,sizeof(double)*size_num);
	memset(Mb,0,sizeof(double)*size_num);
	memset(Wf,0,sizeof(double)*size_num);
	memset(Mf,0,sizeof(double)*size_num);
	memset(Between_Class_Variance,0,sizeof(double)*size_num);

	//히스토그램
	for(int i = 0 ; i <img->height ; i++)
	{
		for(int j = 0 ; j < img->width ; j++)
		{
			histogram[img->source[i][j]]++;
		}
	}

	double BweigthSum=0.;
	double BMeanSum=0.;
	double FweigthSum=0.;
	double FMeanSum=0.;


	for(int T = 0 ; T < 256 ; T++)
	{
		//배경
		for(int i = 0 ; i <=T ; i++)
		{
			BweigthSum += histogram[i];
			BMeanSum += (i * histogram[i]);
		}

		Wb[T] = BweigthSum / (size_num*size_num);
		Mb[T] = BMeanSum / BweigthSum;

		BweigthSum = BMeanSum = 0.;
		//물체 
		for(int j = size_num-1  ; j >= T ; j--)
		{
			FweigthSum += histogram[j];
			FMeanSum += (j * histogram[j]);
		}

		Wf[T] = FweigthSum / (size_num * size_num);
		Mf[T] = FMeanSum / FweigthSum ;
		FweigthSum = FMeanSum =  0. ; 
	}

	double max_num=0.;
	int threshold=0;


	//클래스간 분산값(Netween Class Variance
	for(int T = 0 ; T < size_num ; T++)
	{
		Between_Class_Variance[T] = Wb[T]*Wf[T]*(Mb[T] - Mf[T])*(Mb[T] - Mf[T]);
		//printf("Vb[%d] : %lf \n",T,Between_Class_Variance[T]);
		if(max_num < Between_Class_Variance[T])
		{
			max_num = Between_Class_Variance[T];
			threshold = T;
		}
	}

	free(Mb);
	free(Wb);
	free(Mf);
	free(Wf);

	return threshold;
}

IMAGE_D* SOCV_gy_algorithm(IMAGE_D *img)
{
	IMAGE_D *gray = SOCV_Gray_24to8(img);

	for(int i = 0 ; i < img->height ; i++)
	{
		for(int j = 0 ; j < img->width ; j++)
		{
			double R = 0 , G = 0 , B = 0 ;
			double temp;
			double H; 

			B = img->source[i][j*3];
			G = img->source[i][j*3+1];
			R = img->source[i][j*3+2];

			if( ( R == G) && ( G == B) )
				H = 0 ;
			temp = ((R-G) + (R-B)) / (2 * sqrt( (R-G)*(R-G) + (R-B)*(G-B) ));
			H = acos(temp) * 180 / PI;
			if(B>G)
				H = 360 - H;
			gray->source[i][j] = H;
			//printf(" %4d",H);
		}
		//printf("\n");
	}
	
	SOCV_save_Bitmap("TEST_H_RESULT.bmp",gray);

	return img;
}

HSV_COLOR_D dinocv_conv_rgb2hsv(RGB_COLOR_D rgb)
{
    HSV_COLOR_D hsv;
    unsigned char rgb_min, rgb_max;
    rgb_min = d_min3(rgb.r, rgb.g, rgb.b);
    rgb_max = d_max3(rgb.r, rgb.g, rgb.b);
    hsv.val = rgb_max;
    if (hsv.val == 0) {
        hsv.hue = hsv.sat = 0;
        return hsv;
    }
	//printf("%d %d\n",rgb_max , rgb_min);
    /* Compute hue */
	if( rgb_max == rgb_min){
		hsv.hue = 0;
	}
    else if (rgb_max == rgb.r) {
        hsv.hue = 0 + 43*(rgb.g - rgb.b)/(rgb_max - rgb_min);
    } else if (rgb_max == rgb.g) {
        hsv.hue = 85 + 43*(rgb.b - rgb.r)/(rgb_max - rgb_min);
    } else /* rgb_max == rgb.b */ {
        //hsv.hue = 171 + 43*(rgb.r - rgb.g)/(rgb_max - rgb_min);
		hsv.hue = 171 + 43*(rgb.r - rgb.g)/(rgb_max - rgb_min);
    }
	
    return hsv;
}

void SOCV_NormalizationRGB_image(IMAGE_D *img)
{
	int width = img->width;
	int height = img->height;

	double R,G,B,S;

	for( int i = 0 ; i < height ; i++)
	{
		for( int j = 0 ; j < width ; j++)
		{
			R = (double)img->source[i][j*3];
			G = (double)img->source[i][j*3+1];
			B = (double)img->source[i][j*3+2];
			S = R + G + B;
			
			img->source[i][j*3] = (int)R / S *255;
			img->source[i][j*3+1] = (int)G / S *255;
			img->source[i][j*3+2] = (int)B / S *255;

		}
	}
}

void linetracking(IMAGE_D *img)
{
	if(img->bpp != 8)
	{
		fprintf(stdout,"image is not 8bit");
		exit(1);
	}

	int height=img->height;
	int width=img->width;
	int i,j;
	int d; //방향 조절 변수 
	int count; //8방향 획수 샐 변수
	int nx, ny; //다음 픽셀값 저장
	//int xPoint, yPoint; //외곽선 시작 픽셀값 저장
	//	int num; //외곽선 픽셀 갯수에 사용될 변수
	int x,y; //처음으로 찾은 픽셀값
	int counter=0;
	int LabelCounter =0 ; 
	int dold; 
	//	int minmaxCounter; //레이블카운터용
	int Xmin=height,Xmax=-1,Ymin=height,Ymax=-1; //창을 만들기위한 x,y 최소값 최대값
	int min,max;
	int minX[10],maxX[10],minY[10],maxY[10]; //저장소
	int savePointX=0,savePointY=0;
	int FrontPoint, EndPoint;
	int dir[8][2]={ //외곽선 경우
		{1,0},
		{1,1},
		{0,1},
		{-1,1},
		{-1,0},
		{-1,-1},
		{0,-1},
		{1,-1}
	};
	ContourPoints cp;
	cp.num  = d = count = 0 ; 


	int ** tmpArray;
	tmpArray = (int **)malloc(sizeof(int *)*height-1);
	tmpArray[0] = (int *)malloc(sizeof(int)*height*width);
	for(i=1;i<height;i++)
		tmpArray[i] = tmpArray[i-1] +width;
	memset(*tmpArray,0,sizeof(int)*height*width);


	//copy pixel
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			tmpArray[i][j] = (int)img->source[i][j];
		}
	}

	for(j=1;j<height;j++)
	{
		for(i=1;i<width;i++)
		{
			if(tmpArray[j][i] == 255 && tmpArray[j][i-1] == 0 && tmpArray[j-1][i] == 0 && tmpArray[j-1][i-1] == 0  ) 
			{
				printf("최초 픽셀 -> tmpArray[%d][%d]\n",j,i);
				//printf("tmpArray[%d][%d] -> %d \ntmpArray[%d][%d] -> %d  \ntmpArray[%d][%d] -> %d  \ntmpArray[%d][%d] -> %d \n", 
				//	j,i,tmpArray[j][i],j,i-1,tmpArray[j][i-1],j-1,i,tmpArray[j-1][i],j-1,i-1,tmpArray[j-1][i-1]);
				y = j;
				x = i;
				dold  = count= d = 0 ;
				LabelCounter++;
				//여기서부터 external contour tracing
				FrontPoint = cp.num;
				while(1)
				{
					nx = x + dir[d][0];
					ny = y + dir[d][1];
					if( nx< 0 || nx>=width || ny<0||ny>=height ||tmpArray[ny][nx] ==0||tmpArray[ny][nx] ==-1)
					{
						//진행 방향에 있는 픽셀이 객체가 아닌경우,
						//시계 방향으로 진행 방향을 바꾸고 다시 시도한다.
						tmpArray[ny][nx] =-1;

						if(++d>7) d =0;
						count++;
						// 8방향 모두 배경(background)인 경우
						if(count >=8)
						{
							cp.x[cp.num] =x;
							cp.y[cp.num] =y;
							cp.num++;
							//printf("8방향 모두 배경! \n");

							break;
						}
					}
					else
					{
						//진행 방향의 픽셀이 객체일 경우,
						//현재 점을 외곽선 정보에 저장
						cp.x[cp.num] =x;
						cp.y[cp.num] =y;
						cp.num++;
						//진행 방향으로 이동
						tmpArray[ny][nx] = 256;
						x=nx;
						y=ny;
						//방향정보 초기화

						count = 0;
						dold=d;
						d = (dold +6) % 8; // d = dold -2 와 같은 형태
					}
					// 시작점으로 돌아왔고, 진행 방향이 초기화된 경우 외곽선 추적을 끝낸다.
					if(x==i && y==j && d ==0 )
					{

						EndPoint = cp.num;
						printf("external cp.num -> %d\n",cp.num);
						//(x,y)좌표값계산
						printf("FrontPoint : %d\nEndPoint : %d\n",FrontPoint,EndPoint);
						for(savePointX = FrontPoint ; savePointX<cp.num; savePointX++){
							min = cp.x[savePointX];
							if(Xmin >= min){
								Xmin = min;
							}
							else if(Xmax <= min){
								Xmax = min;
							}
						}
						for (savePointY=FrontPoint ; savePointY<cp.num; savePointY++){
							min = cp.y[savePointY];
							if(Ymin >= min){
								Ymin = min;
							}
							else if(Ymax <= min){
								Ymax = min;
							}
						}
						minX[LabelCounter] = Xmin;
						maxX[LabelCounter] = Xmax;
						minY[LabelCounter] = Ymin;
						maxY[LabelCounter] = Ymax;
						savePointY=savePointX=cp.num;
						//printf("LabelCounter -> %d\n",LabelCounter);
						printf("(%d,%d) - (%d,%d)\n(%d,%d) - (%d,%d) \n",minX[LabelCounter],minY[LabelCounter],maxX[LabelCounter],minY[LabelCounter],minX[LabelCounter],
							maxY[LabelCounter],maxX[LabelCounter],maxY[LabelCounter]);

						//초기화
						Xmin=height,Xmax=-1,Ymin=height,Ymax=-1;

						break;
					}
				}

			}

			if(tmpArray[j][i] == 0 && tmpArray[j][i-1] > 0)
			{
				//printf("tmpArray[%d][%d]->%d\ntmpArray[%d][%d]->%d\ntmpArray[%d][%d]->%d\n",j,i,tmpArray[j][i],j-1,i,tmpArray[j-1][i],j,i+1,tmpArray[j][i+1]);
				//printf("내부픽셀발견! ->tmpArray[%d][%d]\n",j,i);
				y = j;
				x = i;
				dold  = count= 0;
				d = 3;
				tmpArray[j][i] =-1;
				counter++;
				//여기서 부터 internal contour tracing
				while(1){
					nx = x + dir[d][0];
					ny = y + dir[d][1];

					//범위밖이면 종료 
					if( nx< 0 || nx>=width || ny<0||ny>=height )
						break;

					if(tmpArray[ny][nx] > 0 ){
						//printf(" d : - > %d 서치 : -> tmpArray[%d][%d]\n",d,ny,nx);
						//반시계 방향으로 진행 방향을 바꾸고 다시 시도한다.
						tmpArray[ny][nx] = 256;
						if(d == 0 ){
							//printf("d<=0\n");
							d =7;
						}
						d = d-1;
						count++;
						// 8방향 모두 배경(background)인 경우
						if(count >=8)
						{
							cp.x[cp.num] =nx;
							cp.y[cp.num] =ny;
							cp.num++;
							printf("8방향 모두 배경! -> (%d, %d)\n" , nx,ny);
							break;
						}
						//마킹
						cp.x[cp.num] =nx;
						cp.y[cp.num] =ny;
						cp.num++;

					}
					else{
						//printf(" d : - > %d 서치 : -> tmpArray[%d][%d]\n",d,ny,nx);
						//printf("0을 찾음 \n");
						//진행방향 내부점 찾음
						//진행 방향으로 이동
						tmpArray[ny][nx] = -1;
						x=nx;
						y=ny;
						//방향정보 초기화
						count = 0;
						d += 2 ; 
						if(d >7 ) d = 1;
					}
					//printf("d ->%d\n",d);
					if(x==i && y==j && d==3){
						printf("완료\n");
						printf("internal cp.num -> %d\n",cp.num);
						break;
					}

				}

			}

		}
	}
	
	printf("%d\n",cp.num);
	for(j=0;j<height;j++){
		for(i=0;i<width;i++){
			tmpArray[j][i] = 0;
			img->source[j][i] = 0;
		}
	}


	for(i=0;i<cp.num;i++)
		img->source[cp.y[i]][cp.x[i]] = 255;

	for(i=1;i<=LabelCounter;i++){
		for(j = minX[i] ; j<maxX[i] ; j++){
			img->source[minY[i]][j] = 255;
			img->source[maxY[i]][j] = 255;
		}

		for(int k = minY[i] ; k<maxY[i] ; k++){
			img->source[k][minX[i]] = 255;
			img->source[k][maxX[i]] = 255;
		}
	}

	//SOCV_save_Bitmap("result.bmp",img);

	/*
	Color_D color;
	color.red = 255;
	color.blue = 255;
	color.green = 255;

	RECT_D* rect=(RECT_D *)malloc(sizeof(RECT_D *)*LabelCounter+1);

	for( i=1; i<=LabelCounter ;i++)
	{
		rect[i].left = minX[i];
		rect[i].top = minY[i];
		rect[i].right = maxX[i];
		rect[i].bottom = maxY[i];
		printf("%d %d %d %d \n",minX[i] , minY[i] , minX[i], maxY[i]);	
	}
	*/

	printf("객체 갯수 - > : %d\n",LabelCounter);
	free(tmpArray);
	//return rect;
}

IMAGE_D *SOCV_int_hue(IMAGE_D * _24img)
{
	
	IMAGE_D *hue = SOCV_make_Image(_24img->width, _24img->height, 8);
	HSV_COLOR_D hsv;
	RGB_COLOR_D rgb;

	for(int i = 0 ; i < _24img->height ; i++)
	{
		for(int j = 0; j < _24img->width ; j++)
		{

			// HSI 중 H 값만 구함 (색상)
			rgb.b = _24img->source[i][j*3];
			rgb.g = _24img->source[i][j*3+1];
			rgb.r = _24img->source[i][j*3+2];

			hsv = dinocv_conv_rgb2hsv(rgb);
			hue->source[i][j] = hsv.hue;
		}
	}

	return hue;
}

IMAGE_D* SOCV_block_variance(IMAGE_D * _8hue)
{

	int block_width = 10;
	int block_height = 10;

	IMAGE_D* resize = SOCV_make_Image(_8hue->width/block_width, _8hue->height/block_height, 8);

	
	for(int i = 0 ; i < _8hue->height/block_height ; i++)
	{
		for(int j = 0; j < _8hue->width/block_width ; j++)
		{
			int i_index = i * block_height;
			int j_index = j * block_width;
			
			int variance=0;
			double average=0;
			int sum=0;
			int sum_deviation=0;

			for(int y = 0; y < block_height; y++)
			{
				for(int x = 0 ; x < block_width; x++)
				{
					if( y+i_index > _8hue->height || x+j_index > _8hue->width)
						continue;
					sum += _8hue->source[y+i_index][x+j_index];
				}
			}

			average = sum / (block_height * block_width);

			//분산
			for(int y = 0; y < block_height; y++)
			{
				for(int x = 0 ; x < block_width; x++)
				{
					if( y+i_index > _8hue->height || x+j_index > _8hue->width)
						continue;
					int square = ( _8hue->source[y+i_index][x+j_index] - (int)average);
					square = square * square;
					sum_deviation +=square;
				}
			}

			variance = (int)sum_deviation / (block_height * block_width);
			//printf("variance : %d %lf\n",variance,average);

			
			//그리기
			/*
			for(int y = 0; y < block_height; y++)
			{
				for(int x = 0 ; x < block_width; x++)
				{
					if( y+i_index > _8hue->height || x+j_index > _8hue->width)
						continue;

					if( variance < 4 )
						_8hue->source[y+i_index][x+j_index] = 255;
					else
						_8hue->source[y+i_index][x+j_index] = 0;
				}
			}
			*/
			
			if( variance < 10 )
				resize->source[i][j] = 255;
			else
				resize->source[i][j] = 0;
			
			variance = 0;
			sum = 0;
			sum_deviation = 0;
		}
	}

	return resize;
}

IMAGE_D *SOCV_hue_hough(IMAGE_D* _8hue)
{

	int w = _8hue->width;
	int h = _8hue->height;
	int maxinum=0;
	int num_rho = (int)(sqrt( (double)  w*w + h*h));
	int num_ang = 180;

	
	double* tsin = (double *)malloc(sizeof(double)*num_ang);
	double* tcos = (double *)malloc(sizeof(double)*num_ang);

	for(int i = 0 ; i < num_ang ; i++)
	{
		tsin[i] = (double)sin(i*PI/num_ang);
		tcos[i] = (double)cos(i*PI/num_ang);
	}

	//축적배열에 사용될 이미지 
	IMAGE_D* tmp = SOCV_make_Image(num_ang+1,num_rho*2+1,8);
	//printf("hough image ... : %d %d \n", num_rho, num_ang);
	int count=0;

	//좌표변환

	for(int i = 0 ; i<_8hue->height ; i ++)
	{
		for(int j = 0 ; j < _8hue->width ; j++)
		{
			if(_8hue->source[i][j] == 255)
			{
				for(int ang = 0 ; ang<num_ang ; ang++)
				{
					int b = (int)floor(j * tsin[ang] + i * tcos[ang] + 0.5) + num_rho;
					//printf("%d %d \n", b, ang);
					tmp->source[b][ang]++;
				}
			}
		}
	}

	free(tsin);
	free(tcos);
	
	return tmp;
}

void _houghLine(IMAGE_D * _8img, LineParam line, BYTE c)
{
	int x,y;
	printf("line.ang : %lf line.rho : %lf \n",line.ang ,line.rho);
	if( line.ang < 0.0 || line.ang > 180.0) return;
	if( line.rho < -160.0 || line.rho > 160.0) return;
	//if(line.rho < 0) return ;
	int w = _8img->width;
	int h = _8img->height;


	//if( line.ang > 85 && line.ang < 95) //수직선
	if(line.ang == 90.0)
	{
		printf("수직선\n");
		x = (int)(line.rho - 0.5);
		printf("x : %d \n",x);
		if( x > _8img->width) return ;
		
		for( y = 0 ; y < h ; y++)
			_8img->source[y][x] = c;
		
		return ;
	}

	//if( (line.ang > 175 && line.ang < 185) || line.ang < 5) // 수평선 
	if( line.ang == 180.0 || line.ang == 0.0)
	{
		printf("수평선\n");
		
		y = (int)(line.rho - 0.5);
		if( y > _8img->height) return ;
		for( x = 0 ; x < w ; x++)
			_8img->source[y][x] = c;
		
		return ;
	}


	/*
	int x1=0;
	int y1=(int)floor(line.rho / cos(line.ang*PI/180) + 0.5);
	int x2= _8img->width -1;
	int y2=(int)floor((line.rho - x2*sin(line.ang*PI/180)) / cos(line.ang*PI/180 + 0.5));
	*/
	
	int x1 = 0;
	int y1 = (int)line.rho / cos(line.ang*PI/180);
	int x2 = (int)line.rho / sin(line.ang*PI/180);
	int y2 = 0;
	
	double m = (double)(y2-y1)/(x2-x1);
	//printf("(%d %d) (%d %d)\n",x1,y1,x2,y2);
	//좌표설정

	if( y1 < 0)
	{
		y1 = m * _8img->width + y1;
		x1 = _8img->width;
	}

	if( x1 < x2 && x1 < 0) SOCV_swap(&x1,&x2);
	if( y1 < y2 && y1 < 0) SOCV_swap(&y1,&y2);

	if( y2 < 0)
	{
		//y2 = y = (int)floor(m * (_8img->height - x1) + y1 + 0.5);
		//printf("m : %lf \n",m);
		y2 = m * _8img->width + y2;
		x2 = _8img->width;
	}
	if( x2 < 0)
	{
		x2 = line.rho/sin(line.ang*PI/180) - cos(line.ang*PI/180)/sin(line.ang*PI/180)*(_8img->width);
		y2 = _8img->width;
	}

	printf("(%d %d) (%d %d)\n",x1,y1,x2,y2);
	//_houghDrawline(_8img,x1,x2,y1,y2,255);
}

void _houghDrawline(IMAGE_D * _img, int x1, int x2, int y1, int y2, Color_D *color, int color_select)
{
	int x,y;
	double m;

	int w = _img->width;
	int h = _img->height;

	m = (double)(y2-y1)/(x2-x1);
	//printf("%d %d %d %d \n",x1,y1,x2,y2);
	//printf("m : %lf\n",m);
	if( ( m >= -1) && ( m <= 1))
	{
		if( x1 > x2)
		{
			SOCV_swap(&x1,&x2);
			SOCV_swap(&y1,&y2);
		}
		//printf("x %d %d \n",x1,x2);
		if( _img->bpp == 8)
		{
			for( x = x1 ; x <= x2 ; x++)
			{
				y = (int)floor(m * (x - x1) + y1 + 0.5);
				if( y >= 0 && y < h && x >= 0 && x < w)
					_img->source[y][x] = 128;
			}
		}
		else //24 
		{
			for( x = x1 ; x <= x2 ; x++)
			{
				y = (int)floor(m * (x - x1) + y1 + 0.5);
				if( y >= 0 && y < h && x >= 0 && x < w)
				{
					_img->source[y][x*3] = color[color_select].blue;
					_img->source[y][x*3+1] = color[color_select].green;
					_img->source[y][x*3+2] = color[color_select].red;
				}
			}
		}
	}
	else
	{
		if( y1 > y2)
		{
			SOCV_swap(&x1,&x2);
			SOCV_swap(&y1,&y2);
		}
		//printf("y %d %d \n",y1,y2);
		if( _img->bpp == 8)
		{
			for( y = y1 ; y <= y2; y++)
			{
				x = (int)floor( (y - y1) / m + x1 + 0.5);
				if( y >= 0 && y < h && x >= 0 && x < w)
					_img->source[y][x] = 128;
			}
		}
		else
		{
			for( y = y1 ; y <= y2; y++)
			{
				x = (int)floor( (y - y1) / m + x1 + 0.5);
				if( y >= 0 && y < h && x >= 0 && x < w)
				{
					_img->source[y][x*3] = color[color_select].blue;
					_img->source[y][x*3+1] = color[color_select].green;
					_img->source[y][x*3+2] = color[color_select].red;
				}
			}
		}
	}
}

BOOL _8x8filtering(IMAGE_D * _8img)
{
	BYTE tmp8x8[8];
	memset(&tmp8x8,0,sizeof(tmp8x8));

	
	for(int i = 1 ; i < _8img->height -1 ; i++)
	{
		for(int j = 1 ; j < _8img->width -1 ;j++)
		{
			
			if(_8img->source[i][j] == 255)
			{
				tmp8x8[0] = _8img->source[i-1][j-1];
				tmp8x8[1] = _8img->source[i-1][j];
				tmp8x8[2] = _8img->source[i-1][j+1];
				tmp8x8[3] = _8img->source[i][j-1];
				tmp8x8[4] = _8img->source[i][j+1];
				tmp8x8[5] = _8img->source[i+1][j-1];
				tmp8x8[6] = _8img->source[i+1][j];
				tmp8x8[7] = _8img->source[i+1][j+1];

				int count = 0;

				for(int x = 0 ; x < 8 ; x++)
				{
					if(tmp8x8[x] == 0)
						count++;
				}

				if(count > 4)
				{
					_8img->source[i][j] = 0;
				}

				count = 0;
			}
		}
	}

	return 1;
}

IMAGE_D *draw_hue(IMAGE_D * _8img)
{
	int block_width = 10;
	int block_height = 10;

	IMAGE_D *draw = SOCV_make_Image(_8img->width * block_width , _8img->height * block_height, 8);

	for(int i = 0 ; i < _8img->height ; i++)
	{
		for(int j = 0 ; j < _8img->width ; j++)
		{
			int i_index = i * block_height;
			int j_index = j * block_width;

			if( _8img->source[i][j] == 255)
			{
				//printf("%d %d \n", i,j);
				//그린다.
				for(int y = 0 ; y < block_height ; y++)
				{
					for(int x = 0 ; x < block_width; x++)
					{
						if( y+i_index > draw->height || x+j_index > draw->width)
							continue;
						draw->source[i_index+y][j_index+x] = 255;
					}
				}
			}
		}
	}

	return draw;
}


PeakPoint* find4PeakPoint(IMAGE_D* _houghed_img)
{
	int block_width = 10;
	int block_height = 10;
	int maxinum = 0;
	int maxinum_x = 0;
	int maxinum_y = 0;
	int h = _houghed_img->height/block_height;
	int w = _houghed_img->width/block_width;

	PeakPoint* peak = (PeakPoint *)malloc(sizeof(PeakPoint)*h*w);
	memset(peak,0,sizeof(PeakPoint)*w*h);
	
	int count =0 ;
	int sum = 0;
	for(int i = 0 ; i < _houghed_img->height/block_height ; i++)
	{
		for(int j = 0 ; j < _houghed_img->width/block_width ; j++)
		{
			int i_index = i * block_height;
			int j_index = j * block_width;

			for(int y = 0; y < block_height; y++)
			{
				for(int x = 0 ; x < block_width; x++)
				{
					if( y+i_index > _houghed_img->height || x+j_index > _houghed_img->width)
						continue;

					//주어진 구간에서 최대값을 찾는다.

					if( _houghed_img->source[y+i_index][x+j_index] > 0)
					{
						if( peak[count].value < _houghed_img->source[y+i_index][x+j_index] )
						{
							peak[count].value =  _houghed_img->source[y+i_index][x+j_index];
							peak[count].ang = (x+j_index);
							peak[count].rho = y+i_index-80;
							sum += peak[count].value;
						}
					}
				}
			}

			maxinum = 0;
			if( sum > 0)
			{
				count++;
			}
			sum = 0;
		}
	}
	/*
	for(int i = 0; i < count ; i++)
		printf("[%d] %d %d %d \n", i, peak[i].ang, peak[i].rho, peak[i].value);
	printf("======================================\n");
	*/
	BubbleSort(peak,count);
	/*
	for(int i = 0; i < count ; i++)
		printf("[%d] %d %d %d \n", i, peak[i].ang, peak[i].rho, peak[i].value);
	*/
	return peak;
}

void BubbleSort(PeakPoint *peak, int length){
	int i,j,temp;
	int temp_value, temp_ang, temp_rho;
	for(i=0;i<length-1;i++)
	{ 
		for(j=0;j<length-i-1;j++)
		{
			if(peak[j].value <peak[j+1].value)
			{ 
				temp_value = peak[j].value;
				peak[j].value = peak[j+1].value;
				peak[j+1].value = temp_value;

				temp_ang = peak[j].ang;
				peak[j].ang = peak[j+1].ang;
				peak[j+1].ang = temp_ang;
				
				temp_rho = peak[j].rho ;
				peak[j].rho = peak[j+1].rho;
				peak[j+1].rho = temp_rho;
			}
		}
	}
}

PeakPoint* SearchPeakPoint(PeakPoint *peak, int length)
{
	int i,j;
	
	int threshold_theta = 5;
	int threshold_max_rho = 50;
	int threshold_min_rho = 3;
	double threshold_length = 0.1;
	int count = 0;
	PeakPoint* candidate_peak = (PeakPoint*)malloc(sizeof(PeakPoint)*4);
	memset(candidate_peak,0,sizeof(PeakPoint)*4);
	
	int d_angle = 1 ; 
	int state = 0;

	for( i = 0 ; i < length-1 ; i++)
	{
		for( j = i+1 ; j < length ; j++)
		{
			if( count == 4)
			{
				/*
				for(int i = 0; i < 4 ; i++)
				{
					printf("%d ang : %d rho : %d %d\n",i, candidate_peak[i].ang,candidate_peak[i].rho,candidate_peak[i].value);
				}
				*/
				return candidate_peak;
			}
			//printf("[%d %d]%d %d %d \n", i,j,abs(peak[i].ang - peak[j].ang) ,abs(peak[i].rho + peak[j].rho),abs(peak[i].value - peak[j].value));
			if( abs(peak[i].ang - peak[j].ang) <= threshold_theta &&
				abs(peak[i].rho - peak[j].rho) < threshold_max_rho && 
				abs(peak[i].rho - peak[j].rho) > threshold_min_rho && 
				abs(peak[i].value - peak[j].value < (int)(threshold_length * (peak[i].value + peak[j].value)) / 2.0))
			{
				
				switch(state)
				{
					case 0:
					{	// 후보 쌍 하나를 찾음.
						//printf("i : %d j : %d \n", i,j );
						candidate_peak[count].ang = peak[i].ang;
						candidate_peak[count].rho = peak[i].rho;
						candidate_peak[count].value = peak[i].value;
						count++;
						candidate_peak[count].ang = peak[j].ang;
						candidate_peak[count].rho = peak[j].rho;
						candidate_peak[count].value = peak[i].value;
						count++;
						//printf("[후보1] ang : %d , rho : %d \n",peak[i].ang, peak[i].rho);
						state++;
						break;
					}
					case 1:
					{
						d_angle = abs(candidate_peak[0].ang - peak[j].ang);
						//printf("d_angle : %d \n",d_angle);
						//printf("peak[0].ang : %d peak[j].ang : %d\n",peak[0].ang,peak[j].ang);
						if(d_angle > 75 && d_angle < 125)
						{
							//printf("i : %d j : %d \n", i,j );
							candidate_peak[count].ang = peak[i].ang;
							candidate_peak[count].rho = peak[i].rho;
							candidate_peak[count].value = peak[i].value;
							count++;
							candidate_peak[count].ang = peak[j].ang;
							candidate_peak[count].rho = peak[j].rho;
							candidate_peak[count].value = peak[i].value;
							count++;
						}
						break;
					}

					default: 
						break ;
				}
			}
		}
	}
	//printf("NULL\n");
	return NULL;
}

POINT_D find_vertices(PeakPoint *peak, TrackingInfo *trackinginfo,IMAGE_D *img)
{
	
	int x,y;
	POINT_D point[4]={0,};
	int count = 0;
	//memset(_8img->source[0],0,sizeof(BYTE)*_8img->width * _8img->height);
	for(int i = 0; i < 2 ; i++)
	{
		for(int j = 2 ; j < 4 ; j++)
		{
			if(peak[i].ang == 45.0 || peak[j].ang == 45.0)
			{
				y = 0.7 * (peak[i].rho - peak[j].rho);
				x = 0.7 * (peak[i].rho + peak[j].rho);
				point[count].x = x;
				point[count].y = y;
				count++;
				//printf("y : %d x : %d \n",y,x);
			}
			else
			{
				double sin1 = sin(peak[i].ang*PI/180);
				double sin2 = sin(peak[j].ang*PI/180);
				double cos1 = cos(peak[i].ang*PI/180);
				double cos2 = cos(peak[j].ang*PI/180);

				int y =  (int)(peak[i].rho  - (sin1*peak[j].rho / sin2)) / (cos1 - (cos2*sin1)/sin2);
				int x =  (int)(peak[i].rho - (cos1*peak[j].rho / cos2)) / (sin1 - (sin2*cos1)/cos2);

				point[count].x = x;
				point[count].y = y;
				//printf("x : %d y : %d \n",x,y);
				count++;
			}
		}
	}
	
	double max = 0 ;
	int far_number;
	for(int i = 1 ; i < 4; i++)
	{
		double dx = point[0].x - point[i].x;
		dx = dx * dx;
		double dy = point[0].y - point[i].y;
		dy = dy * dy;

		double distance = sqrt(dx + dy);
		if( max < distance)
		{
			max = distance;
			far_number = i;
		}
	}
	//printf("max : %lf %d\n",max,far_number);
	//직선 그림

	// roi 설정 
	int top = 64;
	int bottom = 0;
	int right = 0;
	int left = 48;

	for(int i = 0 ; i < 4 ; i++)
	{
		int y = point[i].y;
		int x = point[i].x;

		if( top > y )
			top = y;
		if( bottom < y)
			bottom = y;
		if( right < x )
			right = x;
		if( left > x)
			left = x;

		trackinginfo->roi->bottom = bottom;
		trackinginfo->roi->left = left;
		trackinginfo->roi->right = right;
		trackinginfo->roi->top = top;

	}
	
	//printf("top : %d left : %d bottom %d right %d \n", top,left,bottom,right);
	
	/*
	for(int i = 1 ; i < 4 ; i++)
	{
		if( i != far_number)
		{
			if(point[i].x > 0 && point[i].y > 0)
			{
				_houghDrawline(img,point[0].x*10,point[i].x*10,point[0].y*10,point[i].y*10,255);
				_houghDrawline(img,point[far_number].x*10,point[i].x*10,point[far_number].y*10,point[i].y*10,255);
			}
		}
	}
	*/

	int y_start = trackinginfo->roi->top*10;
	int y_end = trackinginfo->roi->bottom*10;
	int x_start = trackinginfo->roi->left*10;
	int x_end = trackinginfo->roi->right*10;
	
	trackinginfo->roi->top = y_start;
	trackinginfo->roi->bottom  = y_end;
	trackinginfo->roi->left = x_start;
	trackinginfo->roi->right = x_end;
	POINT_D point_mid;
	point_mid.x = (left + right) *5 ;
	point_mid.y = (top + bottom) *5 ;
	trackinginfo->middlePoint.x = point_mid.x;
	trackinginfo->middlePoint.y = point_mid.y;

	//printf("[mid] y :%d x : %d\n",point_mid.y,point_mid.x);
	
	return point_mid;
}


BOOL Tracking_info(TrackingInfo *trackingInfo, IMAGE_D *_8hue)
{
	//printf("img bpp : %d\n",_8hue->bpp);
	BYTE Hhistogram[256] ={0,};
	int averageH;
	int variance;
	int sum = 0;
	int sum_deviation=0;
	int size;
	int y_start = trackingInfo->roi->top;
	int y_end = trackingInfo->roi->bottom;
	int x_start = trackingInfo->roi->left;
	int x_end = trackingInfo->roi->right;

	int x = trackingInfo->middlePoint.x;
	int y = trackingInfo->middlePoint.y;
	int interval = 4;
	double average = 0;
	int SumHue =0;


	//범위넘어간 사각형 제거
	if( y_start <= 0 || y_end >= _8hue->height || x_start <= 0 || x_end >= _8hue->width)
	{
		//printf("탈출\n");
		return false;
	}

	//평균
	for(int i = y - interval ; i < y + interval ; i++)
	{
		for(int j = x - interval ; j < x + interval ; j++)
		{
			if( y <= 0 || y >= _8hue->height || x <= 0 || x >= _8hue->width)
			{
				//printf("탈출\n");
				return false;
			}
			SumHue += _8hue->source[i][j];
		}
	}

	average = SumHue / ((interval*2) * (interval*2));

	//분산
	for(int i = y - interval; i < y + interval; i++)
	{
		for(int j = x - interval ; j < x + interval ; j++)
		{
			if( y <= 0 || y >= _8hue->height || x <= 0 || x >= _8hue->width)
			{
				//printf("탈출\n");
				return false;
			}
			int square = ( _8hue->source[i][j] - (int)average);
			square = square * square;
			sum_deviation +=square;
		}
	}

	variance = (int)sum_deviation / ((interval*2) * (interval*2));

	if( variance > 5)
	{
		//fprintf(stderr,"no rectangle \n");
		return false;
	}
	else
	{
		trackingInfo->averageH = average;
		//printf("variance : %d  AverageHue : %lf \n",variance, average);
	}

	return true;
}

void Tracking(TrackingInfo *trackinginfo, IMAGE_D *_8hue)
{
	RECT_D *rect = (RECT_D *)malloc(sizeof(RECT_D));
	int y_start = trackinginfo->roi->top*10;
	int y_end = trackinginfo->roi->bottom*10;
	int x_start = trackinginfo->roi->left*10;
	int x_end = trackinginfo->roi->right*10;

	rect->top = y_start;
	rect->bottom  = y_end;
	rect->left = x_start;
	rect->right = x_end;

	Color_D color;
	color.red = 255;
	color.blue = 0;
	color.green = 0;

	//SOCV_img_Draw(_8hue,rect,color,2);
	free(rect);
}

void middle_view(IMAGE_D *_img, ColorObjectInfo *cbj, Color_D *color, int Color_select)
{
	int state = 0;
	time_t now;
	struct tm *time_;
	now = time(NULL);
	time_ = localtime(&now);
	int hour = time_->tm_hour;
	int min = time_->tm_min;
	int sec = time_->tm_sec;
	int trans_sec = hour * 3600 + min * 60 + sec;
	POINT_D pre_tail = {0,0};

	if( cbj == NULL)
	{

	}
	else if( cbj != NULL)
	{
		NODE *p;
		p = cbj->plist;
		int count = 0;
		while(p)
		{
			int y = p->point.y;
			int x = p->point.x;
			
			for(int i = y - 2  ; i < y + 2; i++)
			{
				for(int j = x - 2 ; j < x + 2 ; j++)
				{
					if( i >= _img->height || i <= 0 || j >= _img->width || j <= 0) 
						continue;

					_img->source[i][j*3] = color[Color_select].blue;
					_img->source[i][j*3+1] = color[Color_select].green;
					_img->source[i][j*3+2] = color[Color_select].red;
				}
			}
			
			switch(state)
			{
			case 0:
				pre_tail.x = p->point.x;
				pre_tail.y = p->point.y;
				state = 1;
				p = p ->link;
				break;
			case 1:
				cbj->middlePoint.x = p->point.x;
				cbj->middlePoint.y = p->point.y;
				state = 0;
			default:
				_houghDrawline(_img,pre_tail.x,cbj->middlePoint.x, pre_tail.y, cbj->middlePoint.y,color,Color_select);
			}

			//p = p ->link;
			count++;
			
		}

		//일정횟수 초과시 초기화
		if(count > 200)
		{
			destroy_list(cbj->plist);
			cbj->plist = NULL;
		}

		//일정시간 들어오지 않으면 초기화 기준 : 3초 
		int interval_time = trans_sec - cbj->LastTime;
		if( interval_time == 5)
		{
			destroy_list(cbj->plist);
			cbj->plist = NULL;
		}
		
	}
}

void judge_rectangle(TrackingInfo *trackinginfo)
{
	
}

TrackingInfo* make_trackingInfo(TrackingInfo* trackinginfo)
{
	TrackingInfo *trackingInfo = (TrackingInfo *)malloc(sizeof(TrackingInfo));
	trackingInfo->roi = (RECT_D *)malloc(sizeof(RECT_D));
	trackingInfo->plist=NULL;
	trackingInfo->averageH =0;
	trackingInfo->variance =0;

	return trackingInfo;
}

ColorObjectInfo* make_ColorObjectInfo(ColorObjectInfo* objectInfo)
{
	objectInfo = (ColorObjectInfo *)malloc(sizeof(ColorObjectInfo));
	objectInfo->plist=NULL;
	objectInfo->LastTime = 0;
	
	return objectInfo;
}

BOOL checkDupilcation(TrackingInfo *trackinginfo, POINT_D *midpoint)
{
	int top = trackinginfo->roi->top;
	int bottom = trackinginfo->roi->bottom;
	int left = trackinginfo->roi->left;
	int right = trackinginfo->roi->right;

	int y = midpoint->y;
	int x = midpoint->x;

	printf("%d %d %d %d . %d %d \n",top,bottom,left,right,y,x);

	//전에 찾았던 객체
	if( y >= top &&
		y <= bottom &&
		x >= left &&
		x <= right)
	{
		printf("전에 있었던것 같다.\n");
		return NULL;
	}

	return 1;
}

void SOCV_GammaCorrection(IMAGE_D *img, double gamma)
{
	int width = img->width;
	int height = img->height;
	int i,j;
	double invgamma = 1.f / gamma;

	if(img->bpp != 8)
	{
		fprintf(stderr, "bpp is 8\n");
	}
	else
	{
		for( i = 0 ; i < height ;i++)
		{
			for( j = 0 ; j < width; j++)
			{
				img->source[i][j] = (BYTE)limit(pow((img->source[i][j]/255.0) , invgamma) *255 + 0.5);
			}
		}
	}
}

BOOL object_distinguish(TrackingInfo *trackinginfo, IMAGE_D *img, Color_D *color, ObjectInfo* obj)
{
	time_t now;
	struct tm *time_;
	now = time(NULL);
	time_ = localtime(&now);
	int hour = time_->tm_hour;
	int min = time_->tm_min;
	int sec = time_->tm_sec;
	int trans_sec = hour * 3600 + min * 60 + sec;

	int object_Hue = trackinginfo->averageH;

	if(object_Hue > 190)
	{
		fprintf(stderr,"빨강색 사각형\n");
		SOCV_img_Draw(img,trackinginfo->roi,color[1],3);
		obj->redRectangle->plist = insert_node(obj->redRectangle->plist,NULL,trackinginfo->middlePoint,0);
		obj->redRectangle->LastTime = trans_sec;
	}
	else if(object_Hue < 160 && object_Hue > 140)
	{
		//fprintf(stderr,"파란색 사각형\n");
		SOCV_img_Draw(img,trackinginfo->roi,color[2],3);
		obj->blueRectangle->plist = insert_node(obj->blueRectangle->plist,NULL,trackinginfo->middlePoint,1);
		obj->blueRectangle->LastTime = trans_sec;
	}
	else if(object_Hue < 65 && object_Hue > 55)
	{
		fprintf(stderr,"초록색 사각형\n");
		SOCV_img_Draw(img,trackinginfo->roi,color[3],3);
		obj->greenRectangle->plist = insert_node(obj->greenRectangle->plist,NULL,trackinginfo->middlePoint,2);
		obj->greenRectangle->LastTime = trans_sec;
	}
	else if(object_Hue < 55 && object_Hue > 45)
	{
		fprintf(stderr,"노란색 사각형\n");
		SOCV_img_Draw(img,trackinginfo->roi,color[4],3);
		obj->yellowRectangle->plist = insert_node(obj->yellowRectangle->plist,NULL,trackinginfo->middlePoint,3);
		obj->yellowRectangle->LastTime = trans_sec;
	}
	else 
	{
		fprintf(stderr,"어떤 사각형 찾음\n");
		SOCV_img_Draw(img,trackinginfo->roi,color[1],4);
	}

	return 1;
}

void dinocv_draw_line(IMAGE_D *img, POINT_D p1, POINT_D p2, Color_D *clr, int color_select)
{
	/* 직선의 기울기에 의한 방법 
	int x1, y1, x2, y2;

	x1=p1[0];
	y1=p1[1];
	x2=p2[0];
	y2=p2[1];

	int dx=x2-x1, dy=y2-y1, steps,i;
	float xlnc, ylnc,x=(float)x1,y=(float)y1;

	if (abs(dx)>abs(dy))
		steps=abs(dx);
	else
		steps=abs(dy);

	xlnc=dx/(float)steps;
	ylnc=dy/(float)steps;
	if(img->bpp==24)
	{
		img->source[(int)y][(int)x*3+0]=clr->b;
		img->source[(int)y][(int)x*3+1]=clr->g;
		img->source[(int)y][(int)x*3+2]=clr->r;
	}
	else
	{
		img->source[(int)y][(int)x] = (clr->b+clr->g+clr->r)/3;
	}
	for (i=0; i<steps; i++)
	{
		x+=xlnc;
		y+=ylnc;
		if(img->bpp==24)
		{
			img->source[(int)y][(int)x*3+0]=clr->b;
			img->source[(int)y][(int)x*3+1]=clr->g;
			img->source[(int)y][(int)x*3+2]=clr->r;
		}
		else
		{
			img->source[(int)y][(int)x] = (clr->b+clr->g+clr->r)/3;
		}
	}
	*/

	/* Bres의 방법 */
	int x1, y1, x2, y2;

	x1=p1.x;
	y1=p1.y;
	x2=p2.x;
	y2=p2.y;

	int dx=abs(x1-x2),dy=abs(y1-y2);
	int p=2*dy-dx;
	int twody=2*dy, twodydx=2*(dy-dx);
	int x,y,xend;


	if(x1>x2){
		x=x2;
		y=y2;
		xend=x1;
	}
	else {
		x=x1;
		y=y1;
		xend=x2;
	}

	if(img->bpp==24)
	{
		img->source[y][x*3+0]=clr[color_select].blue;
		img->source[y][x*3+1]=clr[color_select].green;
		img->source[y][x*3+2]=clr[color_select].red;

	}
	else
	{
		//img->source[y][x] = (clr->b+clr->g+clr->r)/3;
	}

	while(x<xend) {
		x++;
		if(p<0)
			p+=twody;
		else {
			y++;
			p+=twodydx;
		}

		if(img->bpp==24)
		{
			for(int think = -2 ; think < 2 ; think++)
			{
				img->source[y+think][x*3+0]=clr[color_select].blue;
				img->source[y+think][x*3+1]=clr[color_select].green;
				img->source[y+think][x*3+2]=clr[color_select].red;
			}
		}
		else
		{
			//img->source[y][x] = (clr->b+clr->g+clr->r)/3;
		}
	}
}