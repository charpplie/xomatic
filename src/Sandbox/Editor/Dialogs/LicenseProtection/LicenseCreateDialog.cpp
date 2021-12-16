// LicenseLoginDialog.cpp : implementation file
//

#include "stdafx.h"
#include "LicenseCreateDialog.h"

#if defined(IS_PROSDK)
#include "LibSDKEvaluationWrapper.h"

IMPLEMENT_DYNAMIC(CLicenseCreateDialog, CDialog)

const int MinTextLength = 4;
const int MaxTextLength = 20;
const int MessageBuffersize = 128;

//////////////////////////////////////////////////////////////////////////
CLicenseCreateDialog::CLicenseCreateDialog(CWnd* pParent /*=NULL*/) :
	CDialog(CLicenseCreateDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLicenseCreateDialog)
	//}}AFX_DATA_INIT
}


//////////////////////////////////////////////////////////////////////////
BOOL CLicenseCreateDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	RegisterCountryCombo();
	m_eUsername.SetFocus();
	return FALSE;
}


//////////////////////////////////////////////////////////////////////////
void CLicenseCreateDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicenseCreateDialog)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EDIT_Firstname, m_eFirstname);
	DDX_Control(pDX, IDC_EDIT_LASTNAME, m_eLastname);
	DDX_Control(pDX, IDC_EDIT_EMAIL, m_eEmail);
	DDX_Control(pDX, IDC_EDIT_ADDRESS, m_eAddress);
	DDX_Control(pDX, IDC_EDIT_CITY, m_eCity);
	DDX_Control(pDX, IDC_COMBO_COUNTRY, m_cCountry);
	DDX_Control(pDX, IDC_EDIT_STATE, m_eState);
	DDX_Control(pDX, IDC_EDIT_ZIPCODE, m_eZipcode);
	DDX_Control(pDX, IDC_EDIT_CREATEPASSWORD, m_ePassword);
	DDX_Control(pDX, IDC_EDIT_CREATEUSERNAME, m_eUsername);
	DDX_Control(pDX, IDC_BUTTON_CREATESUBMIT, m_buttonSubmit);
	DDX_Control(pDX, IDC_BUTTON_CREATECANCEL, m_buttonCancel);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CLicenseCreateDialog, CDialog)
	//{{AFX_MSG_MAP(CLicenseCreateDialog)
	// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_CREATESUBMIT, &CLicenseCreateDialog::OnBnClickedButtonCreatesubmit)
	ON_BN_CLICKED(IDC_BUTTON_CREATECANCEL, &CLicenseCreateDialog::OnBnClickedButtonCreatecancel)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////

void CLicenseCreateDialog::OnBnClickedButtonCreatesubmit()
{
	// TODO: Add your control notification handler code here
	MakeDisableInput();
	CreateAccountImpl();
	MakeEnableInput();
}

void CLicenseCreateDialog::OnBnClickedButtonCreatecancel()
{
	// TODO: Add your control notification handler code here
	EndDialog(0);
}

bool CLicenseCreateDialog::IsValidEditText()
{
	if (false == CheckUsernameEdit())
		return false;
	if (false == CheckPasswordEdit())
		return false;
	if (false == CheckEachEdit(m_eFirstname, "First name"))
		return false;
	if (false == CheckEachEdit(m_eLastname, "Last name"))
		return false;
	if (false == CheckEachEdit(m_eEmail, "Email address"))
		return false;
	CString emailText;
	m_eEmail.GetWindowText(emailText);
	if (-1 == emailText.Find('@') || -1 == emailText.Find('.'))
	{
		MessageBox("Invalid Email address.", "Error", MB_ICONERROR);
		m_eEmail.SetFocus();
		return false;
	}

	if (false == CheckEachEdit(m_eAddress, "Address"))
		return false;
	if (false == CheckEachEdit(m_eCity, "City"))
		return false;

	CString countryText;
	m_cCountry.GetWindowText(countryText);
	if (countryText.IsEmpty())
	{
		const int buffersize = 128;
		char message[buffersize] = {0,};
		sprintf_s(message, buffersize, "Please select Country.", MB_ICONERROR);
		MessageBox(message, "Error");
		m_cCountry.SetFocus();
		return false;
	}
	if (false == CheckEachEdit(m_eState, "State"))
		return false;
	if (false == CheckEachEdit(m_eZipcode, "Zip code"))
		return false;

	return true;
}


