#ifndef __SWNotify_h__
#define __SWNotify_h__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef SEG_WORLD
#define SW_ON_OBJ_NEW(obj)							if(obj) CSWManager::Get().GetDoc().OnObjectCreated(obj);
#define SW_ON_OBJ_DEL(obj)								if(obj) CSWManager::Get().GetDoc().OnObjectDelete(obj);
#define SW_ON_OBJ_MOD(obj)							if(obj) CSWManager::Get().GetDoc().OnObjectModified(obj);
#define SW_ON_OBJ_EXTERNAL(obj)					if(obj) CSWManager::Get().GetDoc().OnObjectMakeExternal(obj);
#define SW_ON_OBJ_MOVETO(obj, pos, layer)	if(obj) CSWManager::Get().GetDoc().OnObjectMoveTo(obj, pos, layer);
#define SW_ON_VEG_MOD(aabb)						CSWManager::Get().GetDoc().OnVegetationModified(aabb);
#define SW_ON_HMAP_MOD(aabb)						CSWManager::Get().GetDoc().OnHeightmapModified(aabb);
#define SW_ON_HMAP_PAINT(x, y, w, h, cond)				CSWManager::Get().GetDoc().OnTerrainLayerIDPainted(x, y, w, h, cond);
#define SW_ON_ADD_LINK(owner, target)						if(owner && target) CSWManager::Get().GetDoc().OnAddLink(owner, target);
#define SW_ON_REMOVE_LINK(owner, target)				if(owner && target) CSWManager::Get().GetDoc().OnRemoveLink(owner, target);
#define SW_ENABLE_FIND_OBJ(enable)							CSWManager::Get().GetDoc().EnableSWFindObject(enable);
#define SW_TEST_WORLD_DATA_MOD(ntype)					if(!CSWManager::Get().GetDoc().OnWorldDataModified(ntype)) return;
#define SW_TEST_OBJ_PLACETO(pos, layer, lock)				if(!CSWManager::Get().GetDoc().CanObjectPlaceTo(pos, layer, lock)) return;
#define SW_TEST_OBJ_PLACETO_MCB(pos, layer, lock)		if(!CSWManager::Get().GetDoc().CanObjectPlaceTo(pos, layer, lock)) return MOUSECREATE_CONTINUE;
#define SW_TEST_OBJ_PLACETO_MCB2(pos, layer, lock)	if(!CSWManager::Get().GetDoc().CanObjectPlaceTo(pos, layer, lock)) return false;
#define SW_TEST_OBJ_MOVETO(obj, pos, layer, lock)				if(!CSWManager::Get().GetDoc().CanObjectMoveTo(obj, pos, layer, lock)) return;
#define SW_TEST_OBJ_MOVETO_MCB(obj, pos, layer, lock)		if(!CSWManager::Get().GetDoc().CanObjectMoveTo(obj, pos, layer, lock)) return false;
#define SW_TEST_OBJ_MOD(obj, del, lock) \
	if(!CSWManager::Get().GetDoc().CanModify(obj, del, lock)) \
	{ CSWManager::SWMsgBox(sw::SWMsgType_AccessDenied_Object); return; }
#define SW_TEST_OBJ_MOD_MCB(obj, del, lock) \
	if(!CSWManager::Get().GetDoc().CanModify(obj, del, lock)) \
	{ CSWManager::SWMsgBox(sw::SWMsgType_AccessDenied_Object); return false; }
#define SW_TEST_VEG_MOD(aabb) \
	if(!CSWManager::Get().GetDoc().CanModify(aabb)) \
	{ CSWManager::SWMsgBox(sw::SWMsgType_AccessDenied_Object); return; }
#define SW_TEST_GRAPH_MOD(graph, lock) \
	if(!CSWManager::Get().GetDoc().CanModify(graph, lock)) \
	{ CSWManager::SWMsgBox(sw::SWMsgType_AccessDenied_Graph); return; }
#define SW_TEST_TERRAIN_MOD(x1, y1, x2, y2, cond1, cond2) \
	if(!CSWManager::Get().GetDoc().CanModify(x1, y1, x2, y2, cond1, cond2)) \
	{ CSWManager::SWMsgBox(sw::SWMsgType_AccessDenied); return ; }
#else
#define SW_ON_OBJ_NEW(obj)
#define SW_ON_OBJ_DEL(obj)
#define SW_ON_OBJ_MOD(obj)
#define SW_ON_OBJ_EXTERNAL(obj)
#define SW_ON_OBJ_MOVETO(obj, pos, layer)
#define SW_ON_VEG_MOD(aabb)
#define SW_ON_HMAP_MOD(aabb)
#define SW_ON_HMAP_PAINT(x, y, w, h, cond)
#define SW_ON_ADD_LINK(owner, target)
#define SW_ON_REMOVE_LINK(owner, target)
#define SW_ENABLE_FIND_OBJ(enable)
#define SW_TEST_WORLD_DATA_MOD(ntype)
#define SW_TEST_OBJ_PLACETO(pos, layer, lock)
#define SW_TEST_OBJ_PLACETO_MCB(pos, layer, lock)
#define SW_TEST_OBJ_PLACETO_MCB2(pos, layer, lock)
#define SW_TEST_OBJ_MOVETO(obj, pos, layer, lock)
#define SW_TEST_OBJ_MOVETO_MCB(obj, pos, layer, lock)
#define SW_TEST_OBJ_MOD(obj, del, lock)
#define SW_TEST_OBJ_MOD_MCB(obj, del, lock)
#define SW_TEST_VEG_MOD(aabb)
#define SW_TEST_GRAPH_MOD(graph, lock)
#define SW_TEST_TERRAIN_MOD(x1, y1, x2, y2, cond1, cond2)
#endif

#endif __SWNotify_h__