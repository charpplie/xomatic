////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AVCompressionDialog.cpp
//  Created:     30/4/2010 by John Stewart.
//  Description: This dialog is used to configure the options for AV Compression
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "resource.h"
#include "AVCompression.h"
#include "AVCompressionDialog.h"

BEGIN_MESSAGE_MAP(CAVCompressionDialog, CDialog)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO0, &CAVCompressionDialog::OnBnClickedAVCRecordsize0Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO1, &CAVCompressionDialog::OnBnClickedAVCRecordsize1Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO2, &CAVCompressionDialog::OnBnClickedAVCRecordsize2Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO3, &CAVCompressionDialog::OnBnClickedAVCRecordsize3Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO4, &CAVCompressionDialog::OnBnClickedAVCRecordsize4Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO5, &CAVCompressionDialog::OnBnClickedAVCRecordsize5Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO6, &CAVCompressionDialog::OnBnClickedAVCRecordsize6Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO7, &CAVCompressionDialog::OnBnClickedAVCRecordsize7Radio)
	ON_BN_CLICKED(IDC_AVC_SIZE_RADIO8, &CAVCompressionDialog::OnBnClickedAVCRecordsize8Radio)
	ON_BN_CLICKED(IDC_AVC_RECORDMODE_RADIO0, &CAVCompressionDialog::OnBnClickedAVCRecordMode0Radio)
	ON_BN_CLICKED(IDC_AVC_RECORDMODE_RADIO1, &CAVCompressionDialog::OnBnClickedAVCRecordMode1Radio)
	ON_BN_CLICKED(IDC_AVC_IDOK, &CAVCompressionDialog::OnBnClickedAVCOptionsOkButton)
	ON_BN_CLICKED(IDC_AVC_IDCANCEL, &CAVCompressionDialog::OnBnClickedAVCOptionsCancelButton)
END_MESSAGE_MAP()



// Constructor
CAVCompressionDialog::CAVCompressionDialog(int RecordSize,unsigned int RecordMode,unsigned int CustomWidth, unsigned int CustomHeight,
										 int MaxFileSize,unsigned int RecordTime,float RecordFrameRate,unsigned int RecordQuality)
:	CDialog(IDD)
{


	// Get the initial values from the calling program
	m_AVC_RecordSize = RecordSize;
	m_AVC_RecordMode = RecordMode;
	m_AVC_Maxfilesize = MaxFileSize;
	m_AVC_RecordTime = RecordTime;
	m_AVC_RecordFrameRate = RecordFrameRate;
	m_AVC_RecordQuality = RecordQuality;
	m_AVC_CustomWidth = CustomWidth;
	m_AVC_CustomHeight = CustomHeight;
}


void CAVCompressionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX,IDC_AVC_EDIT_CUSTOMWIDTH,m_EditCustom_Width);
	DDX_Control(pDX,IDC_AVC_EDIT_CUSTOMHEIGHT,m_EditCustom_Height);
	DDX_Control(pDX,IDC_AVC_EDIT_RECORDSIZE,m_EditRecordSize);
	DDX_Control(pDX,IDC_AVC_EDIT_RECORDTIME,m_EditRecordTime);
	DDX_Control(pDX,IDC_AVC_EDIT_FRAMERATE,m_EditFrameRate);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO0,m_RecordSizeButton0);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO1,m_RecordSizeButton1);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO2,m_RecordSizeButton2);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO3,m_RecordSizeButton3);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO4,m_RecordSizeButton4);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO5,m_RecordSizeButton5);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO6,m_RecordSizeButton6);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO7,m_RecordSizeButton7);
	DDX_Control(pDX,IDC_AVC_SIZE_RADIO8,m_RecordSizeButton8);
	DDX_Control(pDX,IDC_AVC_RECORDMODE_RADIO0,m_RecordModeButton0);
	DDX_Control(pDX,IDC_AVC_RECORDMODE_RADIO1,m_RecordModeButton1);
	DDX_Control(pDX,IDC_AVC_RECORDQUALITY_SLIDER,m_QualitySlider);
}