bool CLicenseCreateDialog::CheckEachEdit(CEdit& editObj, CString editTitle)
{
	CString editText;
	editObj.GetWindowText(editText);
	if (editText.IsEmpty())
	{
		const int buffersize = 128;
		char message[buffersize] = {0,};
		sprintf_s(message, buffersize, "Please fill %s.", editTitle);
		MessageBox(message, "Error", MB_ICONERROR);
		editObj.SetFocus();
		return false;
	}
	return true;
}

void CLicenseCreateDialog::RegisterCountryCombo()
{
#define COUNTRY_NAME(name) m_cCountry.AddString(name)
	COUNTRY_NAME("Aland Islands");
	COUNTRY_NAME("Albania");
	COUNTRY_NAME("Algeria");
	COUNTRY_NAME("American Samoa");
	COUNTRY_NAME("Andorra");
	COUNTRY_NAME("Angola");
	COUNTRY_NAME("Anguilla");
	COUNTRY_NAME("Antarctica");
	COUNTRY_NAME("Antigua and Barbuda");
	COUNTRY_NAME("Argentina");
	COUNTRY_NAME("Armenia");
	COUNTRY_NAME("Aruba");
	COUNTRY_NAME("Ascension Islands");
	COUNTRY_NAME("Asia/Pacific Region");
	COUNTRY_NAME("Australia");
	COUNTRY_NAME("Austria");
	COUNTRY_NAME("Azerbaijan");
	COUNTRY_NAME("Bahamas");
	COUNTRY_NAME("Bahrain");
	COUNTRY_NAME("Bangladesh");
	COUNTRY_NAME("Barbados");
	COUNTRY_NAME("Belarus");
	COUNTRY_NAME("Belgium");
	COUNTRY_NAME("Belize");
	COUNTRY_NAME("Benin");
	COUNTRY_NAME("Bermuda");
	COUNTRY_NAME("Bhutan");
	COUNTRY_NAME("Bolivia");
	COUNTRY_NAME("Bosnia and Herzegovina");
	COUNTRY_NAME("Botswana");
	COUNTRY_NAME("Bouvet Island");
	COUNTRY_NAME("Brazi");
	COUNTRY_NAME("British Indian Ocean Territory");
	COUNTRY_NAME("Brunei Darussalam");
	COUNTRY_NAME("Bulgaria");
	COUNTRY_NAME("Burkina Faso");
	COUNTRY_NAME("Burundi");
	COUNTRY_NAME("Cambodia");
	COUNTRY_NAME("Cameroon");
	COUNTRY_NAME("Canada");
	COUNTRY_NAME("Cape Verde");
	COUNTRY_NAME("Cayman Islands");
	COUNTRY_NAME("Central African Republic");
	COUNTRY_NAME("Chad");
	COUNTRY_NAME("Chile");
	COUNTRY_NAME("China");
	COUNTRY_NAME("Cocos Islands");
	COUNTRY_NAME("Colombia");
	COUNTRY_NAME("Comoros");
	COUNTRY_NAME("Congo");
	COUNTRY_NAME("Cook Islands");
	COUNTRY_NAME("Costa Rica");
	COUNTRY_NAME("Cote D'Ivoire");
	COUNTRY_NAME("Croatia");
	COUNTRY_NAME("Cuba");
	COUNTRY_NAME("Cyprus");
	COUNTRY_NAME("Czech Republic");
	COUNTRY_NAME("Denmark");
	COUNTRY_NAME("Djibouti");
	COUNTRY_NAME("Dominica");
	COUNTRY_NAME("East Timor");
	COUNTRY_NAME("Ecuador");
	COUNTRY_NAME("Egypt");
	COUNTRY_NAME("El Salvador");
	COUNTRY_NAME("Equatorial Guinea");
	COUNTRY_NAME("Eritrea");
	COUNTRY_NAME("Estonia");
	COUNTRY_NAME("Ethiopia");
	COUNTRY_NAME("Falkland Islands (Malvinas)");
	COUNTRY_NAME("Faroe Islands");
	COUNTRY_NAME("Fiji");
	COUNTRY_NAME("Finland");
	COUNTRY_NAME("France");
	COUNTRY_NAME("French Guiana");
	COUNTRY_NAME("French Polynesia");
	COUNTRY_NAME("Gabon");
	COUNTRY_NAME("Gambia");
	COUNTRY_NAME("Georgia");
	COUNTRY_NAME("Germany");
	COUNTRY_NAME("Ghana");
	COUNTRY_NAME("Gibraltar");
	COUNTRY_NAME("Greece");
	COUNTRY_NAME("Greenland");
	COUNTRY_NAME("Grenada");
	COUNTRY_NAME("Guadeloupe");
	COUNTRY_NAME("Guam");
	COUNTRY_NAME("Guatemala");
	COUNTRY_NAME("Guernsey");
	COUNTRY_NAME("Guinea");
	COUNTRY_NAME("Guinea-Bissau");
	COUNTRY_NAME("Guyana");
	COUNTRY_NAME("Haiti");
	COUNTRY_NAME("Heard Island and McDonald Islands");
	COUNTRY_NAME("Holy See (Vatican City State)");
	COUNTRY_NAME("Honduras");
	COUNTRY_NAME("Hong Kong");
	COUNTRY_NAME("Hungary");
	COUNTRY_NAME("Iceland");
	COUNTRY_NAME("India");
	COUNTRY_NAME("Indonesia");
	COUNTRY_NAME("Iran");
	COUNTRY_NAME("Iraq");
	COUNTRY_NAME("Ireland");
	COUNTRY_NAME("Isle of Man");
	COUNTRY_NAME("Israe");
	COUNTRY_NAME("Italy");
	COUNTRY_NAME("Jamaica");
	COUNTRY_NAME("Japan");
	COUNTRY_NAME("Jersey");
	COUNTRY_NAME("Jordan");
	COUNTRY_NAME("Kazakstan");
	COUNTRY_NAME("Kenya");
	COUNTRY_NAME("Kiribati");
	COUNTRY_NAME("Korea");
	COUNTRY_NAME("Kuwait");
	COUNTRY_NAME("Kyrgyzstan");
	COUNTRY_NAME("Lao People's Democratic Republic");
	COUNTRY_NAME("Latvia");
	COUNTRY_NAME("Lebanon");
	COUNTRY_NAME("Lesotho");
	COUNTRY_NAME("Liberia");
	COUNTRY_NAME("Libyan Arab Jamahiriya");
	COUNTRY_NAME("Liechtenstein");
	COUNTRY_NAME("Lithuania");
	COUNTRY_NAME("Luxembourg");
	COUNTRY_NAME("Macau");
	COUNTRY_NAME("Macedonia");
	COUNTRY_NAME("Madagascar");
	COUNTRY_NAME("Malawi");
	COUNTRY_NAME("Malaysia");
	COUNTRY_NAME("Maldives");
	COUNTRY_NAME("Mali");
	COUNTRY_NAME("Malta");
	COUNTRY_NAME("Marshall Islands");
	COUNTRY_NAME("Martinique");
	COUNTRY_NAME("Mauritania");
	COUNTRY_NAME("Mauritius");
	COUNTRY_NAME("Mayotte");
	COUNTRY_NAME("Mexico");
	COUNTRY_NAME("Micronesia, Federated States of");
	COUNTRY_NAME("Moldova, Republic of");
	COUNTRY_NAME("Monaco");
	COUNTRY_NAME("Mongolia");
	COUNTRY_NAME("Montenegro");
	COUNTRY_NAME("Montserrat");
	COUNTRY_NAME("Morocco");
	COUNTRY_NAME("Mozambique");
	COUNTRY_NAME("Myanmar");
	COUNTRY_NAME("Namibia");
	COUNTRY_NAME("Nauru");
	COUNTRY_NAME("Nepa");
	COUNTRY_NAME("Netherlands");
	COUNTRY_NAME("Netherlands Antilles");
	COUNTRY_NAME("New Caledonia");
	COUNTRY_NAME("New Zealand");
	COUNTRY_NAME("Nicaragua");
	COUNTRY_NAME("Niger");
	COUNTRY_NAME("Nigeria");
	COUNTRY_NAME("Niue");
	COUNTRY_NAME("Norfolk Island");
	COUNTRY_NAME("Northern Mariana Islands");
	COUNTRY_NAME("Norway");
	COUNTRY_NAME("Oman");
	COUNTRY_NAME("Pakistan");
	COUNTRY_NAME("Palau");
	COUNTRY_NAME("Palestinian Territory, Occupied");
	COUNTRY_NAME("Panama");
	COUNTRY_NAME("Papua New Guinea");
	COUNTRY_NAME("Paraguay");
	COUNTRY_NAME("Peru");
	COUNTRY_NAME("Philippines");
	COUNTRY_NAME("Pitcairn Islnds");
	COUNTRY_NAME("Poland");
	COUNTRY_NAME("Portuga");
	COUNTRY_NAME("Puerto Rico");
	COUNTRY_NAME("Qatar");
	COUNTRY_NAME("Reunion");
	COUNTRY_NAME("Romania");
	COUNTRY_NAME("Russian Federation");
	COUNTRY_NAME("Rwanda");
	COUNTRY_NAME("Saint Kitts and Nevis");
	COUNTRY_NAME("Saint Lucia");
	COUNTRY_NAME("Saint Pierre and Miquelon");
	COUNTRY_NAME("Saint Vincent and the Grenadines");
	COUNTRY_NAME("Samoa");
	COUNTRY_NAME("San Marino");
	COUNTRY_NAME("Sao Tome and Principe");
	COUNTRY_NAME("Saudi Arabia");
	COUNTRY_NAME("Senega");
	COUNTRY_NAME("Serbia");
	COUNTRY_NAME("Serbia-Montenegro");
	COUNTRY_NAME("Seychelles");
	COUNTRY_NAME("Sierra Leone");
	COUNTRY_NAME("Singapore");
	COUNTRY_NAME("Slovakia");
	COUNTRY_NAME("Slovenia");
	COUNTRY_NAME("Solomon Islands");
	COUNTRY_NAME("Somalia");
	COUNTRY_NAME("South Africa");
	COUNTRY_NAME("Spain");
	COUNTRY_NAME("Sri Lanka");
	COUNTRY_NAME("St. Helena");
	COUNTRY_NAME("Sudan");
	COUNTRY_NAME("Suriname");
	COUNTRY_NAME("Svalbard");
	COUNTRY_NAME("Swaziland");
	COUNTRY_NAME("Sweden");
	COUNTRY_NAME("Switzerland");
	COUNTRY_NAME("Syrian Arab Republic");
	COUNTRY_NAME("Taiwan");
	COUNTRY_NAME("Tajikistan");
	COUNTRY_NAME("Tanzania, United Republic of");
	COUNTRY_NAME("Thailand");
	COUNTRY_NAME("Togo");
	COUNTRY_NAME("Tokelau");
	COUNTRY_NAME("Tonga");
	COUNTRY_NAME("Trinidad and Tobago");
	COUNTRY_NAME("Tunisia");
	COUNTRY_NAME("Turkey");
	COUNTRY_NAME("Turkmenistan");
	COUNTRY_NAME("Turks and Caicos Islands");
	COUNTRY_NAME("Tuvalu");
	COUNTRY_NAME("Uganda");
	COUNTRY_NAME("Ukraine");
	COUNTRY_NAME("United Arab Emirates");
	COUNTRY_NAME("United Kingdom");
	COUNTRY_NAME("USA");
	COUNTRY_NAME("Uruguay");
	COUNTRY_NAME("Uzbekistan");
	COUNTRY_NAME("Vanuatu");
	COUNTRY_NAME("Venezuela");
	COUNTRY_NAME("Vietnam");
	COUNTRY_NAME("Virgin Islands, British");
	COUNTRY_NAME("Virgin Islands, U.S.");
	COUNTRY_NAME("Wallis and Futuna");
	COUNTRY_NAME("Yemen");
	COUNTRY_NAME("Yugoslavia");
	COUNTRY_NAME("Zambia");
	COUNTRY_NAME("Zimbabwe");
	COUNTRY_NAME("[other]");
#undef COUNTRY_NAME
}

