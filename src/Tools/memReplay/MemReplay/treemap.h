#ifndef __TREEMAP_H__
#define __TREEMAP_H__

#include "lists.h"

struct treemapnode
{
	dlinkdef link;
	float x,y;
	float w,h;
	float area;
};

typedef struct treemap_s
{
	dlinklistdef list;
	treemapnode root;
} treemap;

void treemapCreate(treemap *t, float width, float height);
void treemapAddNode(treemap *t, treemapnode *n, float area);
void treemapBuild(treemap *t);

#endif //__TREEMAP_H__