// Initialize the dialog box buttons and text boxes
BOOL CAVCompressionDialog::OnInitDialog()
{

	CDialog::OnInitDialog();

	m_EditCustom_Width.EnableWindow(FALSE);
	m_EditCustom_Height.EnableWindow(FALSE);

	switch(m_AVC_RecordSize) 
	{

	case AVCRECORDSIZEFULLSCREEN :	m_RecordSizeButton0.SetCheck(BST_CHECKED);
									break;
	case AVCRECORDSIZECUSTOM	:	m_RecordSizeButton8.SetCheck(BST_CHECKED);
									m_EditCustom_Width.EnableWindow(TRUE);
									m_EditCustom_Height.EnableWindow(TRUE);
									break;
	case AVCRECORDSIZESTANDARD0 :	m_RecordSizeButton1.SetCheck(BST_CHECKED);
									break;
	case AVCRECORDSIZESTANDARD1 :	m_RecordSizeButton2.SetCheck(BST_CHECKED);
									break;
	case AVCRECORDSIZESTANDARD2 :	m_RecordSizeButton3.SetCheck(BST_CHECKED);
									break;
	case AVCRECORDSIZESTANDARD3 :	m_RecordSizeButton4.SetCheck(BST_CHECKED);
									break;
	case AVCRECORDSIZESTANDARD4 :	m_RecordSizeButton5.SetCheck(BST_CHECKED);
									break;
	case AVCRECORDSIZESTANDARD5 :	m_RecordSizeButton6.SetCheck(BST_CHECKED);
									break;
	case AVCRECORDSIZESTANDARD6 :	 m_RecordSizeButton7.SetCheck(BST_CHECKED);
									break;
	}


	if(m_AVC_RecordMode == AVCRECORDMODECONTINUOUS) 
	{
		m_RecordModeButton0.SetCheck(BST_CHECKED);
		m_RecordModeButton1.SetCheck(BST_UNCHECKED);
		m_EditRecordSize.EnableWindow(TRUE);
		m_EditRecordTime.EnableWindow(FALSE);
	}
	else 
	{
	  	m_RecordModeButton0.SetCheck(BST_UNCHECKED);
		m_RecordModeButton1.SetCheck(BST_CHECKED);
		m_EditRecordSize.EnableWindow(FALSE);
		m_EditRecordTime.EnableWindow(TRUE);
	}


	CString buf;

	buf.Format("%d",m_AVC_CustomWidth);
	m_EditCustom_Width.SetWindowText(buf);

	buf.Format("%d",m_AVC_CustomHeight);
	m_EditCustom_Height.SetWindowText(buf);

	buf.Format("%d",m_AVC_Maxfilesize);
	m_EditRecordSize.SetWindowText(buf);

	buf.Format("%d",m_AVC_RecordTime);
	m_EditRecordTime.SetWindowText(buf);

	buf.Format("%3.1f",m_AVC_RecordFrameRate);
	m_EditFrameRate.SetWindowText(buf);



	m_QualitySlider.SetRange(0,QUALITYMAXVALUE);
	m_QualitySlider.SetPos(m_AVC_RecordQuality);

	return TRUE;
}




void CAVCompressionDialog::OnBnClickedAVCOptionsOkButton()
{
    // On ok, retrieve the various control values

	CString buf;

	m_EditCustom_Width.GetWindowText(buf);
	int tmp = atoi(buf);
	if(tmp > 0) m_AVC_CustomWidth = tmp;

	m_EditCustom_Height.GetWindowText(buf);
	tmp = atoi(buf);
	if(tmp > 0) m_AVC_CustomHeight = tmp;

	m_EditRecordSize.GetWindowText(buf);
	tmp = atoi(buf);
	m_AVC_Maxfilesize = tmp; 

	m_EditRecordTime.GetWindowText(buf);
	tmp = atoi(buf);
	if(tmp > 0) m_AVC_RecordTime = tmp;  

	m_EditFrameRate.GetWindowText(buf);
	float ftmp = atof(buf);
	if(ftmp > 0) m_AVC_RecordFrameRate = ftmp; 

	m_AVC_RecordQuality =  m_QualitySlider.GetPos();

	EndDialog(IDOK);
}