void CLicenseCreateDialog::CreateAccountImpl()
{
	if (false == IsValidEditText())
		return;

	SCreateAccountParam param;
	GetAccountParamText(param);
	
	string resultString, resultDetail;
	EPMErrorCode subErrorCode = PMEC_NO_ERROR;
	IEvaluationManager* pEM = GetISystem()->GetEvaluationManager();

	ECreateAccountResult resultCode;
	resultCode = pEM->RequestCreateAccount(param, subErrorCode);

	// if it takes long that user inputs userdata to dialog, socket could be timeout.
	// Retry send account information to LicenseServer, 
	if (resultCode == eCA_Undefined && (subErrorCode == PMEC_CANT_WRITE_TO_SOCKET || subErrorCode == PMEC_CANT_READ_FROM_SOCKET))
		resultCode = pEM->RequestCreateAccount(param, subErrorCode);

	if (eCA_Success == resultCode)
	{
		MessageBox("Your account has been created successfully.\n\nPlease check your e-mail for authentication.", "CreateAccount", MB_ICONINFORMATION);
		EndDialog(IDOK);
	}
	else if (eCA_BanCharacter == resultCode)
	{
		MessageBox("Please use only alphanumeric characters for your Username.", "CreateAccount Error", MB_ICONERROR);
		m_eUsername.SetFocus();
	}
	else if (eCA_DuplicateUsername == resultCode)
	{
		MessageBox("Duplicate Username.\n\nPlease try again with another Username.", "CreateAccount Error", MB_ICONERROR);
		m_eUsername.SetFocus();
	}
	else if (eCA_DuplicateEmail == resultCode)
	{
		MessageBox("Duplicate e-mail address.\n\nPlease try again with another e-mail address.", "CreateAccount Error", MB_ICONERROR);
		m_eEmail.SetFocus();
	}
	else if (eCA_InvalidDomain == resultCode)
	{
		MessageBox("The e-mail domain must comply with your Universities domain.\n\nPlease try again with valid e-mail domain.", "CreateAccount Error", MB_ICONERROR);
		m_eEmail.SetFocus();
	}
	else if (eCA_Undefined == resultCode)
	{
		if (PMEC_NO_ERROR == subErrorCode)
		{
			MessageBox("Undefined error.\n\nPlease try again later.", "CreateAccount Error", MB_ICONERROR);
		}
		else
		{
			pEM->AlertServerErrorMessage(subErrorCode, GetSafeHwnd());
		}
	}
}

