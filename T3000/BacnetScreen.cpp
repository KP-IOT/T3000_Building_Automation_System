// BacnetScreen.cpp : implementation file
//

#include "stdafx.h"
#include "T3000.h"
#include "BacnetScreen.h"
#include "afxdialogex.h"

#include "CM5/ud_str.h"
#include "Bacnet_Include.h"
#include "globle_function.h"
#include "gloab_define.h"
#include "MainFrm.h"
#include "BacnetScreenEdit.h"
CBacnetScreenEdit * ScreenEdit_Window = NULL;
Control_group_point m_temp_screen_data[BAC_SCREEN_COUNT];
IMPLEMENT_DYNAMIC(BacnetScreen, CDialogEx)

BacnetScreen::BacnetScreen(CWnd* pParent /*=NULL*/)
	: CDialogEx(BacnetScreen::IDD, pParent)
{

}

BacnetScreen::~BacnetScreen()
{
}

void BacnetScreen::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SCREEN, m_screen_list);
}


BEGIN_MESSAGE_MAP(BacnetScreen, CDialogEx)
	ON_MESSAGE(MY_RESUME_DATA, ScreenCallBack)
	ON_MESSAGE(WM_REFRESH_BAC_SCREEN_LIST,Fresh_Screen_List)	
	ON_MESSAGE(WM_LIST_ITEM_CHANGED,Fresh_Screen_Item)	
	ON_NOTIFY(NM_CLICK, IDC_LIST_SCREEN, &BacnetScreen::OnNMClickListScreen)
	ON_MESSAGE(WM_HOTKEY,&BacnetScreen::OnHotKey)//快捷键消息映射手动加入
	ON_MESSAGE(WM_SCREENEDIT_CLOSE,&BacnetScreen::Screeenedit_close_handle)//快捷键消息映射手动加入
	ON_WM_CLOSE()
	ON_WM_TIMER()
//	ON_WM_DESTROY()
END_MESSAGE_MAP()


// BacnetScreen message handlers


LRESULT  BacnetScreen::ScreenCallBack(WPARAM wParam, LPARAM lParam)
{
	_MessageInvokeIDInfo *pInvoke =(_MessageInvokeIDInfo *)lParam;
	bool msg_result=WRITE_FAIL;
	msg_result = MKBOOL(wParam);
	CString Show_Results;
	CString temp_cs = pInvoke->task_info;
	if(msg_result)
	{
		Show_Results = temp_cs + _T("Success!");
		SetPaneString(BAC_SHOW_MISSION_RESULTS,Show_Results);
		//MessageBox(_T("Bacnet operation success!"));
	}
	else
	{
		memcpy_s(&m_screen_data.at(pInvoke->mRow),sizeof(Control_group_point),&m_temp_screen_data[pInvoke->mRow],sizeof(Control_group_point));//还原没有改对的值
		PostMessage(WM_REFRESH_BAC_SCREEN_LIST,pInvoke->mRow,REFRESH_ON_ITEM);
		Show_Results = temp_cs + _T("Fail!");
		SetPaneString(BAC_SHOW_MISSION_RESULTS,Show_Results);
		//AfxMessageBox(Show_Results);
		//MessageBox(_T("Bacnet operation fail!"));
	}
	if((pInvoke->mRow%2)==0)	//恢复前景和 背景 颜色;
		m_screen_list.SetItemBkColor(pInvoke->mRow,pInvoke->mCol,LIST_ITEM_DEFAULT_BKCOLOR,0);
	else
		m_screen_list.SetItemBkColor(pInvoke->mRow,pInvoke->mCol,LIST_ITEM_DEFAULT_BKCOLOR_GRAY,0);
	m_screen_list.RedrawItems(pInvoke->mRow,pInvoke->mRow);


	if(pInvoke)
		delete pInvoke;
	return 0;
}