void CAVCompressionDialog::OnBnClickedAVCRecordsize0Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZEFULLSCREEN;
	m_RecordSizeButton0.SetCheck(BST_CHECKED);

}


void CAVCompressionDialog::OnBnClickedAVCRecordsize1Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZESTANDARD0;
	m_RecordSizeButton1.SetCheck(BST_CHECKED);

}

void CAVCompressionDialog::OnBnClickedAVCRecordsize2Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZESTANDARD1;
	m_RecordSizeButton2.SetCheck(BST_CHECKED);

}
void CAVCompressionDialog::OnBnClickedAVCRecordsize3Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZESTANDARD2;
	m_RecordSizeButton3.SetCheck(BST_CHECKED);

}
void CAVCompressionDialog::OnBnClickedAVCRecordsize4Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZESTANDARD3;
	m_RecordSizeButton4.SetCheck(BST_CHECKED);

}
void CAVCompressionDialog::OnBnClickedAVCRecordsize5Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZESTANDARD4;
	m_RecordSizeButton5.SetCheck(BST_CHECKED);

}
void CAVCompressionDialog::OnBnClickedAVCRecordsize6Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZESTANDARD5;
	m_RecordSizeButton6.SetCheck(BST_CHECKED);

}
void CAVCompressionDialog::OnBnClickedAVCRecordsize7Radio()
{

	AVC_clear_size_button(false);
	m_AVC_RecordSize = AVCRECORDSIZESTANDARD6;
	m_RecordSizeButton7.SetCheck(BST_CHECKED);

}
void CAVCompressionDialog::OnBnClickedAVCRecordsize8Radio()
{

	AVC_clear_size_button(true);
	m_AVC_RecordSize = AVCRECORDSIZECUSTOM;
	m_RecordSizeButton8.SetCheck(BST_CHECKED);

}

void CAVCompressionDialog::OnBnClickedAVCRecordMode0Radio()
{

	m_RecordModeButton0.SetCheck(BST_CHECKED);
	m_RecordModeButton1.SetCheck(BST_UNCHECKED);
	m_EditRecordSize.EnableWindow(TRUE);
	m_EditRecordTime.EnableWindow(FALSE);
	CString buf;
	buf.Format("%d",m_AVC_Maxfilesize);
	m_EditRecordSize.SetWindowText(buf);
	m_AVC_RecordMode = AVCRECORDMODECONTINUOUS;
}



void CAVCompressionDialog::OnBnClickedAVCRecordMode1Radio()
{

	m_RecordModeButton0.SetCheck(BST_UNCHECKED);
	m_RecordModeButton1.SetCheck(BST_CHECKED);
	m_EditRecordSize.EnableWindow(FALSE);
	m_EditRecordTime.EnableWindow(TRUE);
	CString buf;
	buf.Format("%d",m_AVC_RecordTime);
	m_EditRecordTime.SetWindowText(buf);
	m_AVC_RecordMode = AVCRECORDMODELIMITED;
}


void CAVCompressionDialog::OnBnClickedAVCOptionsCancelButton()
{
	EndDialog(IDCANCEL);
}


// unchecks the previous record size button
// and enables/disables the custom edit box
void CAVCompressionDialog::AVC_clear_size_button(bool seteditbox)
{


	switch(m_AVC_RecordSize)
	{
	case AVCRECORDSIZEFULLSCREEN :	m_RecordSizeButton0.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZECUSTOM	:	m_RecordSizeButton8.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZESTANDARD0 :	m_RecordSizeButton1.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZESTANDARD1 :	m_RecordSizeButton2.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZESTANDARD2 :	m_RecordSizeButton3.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZESTANDARD3 :	m_RecordSizeButton4.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZESTANDARD4 :	m_RecordSizeButton5.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZESTANDARD5 :	m_RecordSizeButton6.SetCheck(BST_UNCHECKED);
									break;
	case AVCRECORDSIZESTANDARD6 :	m_RecordSizeButton7.SetCheck(BST_UNCHECKED);
									break;

	}
	CString buf;

	buf.Format("%d",m_AVC_CustomWidth);
	m_EditCustom_Width.SetWindowText(buf);

	buf.Format("%d",m_AVC_CustomHeight);
	m_EditCustom_Height.SetWindowText(buf);

	m_EditCustom_Width.EnableWindow(seteditbox);
	m_EditCustom_Height.EnableWindow(seteditbox);

}