void CLicenseCreateDialog::MakeEnableInput()
{
	m_buttonSubmit.EnableWindow(TRUE);
	m_buttonCancel.EnableWindow(TRUE);
}

void CLicenseCreateDialog::MakeDisableInput()
{
	m_buttonSubmit.EnableWindow(FALSE);
	m_buttonCancel.EnableWindow(FALSE);
}

void CLicenseCreateDialog::GetAccountParamText( SCreateAccountParam& param )
{
	ConvertEditText(m_eUsername, param.username);
	ConvertEditText(m_ePassword, param.password);
	ConvertEditText(m_eFirstname, param.firstname);
	ConvertEditText(m_eLastname, param.lastname);
	ConvertEditText(m_eEmail, param.email);
	ConvertEditText(m_eAddress, param.address);
	ConvertEditText(m_eCity, param.city);
	ConvertEditText(m_eState, param.state);
	ConvertEditText(m_eZipcode, param.zipcode);

	CString countryText;
	m_cCountry.GetWindowText(countryText);
	param.country.assign(countryText.GetString(), countryText.GetLength());
}

void CLicenseCreateDialog::ConvertEditText( CEdit& editObj, string& text )
{
	CString cstringText;
	editObj.GetWindowText(cstringText);
	text.assign(cstringText.GetString(), cstringText.GetLength());
}