BOOL BacnetScreen::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN) 
	{
		CRect list_rect,win_rect;
		m_screen_list.GetWindowRect(list_rect);
		ScreenToClient(&list_rect);
		::GetWindowRect(m_screen_dlg_hwnd,win_rect);
		m_screen_list.Set_My_WindowRect(win_rect);
		m_screen_list.Set_My_ListRect(list_rect);

		m_screen_list.Get_clicked_mouse_position();
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}
LRESULT BacnetScreen::Screeenedit_close_handle(WPARAM wParam,LPARAM lParam)
{
	Reg_Hotkey();
	if(ScreenEdit_Window)
	{
		delete ScreenEdit_Window;
		ScreenEdit_Window = NULL;
	}
	return 0;
}


LRESULT BacnetScreen::OnHotKey(WPARAM wParam,LPARAM lParam)
{
	if (wParam==KEY_INSERT)
	{
		//OnBnClickedButtonProgramEdit();
		for (int i=0;i<m_screen_list.GetItemCount();++i)
		{
			if(m_screen_list.GetCellChecked(i,0))
			{
				screen_list_line = i;
				break;
			}
		}


		bac_cm5_graphic = true;
		Unreg_Hotkey();


		if(ScreenEdit_Window != NULL)
		{
			delete ScreenEdit_Window;
		}
		ScreenEdit_Window = new CBacnetScreenEdit;
		ScreenEdit_Window->Create(IDD_DIALOG_BACNET_SCREENS_EDIT,this);	
		ScreenEdit_Window->ShowWindow(SW_SHOW);




		// TODO: Add your command handler code here
	}
	return 0;
}

BOOL BacnetScreen::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	Initial_List();
	PostMessage(WM_REFRESH_BAC_SCREEN_LIST,NULL,NULL);

//	RegisterHotKey(GetSafeHwnd(),KEY_INSERT,NULL,VK_INSERT);//F2键
	SetTimer(1,BAC_LIST_REFRESH_TIME,NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void BacnetScreen::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialogEx::OnOK();
}


