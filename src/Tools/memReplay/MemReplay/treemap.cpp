#include "stdafx.h"
#include "treemap.h"
#include "lists.h"

#include "ReplaySDK/common.h"

static int compareRatio(dlinklistdef *row, treemapnode *next, float w)
{
	float min=MAX_FLOAT;
	dlinkdef *link;
	float sum=0.0f;
	float max=0.0f;
	float ratio;
	for (link=dllGetFirst(row); !dllIsEnd(link); link=dllGetNext(link))
	{
		treemapnode *node=(treemapnode*)dllGetData(row, link);
		max=MAX(max, node->area);
		min=MIN(min, node->area);
		sum+=node->area;
	}
	ratio=MAX((SQR(w)*max)/SQR(sum), SQR(sum)/(SQR(w)*min));
	max=MAX(max, next->area);
	min=MIN(min, next->area);
	sum+=next->area;
	return ratio<=MAX((SQR(w)*max)/SQR(sum), SQR(sum)/(SQR(w)*min));
}

static void layoutRow(treemapnode *parent, dlinklistdef *row, treemapnode *newNode)
{
	dlinkdef *link=dllGetFirst(row);
	float x=parent->x;
	float y=parent->y;
	float sum=0.0f;
	float stride;
	int horizontal=parent->w<parent->h;
	for (link=dllGetFirst(row); !dllIsEnd(link); link=dllGetNext(link))
	{
		treemapnode *node=(treemapnode*)dllGetData(row, link);
		sum+=node->area;
	}
	stride=sum/MIN(parent->w, parent->h);
	if (horizontal)
	{
		y+=parent->h-stride;
	}
	else
	{
		y+=parent->h;
	}
	for (link=dllGetFirst(row); !dllIsEnd(link); link=dllGetNext(link))
	{
		treemapnode *node=(treemapnode*)dllGetData(row, link);
		node->x=x;
		if (horizontal)
		{
			node->h=stride;
			node->w=node->area/node->h;
			x+=node->w;
		}
		else
		{
			node->w=stride;
			node->h=node->area/node->w;
			y-=node->h;
		}
		node->y=y;
	}
	if (newNode)
	{
		if (horizontal)
		{
			newNode->x=parent->x;
			newNode->y=parent->y;
			newNode->w=parent->w;
			newNode->h=parent->h-stride;
		}
		else
		{
			newNode->x=parent->x+stride;
			newNode->y=parent->y;
			newNode->w=parent->w-stride;
			newNode->h=parent->h;
		}
	}
}

static void squarify(dlinklistdef *toAdd, dlinklistdef *row, treemapnode *parent)
{
top:
	dlinkdef *link=dllGetFirst(toAdd);
	treemapnode *node=(treemapnode*)dllGetData(toAdd, link);
	if (dllIsEnd(link))
	{
		layoutRow(parent, row, NULL);
		return;
	}
	dllRemove(link);
	if (!compareRatio(row, node, MIN(parent->w,parent->h)))
	{
		dllAddTail(row, link);
		goto top;
	}
	else
	{
		treemapnode newnode;
		dlinklistdef thisRow;
		dllInitList(&thisRow, DATA_OFFSET(link, node));
		dllAddTail(&thisRow, link);
		layoutRow(parent, row, &newnode);
		squarify(toAdd, &thisRow, &newnode);
	}
}

void treemapCreate(treemap *t, float width, float height)
{
	dllInitList(&t->list, DATA_OFFSET(&t->root.link, &t->root));
	t->root.x=t->root.y=0.0f;
	t->root.w=width;
	t->root.h=height;
}

void treemapAddNode(treemap *t, treemapnode *n, float area)
{
	dllAddTail(&t->list, &n->link);
	n->area=area;
}

void treemapBuild(treemap *t)
{
	dlinklistdef thisRow;
	dlinkdef *link=dllGetFirst(&t->list);
	dllInitList(&thisRow, DATA_OFFSET(&t->root.link, &t->root));
	dllRemove(link);
	dllAddTail(&thisRow, link);
	squarify(&t->list, &thisRow, &t->root);
}
