#ifndef __LISTS_H__
#define __LISTS_H__

#define DATA_OFFSET(link, base) ((u8*)(link)-(u8*)(base))

typedef struct slinkdef_s
{
	struct slinkdef_s *next;
} slinkdef;

typedef struct slinklistdef_s
{
	slinkdef head;
	slinkdef tail;
	int offset;
} slinklistdef;

void sllInitList(slinklistdef *list, int offset);
void sllInitLink(slinkdef *link);
void* sllGetData(slinklistdef *list, slinkdef *link);
int sllIsEmpty(slinklistdef *list);

slinkdef* sllGetFirst(slinklistdef *list);
slinkdef* sllGetNext(slinkdef *link);
int sllIsEnd(slinkdef *link);
int sllInList(slinkdef *link);

void sllRemove(slinklistdef *list, slinkdef *link);
void sllAddHead(slinklistdef *list, slinkdef *link);
void sllAddTail(slinklistdef *list, slinkdef *link);
void sllAddAfter(slinkdef *node, slinkdef *toAdd);

typedef struct dlinkdef_s
{
	struct dlinkdef_s *prev;
	struct dlinkdef_s *next;
} dlinkdef;

typedef struct dlinklistdef_s
{
	dlinkdef head;
	dlinkdef tail;
	int offset;
} dlinklistdef;

void dllInitList(dlinklistdef *list, int offset);
void dllInitLink(dlinkdef *link);
void* dllGetData(dlinklistdef *list, dlinkdef *link);
int dllIsEmpty(dlinklistdef *list);

dlinkdef* dllGetFirst(dlinklistdef *list);
dlinkdef* dllGetLast(dlinklistdef *list);
dlinkdef* dllGetNext(dlinkdef *link);
dlinkdef* dllGetPrev(dlinkdef *link);
int dllIsEnd(dlinkdef *link);
int dllInList(dlinkdef *link);

void dllRemove(dlinkdef *link);
void dllAddHead(dlinklistdef *list, dlinkdef *link);
void dllAddTail(dlinklistdef *list, dlinkdef *link);
void dllAddBefore(dlinkdef *node, dlinkdef *toAdd);
void dllAddAfter(dlinkdef *node, dlinkdef *toAdd);

#endif //__LISTS_H__
