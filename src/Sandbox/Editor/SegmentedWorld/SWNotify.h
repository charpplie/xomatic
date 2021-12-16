#ifndef __SWNotify_h__
#define __SWNotify_h__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef SEG_WORLD
#define SW_ON_OBJ_NEW(obj)							if(obj) GetIEditor()->GetSegmentedWorldDoc().OnObjectCreated(obj);
#define SW_ON_OBJ_DEL(obj)								if(obj) GetIEditor()->GetSegmentedWorldDoc().OnObjectDelete(obj);
#define SW_ON_OBJ_MOD(obj)							if(obj) GetIEditor()->GetSegmentedWorldDoc().OnObjectModified(obj);
#define SW_ON_OBJ_SEL_MOD(sel)					for (int i = 0, count = sel.size(); i < count; ++i) SW_ON_OBJ_MOD(sel[i]);
#define SW_ON_OBJ_EXTERNAL(obj)					if(obj) GetIEditor()->GetSegmentedWorldDoc().OnObjectMakeExternal(obj);
#define SW_ON_OBJ_MOVETO(obj, pos, layer)	if(obj) GetIEditor()->GetSegmentedWorldDoc().OnObjectMoveTo(obj, pos, layer);
#define SW_ON_VEG_MOD(aabb)						GetIEditor()->GetSegmentedWorldDoc().OnVegetationModified(aabb);
#define SW_ON_HMAP_MOD(aabb)						GetIEditor()->GetSegmentedWorldDoc().OnHeightmapModified(aabb);
#define SW_ON_HMAP_PAINT(x, y, w, h, cond)				GetIEditor()->GetSegmentedWorldDoc().OnTerrainLayerIDPainted(x, y, w, h, cond);
#define SW_ON_ADD_LINK(owner, target)						if(owner && target) GetIEditor()->GetSegmentedWorldDoc().OnAddLink(owner, target);
#define SW_ON_REMOVE_LINK(owner, target)				if(owner && target) GetIEditor()->GetSegmentedWorldDoc().OnRemoveLink(owner, target);
#define SW_ENABLE_FIND_OBJ(enable)							GetIEditor()->GetSegmentedWorldDoc().EnableSWFindObject(enable);
#define SW_TEST_WORLD_DATA_MOD(ntype)					if(!GetIEditor()->GetSegmentedWorldDoc().OnWorldDataModified(ntype)) return;
#define SW_TEST_OBJ_PLACETO(pos, layer, lock)				if(!GetIEditor()->GetSegmentedWorldDoc().CanObjectPlaceTo(pos, layer, lock)) return;
#define SW_TEST_OBJ_PLACETO_MCB(pos, layer, lock)		if(!GetIEditor()->GetSegmentedWorldDoc().CanObjectPlaceTo(pos, layer, lock)) return MOUSECREATE_CONTINUE;
#define SW_TEST_OBJ_PLACETO_MCB2(pos, layer, lock)	if(!GetIEditor()->GetSegmentedWorldDoc().CanObjectPlaceTo(pos, layer, lock)) return false;
#define SW_TEST_OBJ_MOVETO(obj, pos, layer, lock)				if(!GetIEditor()->GetSegmentedWorldDoc().CanObjectMoveTo(obj, pos, layer, lock)) return;
#define SW_TEST_OBJ_MOVETO_MCB(obj, pos, layer, lock)		if(!GetIEditor()->GetSegmentedWorldDoc().CanObjectMoveTo(obj, pos, layer, lock)) return false;
#define SW_TEST_OBJ_MOD(obj, del, lock) \
	if(!GetIEditor()->GetSegmentedWorldDoc().CanModify(obj, del, lock)) \
	{ CSegmentedWorldManager::SWMsgBox(sw::SWMsgType_AccessDenied_Object); return; }
#define SW_TEST_OBJ_SEL_MOD(sel, del, lock) \
	for (int i = 0, count = sel.size(); i < count; ++i) \
		SW_TEST_OBJ_MOD(sel[i], del, lock);
#define SW_TEST_OBJ_MOD_MCB(obj, del, lock) \
	if(!GetIEditor()->GetSegmentedWorldDoc().CanModify(obj, del, lock)) \
	{ CSegmentedWorldManager::SWMsgBox(sw::SWMsgType_AccessDenied_Object); return false; }
#define SW_TEST_VEG_MOD(aabb) \
	if(!GetIEditor()->GetSegmentedWorldDoc().CanModify(aabb)) \
	{ CSegmentedWorldManager::SWMsgBox(sw::SWMsgType_AccessDenied_Object); return; }
#define SW_TEST_GRAPH_MOD(graph, lock) \
	if(!GetIEditor()->GetSegmentedWorldDoc().CanModify(graph, lock)) \
	{ CSegmentedWorldManager::SWMsgBox(sw::SWMsgType_AccessDenied_Graph); return; }
#define SW_TEST_TERRAIN_MOD(x1, y1, x2, y2, cond1, cond2) \
	if(!GetIEditor()->GetSegmentedWorldDoc().CanModify(x1, y1, x2, y2, cond1, cond2)) \
	{ CSegmentedWorldManager::SWMsgBox(sw::SWMsgType_AccessDenied); return ; }
#else
#define SW_ON_OBJ_NEW(obj)
#define SW_ON_OBJ_DEL(obj)
#define SW_ON_OBJ_MOD(obj)
#define SW_ON_OBJ_SEL_MOD(sel)
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
#define SW_TEST_OBJ_SEL_MOD(sel, del, lock)
#define SW_TEST_OBJ_MOD_MCB(obj, del, lock)
#define SW_TEST_VEG_MOD(aabb)
#define SW_TEST_GRAPH_MOD(graph, lock)
#define SW_TEST_TERRAIN_MOD(x1, y1, x2, y2, cond1, cond2)
#endif

#endif __SWNotify_h__
