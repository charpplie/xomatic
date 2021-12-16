// LicenseLoginDialog.cpp : implementation file
//

#include "stdafx.h"
#include "LicenseAgreementDialog.h"

#if defined(IS_PROSDK)

IMPLEMENT_DYNAMIC(CLicenseAgreementDialog, CDialog)


//////////////////////////////////////////////////////////////////////////
CLicenseAgreementDialog::CLicenseAgreementDialog(CWnd* pParent /*=NULL*/) :
CDialog(CLicenseAgreementDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLicenseAgreementDialog)
	//}}AFX_DATA_INIT
}


//////////////////////////////////////////////////////////////////////////
BOOL CLicenseAgreementDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	ConfigureAgreement();
	SetFocus();
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CLicenseAgreementDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicenseAgreementDialog)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EDIT_AGREEMENT, m_eAgreement);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CLicenseAgreementDialog, CDialog)
	//{{AFX_MSG_MAP(CLicenseAgreementDialog)
	// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_ACCEPT, &CLicenseAgreementDialog::OnBnClickedButtonAccept)
	ON_BN_CLICKED(IDC_BUTTON_DENY, &CLicenseAgreementDialog::OnBnClickedButtonDeny)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
void CLicenseAgreementDialog::ConfigureAgreement()
{
	const int BufferLength = 1024*8;
	char* agreementBody = new char[BufferLength];
	memset(agreementBody, 0, BufferLength);
	strcat_s(agreementBody, BufferLength, "CRYENGINE\r\n");
	strcat_s(agreementBody, BufferLength, "STUDENT/TEACHING STAFF\r\n");
	strcat_s(agreementBody, BufferLength, "END USER LICENSE AGREEMENT\r\n");
	strcat_s(agreementBody, BufferLength, "========================================================================\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "This CryEngine Student/Teaching Staff EULA (the 'Agreement') is entered into between Crytek GmbH, Frankfurt am Main ('Crytek'), and you, the end user.\r\n");
	strcat_s(agreementBody, BufferLength, "Crytek has entered into a Cost-Free Educational Site License Agreement with the educational institution of which you are a student or member of teaching staff. IF THIS IS NOT TRUE YOU ARE NOT ALLOWED TO USE THE CRYENGINE IN ANY WAY.\r\n");
	strcat_s(agreementBody, BufferLength, "You hereby acknowledge and confirm to be personally bound to the terms and conditions set forth below (you shall be referred to as 'Licensee' or 'you'). IF YOU DO NOT AGREE TO THAT TERMS AND CONDITIONS YOU ARE NOT ALLOWED TO USE THE CRYENGINE IN ANY WAY.\r\n");

	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "1. Grant of License\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "1.1. Grant: Subject to strict and continuous compliance with the restrictions in the Agreement by Licensee, Crytek grants to Licensee a non-exclusive, non-transferable, cost-free educational limited license on the campus of the educational institution you are a member of:\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "1.1.1. to use the CryEngine for the creation of games or other audiovisual projects and related tuition;\r\n");
	strcat_s(agreementBody, BufferLength, "1.1.2. to use rendered screenshots and videos of the self-created stand-alone games or other audiovisual projects as promotional material for Licensee ('portfolios').\r\n");

	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "1.2. Restrictions on Use: Licensee shall in no case:\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.1. COMMERCIALLY EXPLOIT THE SELF CREATED GAMES OR OTHER AUDIOVISUAL PROJECTS CREATED BY USING THE CRYENGINE IN ANY FORM (BE IT IN CONNECTION WITH THE CRYENGINE OR IN RENDERED FORM);\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.2. publish, distribute or exploit the CryEngine in any form, be it as source code or as object code, in whole or in part, be it embedded in other products or not;\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.3. use CryEngine in any manner which may disclose the CryEngine source code or other Confidential Information to any third party;\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.4. give anybody access to the CryEngine outside the Territory (e.g. by internet) or to any other person as its students or teaching staff;\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.5. use, copy, reproduce or duplicate, in any manner or form, in whole or in part CryEngine and any supporting documentation pertaining thereto (except only as necessary to exercise its license rights herein);\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.6. reverse engineer and/or decompile the CryEngine object code;\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.7. copy, replicate or mimic Crytek games (e.g. 'Crysis'), or the CryEngine itself, in any way;\r\n");
	strcat_s(agreementBody, BufferLength, "1.2.8. create any assets which infringe any third party right or which are libelous, defamatory, obscene, false, misleading or otherwise illegal or unlawful.\r\n");

	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "2. Warranty, Indemnification\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "2.1. CRYTEK WARRANTY DISCLAIMER. CRYTEK GIVES NO WARRANTY. THE CRYENGINE IS DELIVERED 'AS IS'.\r\n");
	strcat_s(agreementBody, BufferLength, "2.1.1. CRYTEK INDEMNIFICATION DISCLAIMER. IN NO EVENT SHALL CRYTEK BE LIABLE FOR ANY INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES, OR DAMAGES FOR LOSS OF PROFITS, REVENUE, DATA OR USE, INCURRED BY LICENSEE OR ANY THIRD PARTY, WHETHER IN AN ACTION IN CONTRACT OR TORT (INCLUDING NEGLIGENCE) OR OTHERWISE, EVEN IF CRYTEK HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. CRYTEK’S MAXIMUM AGGREGATE LIABILITY TO LICENSEE IN CONNECTION WITH OR IN ANY MANNER RELATED TO THIS AGREEMENT (WHETHER IN AN ACTION IN CONTRACT OR TORT (INCLUDING NEGLIGENCE) OR OTHERWISE) WILL BE LIMITED TO THE TOTAL AMOUNT PAID BY OR ON BEHALF OF LICENSEE TO CRYTEK UNDER THIS AGREEMENT. THE FOREGOING ALLOCATION OF RISK IS REFLECTED IN THE AMOUNT OF THE COMPENSATION CONTEMPLATED UNDER THIS AGREEMENT (NONE).\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "3. Proprietary Rights\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "Licensee acknowledges and agrees that nothing in this Agreement constitutes, or shall be construed to constitute, any transfer of title or ownership with respect to CryEngine including without limitation transfer of any copyrights, patents, trademarks, trade secrets, intellectual property, source code, improvements, enhancements, sequence, logic, structure, and other proprietary rights therein, now or hereafter existing.\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "4. General provisions\r\n");
	strcat_s(agreementBody, BufferLength, "\r\n");
	strcat_s(agreementBody, BufferLength, "4.1. If any provision of this Agreement shall be found to be invalid or unenforceable for legal or factual reasons the invalidity or unenforceability of such provisions shall not affect the other provisions of this Agreement and all provisions of this Agreement not affected by such invalidity or unenforceability shall remain in full force and effect. The parties hereby agree to attempt to substitute for any invalid or unenforceable provision a valid or enforceable provision which achieves to the greatest extent possible the economic and legal objective of the Agreement. This applies as well in case of an omission or invalidity of the whole Agreement.\r\n");
	strcat_s(agreementBody, BufferLength, "4.2. This Agreement is governed and construed by the laws of the Federal Republic of Germany. Place of performance and jurisdiction shall be Frankfurt am Main.\r\n");
	m_eAgreement.SetWindowText(agreementBody);

	delete [] agreementBody;
}

void CLicenseAgreementDialog::OnBnClickedButtonAccept()
{
	// TODO: Add your control notification handler code here
	EndDialog(IDOK);
}

void CLicenseAgreementDialog::OnBnClickedButtonDeny()
{
	// TODO: Add your control notification handler code here
	EndDialog(IDCANCEL);
}

#endif 
