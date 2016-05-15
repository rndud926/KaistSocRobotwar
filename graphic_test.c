#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>
#include <time.h>
#include "amazon2_sdk.h"
#include "graphic_api.h"
#include "imgcore_robot.h"
#include "robot_protocol.h"
#include "robot_motion.h"
#include "uart_api.h"
#include "Motion.h"

IMAGE_D *rgb888;
U16* fpga_videodata;
int EnemyState[2]={0,};
int EnemyState_distance[2]={0,};
int flga_EnemyState=1;
int interval = 2;
int DEBUG;
int strategy;

void RGB565toRGB888_back(U16* rgb565_fpgaData, IMAGE_D *rgb888);
int FindHeight_back(IMAGE_D *rgb888, U16 *fpga_videodata, int y_start, int y_end);
int FindWidth_back(IMAGE_D *rgb888, U16 *fpga_videodata, int x_start, int x_end);
void RGB565toRGB888(U16* rgb565_fpgaData, IMAGE_D *rgb888);
int FindWidth(IMAGE_D *rgb888, U16 *fpga_videodata, int x_start, int x_end);
int FindHeight(IMAGE_D *rgb888, U16 *fpga_videodata, int y_start, int y_end);
int CheckHue(RGB_COLOR_D rgb);
int ImageAnalysis(ROBOT_D *robot);
int strategy2_KickRoll(ROBOT_D *robot);
int strategy3_rolling(ROBOT_D *robot);
int strategy4_OnlyKick(ROBOT_D *robot);
int RobotPosition(int x);
int RobotDistance(int dx);
void readframe(ROBOT_D *robot);
int AttackMotion(void);
int AttackMotionTime(int motion);
int getTime(void);
void readframe_back(ROBOT_D *robot);


#define AMAZON2_GRAPHIC_VERSION		"v0.3"

#define WIDTH 120
#define HEIGHT 180
#define d_max(x,y) ((x)>(y) ? x : y)
#define d_min(x,y) ((x)<(y) ? x : y)
#define d_min3(x,y,z) (d_min(d_min((x),(y)), (z)))
#define d_max3(x,y,z) (d_max(d_max((x),(y)), (z)))


// 16bit rgb separation
#define get_16r(pixel) (pixel>>11)
#define get_16g(pixel) ((pixel&0x7E0)>>5)
#define get_16b(pixel) (pixel&0x1F)

//red 및 blue component 에 대해
unsigned char table_5_to_8[32] =
{
	0, 8, 16, 24, 32, 41, 49, 57,
	65, 74, 82, 90, 98, 106, 115, 123,
	131, 139, 148, 156, 164, 172, 180, 189,
	197, 205, 213, 222, 230, 238, 246, 255,
};

//green component 에 대해
unsigned char table_6_to_8[64] =
{
	0, 4, 8, 12, 16, 20, 24, 28,
	32, 36, 40, 44, 48, 52, 56, 60,
	64, 68, 72, 76, 80, 85, 89, 93,
	97, 101, 105, 109, 113, 117, 121, 125,
	129, 133, 137, 141, 145, 149, 153, 157,
	161, 165, 170, 174, 178, 182, 186, 190,
	194, 198, 202, 206, 210, 214, 218, 222,
	226, 230, 234, 238, 242, 246, 250, 255
};

#include <termios.h>
static struct termios inittio, newtio;

void init_console(void)
{
    tcgetattr(0, &inittio);
    newtio = inittio;
    newtio.c_lflag &= ~ICANON;
    newtio.c_lflag &= ~ECHO;
    newtio.c_lflag &= ~ISIG;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;

    cfsetispeed(&newtio, B115200);

    tcsetattr(0, TCSANOW, &newtio);
}



int main(int argc, char **argv)
{
	int Loop = 1;
	int left, right, top, bottom=0;
	int ret;
	int NowTime = 0 ;
	DEBUG = atoi(argv[1]);
	RECT_D roi;
	RECT_D rect;
	ROBOT_D robot;
	roi.top = 0;
	roi.left = 35;
	roi.bottom = 120;
	roi.right = 145;
	strategy = atoi(argv[2]);
	printf("DEBUD : %d, strategy : %d\n", DEBUG, strategy);
	memset(&robot,0,sizeof(ROBOT_D));
	if (open_graphic() < 0) {
		return -1;
	}
	
	ret = uart_open();
	if (ret < 0) return EXIT_FAILURE;
	uart_config(UART1, 115200, 8, UART_PARNONE, 1);

	fpga_videodata = (U16*)malloc(180 * 120 * 2);

	rgb888 = SOCV_make_Image(WIDTH, HEIGHT, 24);

	init_robot();

	direct_camera_display_off();
	if(direct_camera_display_stat() > 0) {
		printf("direct camera display on\n");
		printf("please direct camera diplay off\n");
	}
	
	//토큰온 
	//인사
	//Motion(Hello);
	//usleep(time_Hello);
	
	if(DEBUG)
	{
		Motion(Hello2);
		usleep(time_Hello2);
	}
	//checkTime = pTmNow->tm_min*60+ pTmNow->tm_sec + 30;
	while(Loop)
	{
		clear_screen();
		read_fpga_video_data(fpga_videodata); // 프레임 정보 읽어옴 
		RGB565toRGB888(fpga_videodata, rgb888);

		left = FindWidth(rgb888, fpga_videodata, 45, 135);
		right = FindWidth(rgb888, fpga_videodata, 135, 45);
		top = FindHeight(rgb888, fpga_videodata , 30, 90);
		bottom = FindHeight(rgb888, fpga_videodata, 90, 30);
		
		//display 용
		rect.top = top;
		rect.bottom = bottom;
		rect.left = left;
		rect.right = right;
		
		robot.area = (right - left ) * ( bottom - top);
		robot.mid.x = (right + left) / 2;
		robot.mid.y = (bottom - top) / 2;
		robot.rect.top = top;
		robot.rect.bottom = bottom;
		robot.rect.left = left;
		robot.rect.right = right;
	
		if( strategy == BOXNOKICK || strategy == BOXKICK)
		{	// 난타전 킥 
			ret = ImageAnalysis(&robot);
		}
		else if( strategy == ONLYKICK) // 온리킥
		{
			ret = strategy4_OnlyKick(&robot);
		}
		else if( strategy == KICKROLL) // 발차기, 롤링
		{
			strategy2_KickRoll(&robot);
		}
		else if( strategy == BOXROOL) // 롤링 후 30초 난타전
		{
			if(robot.TimeFlag == 1)
			{
				NowTime = getTime();
				ret = ImageAnalysis(&robot);
				printf("TimeChecking... now : %d check : %d\n", NowTime, robot.CheckTime);
				if( robot.CheckTime < NowTime)	robot.TimeFlag = 0;

			}
			else ret = strategy3_rolling(&robot);

		}
		else
			return 0;

		SOCV_img_Draw(fpga_videodata,rect,1);
		SOCV_img_Draw(fpga_videodata,roi,1);
		
		draw_img_from_buffer(fpga_videodata, 320, 0, 0, 0, 2.67, 90);
		flip(); // 없애면 앙대영
	}
	
	uart_close();
	free(fpga_videodata);
	SOCV_release_Image(rgb888);
	close_graphic();
	return 0;
}


