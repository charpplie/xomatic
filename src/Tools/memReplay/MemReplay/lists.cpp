#include "stdafx.h"
#include "lists.h"

void sllInitList(slinklistdef *list, int offset)
{
	list->head.next=&list->tail;
	list->tail.next=&list->tail;
	list->offset=offset;
}

void sllInitLink(slinkdef *link)
{
	link->next=link;
}

void* sllGetData(slinklistdef *list, slinkdef *link)
{
	return (u8*)link-list->offset;
}

int sllIsEmpty(slinklistdef *list)
{
	return sllIsEnd(sllGetFirst(list));
}

slinkdef* sllGetFirst(slinklistdef *list)
{
	return list->head.next;
}

slinkdef* sllGetNext(slinkdef *link)
{
	return link->next;
}

int sllIsEnd(slinkdef *link)
{
	return (link->next==link);
}

int sllInList(slinkdef *link)
{
	return (link->next!=link);
}

void sllRemove(slinklistdef *list, slinkdef *link)
{
	slinkdef *l=&list->head;
	while (!sllIsEnd(l))
	{
		slinkdef *n=sllGetNext(l);
		if (n==link)
		{
			l->next=link->next;
			link->next=link;
			return;
		}
		l=n;
	}
	printf("Removed link not in list\n");
}

void sllAddHead(slinklistdef *list, slinkdef *link)
{
	link->next=list->head.next;
	list->head.next=link;
}

void sllAddTail(slinklistdef *list, slinkdef *link)
{
	slinkdef *l=&list->head;
	slinkdef *n;
	while ((n=sllGetNext(l)))
	{
		if (n==&list->tail)
		{
			l->next=link;
			link->next=n;
			return;
		}
		l=n;
	}
}

void sllAddAfter(slinkdef *node, slinkdef *toAdd)
{
	toAdd->next=node->next;
	node->next=toAdd;
}

void dllInitList(dlinklistdef *list, int offset)
{
	list->head.prev=&list->head;
	list->head.next=&list->tail;
	list->tail.prev=&list->head;
	list->tail.next=&list->tail;
	list->offset=offset;
}

void dllInitLink(dlinkdef *link)
{
	link->prev=link;
	link->next=link;
}

void* dllGetData(dlinklistdef *list, dlinkdef *link)
{
	return (u8*)link-list->offset;
}

int dllIsEmpty(dlinklistdef *list)
{
	return dllIsEnd(dllGetFirst(list));
}

dlinkdef* dllGetFirst(dlinklistdef *list)
{
	return list->head.next;
}

dlinkdef* dllGetLast(dlinklistdef *list)
{
	return list->tail.prev;
}

dlinkdef* dllGetNext(dlinkdef *link)
{
	return link->next;
}

dlinkdef* dllGetPrev(dlinkdef *link)
{
	return link->prev;
}

int dllIsEnd(dlinkdef *link)
{
	return (link->next==link || link->prev==link);
}

int dllInList(dlinkdef *link)
{
	return (link->next!=link && link->prev!=link);
}

void dllRemove(dlinkdef *link)
{
	link->prev->next=link->next;
	link->next->prev=link->prev;
	link->next=link;
	link->prev=link;
}

void dllAddHead(dlinklistdef *list, dlinkdef *link)
{
	link->next=list->head.next;
	link->prev=&list->head;
	link->prev->next=link;
	link->next->prev=link;
}

void dllAddTail(dlinklistdef *list, dlinkdef *link)
{
	link->prev=list->tail.prev;
	link->next=&list->tail;
	link->prev->next=link;
	link->next->prev=link;
}

void dllAddBefore(dlinkdef *node, dlinkdef *toAdd)
{
	toAdd->prev=node->prev;
	toAdd->next=node;
	toAdd->prev->next=toAdd;
	toAdd->next->prev=toAdd;
}

void dllAddAfter(dlinkdef *node, dlinkdef *toAdd)
{
	toAdd->prev=node;
	toAdd->next=node->next;
	toAdd->prev->next=toAdd;
	toAdd->next->prev=toAdd;
}