void BacnetScreen::Initial_List()
{


	m_screen_list.ModifyStyle(0, LVS_SINGLESEL|LVS_REPORT|LVS_SHOWSELALWAYS);
	m_screen_list.SetExtendedStyle(m_screen_list.GetExtendedStyle()  |LVS_EX_GRIDLINES&(~LVS_EX_FULLROWSELECT));//Not allow full row select.
	m_screen_list.InsertColumn(SCREEN_NUM, _T("NUM"), 40, ListCtrlEx::CheckBox, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_screen_list.InsertColumn(SCREEN_DESCRIPTION, _T("Full Label"), 180, ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_screen_list.InsertColumn(SCREEN_LABEL, _T("Label"), 120, ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_screen_list.InsertColumn(SCREEN_PIC_FILE, _T("Picture File"), 140, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_screen_list.InsertColumn(SCREEN_MODE, _T("Mode"), 80, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_screen_list.InsertColumn(SCREEN_REFRESH, _T("Refresh Time"), 110, ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	m_screen_dlg_hwnd = this->m_hWnd;
	g_hwnd_now = m_screen_dlg_hwnd;
	
	CRect list_rect,win_rect;
	m_screen_list.GetWindowRect(list_rect);
	ScreenToClient(&list_rect);
	::GetWindowRect(m_screen_dlg_hwnd,win_rect);
	m_screen_list.Set_My_WindowRect(win_rect);
	m_screen_list.Set_My_ListRect(list_rect);


	m_screen_list.DeleteAllItems();
	for (int i=0;i<(int)m_screen_data.size();i++)
	{
		CString temp_item,temp_value,temp_cal,temp_filter,temp_status,temp_lable;
		CString temp_des;
		CString temp_units;
		temp_item.Format(_T("%d"),i+1);
		m_screen_list.InsertItem(i,temp_item);
		m_screen_list.SetCellEnabled(i,SCREEN_NUM,0);
		m_screen_list.SetCellChecked(0,0,1);// default will set the first one checked


		for (int x=0;x<SCREEN_COL_NUMBER;x++)
		{
			if((i%2)==0)
				m_screen_list.SetItemBkColor(i,x,LIST_ITEM_DEFAULT_BKCOLOR);
			else
				m_screen_list.SetItemBkColor(i,x,LIST_ITEM_DEFAULT_BKCOLOR_GRAY);		
		}

	}
}

LRESULT BacnetScreen::Fresh_Screen_List(WPARAM wParam,LPARAM lParam)
{
	int Fresh_Item;
	int isFreshOne = (int)lParam;
	if(isFreshOne == REFRESH_ON_ITEM)
	{
		Fresh_Item = (int)wParam;
	}
	else
	{
		if(m_screen_list.IsDataNewer((char *)&m_screen_data.at(0),sizeof(Control_group_point) * BAC_SCREEN_COUNT))
		{
			//避免list 刷新时闪烁;在没有数据变动的情况下不刷新List;
			m_screen_list.SetListData((char *)&m_screen_data.at(0),sizeof(Control_group_point) * BAC_SCREEN_COUNT);
		}
		else
		{
			return 0;
		}
	}

	for (int i=0;i<(int)m_screen_data.size();i++)
	{
		CString temp_des,temp_label,temp_pic_file;

		if(isFreshOne)
		{
			i = Fresh_Item;
		}

		MultiByteToWideChar( CP_ACP, 0, (char *)m_screen_data.at(i).description, (int)strlen((char *)m_screen_data.at(i).description)+1, 
			temp_des.GetBuffer(MAX_PATH), MAX_PATH );
		temp_des.ReleaseBuffer();

		m_screen_list.SetItemText(i,SCREEN_DESCRIPTION,temp_des);

		MultiByteToWideChar( CP_ACP, 0, (char *)m_screen_data.at(i).label, (int)strlen((char *)m_screen_data.at(i).label)+1, 
			temp_label.GetBuffer(MAX_PATH), MAX_PATH );
		temp_label.ReleaseBuffer();

		m_screen_list.SetItemText(i,SCREEN_LABEL,temp_label);


		MultiByteToWideChar( CP_ACP, 0, (char *)m_screen_data.at(i).picture_file, (int)strlen((char *)m_screen_data.at(i).picture_file)+1, 
			temp_pic_file.GetBuffer(MAX_PATH), MAX_PATH );
		temp_pic_file.ReleaseBuffer();

		m_screen_list.SetItemText(i,SCREEN_PIC_FILE,temp_pic_file);

		m_screen_list.SetItemText(i,SCREEN_MODE,_T("Graphic"));
		CString cs_refresh_time;
		cs_refresh_time.Format(_T("%d"),m_screen_data.at(i).update);
		m_screen_list.SetItemText(i,SCREEN_REFRESH,cs_refresh_time);
		if(isFreshOne)
		{
			break;
		}
	}
	return 0;
}

LRESULT BacnetScreen::Fresh_Screen_Item(WPARAM wParam,LPARAM lParam)
{
	int Changed_Item = (int)wParam;
	int Changed_SubItem = (int)lParam;

	CString temp_task_info;
	CString New_CString =  m_screen_list.GetItemText(Changed_Item,Changed_SubItem);
	CString cstemp_value;

	memcpy_s(&m_temp_screen_data[Changed_Item],sizeof(Control_group_point),&m_screen_data.at(Changed_Item),sizeof(Control_group_point));

	if(Changed_SubItem == SCREEN_DESCRIPTION)
	{
		CString cs_temp = m_screen_list.GetItemText(Changed_Item,Changed_SubItem);
		if(cs_temp.GetLength()>= STR_SCREEN_DESCRIPTION_LENGTH)	//长度不能大于结构体定义的长度;
		{
			MessageBox(_T("Warning"),_T("Length can not higher than 20"));
			PostMessage(WM_REFRESH_BAC_SCREEN_LIST,NULL,NULL);
			return 0;
		}

		char cTemp1[255];
		memset(cTemp1,0,255);
		WideCharToMultiByte( CP_ACP, 0, cs_temp.GetBuffer(), -1, cTemp1, 255, NULL, NULL );
		memcpy_s(m_screen_data.at(Changed_Item).description,STR_SCREEN_DESCRIPTION_LENGTH,cTemp1,STR_SCREEN_DESCRIPTION_LENGTH);
	}

	if(Changed_SubItem == SCREEN_LABEL)
	{
		CString cs_temp = m_screen_list.GetItemText(Changed_Item,Changed_SubItem);
		if(cs_temp.GetLength()>= STR_SCREEN_LABLE_LENGTH)	//长度不能大于结构体定义的长度;
		{
			MessageBox(_T("Length can not higher than 8"),_T("Warning"));
			PostMessage(WM_REFRESH_BAC_SCREEN_LIST,NULL,NULL);
			return 0;
		}
		cs_temp.MakeUpper();
		char cTemp1[255];
		memset(cTemp1,0,255);
		WideCharToMultiByte( CP_ACP, 0, cs_temp.GetBuffer(), -1, cTemp1, 255, NULL, NULL );
		memcpy_s(m_screen_data.at(Changed_Item).label,STR_SCREEN_LABLE_LENGTH,cTemp1,STR_SCREEN_LABLE_LENGTH);
	}

	if (Changed_SubItem == SCREEN_REFRESH)
	{
		int temp_value;
		if((New_CString.GetLength()>=4) || (_wtoi(New_CString)>255))
		{
			MessageBox(_T("Please input a value between 0 - 255"),_T("Warning"),MB_OK | MB_ICONINFORMATION);
			PostMessage(WM_REFRESH_BAC_SCREEN_LIST,NULL,NULL);//restore the list.
			return 0;
		}
		temp_value = _wtoi(New_CString);
		m_screen_data.at(Changed_Item).update = temp_value;
	}
	int cmp_ret = memcmp(&m_temp_screen_data[Changed_Item],&m_screen_data.at(Changed_Item),sizeof(Control_group_point));
	if(cmp_ret!=0)
	{
		m_screen_list.SetItemBkColor(Changed_Item,Changed_SubItem,LIST_ITEM_CHANGED_BKCOLOR);
		temp_task_info.Format(_T("Write Screen List Item%d .Changed to \"%s\" "),Changed_Item + 1,New_CString);
		Post_Write_Message(g_bac_instance,WRITESCREEN_T3000,Changed_Item,Changed_Item,sizeof(Control_group_point),m_screen_dlg_hwnd ,temp_task_info,Changed_Item,Changed_SubItem);

	}


	
	return 0;
}




void BacnetScreen::OnNMClickListScreen(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	DWORD dwPos=GetMessagePos();//Get which line is click by user.Set the check box, when user enter Insert it will jump to program dialog
	CPoint point( LOWORD(dwPos), HIWORD(dwPos));
	m_screen_list.ScreenToClient(&point);
	LVHITTESTINFO lvinfo;
	lvinfo.pt=point;
	lvinfo.flags=LVHT_ABOVE;
	int nItem=m_screen_list.SubItemHitTest(&lvinfo);
	if(nItem!=-1)
	{ 
		m_screen_list.SetCellChecked(nItem,0,1);
		screen_list_line = nItem;
		for (int i=0;i<m_screen_list.GetItemCount();++i)
		{
			if(i == nItem)
				continue;
			m_screen_list.SetCellChecked(i,0,FALSE);
		}
	}
	int lRow = 0;
	int lCol = 0;
	lRow = lvinfo.iItem;
	lCol = lvinfo.iSubItem;


	if(lRow>m_screen_list.GetItemCount()) //如果点击区超过最大行号，则点击是无效的;
		return;
	if(lRow<0)
		return;
	m_screen_list.Set_Edit(true);
	CString New_CString;
	CString temp_task_info;

	if(lCol == SCREEN_PIC_FILE)
	{
		CString FilePath;
		CString image_fordor;
		CString ApplicationFolder;
		GetModuleFileName(NULL, ApplicationFolder.GetBuffer(MAX_PATH), MAX_PATH);
		PathRemoveFileSpec(ApplicationFolder.GetBuffer(MAX_PATH));
		ApplicationFolder.ReleaseBuffer();
		image_fordor = ApplicationFolder + _T("\\Database\\image");
		SetCurrentDirectoryW(image_fordor);
		//选择图片,如果选的不在database目录下就copy一份过来;如果在的话就重命名，因为文件名长度不能超过10个字节;
		CString strFilter = _T("jpg file;bmp file;png file|*.jpg;*.bmp;*.png|all File|*.*||");
		CFileDialog dlg(true,_T("bmp"),NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,strFilter);
		if(IDOK==dlg.DoModal())
		{
			FilePath=dlg.GetPathName();
			CString FileName;
			GetFileNameFromPath(FilePath,FileName);
			
			CString temp1;
			temp1 = FilePath;
			PathRemoveFileSpec(temp1.GetBuffer(MAX_PATH));
			temp1.ReleaseBuffer();

			if(FileName.GetLength() >=11)
			{
				FileName = FileName.Right(10);
			}
			CString new_file_path;
			new_file_path = image_fordor + _T("\\") + FileName;
			if(temp1.CompareNoCase(image_fordor) != 0)//如果就在当前目录就不用copy过来了;
			{
				CopyFile(FilePath,new_file_path,false);
			}
			else
			{
				CFile::Rename(FilePath, new_file_path);
			}
			memcpy_s(&m_temp_screen_data[lRow],sizeof(Control_group_point),&m_screen_data.at(lRow),sizeof(Control_group_point));

			char cTemp1[255];
			memset(cTemp1,0,255);
			WideCharToMultiByte( CP_ACP, 0, FileName.GetBuffer(), -1, cTemp1, 255, NULL, NULL );
			memcpy_s(m_screen_data.at(lRow).picture_file,STR_SCREEN_PIC_FILE_LENGTH,cTemp1,STR_SCREEN_PIC_FILE_LENGTH);

			m_screen_list.SetItemText(lRow,SCREEN_PIC_FILE,FileName);
			New_CString = FileName;
		}
	}
	else
		return;

	m_screen_list.Set_Edit(false);

	int cmp_ret = memcmp(&m_temp_screen_data[lRow],&m_screen_data.at(lRow),sizeof(Control_group_point));
	if(cmp_ret!=0)
	{
		m_screen_list.SetItemBkColor(lRow,lCol,LIST_ITEM_CHANGED_BKCOLOR);
		temp_task_info.Format(_T("Write Screen List Item%d .Changed to \"%s\" "),lRow + 1,New_CString);
		Post_Write_Message(g_bac_instance,WRITESCREEN_T3000,lRow,lRow,sizeof(Control_group_point),m_screen_dlg_hwnd ,temp_task_info,lRow,lCol);

	}


	*pResult = 0;
}


void BacnetScreen::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	UnregisterHotKey(GetSafeHwnd(),KEY_INSERT);
	KillTimer(1);
	CDialogEx::OnClose();
	::PostMessage(BacNet_hwd,WM_DELETE_NEW_MESSAGE_DLG,DELETE_WINDOW_MSG,0);
}

void BacnetScreen::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
//	KillTimer(1);
//	m_screen_dlg_hwnd = NULL;
	::PostMessage(BacNet_hwd,WM_DELETE_NEW_MESSAGE_DLG,DELETE_WINDOW_MSG,0);
//	CDialogEx::OnCancel();
}

void BacnetScreen::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if(this->IsWindowVisible())
	{
	PostMessage(WM_REFRESH_BAC_SCREEN_LIST,NULL,NULL);
	if(bac_select_device_online)
		Post_Refresh_Message(g_bac_instance,READSCREEN_T3000,0,BAC_SCREEN_COUNT - 1,sizeof(Control_group_point),BAC_SCREEN_GROUP);
	}
	CDialogEx::OnTimer(nIDEvent);
}

void BacnetScreen::Reg_Hotkey()
{
	RegisterHotKey(GetSafeHwnd(),KEY_INSERT,NULL,VK_INSERT);//Insert键
}

void BacnetScreen::Unreg_Hotkey()
{
	UnregisterHotKey(GetSafeHwnd(),KEY_INSERT);
}