int CheckHue(RGB_COLOR_D rgb)
{
	U8 hue = 0;
	if( rgb.b < rgb.g )
	{
		unsigned char rgb_min, rgb_max;
		rgb_min = d_min3(rgb.r, rgb.g, rgb.b);
		rgb_max = d_max3(rgb.r, rgb.g, rgb.b);
		if (rgb_max == rgb.r) {
				hue = 0 + 43*(rgb.g - rgb.b)/(rgb_max - rgb_min);
		} else if (rgb_max == rgb.g) {
			hue = 85 + 43*(rgb.b - rgb.r)/(rgb_max - rgb_min);
		}
	
		if( hue > 65 && hue < 85 )	return 1;
		else return 0;
	}

	return 0;
}

int FindHeight(IMAGE_D *rgb888, U16 *fpga_videodata, int y_start, int y_end)
{
	int i,j;
	RGB_COLOR_D rgb;
	int count=0;
	int y_index = 0;
	int flag = 0;
	if( y_start < y_end) flag = 0;
	if( y_start > y_end) flag = 1;

	if(flag == 0)
	{
		for( i = 0 ; i < 90 ; i = i + interval)
		{
			for( j = 35 ; j < 145 ; j = j + interval)
			{
				rgb.b = rgb888->source[i][j*3+0];
				rgb.g = rgb888->source[i][j*3+1];
				rgb.r = rgb888->source[i][j*3+2];

				if(CheckHue(rgb))
				{	
					count++;
					//index = 180 * i + j;
					//fpga_videodata[index] = 0xf800;
				}
			}
		
			if( count > 3)
			{
				y_index = i;
				break;
			}
		
			count = 0;
			//rise = rise * 2;
		}
	}
	else if(flag == 1)
	{
		for( i = 90 ; i > 20 ; i = i - interval)
		{
			for( j = 35 ; j < 145 ; j = j + interval)
			{
				rgb.b = rgb888->source[i][j*3+0];
				rgb.g = rgb888->source[i][j*3+1];
				rgb.r = rgb888->source[i][j*3+2];

				if(CheckHue(rgb))
				{	
					count++;
					//index = 180 * i + j;
					//fpga_videodata[index] = 0xf800;
				}
			}
		
			if( count > 3)
			{
				y_index = i;
				break;
			}
		
			count = 0;
			//drop = drop * 2;
		}
	}
	/*
	for( i = 45; i < 135 ; i++)
	{
		index = 180 * y_index + i;
		fpga_videodata[index] = 0xf800;
	}
	*/
	return y_index;
}

int FindWidth(IMAGE_D *rgb888, U16 *fpga_videodata, int x_start, int x_end)
{
	int i,j;
	RGB_COLOR_D rgb;
	int count=0;
	int x_index = 0;
	int flag =0;
	if( x_start < x_end) flag = 0;
	else if( x_start > x_end) flag = 1;

	if(flag == 0)
	{
		for( i = 35 ; i < 145 ; i = i + interval )
		{
			for( j = 20 ; j < 90 ; j = j + interval )
			{
				
				rgb.b = rgb888->source[j][i*3+0];
				rgb.g = rgb888->source[j][i*3+1];
				rgb.r = rgb888->source[j][i*3+2];

				if(CheckHue(rgb))
				{	
					count++;
					//index = 180 * j + i;
					//fpga_videodata[index] = 0xf800;
				}
			}
			
			if( count > 3)
			{
				x_index = i;
				break;
			}
			//printf("i : %d\n",i);
			count = 0;
			//rise = rise * 2;
		}
	}
	else if(flag == 1)
	{
		for( i = 145 ; i > 35 ; i = i - interval )
		{
			for( j = 20 ; j < 90 ; j = j + interval )
			{
				rgb.b = rgb888->source[j][i*3+0];
				rgb.g = rgb888->source[j][i*3+1];
				rgb.r = rgb888->source[j][i*3+2];

				if(CheckHue(rgb))
				{	
					count++;
					//index = 180 * j + i;
					//fpga_videodata[index] = 0xf800;
				}
			}
		
			if( count > 3 )
			{
				x_index = i;
				break;
			}
		
			count = 0;
			//drop = drop * 2;
		}
	}

	/*
	for( i = 30; i < 90 ; i++)
	{
		index = 180 * i + x_index;
		fpga_videodata[index] = 0xf800;
	}
	*/
	return x_index;
}

void RGB565toRGB888(U16* rgb565_fpgaData, IMAGE_D *rgb888)
{

	int i,j;
	int index;
	U16 pixel;
	/* 20 - 90  */
	for( i = 0 ; i < 90 ; i++)
	{
		for( j = 35 ; j < 145 ; j++)
		{
			index = j + 180 * i;
			pixel = rgb565_fpgaData[index];
			rgb888->source[i][j*3+0] = table_5_to_8[get_16b(pixel)];
			rgb888->source[i][j*3+1] = table_6_to_8[get_16g(pixel)];
			rgb888->source[i][j*3+2] = table_5_to_8[get_16r(pixel)];
		}
	}
}

