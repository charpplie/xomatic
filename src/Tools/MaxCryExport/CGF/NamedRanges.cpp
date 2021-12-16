//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#include "stdafx.h"
#include "Utility.h"

void FillList(HWND hWnd, CSExportUtility* util, int active_id)
{
	//Fill the list
	ListView_DeleteAllItems(hWnd);
	
	for(int i=0;i<util->Ranges.size();i++)
	{
		NamedRange ent=util->Ranges[i];
		CStr ss,se;
		ss.printf("%d",ent.nStart);
		se.printf("%d",ent.nEnd);
		
		LVITEM lvi;
		lvi.mask		= LVIF_TEXT;
		lvi.iItem		= i;
		lvi.iSubItem 	= 0;
		lvi.pszText		= ss;
		ListView_InsertItem(hWnd,&lvi);

		lvi.mask		= LVIF_TEXT;
		lvi.iItem		= i;
		lvi.iSubItem 	= 1;
		lvi.pszText		= se;
		ListView_SetItem(hWnd,&lvi);

		lvi.mask		= LVIF_TEXT;
		lvi.iItem		= i;
		lvi.iSubItem 	= 2;
		lvi.pszText		= const_cast<char*>(ent.sName.c_str());
		ListView_SetItem(hWnd,&lvi);
	}

	LVITEM lvi;
	lvi.mask		= LVIF_TEXT;
	lvi.iItem		= i;
	lvi.iSubItem 	= 0;
	lvi.pszText		= "";
	ListView_InsertItem(hWnd,&lvi);

	lvi.mask		= LVIF_TEXT;
	lvi.iItem		= i;
	lvi.iSubItem 	= 1;
	lvi.pszText		= "";
	ListView_SetItem(hWnd,&lvi);

	lvi.mask		= LVIF_TEXT;
	lvi.iItem		= i;
	lvi.iSubItem 	= 2;
	lvi.pszText		= "<New Range>";
	ListView_SetItem(hWnd,&lvi);

	if(active_id!=-1)
	{
		ListView_SetItemState(hWnd, active_id, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void UpdateItem(HWND hWnd, CSExportUtility* util, int id)
{
	if(id<0 || id>util->Ranges.size()) return;
	if(id==util->Ranges.size())
	{
		NamedRange ent;
		util->Ranges.push_back(ent);
	}

	NamedRange *ent=&util->Ranges.at(id);
	
	ISpinnerControl *sp=GetISpinner(GetDlgItem(hWnd,IDC_START_SP));
	ent->nStart=sp->GetIVal();
	ReleaseISpinner(sp);

	sp=GetISpinner(GetDlgItem(hWnd,IDC_END_SP));
	ent->nEnd=sp->GetIVal();
	ReleaseISpinner(sp);

	char szBuffer[256] = "too long";
	GetWindowText(GetDlgItem(hWnd,IDC_NAME),szBuffer,sizeof(szBuffer));
	ent->sName = szBuffer;

	FillList(GetDlgItem(hWnd,IDC_RANGE_LIST),util,id);

	util->SaveConfigToMaxFile();
}

INT_PTR CALLBACK NamedRangesDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL				accels_enabled	=TRUE;
	static CSExportUtility*	util			=NULL;
	static ISpinnerControl* start_sp		=NULL;
	static ISpinnerControl* end_sp			=NULL;
	static int				active_id		=-1;
	static HWND				hlv				=NULL;
	static bool				inhibit			=false;

	switch (msg) 
	{
	case WM_INITDIALOG:
		{
			active_id=-1;
			util = (CSExportUtility *)lParam;
			accels_enabled= AcceleratorsEnabled();
			DisableAccelerators();
			//Set List properties
			hlv=GetDlgItem(hWnd,IDC_RANGE_LIST);
			int ex_style = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES;
			ListView_SetExtendedListViewStyleEx(hlv, ex_style, ex_style);
			
			//Setup columns
			LVCOLUMN lvc;
			lvc.mask		= LVCF_SUBITEM | LVCF_TEXT ;
			lvc.iSubItem	= 0;
			lvc.pszText		= "start";
			ListView_InsertColumn(hlv,0,&lvc);

			lvc.mask		= LVCF_SUBITEM | LVCF_TEXT ;
			lvc.iSubItem	= 1;
			lvc.pszText		= "end";
			ListView_InsertColumn(hlv,1,&lvc);

			lvc.mask		= LVCF_SUBITEM | LVCF_TEXT ;
			lvc.iSubItem	= 2;
			lvc.pszText		= "name";
			ListView_InsertColumn(hlv,2,&lvc);

			//Set Column Widths
			ListView_SetColumnWidth(hlv,0,40 );
			ListView_SetColumnWidth(hlv,1,40 );
			ListView_SetColumnWidth(hlv,2,LVSCW_AUTOSIZE_USEHEADER);//190 );
			
			FillList(hlv,util,active_id);

			start_sp=SetupIntSpinner(hWnd,IDC_START_SP,IDC_START_ED,-10000,10000,0);
			end_sp  =SetupIntSpinner(hWnd,IDC_END_SP,IDC_END_ED,-10000,10000,0);

			start_sp->Enable(false);
			end_sp->Enable(false);
			EnableWindow(GetDlgItem(hWnd,IDC_NAME),false);
			EnableWindow(GetDlgItem(hWnd,IDC_DELETE),false);

			return TRUE;
		}

	case WM_ACTIVATE:
		if(LOWORD(wParam)==WA_INACTIVE)
		{
			if(accels_enabled) EnableAccelerators();
		}
		else
		{
			DisableAccelerators();
		}
		break;

	case  WM_DESTROY:
		if(accels_enabled) EnableAccelerators();
		if(start_sp) ReleaseISpinner(start_sp);
		if(end_sp) ReleaseISpinner(end_sp);
		return FALSE;

	case WM_NOTIFY:
		{
			int		idCtrl	= (int) wParam; 
			NM_LISTVIEW *nm	=(LPNMLISTVIEW) lParam;
			
			switch (nm->hdr.code)
			{
			case NM_CLICK:
				{
					active_id=nm->iItem;
					if(active_id>=0)
					{
						NamedRange ent;
						ent.nStart	=(active_id>0)?util->Ranges[active_id-1].nEnd+1:0;
						ent.nEnd		=ent.nStart;
						ent.sName = "Unnamed";
						if(active_id<util->Ranges.size()) ent=util->Ranges[active_id];
						
						start_sp->SetValue(ent.nStart,FALSE);
						end_sp->SetValue(ent.nEnd,FALSE);
						inhibit=true;
						SetWindowText(GetDlgItem(hWnd,IDC_NAME),ent.sName.c_str());
						inhibit=false;
					}
					start_sp->Enable(active_id>=0);
					end_sp->Enable(active_id>=0);
					EnableWindow(GetDlgItem(hWnd,IDC_NAME),active_id>=0);
					EnableWindow(GetDlgItem(hWnd,IDC_DELETE),active_id>=0 && active_id<util->Ranges.size());

				}
				return FALSE;

			case LVN_ENDLABELEDIT:
				return TRUE;
			}
		}
		break;

	case CC_SPINNER_CHANGE:
		{
			UpdateItem(hWnd, util, active_id);
		}
		break;


	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_NAME:
			if(HIWORD(wParam)==EN_CHANGE && !inhibit) 
				UpdateItem(hWnd,util,active_id);
			return TRUE;

		case IDC_DELETE:
			if(active_id>=0 && active_id<util->Ranges.size())
			{
				util->Ranges.erase(util->Ranges.begin() + active_id);
				//active_id=-1;
				FillList(hlv,util,active_id);
			}
			return TRUE;

		case IDC_COPY_RANGES_TO_TIME_TAGS:
			util->CopyRangesToTimeTags();
			break;

		case IDC_OK:
			ShowWindow(hWnd, SW_HIDE);
			util->SaveConfigToMaxFile();
			return TRUE;
		}
		break;

	default: 
		return FALSE;
	}

	return FALSE;
} 

#define NAMED_RANGE_MAX_CHUNK 0

IOResult CSExportUtilClassDesc::Save(ISave *s)
{
	//int n_range=theCSExportUtility.Ranges.size();
	//if(!n_range) return IO_OK;

	//ULONG nwrit;

	////save num ranges
	//s->BeginChunk(0);
	//IOResult res=s->Write(&n_range, sizeof(n_range), &nwrit);

	////save the ranges
	//for(int i=0;i<n_range;i++)
	//{
	//	NamedRange ent=theCSExportUtility.Ranges[i];
	//	res=s->Write(&ent, sizeof(ent), &nwrit);
	//}
	//s->EndChunk();

	return IO_OK;
}

IOResult CSExportUtilClassDesc::Load(ILoad *l)
{
	//ULONG nread;
	//IOResult res;	

	//while (IO_OK==(res=l->OpenChunk())) 
	//{
	//	switch(l->CurChunkID())  
	//	{
	//	case 0:
	//		{	
	//			int n_ranges=0;
	//			res=l->Read(&n_ranges, sizeof(n_ranges), &nread);
	//			if (res!=IO_OK) return res;

	//			if(n_ranges)
	//			{
	//				theCSExportUtility.Ranges.clear();
	//				for(int i=0;i<n_ranges;i++)
	//				{
	//					NamedRange ent;
	//					res=l->Read(&ent, sizeof(ent), &nread);
	//					if (res!=IO_OK) return res;
	//					theCSExportUtility.Ranges.push_back(ent);
	//				}
	//			}
	//		}
	//		break;
	//	}
	//	l->CloseChunk();
	//}
	
	return IO_OK;
}