bool CLicenseCreateDialog::CheckUsernameEdit()
{
	CString editText;
	m_eUsername.GetWindowText(editText);
	if (editText.IsEmpty())
	{
		char message[MessageBuffersize] = {0,};
		sprintf_s(message, MessageBuffersize, "Please fill %s.", "Username");
		MessageBox(message, "Error", MB_ICONERROR);
		m_eUsername.SetFocus();
		return false;
	}
	if (editText.GetLength() < MinTextLength)
	{
		char message[MessageBuffersize] = {0,};
		sprintf_s(message, MessageBuffersize, "Username must be longer than %d.", MinTextLength);
		MessageBox(message, "Error", MB_ICONERROR);
		m_eUsername.SetFocus();
		return false;
	}
	if (editText.GetLength() > MaxTextLength)
	{
		char message[MessageBuffersize] = {0,};
		sprintf_s(message, MessageBuffersize, "Username must be less than %d.", MaxTextLength);
		MessageBox(message, "Error", MB_ICONERROR);
		m_eUsername.SetFocus();
		return false;
	}
	return true;
}

bool CLicenseCreateDialog::CheckPasswordEdit()
{
	CString editText;
	m_ePassword.GetWindowText(editText);
	if (editText.IsEmpty())
	{
		char message[MessageBuffersize] = {0,};
		sprintf_s(message, MessageBuffersize, "Please fill Password.");
		MessageBox(message, "Error", MB_ICONERROR);
		m_ePassword.SetFocus();
		return false;
	}
	if (editText.GetLength() < MinTextLength)
	{
		char message[MessageBuffersize] = {0,};
		sprintf_s(message, MessageBuffersize, "Password must be longer than %d.", MinTextLength);
		MessageBox(message, "Error", MB_ICONERROR);
		m_ePassword.SetFocus();
		return false;
	}
	if (editText.GetLength() > MaxTextLength)
	{
		char message[MessageBuffersize] = {0,};
		sprintf_s(message, MessageBuffersize, "Password must be less than %d.", MaxTextLength);
		MessageBox(message, "Error", MB_ICONERROR);
		m_ePassword.SetFocus();
		return false;
	}
	return true;
}
#endif 