int ImageAnalysis(ROBOT_D *robot)
{
	
	// 1. 위치 거리 판단
	int x = robot->mid.x;
	int y = robot->mid.y;
	int position=0;
	int distance=0;
	int top = robot->rect.top;
	int left = robot->rect.left;
	int right = robot->rect.right;
	int bottom = robot->rect.bottom;
	int dx = right - left;
	int dy = bottom - top;
	int attack = 0;
	int attack_time = 0;
	printf("x : %d y : %d dx : %d dy : %d\n", x,y, dx, dy);

	// 위치
	if( x != 0 && y != 0)
	{
		position = RobotPosition(x);
		if(position < 0 ) return -1;

		distance = RobotDistance(dx);
		if(distance < 0) return -1;

		EnemyState[0] = EnemyState[1];
		EnemyState[1] = position;

		EnemyState_distance[0] = EnemyState_distance[1];
		EnemyState_distance[1] = distance;
	}
	else 
	{
		printf("상대가없어부럿어.,.\n");
		if( EnemyState[0] == LEFT || EnemyState[0] == MORELEFT)
		{
			if(DEBUG)
			{
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					printf("머리돌려서 찾았다\n");
					Motion(Rturn30);
					usleep(time_Rturn30);
					Motion(Rturn10);
					usleep(time_Rturn10);

					return 1;
				}
				else robot->disapperLeft = 1;
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					printf("머리돌려서 찾았다\n");
					Motion(Lturn30);
					usleep(time_Lturn30);
					Motion(Lturn10);
					usleep(time_Lturn10);
					robot->disapperLeft = 0;
					return 1;
				}
				else robot->disapperRight = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch1);
						usleep(time_FGpunch1);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
					}
				}
			}
		}
		else if( EnemyState[0] == RIGHT ||  EnemyState[0] == MORERIGHT )
		{
			if(DEBUG)
			{
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					Motion(Lturn30);
					usleep(time_Lturn30);
					Motion(Lturn10);
					usleep(time_Lturn10);
					return 1;
				}
				else robot->disapperRight = 1;
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left && robot->rect.right)
				{
					Motion(Rturn30);
					usleep(time_Rturn30);
					Motion(Rturn10);
					usleep(time_Rturn10);
					robot->disapperRight = 0;
					return 1;
				}
				else robot->disapperLeft = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch1);
						usleep(time_FGpunch1);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
					}
				}
			}
		}
		else if(EnemyState[0] == MID)
		{
			if(DEBUG)
			{
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					Motion(Lturn30);
					usleep(time_Lturn30);
					Motion(Lturn10);
					usleep(time_Lturn10);
					return 1;
				}
				else robot->disapperRight = 1;
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left && robot->rect.right)
				{
					Motion(Rturn30);
					usleep(time_Rturn30);
					Motion(Rturn10);
					usleep(time_Rturn10);
					robot->disapperRight = 0;
					return 1;
				}
				else robot->disapperLeft = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch1);
						usleep(time_FGpunch1);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
					}
				}
			}
		}
		else
		{
			Motion(Front1);
			usleep(time_Front1);
		}
	}

	printf("position :%d distance %d\n", position, distance);
	printf("kick : %d\n",robot->MidAttack);
	if( position == MORELEFT)
	{
		Motion(Rturn30);
		usleep(time_Rturn30);
	}
	else if( position == MORERIGHT)
	{
		Motion(Lturn30);
		usleep(time_Lturn30);

	}
	else if( position == LEFT && distance == DISTANCE30)
	{
		printf("[모션] 왼쪽 먼거리\n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE30)
	{
		printf("[모션] 중간 먼거리 \n");
		if(DEBUG)
		{
			Motion(Front5);
			usleep(time_Front5);
		}
	}
	else if( position == RIGHT && distance == DISTANCE30)
	{
		printf("[모션] 오른쪽 먼거리 \n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE50)
	{
		printf("[모션] 왼쪽 중간 or 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE50)
	{
		printf("[모션] 중간 중간거리 \n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);
		}
	}
	else if( position == RIGHT && distance == DISTANCE50)
	{
		printf("[모션] 오른쪽 중간 or 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE70)
	{
		printf("[모션] 왼쪽 가까운거리\n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);
		}
	}
	else if( position == MID && distance == DISTANCE70)
	{
		printf("[모션] 중간인데 가까운거리\n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);
			robot->MidAttack++; 
			if( strategy == BOXNOKICK)
			{
				//////////////////
				if(robot->MidAttack > 3)
				{
					Motion(Rturn30);
					usleep(time_Rturn10);
					Motion(Rturn30);
					usleep(time_Rturn10);
					robot->MidAttack = 0;
					Motion(Skick);
					usleep(time_Skick);
					printf("dkjdqwkjwkjj kc!!\n");	
				}
			}
			
		}
	}
	else if( position == RIGHT && distance == DISTANCE70)
	{
		printf("[모션] 오른쪽인데 가까운거리\n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);
		}
	}
	else if( position == LEFT && distance == DISTANCE100)
	{
		printf("[모션] 왼쪽인데 개가까거리@@@@\n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);

		}
	}
	else if( position == MID && distance == DISTANCE100)
	{
		printf("[모션] 중간인데 개가까거리\n");
		if(DEBUG)
		{
			robot->MidAttack++;
			if( strategy == BOXNOKICK)
			{
				if(robot->MidAttack > 3)
				{
					Motion(Rturn30);
					usleep(time_Rturn10);
					
					Motion(Rturn30);
					usleep(time_Rturn10);
					robot->MidAttack = 0;
					Motion(Skick);
					usleep(time_Skick);
					printf("dkjdqwkjwkjj kc!!\n");	
					/*
					readframe(robot);
					if(robot->position == MID && robot->distance >= DISTANCE70)
					{
						Motion(Skick);
						usleep(time_Skick);
						robot->MidAttack = 0;
					}
					else if(robot->position == MID && robot->distance == DISTANCE50)
					{
						Motion(Skick);
						usleep(time_Skick);
						robot->MidAttack = 0;
					}
					*/
				}
			}
			else
			{
				attack = AttackMotion();
				attack_time = AttackMotionTime(attack);
				Motion(attack);
				usleep(attack_time);
			}
		}
	}
	else if( position == RIGHT && distance == DISTANCE100)
	{
		printf("[모션] 오른쪽인데 개가까움\n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);
		}
	}
	else
	{
		Motion(Front1);
		usleep(time_Front1);
	}

	return 1;
}

int strategy2_KickRoll(ROBOT_D *robot)
{
	// 1. 위치 거리 판단
	int x = robot->mid.x;
	int y = robot->mid.y;
	int position=0;
	int distance=0;
	int top = robot->rect.top;
	int left = robot->rect.left;
	int right = robot->rect.right;
	int bottom = robot->rect.bottom;
	int dx = right - left;
	int dy = bottom - top;
	int attack =0;
	int attack_time =0;
	printf("x : %d y : %d dx : %d dy : %d\n", x,y, dx, dy);

	if( x != 0 && y != 0)
	{
		position = RobotPosition(x);
		if(position < 0 ) return -1;

		distance = RobotDistance(dx);
		if(distance < 0) return -1;

		EnemyState[0] = EnemyState[1];
		EnemyState[1] = position;

		EnemyState_distance[0] = EnemyState_distance[1];
		EnemyState_distance[1] = distance;
	}
	else 
	{
		printf("상대가없어부럿어.,.\n");
		if( EnemyState[0] == LEFT || EnemyState[0] == MORELEFT )
		{
			if(DEBUG)
			{
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					printf("머리돌려서 찾았다\n");
					Motion(Rturn30);
					usleep(time_Rturn30);
					return 1;
				}
				else robot->disapperLeft = 1;
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					printf("머리돌려서 찾았다\n");
					Motion(Lturn30);
					usleep(time_Lturn30);
					robot->disapperLeft = 0;
					return 1;
				}
				else robot->disapperRight = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch6);
						usleep(time_FGpunch6);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}

				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
		else if( EnemyState[0] == RIGHT || EnemyState[0] == MORERIGHT )
		{
			if(DEBUG)
			{
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					Motion(Lturn30);
					usleep(time_Lturn30);
					return 1;
				}
				else robot->disapperRight = 1;
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left && robot->rect.right)
				{
					Motion(Rturn30);
					usleep(time_Rturn30);
					robot->disapperRight = 0;
					return 1;
				}
				else robot->disapperLeft = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch6);
						usleep(time_FGpunch6);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}
				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
		else if(EnemyState[0] == MID)
		{
			if(DEBUG)
			{
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					Motion(Lturn30);
					usleep(time_Lturn30);
					Motion(Lturn10);
					usleep(time_Lturn10);
					return 1;
				}
				else robot->disapperRight = 1;
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left && robot->rect.right)
				{
					Motion(Rturn30);
					usleep(time_Rturn30);
					Motion(Rturn10);
					usleep(time_Rturn10);
					robot->disapperRight = 0;
					return 1;
				}
				else robot->disapperLeft = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch1);
						usleep(time_FGpunch1);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}
				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
	}

	if( position == MORELEFT)
	{
		Motion(Rturn30);
		usleep(time_Rturn30);
	}
	else if( position == MORERIGHT)
	{
		Motion(Lturn30);
		usleep(time_Lturn30);

	}
	else if( position == LEFT && distance == DISTANCE30)
	{
		printf("[모션] 왼쪽 먼거리\n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE30)
	{
		printf("[모션] 중간 먼거리 \n");
		if(DEBUG)
		{
			Motion(Front1);
			usleep(time_Front1);
		}
	}
	else if( position == RIGHT && distance == DISTANCE30)
	{
		printf("[모션] 오른쪽 먼거리 \n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE50)
	{
		printf("[모션] 왼쪽 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE50)
	{
		printf("[모션] 중간 중간거리 \n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);
		}
	}
	else if( position == RIGHT && distance == DISTANCE50)
	{
		printf("[모션] 오른쪽 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE70)
	{
		printf("[모션] 왼쪽 가까운거리\n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE70)
	{
		printf("[모션] 중간인데 가까운거리\n");
		if(DEBUG)
		{
			attack = AttackMotion();
			attack_time = AttackMotionTime(attack);
			Motion(attack);
			usleep(attack_time);
			robot->MidAttack++;
			if(robot->MidAttack >= 5)
			{
				Motion(backSkick);
				usleep(time_backSkick);
				robot->MidAttack = 0;
				usleep(2000000);
			}
		}
	}
	else if( position == RIGHT && distance == DISTANCE70)
	{
		printf("[모션] 오른쪽인데 가까운거리\n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE100)
	{
		printf("[모션] 왼쪽인데 개가까거리\n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE100)
	{
		printf("[모션] 중간인데 개가까거리\n");
		if(DEBUG)
		{
			srand((unsigned)time(NULL));
			int attack = rand()%2;
			if(attack)
			{
				attack = AttackMotion();
				attack_time = AttackMotionTime(attack);
				Motion(attack);
				usleep(attack_time);
				robot->MidAttack++;
				if(robot->MidAttack >= 5)
				{
					Motion(backSkick);
					usleep(time_backSkick);
					robot->MidAttack = 0;
					usleep(2000000);
				}
			}
			else
			{
				Motion(Rready);
				usleep(time_Rready);
				readframe(robot);
				printf("left : %d right : %d \n", robot->rect.left, robot->rect.right);
				if(robot->rect.left > 0 && robot->rect.right > 0 )
				{
					printf("쌍편치\n");
					Motion(FGpunch6);
					usleep(time_FGpunch6);
					Motion(head90);
					usleep(time_head90);
					readframe(robot);
					printf("머가리 돌림\n");
					if(robot->position == RIGHT && robot->distance > 69)
					{
						Motion(Rkick);
						usleep(time_Rkick);
						usleep(2000000);
					}
					else	return 1;
			
				}
				else
				{
					Motion(head90);
					usleep(time_head90);
					readframe(robot);
					printf("머가리 돌림\n");
					if(robot->position == RIGHT)
					{
						Motion(Rkick);
						usleep(time_Rkick);
						usleep(2000000);
					}
					else return 1;
				}
			}
		}
	}
	else if( position == RIGHT && distance == DISTANCE100)
	{
		printf("[모션] 오른쪽인데 개가까거리\n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else
	{
		Motion(Back1);
		usleep(time_Back1);
	}

	return 1;
}

int strategy3_rolling(ROBOT_D *robot)
{
	// 1. 위치 거리 판단
	int x = robot->mid.x;
	int y = robot->mid.y;
	int position=0;
	int distance=0;
	int top = robot->rect.top;
	int left = robot->rect.left;
	int right = robot->rect.right;
	int bottom = robot->rect.bottom;
	int dx = right - left;
	int dy = bottom - top;
	printf("x : %d y : %d dx : %d dy : %d\n", x,y, dx, dy);

	if( x != 0 && y != 0)
	{
		position = RobotPosition(x);
		if(position < 0 ) return -1;

		distance = RobotDistance(dx);
		if(distance < 0) return -1;

		EnemyState[0] = EnemyState[1];
		EnemyState[1] = position;
	}
	else 
	{
		if( EnemyState[0] == LEFT || EnemyState[0] == MORELEFT )
		{
			if(DEBUG)
			{
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					printf("머리돌려서 찾았다\n");
					Motion(Rturn30);
					usleep(time_Rturn30);
					Motion(Rturn10);
					usleep(time_Rturn10);

					return 1;
				}
				else robot->disapperLeft = 1;
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					printf("머리돌려서 찾았다\n");
					Motion(Lturn30);
					usleep(time_Lturn30);
					Motion(Lturn10);
					usleep(time_Lturn10);
					robot->disapperLeft = 0;
					return 1;
				}
				else robot->disapperRight = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch1);
						usleep(time_FGpunch1);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}

				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
		else if( EnemyState[0] == RIGHT || EnemyState[0] == MORERIGHT )
		{
			if(DEBUG)
			{
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					Motion(Lturn30);
					usleep(time_Lturn30);
					Motion(Lturn10);
					usleep(time_Lturn10);
					return 1;
				}
				else robot->disapperRight = 1;
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe_back(robot);
				if(robot->rect.left && robot->rect.right)
				{
					Motion(Rturn30);
					usleep(time_Rturn30);
					Motion(Rturn10);
					usleep(time_Rturn10);
					robot->disapperRight = 0;
					return 1;
				}
				else robot->disapperLeft = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch1);
						usleep(time_FGpunch1);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}
				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
		else if(EnemyState[0] == MID)
		{
			if(DEBUG)
			{
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					Motion(Lturn30);
					usleep(time_Lturn30);
					Motion(Lturn10);
					usleep(time_Lturn10);
					return 1;
				}
				else robot->disapperRight = 1;
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				if(robot->rect.left && robot->rect.right)
				{
					Motion(Rturn30);
					usleep(time_Rturn30);
					Motion(Rturn10);
					usleep(time_Rturn10);
					robot->disapperRight = 0;
					return 1;
				}
				else robot->disapperLeft = 1;
				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						Motion(FGpunch1);
						usleep(time_FGpunch1);
						robot->disapperLeft = 0;
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}
				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
		else
		{
			Motion(Front1);
			usleep(time_Front1);
		}
	}

	//모션
	if( position == MORELEFT)
	{
		Motion(Rturn30);
		usleep(time_Rturn30);
	}
	else if( position == MORERIGHT)
	{
		Motion(Lturn30);
		usleep(time_Lturn30);

	}
	else if( position == LEFT && distance == DISTANCE30)
	{
		printf("[모션] 왼쪽 먼거리\n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE30)
	{
		printf("[모션] 중간 먼거리 \n");
		if(DEBUG)
		{
			Motion(Front1);
			usleep(time_Front1);
		}
	}
	else if( position == RIGHT && distance == DISTANCE30)
	{
		printf("[모션] 오른쪽 먼거리 \n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE50)
	{
		printf("[모션] 왼쪽 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE50)
	{
		printf("[모션] 중간 중간거리 \n");
		if(DEBUG)
		{
			Motion(Rready);
			usleep(time_Rready);
			readframe(robot);
			printf("left : %d right : %d \n", robot->rect.left, robot->rect.right);
			if(robot->rect.left > 0 && robot->rect.right > 0 )
			{
				printf("쌍편치\n");
				Motion(FGpunch6);
				usleep(time_FGpunch6);
				Motion(head90);
				usleep(time_head90);
				readframe(robot);
				printf("머가리 돌림\n");
				if(robot->position == RIGHT)
				{
					Motion(Rkick);
					usleep(time_Rkick);
					usleep(2000000);
				}
				else
				{
					robot->TimeFlag = 1;
					robot->CheckTime = getTime()+30;
					return 1;
				}
			}
			else
			{
				Motion(head90);
				usleep(time_head90);
				readframe(robot);
				printf("머가리 돌림\n");
				if(robot->position == RIGHT)
				{
					Motion(Rkick);
					usleep(time_Rkick);
					usleep(2000000);
				}
				else return 1;
			}
		}
	}
	else if( position == RIGHT && distance == DISTANCE50)
	{
		printf("[모션] 오른쪽 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE70)
	{
		printf("[모션] 왼쪽 가까운거리\n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE70)
	{
		printf("[모션] 중간인데 가까운거리\n");
		if(DEBUG)
		{
			Motion(Rready);
			usleep(time_Rready);
			readframe(robot);
			printf("left : %d right : %d \n", robot->rect.left, robot->rect.right);
			if(robot->rect.left > 0 && robot->rect.right > 0 )
			{
				printf("쌍편치\n");
				Motion(FGpunch6);
				usleep(time_FGpunch6);
				Motion(head90);
				usleep(time_head90);
				readframe(robot);
				printf("머가리 돌림\n");
				if(robot->position == RIGHT)
				{
					Motion(Rkick);
					usleep(time_Rkick);
					usleep(2000000);
				}
				else
				{
					robot->TimeFlag = 1;
					robot->CheckTime = getTime()+30;
					return 1;
				}
			}
			else
			{
				Motion(head90);
				usleep(time_head90);
				readframe(robot);
				printf("머가리 돌림\n");
				if(robot->position == RIGHT)
				{
					Motion(Rkick);
					usleep(time_Rkick);
					usleep(2000000);
				}
				else return 1;
			}
		}
	}
	else if( position == RIGHT && distance == DISTANCE70)
	{
		printf("[모션] 오른쪽인데 가까운거리\n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE100)
	{
		printf("[모션] 왼쪽인데 개가까거리\n");
		if(DEBUG)

		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE100)
	{
		printf("[모션] 중간인데 개가까거리\n");
		if(DEBUG)
		{
			Motion(Back1);
			usleep(time_Back1);
		}
	}
	else if( position == RIGHT && distance == DISTANCE100)
	{
		printf("[모션] 오른쪽인데 개가까움\n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else
	{
		Motion(Back1);
		usleep(time_Back1);
	}

	return 0;
}

int RobotPosition(int x) // x -> 중점 
{
	if( 35 <= x && x <= 45)
	{
		printf("[위치] 왼쪽끝\n");
		return MORELEFT;
	}
	else if( 45 <= x && x <= 75)
	{	
		printf("[위치] 왼쪽\n");
		return LEFT;
	}
	else if( 75 <= x && x <= 105)
	{
		printf("[위치] 중간\n");
		return MID;
	}
	else if( 105 <= x && x <= 135)
	{
		printf("[위치] 오른쪽\n");
		return RIGHT;
	}
	else if( 135 <= x && x <= 145 )
	{
		printf("[위치] 오른쪽끝\n");
		return MORERIGHT;
	}
	else return -1;
}

int RobotDistance(int dx) // -> dx 
{
	// 거리
	if( 0 <= dx && dx <= 30)
	{
		printf("[거리] 멀다\n");
		return DISTANCE30;
	}
	else if( dx <= 50)
	{
		printf("[거리] 중간\n");
		return DISTANCE50;
	}
	else if( dx <= 70)
	{
		printf("[거리] 가까움\n");
		return DISTANCE70;
	}
	else if( dx <= 120)
	{
		printf("[거리] 개가까움\n");
		return DISTANCE100;
	}
	else return -1;
	
}

void readframe(ROBOT_D *robot)
{
	int left, right, top, bottom;
	int i,j;
	int hue_count = 0;
	int dx;
	clear_screen();
	read_fpga_video_data(fpga_videodata); // 프레임 정보 읽어옴 
	RGB565toRGB888(fpga_videodata, rgb888);

	left = FindWidth(rgb888, fpga_videodata, 45, 135);
	right = FindWidth(rgb888, fpga_videodata, 135, 45);
	top = FindHeight(rgb888, fpga_videodata , 30, 90);
	bottom = FindHeight(rgb888, fpga_videodata, 90, 30);

	robot->area = (right - left ) * ( bottom - top);
	robot->mid.x = (right + left) / 2;
	robot->mid.y = (bottom - top) / 2;
	robot->rect.top = top;
	robot->rect.bottom = bottom;
	robot->rect.left = left;
	robot->rect.right = right;
	
	if(left > 0 && right > 0 && top > 0 && bottom > 0)
	{
		dx = right - left;
		robot->position = RobotPosition(robot->mid.x);
		robot->distance = RobotDistance(dx);
		/*
		for( i = top ; i < bottom ; i++)
		{
			for( j = left ; j < right ; j++)
			{
				RGB_COLOR_D rgb;
				rgb.b = rgb888->source[i][j*3+0];
				rgb.g = rgb888->source[i][j*3+1];
				rgb.r = rgb888->source[i][j*3+2];

				if(CheckHue(rgb)) hue_count++;

			}
		}
		*/

		printf("position : %d distance : %d hue_count : %d \n", robot->position, robot->distance,hue_count);
	}

	printf("[readframe]top : %d left : %d bottom : %d right : %d \n", top, left, bottom, right);
	//SOCV_img_Draw(fpga_videodata,robot->rect,1);
	draw_img_from_buffer(fpga_videodata, 320, 0, 0, 0, 2.67, 90);
	//draw_fpga_video_data(fpga_videodata, 50, 50);
	flip();
}

int AttackMotion(void)
{
	srand((unsigned)time(NULL));
	int attack = rand()%2;
	//printf("Attack : %d\n",attack);
	int attack_motion =0;
	switch(attack)
	{
	case 0:
		attack_motion = FGpunch1;
		break;
	case 1:
		attack_motion = Fpunch;
		break;
	default:
		break;
	}
	return attack_motion;
}

int AttackMotionTime(int motion)
{
	int time = 0;
	switch(motion)
	{
	case FGpunch:
		time = time_FGpunch;
		break;
	case Wave_Punch:
		time = time_Wave_Punch;
		break;
	case Push2:
		time = time_Swing_punch;
		break;
	case FGpunch1:
		time = time_FGpunch1;
		break;
	case Fpunch:
		time = time_Fpunch;
		break;
	default:
		break;
	}

	return time;
}

int getTime(void)
{
	time_t nowTime = time(NULL);
	struct tm * pTmNow = localtime(&nowTime);
	return pTmNow->tm_min*60+ pTmNow->tm_sec;
}

int strategy4_OnlyKick(ROBOT_D *robot)
{
	// 1. 위치 거리 판단
	int x = robot->mid.x;
	int y = robot->mid.y;
	int position=0;
	int distance=0;
	int top = robot->rect.top;
	int left = robot->rect.left;
	int right = robot->rect.right;
	int bottom = robot->rect.bottom;
	int dx = right - left;
	int dy = bottom - top;
	printf("x : %d y : %d dx : %d dy : %d\n", x,y, dx, dy);

	if( x != 0 && y != 0)
	{
		position = RobotPosition(x);
		if(position < 0 ) return -1;

		distance = RobotDistance(dx);
		if(distance < 0) return -1;

		EnemyState[0] = EnemyState[1];
		EnemyState[1] = position;
	}
	else 
	{
		if( EnemyState[0] == LEFT || EnemyState[0] == MORELEFT )
		{
			if(DEBUG)
			{
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					if(robot->position == MID && robot->distance > DISTANCE50)
					{
						Motion(Rturn30);
						usleep(time_Rturn30);
						Motion(Back1);
						usleep(time_Back1);
						Motion(Skick);
						usleep(time_Skick);
						robot->disapperLeft = 0;
						return 1;
					}
					else
					{
						printf("머리돌려서 찾았다\n");
						Motion(Rturn30);
						usleep(time_Rturn30);
						Motion(Rturn10);
						usleep(time_Rturn10);
					}
					return 1;
				}
				else robot->disapperLeft = 1;

				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					if(robot->position == MID && robot->distance > DISTANCE50)
					{
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Back1);
						usleep(time_Back1);
						Motion(Skick);
						usleep(time_Skick);
						robot->disapperLeft = 0;
						return 1;
					}
					else
					{
						printf("머리돌려서 찾았다\n");
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn10);
						usleep(time_Lturn10);
						robot->disapperLeft = 0;
						return 1;
					}
				}
				else robot->disapperRight = 1;

				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						if(robot->position == MID && robot->distance > DISTANCE50)
						{
							Motion(Back1);
							usleep(time_Back1);
							Motion(Skick);
							usleep(time_Skick);
							robot->disapperLeft = 0;
							robot->disapperRight = 0;
							return 1;
						}
						else
						{
							Motion(FGpunch1);
							usleep(time_FGpunch1);
							robot->disapperLeft = 0;
							robot->disapperRight = 0;
						}
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}

				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
			
		}
		else if( EnemyState[0] == RIGHT || EnemyState[0] == MORERIGHT )
		{
			if(DEBUG)
			{
				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					if(robot->position == MID && robot->distance > DISTANCE50)
					{
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Back1);
						usleep(time_Back1);
						Motion(Skick);
						usleep(time_Skick);
						robot->disapperRight = 0;
						return 1;
					}
					else
					{
						printf("머리돌려서 찾았다\n");
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn10);
						usleep(time_Lturn10);
					}
					return 1;
				}
				else robot->disapperRight = 1;

				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					if(robot->position == MID && robot->distance > DISTANCE50)
					{
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Back1);
						usleep(time_Back1);
						Motion(Skick);
						usleep(time_Skick);
						robot->disapperLeft = 0;
						return 1;
					}
					else
					{
						printf("머리돌려서 찾았다\n");
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn10);
						usleep(time_Lturn10);
						robot->disapperLeft = 0;
						return 1;
					}
				}
				else robot->disapperRight = 1;

				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						if(robot->position == MID && robot->distance > DISTANCE50)
						{
							Motion(Back1);
							usleep(time_Back1);
							Motion(Skick);
							usleep(time_Skick);
							robot->disapperLeft = 0;
							robot->disapperRight = 0;
							return 1;
						}
						else
						{
							Motion(FGpunch1);
							usleep(time_FGpunch1);
							robot->disapperLeft = 0;
							robot->disapperRight = 0;
						}
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}

				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
		else if(EnemyState[0] == MID)
		{
			if(DEBUG)
			{
				Motion(Lhead45);
				usleep(time_Lhead45);
				readframe(robot);
				
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					if(robot->position == MID && robot->distance > DISTANCE50)
					{
						Motion(Rturn30);
						usleep(time_Rturn30);
						Motion(Back1);
						usleep(time_Back1);
						Motion(Skick);
						usleep(time_Skick);
						robot->disapperLeft = 0;
						return 1;
					}
					else
					{
						printf("머리돌려서 찾았다\n");
						Motion(Rturn30);
						usleep(time_Rturn30);
						Motion(Rturn10);
						usleep(time_Rturn10);
					}
					return 1;
				}
				else robot->disapperLeft = 1;

				Motion(Rhead45);
				usleep(time_Rhead45);
				readframe_back(robot);
				if(robot->rect.left > 0 && robot->rect.right > 0)
				{
					if(robot->position == MID && robot->distance > DISTANCE50)
					{
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Back1);
						usleep(time_Back1);
						Motion(Skick);
						usleep(time_Skick);
						robot->disapperLeft = 0;
						return 1;
					}
					else
					{
						printf("머리돌려서 찾았다\n");
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn10);
						usleep(time_Lturn10);
						robot->disapperLeft = 0;
						return 1;
					}
				}
				else robot->disapperRight = 1;

				if( robot->disapperLeft && robot->disapperRight)
				{
					Motion(head0);
					usleep(time_head0);
					readframe(robot);
					if(robot->rect.left > 0 && robot->rect.right > 0)
					{
						if(robot->position == MID && robot->distance > DISTANCE50)
						{
							Motion(Lturn30);
							usleep(time_Lturn30);
							Motion(Back1);
							usleep(time_Back1);
							Motion(Skick);
							usleep(time_Skick);
							robot->disapperLeft = 0;
							return 1;
						}
						else
						{
							Motion(FGpunch1);
							usleep(time_FGpunch1);
							robot->disapperLeft = 0;
							robot->disapperRight = 0;
						}
						return 1;
					}
					else
					{
						Motion(Front5);
						usleep(time_Front5);
						Motion(Front5);
						usleep(time_Front5);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
						Motion(Lturn30);
						usleep(time_Lturn30);
					}
				}

				robot->disapperLeft = 0;
				robot->disapperRight = 0;
			}
		}
	}

	//모션
	if( position == MORELEFT)
	{
		Motion(Rturn30);
		usleep(time_Rturn30);
	}
	else if( position == MORERIGHT)
	{
		Motion(Lturn30);
		usleep(time_Lturn30);

	}
	else if( position == LEFT && distance == DISTANCE30)
	{
		printf("[모션] 왼쪽 먼거리\n");
		if(DEBUG)
		{
			Motion(Rturn30);
			usleep(time_Rturn30);
		}
	}
	else if( position == MID && distance == DISTANCE30)
	{
		printf("[모션] 중간 먼거리 \n");
		if(DEBUG)
		{
			Motion(Front5);
			usleep(time_Front5);
		}
	}
	else if( position == RIGHT && distance == DISTANCE30)
	{
		printf("[모션] 오른쪽 먼거리 \n");
		if(DEBUG)
		{
			Motion(Lturn30);
			usleep(time_Lturn30);
		}
	}
	else if( position == LEFT && distance == DISTANCE50)
	{
		printf("[모션] 왼쪽 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE50)
	{
		printf("[모션] 중간 중간거리 \n");
		if(DEBUG)
		{
			Motion(Front1);
			usleep(time_Front1);
		}
	}
	else if( position == RIGHT && distance == DISTANCE50)
	{
		printf("[모션] 오른쪽 가까운거리 \n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE70)
	{
		printf("[모션] 왼쪽 가까운거리\n");
		if(DEBUG)
		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE70)
	{
		printf("[모션] 중간인데 가까운거리\n");
		if(DEBUG)
		{
			Motion(Skick);
			usleep(time_Skick);
		}
	}
	else if( position == RIGHT && distance == DISTANCE70)
	{
		printf("[모션] 오른쪽인데 가까운거리\n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}
	else if( position == LEFT && distance == DISTANCE100)
	{
		printf("[모션] 왼쪽인데 개가까거리\n");
		if(DEBUG)

		{
			Motion(Rturn10);
			usleep(time_Rturn10);
		}
	}
	else if( position == MID && distance == DISTANCE100)
	{
		printf("[모션] 중간인데 개가까거리\n");
		if(DEBUG)
		{
			Motion(Back1);
			usleep(time_Back1);
		}
	}
	else if( position == RIGHT && distance == DISTANCE100)
	{
		printf("[모션] 오른쪽인데 개가까움\n");
		if(DEBUG)
		{
			Motion(Lturn10);
			usleep(time_Lturn10);
		}
	}

	return 0;
}

void readframe_back(ROBOT_D *robot)
{
	int left, right, top, bottom;
	int i,j;
	int hue_count = 0;
	int dx;
	clear_screen();
	read_fpga_video_data(fpga_videodata); // 프레임 정보 읽어옴 
	RGB565toRGB888_back(fpga_videodata, rgb888);

	left = FindWidth_back(rgb888, fpga_videodata, 45, 135);
	right = FindWidth_back(rgb888, fpga_videodata, 135, 45);
	top = FindHeight_back(rgb888, fpga_videodata , 30, 90);
	bottom = FindHeight_back(rgb888, fpga_videodata, 90, 30);

	robot->area = (right - left ) * ( bottom - top);
	robot->mid.x = (right + left) / 2;
	robot->mid.y = (bottom - top) / 2;
	robot->rect.top = top;
	robot->rect.bottom = bottom;
	robot->rect.left = left;
	robot->rect.right = right;
	
	if(left > 0 && right > 0 && top > 0 && bottom > 0)
	{
		dx = right - left;
		robot->position = RobotPosition(robot->mid.x);
		robot->distance = RobotDistance(dx);
		/*
		for( i = top ; i < bottom ; i++)
		{
			for( j = left ; j < right ; j++)
			{
				RGB_COLOR_D rgb;
				rgb.b = rgb888->source[i][j*3+0];
				rgb.g = rgb888->source[i][j*3+1];
				rgb.r = rgb888->source[i][j*3+2];

				if(CheckHue(rgb)) hue_count++;

			}
		}
		*/

		printf("position : %d distance : %d hue_count : %d \n", robot->position, robot->distance,hue_count);
	}

	printf("[readframe]top : %d left : %d bottom : %d right : %d \n", top, left, bottom, right);
	//SOCV_img_Draw(fpga_videodata,robot->rect,1);
	draw_img_from_buffer(fpga_videodata, 320, 0, 0, 0, 2.67, 90);
	//draw_fpga_video_data(fpga_videodata, 50, 50);
	flip();
}

int FindHeight_back(IMAGE_D *rgb888, U16 *fpga_videodata, int y_start, int y_end)
{
	int i,j;
	RGB_COLOR_D rgb;
	int count=0;
	int y_index = 0;
	int flag = 0;
	if( y_start < y_end) flag = 0;
	if( y_start > y_end) flag = 1;

	if(flag == 0)
	{
		for( i = 40 ; i < 120 ; i = i + interval)
		{
			for( j = 35 ; j < 145 ; j = j + interval)
			{
				rgb.b = rgb888->source[i][j*3+0];
				rgb.g = rgb888->source[i][j*3+1];
				rgb.r = rgb888->source[i][j*3+2];

				if(CheckHue(rgb))
				{	
					count++;
				}
			}
		
			if( count > 3)
			{
				y_index = i;
				break;
			}
		
			count = 0;
			//rise = rise * 2;
		}
	}
	else if(flag == 1)
	{
		for( i = 120 ; i > 40 ; i = i - interval)
		{
			for( j = 35 ; j < 145 ; j = j + interval)
			{
				rgb.b = rgb888->source[i][j*3+0];
				rgb.g = rgb888->source[i][j*3+1];
				rgb.r = rgb888->source[i][j*3+2];

				if(CheckHue(rgb))
				{	
					count++;
				}
			}
		
			if( count > 3)
			{
				y_index = i;
				break;
			}
		
			count = 0;
		}
	}

	return y_index;
}

int FindWidth_back(IMAGE_D *rgb888, U16 *fpga_videodata, int x_start, int x_end)
{
	int i,j;
	RGB_COLOR_D rgb;
	int count=0;
	int x_index = 0;
	int flag =0;
	if( x_start < x_end) flag = 0;
	else if( x_start > x_end) flag = 1;

	if(flag == 0)
	{
		for( i = 35 ; i < 145 ; i = i + interval )
		{
			for( j = 40 ; j < 120 ; j = j + interval )
			{
				
				rgb.b = rgb888->source[j][i*3+0];
				rgb.g = rgb888->source[j][i*3+1];
				rgb.r = rgb888->source[j][i*3+2];

				if(CheckHue(rgb))
				{	
					count++;
					//index = 180 * j + i;
					//fpga_videodata[index] = 0xf800;
				}
			}
			
			if( count > 3)
			{
				x_index = i;
				break;
			}
			//printf("i : %d\n",i);
			count = 0;
			//rise = rise * 2;
		}
	}
	else if(flag == 1)
	{
		for( i = 145 ; i > 35 ; i = i - interval )
		{
			for( j = 40 ; j < 120 ; j = j + interval )
			{
				rgb.b = rgb888->source[j][i*3+0];
				rgb.g = rgb888->source[j][i*3+1];
				rgb.r = rgb888->source[j][i*3+2];

				if(CheckHue(rgb))
				{	
					count++;
				}
			}
		
			if( count > 3 )
			{
				x_index = i;
				break;
			}
		
			count = 0;
		}
	}
	return x_index;
}

void RGB565toRGB888_back(U16* rgb565_fpgaData, IMAGE_D *rgb888)
{

	int i,j;
	int index;
	U16 pixel;
	for( i = 40 ; i < 120 ; i++)
	{
		for( j = 35 ; j < 145 ; j++)
		{
			index = j + 180 * i;
			pixel = rgb565_fpgaData[index];
			rgb888->source[i][j*3+0] = table_5_to_8[get_16b(pixel)];
			rgb888->source[i][j*3+1] = table_6_to_8[get_16g(pixel)];
			rgb888->source[i][j*3+2] = table_5_to_8[get_16r(pixel)];
		}
	}
}