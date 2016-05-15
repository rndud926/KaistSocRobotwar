#include "imgcore.h"

NODE *insert_node(NODE *plist, NODE *pprev, POINT_D point, int color_select)
{
	NODE *pnew = NULL;

	if( !(pnew = ( NODE *)malloc(sizeof(NODE))))
	{
		printf("malloc function error\n");
		exit(1);
	}

	pnew->point.x = point.x;
	pnew->point.y = point.y;

	if( pprev == NULL) //���Ḯ��Ʈ ó���� ����
	{
		pnew->link = plist;
		plist = pnew;
	}
	else
	{
		pnew->link = pprev->link;
		pprev->link = pnew;
	}

	return plist;
}


NODE *delete_node(NODE *plist, NODE *pprev, NODE *pcurr)
{
	if( pprev == NULL)
		plist = pcurr->link;
	else
		pprev->link = pcurr->link;

	free(pcurr);
	return plist;
}

void print_list(NODE *plist)
{
	NODE *p;
	p = plist;

	while(p)
	{
		//printf("[%d] x : %d y : %d\n",count,plist->point.x, plist->point.y);
		p = p ->link;
	}
	printf("��³�\n");
}

BOOL _check_linkdupilcation(NODE *plist, POINT_D point)
{
	NODE *p;
	p = plist;
	while(p)
	{
		if( point.x == p->point.x && point.y == p->point.y)
		{
			printf("��ø\n");
			return false;
		}
		p = p ->link;
	}

	return true;
	
}

void destroy_list(NODE *plist)
{
	struct NODE *p = plist;
	struct NODE *next;
	
	//���� �Ҵ� �ݳ�.
	while( p != NULL)
	{
		next = p->link;
		free(p);
		p = next;
	}
}