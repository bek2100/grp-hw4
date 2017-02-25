// PrespectiveControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "CGWork.h"
#include "PrespectiveControlDialog.h"
#include "afxdialogex.h"


// PrespectiveControlDialog dialog

IMPLEMENT_DYNAMIC(PrespectiveControlDialog, CDialogEx)

PrespectiveControlDialog::PrespectiveControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(PrespectiveControlDialog::IDD, pParent)
	, d(1)
	, m_prespective_alpha(0)
{

}

PrespectiveControlDialog::PrespectiveControlDialog(double default_d, double default_alpha, CWnd* pParent /*=NULL*/)
	: CDialogEx(PrespectiveControlDialog::IDD, pParent)
	, d(1)
{
	d = default_d;
	m_prespective_alpha = default_alpha;
}

PrespectiveControlDialog::~PrespectiveControlDialog()
{
}

void PrespectiveControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_PRESPECTIVE_D, d);
	DDV_MinMaxDouble(pDX, d, 0, d*d + 1);

	DDX_Text(pDX, IDC_PRESPECTIVE_ALPHA, m_prespective_alpha);
	DDV_MinMaxDouble(pDX, d, 0, d);
}


BEGIN_MESSAGE_MAP(PrespectiveControlDialog, CDialogEx)
END_MESSAGE_MAP()


// PrespectiveControlDialog message handlers