void CAVCompressionDialog::copyParms(AVCRecordParameters *recordParameters) 
{
	recordParameters->inputWidth = GetIEditor()->GetRenderer()->GetWidth();
	recordParameters->inputHeight = GetIEditor()->GetRenderer()->GetHeight();
	recordParameters->customWidth = m_AVC_CustomWidth;
	recordParameters->customHeight = m_AVC_CustomHeight;
	recordParameters->recordQuality = m_AVC_RecordQuality;
	recordParameters->frameRate = m_AVC_RecordFrameRate;
	recordParameters->recordMode = m_AVC_RecordMode;
	recordParameters->recordTime = m_AVC_RecordTime;
	recordParameters->maxFileSize = m_AVC_Maxfilesize;
	recordParameters->recordSize = m_AVC_RecordSize;


	switch(m_AVC_RecordSize) 
	{

		case AVCRECORDSIZECUSTOM	:	recordParameters->displayWidth = m_AVC_CustomWidth;
										recordParameters->displayHeight = m_AVC_CustomHeight;
										break;
		case AVCRECORDSIZEFULLSCREEN :	recordParameters->displayWidth = recordParameters->inputWidth;
										recordParameters->displayHeight = recordParameters->inputHeight;
										break;
		case AVCRECORDSIZESTANDARD0 :	recordParameters->displayWidth = 1920;
										recordParameters->displayHeight = 1080;
										break;
		case AVCRECORDSIZESTANDARD1 :	recordParameters->displayWidth = 1280;
										recordParameters->displayHeight = 720;
										break;
		case AVCRECORDSIZESTANDARD2 :	recordParameters->displayWidth = 854;
										recordParameters->displayHeight = 480;
										break;
		case AVCRECORDSIZESTANDARD3 :	recordParameters->displayWidth = 640;
										recordParameters->displayHeight = 480;
										break;
		case AVCRECORDSIZESTANDARD4 :	recordParameters->displayWidth = 640;
										recordParameters->displayHeight = 360;
										break;
		case AVCRECORDSIZESTANDARD5 :	recordParameters->displayWidth = 320;
										recordParameters->displayHeight = 240;
										break;
		case AVCRECORDSIZESTANDARD6 :	recordParameters->displayWidth = 176;
										recordParameters->displayHeight = 144;
										break;
										
	}

	recordParameters->parmsVaild = true;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////



BEGIN_MESSAGE_MAP(CAVCompressionReencodeDialog, CDialog)
	ON_BN_CLICKED(IDC_AVC_MODEFAST, &CAVCompressionReencodeDialog::OnBnClickedAVCOptionsFast)
	ON_BN_CLICKED(IDC_AVC_MODEFULL, &CAVCompressionReencodeDialog::OnBnClickedAVCOptionsFull)
	ON_BN_CLICKED(IDC_AVC_MODEDISCARD, &CAVCompressionReencodeDialog::OnBnClickedAVCOptionsDiscard)
END_MESSAGE_MAP()



// Constructor
CAVCompressionReencodeDialog::CAVCompressionReencodeDialog()  : CDialog(IDD2)
{
		m_compressMode = TRANSCODENULL;
}


void CAVCompressionReencodeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

// Initialize the dialog box buttons and text boxes
BOOL CAVCompressionReencodeDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}




void CAVCompressionReencodeDialog::OnBnClickedAVCOptionsFast()
{
	m_compressMode = TRANSCODEFAST;
	EndDialog(IDOK);
}

void CAVCompressionReencodeDialog::OnBnClickedAVCOptionsFull()
{
	m_compressMode = TRANSCODEFULL;
	EndDialog(IDOK);
}

void CAVCompressionReencodeDialog::OnBnClickedAVCOptionsDiscard()
{
	m_compressMode = TRANSCODENULL;
	EndDialog(IDOK);
}
