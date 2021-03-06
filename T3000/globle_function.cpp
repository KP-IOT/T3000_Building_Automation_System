
#include "stdafx.h"
#include "../ISP/MyPing.h"
#include "globle_function.h"
#include "Windows.h"
#include "T3000.h"
#include "ado/ADO.h"


#include "T3000RegAddress.h"
#include "gloab_define.h"
#include "DialogCM5_BacNet.h"
#include "CM5\MyOwnListCtrl.h"
#include "BacnetInput.h"
#include "BacnetOutput.h"
#include "BacnetProgram.h"
#include "BacnetVariable.h"
#include "globle_function.h"

#include "gloab_define.h"
#include "datalink.h"
#include "BacnetWait.h"
#include "Bacnet_Include.h"
#include "CM5\ud_str.h"
#include "BacnetWeeklyRoutine.h"
#include "BacnetAnnualRoutine.h"
#include "AnnualRout_InsertDia.h"
#include "BacnetController.h"
#include "BacnetScreen.h"
#include "BacnetMonitor.h"
#include "rs485.h"

#define DELAY_TIME	 10	//MS
#define Modbus_Serial	0
#define	Modbus_TCP	1

#include "modbus_read_write.cpp"
#include "CM5\PTP\ptp.h"
#pragma region For_Bacnet_program_Use

extern char mycode[1024];
extern int my_lengthcode;// the length of the edit of code
#pragma endregion



int read_multi(unsigned char device_var,unsigned short *put_data_into_here,unsigned short start_address,int length)
{
	int retVal;
	retVal =  read_multi_tap(device_var, put_data_into_here, start_address, length);
	return retVal; 
}

/**

A wrapper for modbus_read_one_value which returns BOTH read value and error flag

  @param[in]   device_var	the modbus device address
  @param[in]   address		the offset of the value to be read in the device
  @param[in]   retry_times	the number of times to retry on read failure before giving up

  @return -1, -2, -3 on error, otherwise value read cast to integer

  This interface is provided for compatibility with existing code.
  New code should use modbus_read_one_value() directly,
  since it returns a separate error flag and read value -
  allowing simpler, more easily understood calling code design.

*/

int read_one(unsigned char device_var,unsigned short address,int retry_times)
{
	int value;
	int j = modbus_read_one_value( value, device_var, address, retry_times );
	if (j>0)
	{
		g_strT3000LogString.Format(_T("Read One ID=%d,address=%d,result=%d,Status=OK"),device_var,address,j);
		 ::SendMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
	}
	else
	{
		g_strT3000LogString.Format(_T("Read One ID=%d,address=%d,Status:Fail"),device_var,address);
		 ::SendMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
	}
	 //SetPaneString(3,g_strT3000LogString);
	if( j != 0 ) {
		// there was an error, so return the error flag
		return j;
	} else {
		// no error, so return value read
		return value;
	}
}
//val         the value that you want to write to the register
//the return value == -1 ,no connecting
//the return value == -2 ,try it again
//the return value == -3,Maybe that have more than 2 Tstat is connecting
//the return value == -4 ,between devLo and devHi,no Tstat is connected ,
//the return value == -5 ,the input have some trouble
//the return value == -6 , the bus has bannet protocol,scan stop;
//the return value >=1 ,the devLo!=devHi,Maybe have 2 Tstat is connecting
//清空串口缓冲区
//the return value is the register address
//Sleep(50);       //must use this function to slow computer
int g_CheckTstatOnline_a(unsigned char  devLo,unsigned char devHi, bool bComm_Type){

	BOOL EnableRefreshTreeView_original_value = g_bEnableRefreshTreeView;
	g_bEnableRefreshTreeView = false;
	int j=-1;
	// ensure no other threads attempt to access modbus communications
		CSingleLock singleLock(&register_critical_section);
		singleLock.Lock();
		// call the modbus DLL method
		j=CheckTstatOnline_a(devLo,devHi,bComm_Type);
		// free the modbus communications for other threads
		singleLock.Unlock();
		// increment the number of transmissions we have done
		g_llTxCount++;
		// check for other errors
		// increment the number or replies we have received
		if (j!=-1&&j!=-4)
		{
			g_llRxCount++;
		}
	// check for running in the main GUI thread
	if( AfxGetMainWnd()->GetActiveWindow() != NULL ) {

		// construct status message string
		CString str;
		str.Format(_T("San Command [Tx=%d Rx=%d Err=%d]"), 
			g_llTxCount, g_llRxCount, g_llTxCount-g_llRxCount);

		//Display it
		((CMFCStatusBar *) AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_STATUS_BAR))->SetPaneText(0,str.GetString());

	}
	g_bEnableRefreshTreeView |= EnableRefreshTreeView_original_value;
	return j;
}

int g_NetController_CheckTstatOnline_a(unsigned char  devLo,unsigned char devHi, bool bComm_Type){

	BOOL EnableRefreshTreeView_original_value = g_bEnableRefreshTreeView;
	g_bEnableRefreshTreeView = false;
	int j=-1;
	// ensure no other threads attempt to access modbus communications
	CSingleLock singleLock(&register_critical_section);
	singleLock.Lock();
	// call the modbus DLL method
	j=g_NetController_CheckTstatOnline_a(devLo,devHi,bComm_Type);
	// free the modbus communications for other threads
	singleLock.Unlock();
	// increment the number of transmissions we have done
	g_llTxCount++;
	// check for other errors
	// increment the number or replies we have received
	if (j!=-1&&j!=-4)
	{
		g_llRxCount++;
	}
	// check for running in the main GUI thread
	if( AfxGetMainWnd()->GetActiveWindow() != NULL ) {

		// construct status message string
		CString str;
		str.Format(_T("San Command [Tx=%d Rx=%d Err=%d]"), 
			g_llTxCount, g_llRxCount, g_llTxCount-g_llRxCount);

		//Display it
		((CMFCStatusBar *) AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_STATUS_BAR))->SetPaneText(0,str.GetString());

	}
	g_bEnableRefreshTreeView |= EnableRefreshTreeView_original_value;
	return j;
}
 
void SetPaneString(int nIndext,CString str)
{
	
	CMFCStatusBar * pStatusBar=NULL;
	if(AfxGetMainWnd()->GetActiveWindow()==NULL)//if this function is called by a thread ,return 
		return;
	pStatusBar = (CMFCStatusBar *) AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_STATUS_BAR);
	pStatusBar->SetPaneText(nIndext,str.GetString());
	pStatusBar->SetPaneTextColor (nIndext, RGB(0,0,0));
  	if (nIndext==3)
  	{
  		pStatusBar->SetPaneTextColor (nIndext, RGB(255,255,255));
  		pStatusBar->SetPaneBackgroundColor(nIndext,RGB(42,58,87));
  	}
}
int Write_Multi(unsigned char device_var,unsigned char *to_write,unsigned short start_address,int length,int retry_times)
{
	BOOL bTemp = g_bEnableRefreshTreeView;
	g_bEnableRefreshTreeView = FALSE;
	int j = Write_Multi_org(device_var, to_write, start_address, length, retry_times);
	g_bEnableRefreshTreeView |= bTemp;
 	CString data;
 	for (int i=0;i<length;i++)
 	{
 		CString strTemp;
 		strTemp.Format(_T("%d "),to_write[i]);
 		data+=strTemp;
 	}
	if (j>0)
	{
		g_strT3000LogString.Format(_T("Multi Write ID=%d,start address=%d,length=%d,Status=OK"),device_var,start_address,length,data.GetBuffer());
		 ::SendMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
	}
	else
	{
		g_strT3000LogString.Format(_T("Multi Write ID=%d,start address=%d,length=%d,Status:Fail"),device_var,start_address,length);
		 ::SendMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
	}
	// SetPaneString(3,g_strT3000LogString);
	return j;
}
int Write_Multi_short(unsigned char device_var,unsigned short *to_write,unsigned short start_address,int length,int retry_times)
{
	BOOL bTemp = g_bEnableRefreshTreeView;
	g_bEnableRefreshTreeView = FALSE;
	int j = Write_Multi_org_short(device_var, to_write, start_address, length, retry_times);
	g_bEnableRefreshTreeView |= bTemp;
	CString data;
	for (int i=0;i<length;i++)
	{
		CString strTemp;
		strTemp.Format(_T("%d"),to_write[i]);
		data+=strTemp;
	}
	if (j>0)
	{
		g_strT3000LogString.Format(_T("Multi Write ID=%d,start address=%d,length=%d,result=%s"),device_var,start_address,length,data.GetBuffer());
		::PostMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
	}
	else
	{
		g_strT3000LogString.Format(_T("Multi Write ID=%d,start address=%d,length=%d,result:Fail"),device_var,start_address,length);
		::PostMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
	}
	return j;
}
int Write_Multi_org(unsigned char device_var,unsigned char *to_write,unsigned short start_address,int length,int retry_times)
{
// 	CString str;
// 	str.Format(_T("ID :%d Multi writing start :%d Amount: %d"),device_var,start_address,length);
// 	SetPaneString(2,str);
	int j=0;
	for(int i=0;i<retry_times;i++)
	{
		register_critical_section.Lock();
		j=write_multi(device_var,to_write,start_address,length);
		register_critical_section.Unlock();
		if (g_CommunicationType==Modbus_Serial)
		{
			Sleep(DELAY_TIME);//do this for better quickly
		}
		if(j!=-2)
		{
			//SetPaneString(2,_T("Multi-Write successful!"));
			CString str;
			str.Format(_T("Addr:%d [Tx=%d Rx=%d : Err=%d]"), device_var, ++g_llTxCount, ++g_llRxCount,g_llTxCount-g_llRxCount);
			SetPaneString(0,str);
			return j;
		}
	}
	//SetPaneString(2,_T("Multi-write failure!"));
	CString str;
	str.Format(_T("Addr:%d [Tx=%d Rx=%d : Err=%d]"), device_var, ++g_llTxCount, g_llRxCount,g_llTxCount-g_llRxCount);
	SetPaneString(0,str);
	return j;
}


int Write_Multi_org_short(unsigned char device_var,unsigned short *to_write,unsigned short start_address,int length,int retry_times)
{
	// 	CString str;
	// 	str.Format(_T("ID :%d Multi writing start :%d Amount: %d"),device_var,start_address,length);
	// 	SetPaneString(2,str);
	int j=0;
	for(int i=0;i<retry_times;i++)
	{
		register_critical_section.Lock();
		j=write_multi_Short(device_var,to_write,start_address,length);
		register_critical_section.Unlock();
		if (g_CommunicationType==Modbus_Serial)
		{
			Sleep(DELAY_TIME);//do this for better quickly
		}
		if(j!=-2)
		{
			//SetPaneString(2,_T("Multi-Write successful!"));
			CString str;
			str.Format(_T("Addr:%d [Tx=%d Rx=%d : Err=%d]"), device_var, ++g_llTxCount, ++g_llRxCount,g_llTxCount-g_llRxCount);
			SetPaneString(0,str);
			return j;
		}
	}
	//SetPaneString(2,_T("Multi-write failure!"));
	CString str;
	str.Format(_T("Addr:%d [Tx=%d Rx=%d : Err=%d]"), device_var, ++g_llTxCount, g_llRxCount,g_llTxCount-g_llRxCount);
	SetPaneString(0,str);
	return j;
}
/**

  Read multiple values from a modbus device

  @param[out]  put_data_into_here	the values read
  @param[in]   device_var			the modbus device address
  @param[in]   start_address		the offset of thefirt value to be read in the device
  @param[in]   length				number of values to be read
  @param[in]   retry_times			the number of times to retry on read failure before giving up

  @return  0 if there were no errors

  This does NOT lock the register_critical_section

  This is a wrapper for modbus_read_multi_value
  It is provided for compatibility with existing code.
  New code should use modbus_read_multi_value() directly.

  This does NOT lock the critical section.

  */
//extern int modbus_read_multi_value( 
//	unsigned short *put_data_into_here,
//	unsigned char device_var,
//	unsigned short start_address,
//	int length,
//	int retry_times );
int Read_Multi(unsigned char device_var,unsigned short *put_data_into_here,unsigned short start_address,int length,int retry_times)
{
	 int ret=modbus_read_multi_value(
		put_data_into_here,
		device_var,
		start_address,
		length,
		retry_times );
	 CString data;
	 for (int i=0;i<length;i++)
	 {
		 CString strTemp;
		 strTemp.Format(_T("%d"),put_data_into_here[i]);
		 data+=strTemp;
	 }
	 if (ret>0)
	 {
		 g_strT3000LogString.Format(_T("Multi Read ID=%d,start address=%d,length=%d,Status=OK"),device_var,start_address,length,data.GetBuffer());
		  ::SendMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
		 
	 }
	 else
	 {
		 g_strT3000LogString.Format(_T("Multi Read ID=%d,start address=%d,length=%d,Status:Fail"),device_var,start_address,length);
		  ::SendMessage(MainFram_hwd,WM_SHOW_PANNELINFOR,3,0);
	 }
	// SetPaneString(3,g_strT3000LogString);
	 return ret;
}




int turn_hex_str_to_ten_num(char *source)
{
	int j=0,k=0,l=0;
	for(int i=0;i<2;i++)//***********************************2
		if(j==0)
		{
			switch(source[i])
			{
			case '0':k=0;break;
			case '1':k=1;break;
			case '2':k=2;break;
			case '3':k=3;break;
			case '4':k=4;break;
			case '5':k=5;break;
			case '6':k=6;break;
			case '7':k=7;break;
			case '8':k=8;break;
			case '9':k=9;break;

			case 'a':k=10;break;
			case 'b':k=11;break;
			case 'c':k=12;break;
			case 'd':k=13;break;
			case 'e':k=14;break;
			case 'f':k=15;break;
			case 'A':k=10;break;
			case 'B':k=11;break;
			case 'C':k=12;break;
			case 'D':k=13;break;
			case 'E':k=14;break;
			case 'F':k=15;break;
			
			default:return -1;
			}
			for( ;j<2-i-1;j++)
				k*=16;
		}
		else
		{
			l+=k;
			j=0;
			i--;
		}
		l+=k;
	return l;
}

int turn_hex_char_to_int(char c)
{
		int k=0;
		switch(c)
		{
		case '0':k=0;break;
		case '1':k=1;break;
		case '2':k=2;break;
		case '3':k=3;break;
		case '4':k=4;break;
		case '5':k=5;break;
		case '6':k=6;break;
		case '7':k=7;break;
		case '8':k=8;break;
		case '9':k=9;break;

		case 'a':k=10;break;
		case 'b':k=11;break;
		case 'c':k=12;break;
		case 'd':k=13;break;
		case 'e':k=14;break;
		case 'f':k=15;break;
		case 'A':k=10;break;
		case 'B':k=11;break;
		case 'C':k=12;break;
		case 'D':k=13;break;
		case 'E':k=14;break;
		case 'F':k=15;break;

		default: return -1;//2
		}
		return k;
}

bool turn_hex_file_line_to_unsigned_char(char *str)
{
	char *p_temp=str;
	int itemp=strlen(p_temp);
	for(int i=0;i<itemp;i++)
	{
		*(p_temp+i)=turn_hex_char_to_int(*(p_temp+i));	
		if(*(p_temp+i)==-1)
			return false;
	}
	return true;
}


void turn_int_to_unsigned_char(char *source,int length_source,unsigned char *aim)
{
	char *p_c_temp=source;
	unsigned char *p_uc_temp=aim;
	unsigned char uctemp;
	for(int i=0;i <length_source;i++)
		if(i%2==0)
		{
			char ctemp=*(p_c_temp+i);
			uctemp = ctemp*16;
		}
		else
		{
			char ctemp=*(p_c_temp+i);
			uctemp+=ctemp;
			*(p_uc_temp+i/2)=uctemp;
			uctemp=0;
		}
}






float get_tstat_version(unsigned short tstat_id)
{//get tstat version and judge the tstat is online or no
	//tstat is online ,return >0
	//tstat is not online ,return -2
	
	float tstat_version2=(float)product_register_value[4];//tstat version			
	if(tstat_version2==-2 || tstat_version2==-3)
		return tstat_version2;
	if(tstat_version2 >=240 && tstat_version2 <250)
		tstat_version2 /=10;
	else 
	{
		tstat_version2 = (float)(product_register_value[5]*256+product_register_value[4]);	
		tstat_version2 /=10;
	}//tstat_version
	return tstat_version2;
}

float get_curtstat_version()
{
	float tstat_version2= product_register_value[MODBUS_VERSION_NUMBER_LO];//tstat version			
	if(tstat_version2<=0)
		return tstat_version2;
	if(tstat_version2 >=240 && tstat_version2 <250)
		tstat_version2 /=10;
	else 
	{
		tstat_version2 = (float)(product_register_value[MODBUS_VERSION_NUMBER_HI]*256+product_register_value[MODBUS_VERSION_NUMBER_LO]);	
		tstat_version2 /=10;
	}//tstat_version
	return tstat_version2;

}


int make_sure_isp_mode(int the_tstat_id)
{
	unsigned short isp_unsigned_short[20];
	int i=Read_Multi(the_tstat_id,isp_unsigned_short,100,20);
	if(i==-2 || i==-1)
		return i;//no response
	if(i<0)
		return i;//no in isp mode
	else
	{
		for(int j=0;j<20;j++)
		{
			if(isp_unsigned_short[j]!=1)
				return 0;// no in isp mode
		}
	}
	return 1;//in isp mode 
}

bool get_serialnumber(long & serial,int the_id_of_product)
{

	unsigned short SerialNum[4]={0};
	int nRet=0;
	nRet=Read_Multi(the_id_of_product,&SerialNum[0],0,4);
	serial=0;
	if(nRet>0)
	{
		serial=SerialNum[0]+SerialNum[1]*256+SerialNum[2]*256*256+SerialNum[3]*256*256*256;
		return TRUE;
	}
		return FALSE;
}


UINT get_serialnumber()
{
	return product_register_value[MODBUS_SERIALNUMBER_LOWORD]+product_register_value[MODBUS_SERIALNUMBER_LOWORD+1]*256+product_register_value[MODBUS_SERIALNUMBER_HIWORD]*256*256+product_register_value[MODBUS_SERIALNUMBER_HIWORD+1]*256*256*256;
}





bool multi_read_tstat(int id)
{

	bool return_value=true;
	int i;
	for(i=0;i<7;i++)
	{
		//register_critical_section.Lock();
		//int nStart = GetTickCount();
		if(-2==Read_Multi(id,&product_register_value[i*64],i*64,64,1))
			return_value=false;

		//TRACE(_T("Read_Multi once = %d \n"), GetTickCount()-nStart);
		Sleep(50);
		//register_critical_section.Unlock();
	}
	return return_value;
}


bool can_be_writed_hex_file(int product_model,int hex_file_product_model)
{
	//product model
	// T3-8IO-------------20
	// T3-32I-------------22
	// T3-8i/60-----------23
	// Flexdriver---------25
	//Tstat5A-------------2
	//Tstat5B-------------1
	//Tstat5B2------------3
	//Tstat5C-------------4
	//Tstat5D-------------12
	//Solar---------------30
	//hex_file_product_model parameter is the hex_file_register 0x100 (256)
//	if (product_model==18||product_model==17)
	{
		return true;
	}
	if(hex_file_product_model==255)//////////////old version hex file,before 2005.11.15
		return true;
	if(product_model<=TSTAT_PRODUCT_MODEL && hex_file_product_model<=TSTAT_PRODUCT_MODEL)
		return true;
	if(product_model==LED_PRODUCT_MODEL && hex_file_product_model==LED_PRODUCT_MODEL)
		return true;
	if(product_model==PM_NC && hex_file_product_model==PM_NC)
		return true;
	if(product_model==PM_T3IOA && hex_file_product_model==PM_T3IOA)
		return true;
	if(product_model==PM_T3PT10 && hex_file_product_model==PM_T3PT10)
		return true;
	if(product_model==T3_32I_PRODUCT_MODEL && hex_file_product_model==T3_32I_PRODUCT_MODEL)
		return true;
	if(product_model==T3_8I_16O_PRODUCT_MODEL && hex_file_product_model==T3_8I_16O_PRODUCT_MODEL)
		return true;
	if(product_model==PM_SOLAR && hex_file_product_model==PM_SOLAR)
		return true;
	if(product_model==PM_ZIGBEE && hex_file_product_model==PM_ZIGBEE)
		return true;	
	return false;
}
CString get_product_name_by_product_model(int product_model)
{
	CString return_str;
	if(product_model>0 && product_model<=TSTAT_PRODUCT_MODEL)
		product_model=TSTAT_PRODUCT_MODEL;
	switch(product_model)
	{
	case 19:return_str=_T("Tstat");break;
	case 20:return_str=_T("T3-8IO");break;
	case 22:return_str=_T("T3-32I");break;
	case 23:return_str=_T("T3-8i/60");break;
	case 25:return_str=_T("Flexdriver");break;
	case 30:return_str=_T("Solar");break;
	case PM_ZIGBEE:return_str=_T("ZigBee");break;
	default:return_str=_T("Unknown");break;
	}
	return return_str;
}

// Function : 获得单位名称，此单位用于Input Grid，Output Grid，Output Set Grid，主界面的Grid等等。
// Param: int nRange: 指示当前的Range的选择值。函数应该根据Range的选择以及TStat的型号，
//					获得单位名称，如摄氏度，华氏度，百分比，自定义的单位等。
//           int nPIDNO: 区分PID1 还是PID2，1＝PID1，2＝PID2
// return ： 单位名称 
CString GetTempUnit(int nRange, int nPIDNO)
{
	CString strTemp=_T("");

	if(nRange<0) // 使用默认的温度单位
	{
		UINT uint_temp=GetOEMCP();//get system is for chinese or english
		if(uint_temp!=936 && uint_temp!=950)
		{
			if(product_register_value[MODBUS_DEGC_OR_F]==0)	//121
			{
				strTemp.Format(_T("%cC"),176);
			}
			else
			{
				strTemp.Format(_T("%cF"),176);
			}
		}
		else
		{
			//Chinese.
			if(product_register_value[MODBUS_DEGC_OR_F]==0)//121
			{
				strTemp=_T("℃");
			}
			else
			{
				strTemp=_T("℉");
			}
		}
		return strTemp;
	}

	if(nRange==0)		// Raw value, no unit
		strTemp=_T("");
	else if(nRange==1)
	{//
		UINT uint_temp=GetOEMCP();//get system is for chinese or english
		if(uint_temp!=936 && uint_temp!=950)
		{
			if(product_register_value[MODBUS_DEGC_OR_F]==0)//121
			{
				strTemp.Format(_T("%cC"),176);
			}
			else
			{
				strTemp.Format(_T("%cF"),176);
			}
		}
		else
		{
			//chinese.
			if(product_register_value[MODBUS_DEGC_OR_F]==0)//121
			{
				strTemp=_T("℃");
			}
			else
			{
				strTemp=_T("℉");
			}
		}
		return strTemp;
	}
	else if(nRange==2)
	{//
		strTemp=_T("%");
	}
	else if(nRange==3)
	{//ON/OFF
		strTemp=_T("");
	}
	else if(nRange==4)
	{//Customer Sersor
		if(nPIDNO==1)
		{
			int m_271=product_register_value[MODBUS_UNITS1_HIGH];//271 390
			int m_272=product_register_value[MODBUS_UNITS1_LOW];//272  391
			if(m_271>>8=='0')
			{
				if((m_271 & 0xFF) =='0')
				{
					if(m_272>>8=='0')
						strTemp.Format(_T("%c"),m_272 & 0xFF);
					else
						strTemp.Format(_T("%c%c"),m_272>>8,m_272 & 0xFF);
				}
				else
					strTemp.Format(_T("%c%c%c"),m_271 & 0xFF,m_272>>8,m_272 & 0xFF);
			}
			else
				strTemp.Format(_T("%c%c%c%c"),m_271>>8,m_271 & 0xFF,m_272>>8,m_272 & 0xFF);
		}
		else if(nPIDNO==2)
		{
			int m_273=product_register_value[MODBUS_UNITS2_HIGH];//273  392;
			int m_274=product_register_value[MODBUS_UNITS2_LOW];//274 393;
			if(m_273>>8=='0')
			{
				if((m_273 & 0xFF)=='0')
				{
					if(m_274>>8=='0')
						strTemp.Format(_T("%c"),m_274 & 0xFF);
					else
						strTemp.Format(_T("%c%c"),m_274>>8,m_274 & 0xFF);
				}
				else
					strTemp.Format(_T("%c%c%c"),m_273 & 0xFF,m_274>>8,m_274 & 0xFF);
			}		
			else
				strTemp.Format(_T("%c%c%c%c"),m_273>>8,m_273 & 0xFF,m_274>>8,m_274 & 0xFF);

		}
	}	
	return strTemp;

	//CString strTemp=_T("");
	//
	//if(nRange<0) // 使用默认的温度单位
	//{
	//	UINT uint_temp=GetOEMCP();//get system is for chinese or english
	//	if(uint_temp!=936 && uint_temp!=950)
	//	{
	//		if(multi_register_value[121]==0)
	//		{
	//			strTemp.Format(_T("%cC"),176);
	//		}
	//		else
	//		{
	//			strTemp.Format(_T("%cF"),176);
	//		}
	//	}
	//	else
	//	{
	//		//Chinese.
	//		if(multi_register_value[121]==0)
	//		{
	//			strTemp=_T("°C");
	//		}
	//		else
	//		{
	//			strTemp=_T("°F");
	//		}
	//	}
	//	return strTemp;
	//}

	//if(nRange==0)		// Raw value, no unit
	//	strTemp=_T("");
	//else if(nRange==1)
	//{//
	//	UINT uint_temp=GetOEMCP();//get system is for chinese or english
	//	if(uint_temp!=936 && uint_temp!=950)
	//	{
	//		if(multi_register_value[121]==0)
	//		{
	//			strTemp.Format(_T("%cC"),176);
	//		}
	//		else
	//		{
	//			strTemp.Format(_T("%cF"),176);
	//		}
	//	}
	//	else
	//	{
	//		//chinese.
	//		if(multi_register_value[121]==0)
	//		{
	//			strTemp=_T("°C");
	//		}
	//		else
	//		{
	//			strTemp=_T("°F");
	//		}
	//	}
	//	return strTemp;
	//}
	//else if(nRange==2)
	//{//
	//	strTemp=_T("%");
	//}
	//else if(nRange==3)
	//{//ON/OFF
	//	strTemp=_T("");
	//}
	//else if(nRange==4)
	//{//Customer Sersor
	//	if(nPIDNO==1)
	//	{
	//		int m_271=multi_register_value[271];
	//		int m_272=multi_register_value[272];
	//		if(m_271>>8=='0')
	//		{
	//			if((m_271 & 0xFF) =='0')
	//			{
	//				if(m_272>>8=='0')
	//					strTemp.Format(_T("%c"),m_272 & 0xFF);
	//				else
	//					strTemp.Format(_T("%c%c"),m_272>>8,m_272 & 0xFF);
	//			}
	//			else
	//				strTemp.Format(_T("%c%c%c"),m_271 & 0xFF,m_272>>8,m_272 & 0xFF);
	//		}
	//		else
	//			strTemp.Format(_T("%c%c%c%c"),m_271>>8,m_271 & 0xFF,m_272>>8,m_272 & 0xFF);
	//	}
	//	else if(nPIDNO==2)
	//	{
	//		int m_273=multi_register_value[273];
	//		int m_274=multi_register_value[274];
	//		if(m_273>>8=='0')
	//		{
	//			if((m_273 & 0xFF)=='0')
	//			{
	//				if(m_274>>8=='0')
	//					strTemp.Format(_T("%c"),m_274 & 0xFF);
	//				else
	//					strTemp.Format(_T("%c%c"),m_274>>8,m_274 & 0xFF);
	//			}
	//			else
	//				strTemp.Format(_T("%c%c%c"),m_273 & 0xFF,m_274>>8,m_274 & 0xFF);
	//		}		
	//		else
	//			strTemp.Format(_T("%c%c%c%c"),m_273>>8,m_273 & 0xFF,m_274>>8,m_274 & 0xFF);

	//	}
	//}	
	//return strTemp;
}

CString get_product_class_name_by_model_ID(int nModelID)
{
	CString strClassName;
	switch(nModelID)
	{
	case 2:strClassName=g_strTstat5a;break;
	case 1:strClassName=g_strTstat5b;break;
	case 3:strClassName=g_strTstat5b;break;
	case 4:strClassName=g_strTstat5c;break;
	case 6:strClassName=g_strTstat6;break;
	case 7:strClassName=g_strTstat7;break;
	case 12:strClassName=g_strTstat5d;break;
	case PM_NC:strClassName=g_strnetWork;break;
	case NET_WORK_OR485_PRODUCT_MODEL:strClassName=g_strOR485;break;
	case 17: strClassName=g_strTstat5f;break;
	case 18:strClassName=g_strTstat5g;break;
	case 16:strClassName=g_strTstat5e;break;
	case 19:strClassName=g_strTstat5h;break;
	case PM_LightingController: strClassName = g_strLightingCtrl;
	
	case 13:
	case 14:break;
	default:strClassName=g_strTstat5a;break;
	}

	return strClassName;
}


BOOL GetSerialComPortNumber1(vector<CString>& szComm)
{
	LPCTSTR strRegEntry = _T("HARDWARE\\DEVICEMAP\\SERIALCOMM\\");

	HKEY   hKey;   
	LONG   lReturnCode=0;   
	lReturnCode=::RegOpenKeyEx(HKEY_LOCAL_MACHINE, strRegEntry, 0, KEY_READ, &hKey);   
	USB_Serial.Empty();
	if(lReturnCode==ERROR_SUCCESS)   
	{  
		DWORD dwIndex = 0;
		WCHAR lpValueName[MAX_PATH];
		ZeroMemory(lpValueName, MAX_PATH);
		DWORD lpcchValueName = MAX_PATH; 
		DWORD lpReserved = 0; 
		DWORD lpType = REG_SZ; 
		BYTE		lpData[MAX_PATH]; 
		ZeroMemory(lpData, MAX_PATH);
		DWORD lpcbData = MAX_PATH;
		dwIndex = 0;
		while(RegEnumValue(	hKey, dwIndex, lpValueName, &lpcchValueName, 0, &lpType, lpData, &lpcbData ) != ERROR_NO_MORE_ITEMS)
		{   
			//TRACE("Registry's   Read!");   
			dwIndex++;

			lpcchValueName = MAX_PATH; 
			//lpValueName[0] = '\0'; 
			
			CString strValueName= CString(lpValueName);
		
			WCHAR ch[MAX_PATH];
			ZeroMemory(ch, MAX_PATH);
			memcpy(ch, lpData, lpcbData);
			CString str = CString(ch);
			szComm.push_back(str);

            if(strValueName.Find(_T("USBSER")) >=0)
			{
                DFTrace(_T("Find USB Serial Port!"));
                USB_Serial = str;
            }

			ZeroMemory(lpData, MAX_PATH);
			lpcbData = MAX_PATH;

		}   
		::RegCloseKey(hKey);   		   

		return TRUE;
	}

	return FALSE;   
}
BOOL Post_Refresh_One_Message(uint32_t deviceid,int8_t command,int8_t start_instance,int8_t end_instance,unsigned short entitysize)
{
	_MessageRefreshListInfo *pmy_refresh_info = new _MessageRefreshListInfo;
	pmy_refresh_info->deviceid = deviceid;
	pmy_refresh_info->command = command;
	pmy_refresh_info->start_instance = start_instance;
	pmy_refresh_info->end_instance = end_instance;
	pmy_refresh_info->entitysize = entitysize;
	if(!PostThreadMessage(nThreadID,MY_BAC_REFRESH_ONE,(WPARAM)pmy_refresh_info,NULL))//post thread msg
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
BOOL Post_Refresh_Message(uint32_t deviceid,int8_t command,int8_t start_instance,int8_t end_instance,unsigned short entitysize,int block_size)
{
	_MessageRefreshListInfo *pmy_refresh_info = new _MessageRefreshListInfo;
	pmy_refresh_info->deviceid = deviceid;
	pmy_refresh_info->command = command;
	pmy_refresh_info->start_instance = start_instance;
	pmy_refresh_info->end_instance = end_instance;
	pmy_refresh_info->entitysize = entitysize;
	pmy_refresh_info->block_size = block_size;
	if(!PostThreadMessage(nThreadID,MY_BAC_REFRESH_LIST,(WPARAM)pmy_refresh_info,NULL))//post thread msg
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
BOOL Post_Write_Message(uint32_t deviceid,int8_t command,int8_t start_instance,int8_t end_instance,unsigned short entitysize,HWND hWnd ,CString Task_Info ,int nRow,int nCol)
{
	_MessageWriteListInfo *pmy_write_info = new _MessageWriteListInfo;
	pmy_write_info->deviceid = deviceid;
	pmy_write_info->command = command;
	pmy_write_info->start_instance = start_instance;
	pmy_write_info->end_instance = end_instance;
	pmy_write_info->Write_Info = Task_Info;
	pmy_write_info->entitysize = entitysize;
	pmy_write_info->hWnd = hWnd;
	pmy_write_info->ItemInfo.nRow = nRow;
	pmy_write_info->ItemInfo.nCol = nCol;
	if(!PostThreadMessage(nThreadID,MY_BAC_WRITE_LIST,(WPARAM)pmy_write_info,NULL))//post thread msg
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

//Add 20130516  by Fance
//UINT MsgType
//unsigned char device_id
//unsigned short address
//short new_value
//short old_value
BOOL Post_Invoke_ID_Monitor_Thread(UINT MsgType,
	int Invoke_ID,
	HWND hwnd,
	CString Show_Detail ,
	int nRow,
	int nCol
	)
{
	_MessageInvokeIDInfo *pMy_Invoke_id = new _MessageInvokeIDInfo;
	pMy_Invoke_id->Invoke_ID = Invoke_ID;
	pMy_Invoke_id->hwnd = hwnd;
	pMy_Invoke_id->task_info = Show_Detail;
	pMy_Invoke_id->mRow = nRow;
	pMy_Invoke_id->mCol = nCol;
	if(!PostThreadMessage(nThreadID,MY_INVOKE_ID,(WPARAM)pMy_Invoke_id,NULL))//post thread msg
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL Post_Thread_Message(UINT MsgType,
	unsigned char device_id,
	unsigned short address,
	short new_value,
	short old_value,
	HWND Dlg_hwnd,
	UINT CTRL_ID,
	CString Changed_Name)
{
	_MessageWriteOneInfo *My_Write_Struct = new _MessageWriteOneInfo;
	My_Write_Struct->device_id = device_id;
	My_Write_Struct->address = address;
	My_Write_Struct->new_value = new_value;
	My_Write_Struct->old_value = old_value;
	My_Write_Struct->hwnd = Dlg_hwnd;
	My_Write_Struct->CTRL_ID = CTRL_ID;
	My_Write_Struct->Changed_Name = Changed_Name;

	//search the id ,if not in the vector, push back into the vector.
	bool find_id=false;
	for (int i=0;i<(int)Change_Color_ID.size();i++)
	{
		if(Change_Color_ID.at(i)!=CTRL_ID)
			continue;
		else
			find_id = true;
	}
	if(!find_id)
		Change_Color_ID.push_back(CTRL_ID);
	else
		return FALSE;
	
	if(!PostThreadMessage(nThreadID,MY_WRITE_ONE,(WPARAM)My_Write_Struct,NULL))//post thread msg
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL Post_Read_one_Thread_Message(
	unsigned char device_id,
	unsigned short address,
	HWND Dlg_hwnd)
{
	_MessageReadOneInfo *My_Read_Struct = new _MessageReadOneInfo;
	My_Read_Struct->device_id=device_id;
	My_Read_Struct->address=address;
	My_Read_Struct->new_value = -1;
	My_Read_Struct->hwnd = Dlg_hwnd;
	if(!PostThreadMessage(nThreadID,MY_READ_ONE,(WPARAM)My_Read_Struct,NULL))//post thread msg
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
extern int my_lengthcode;
/***************************************************
**
** Write Bacnet private data to device
** Add by Fance
****************************************************/
int WritePrivateData(uint32_t deviceid,int8_t n_command,int8_t start_instance,int8_t end_instance )
{
	// TODO: Add your control notification handler code here

	unsigned char command = (unsigned char)n_command;

	unsigned short entitysize=0;
	uint8_t apdu[480] = { 0 };
	uint8_t test_value[480] = { 0 };
	int private_data_len = 0;	
	BACNET_APPLICATION_DATA_VALUE data_value = { 0 };
	BACNET_APPLICATION_DATA_VALUE test_data_value = { 0 };
	BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
	BACNET_PRIVATE_TRANSFER_DATA test_data = { 0 };
	bool status = false;

	private_data.vendorID = BACNET_VENDOR_ID;
	private_data.serviceNumber = 1;

	unsigned max_apdu = 0;
	switch(command)
	{
	case WRITEINPUT_T3000:
		entitysize = sizeof(Str_in_point);
		break;
	case WRITEPROGRAM_T3000:
		entitysize = sizeof(Str_variable_point);
		break;
	case WRITEUNIT_T3000:
		entitysize = sizeof(Str_Units_element);
		break;
	case WRITEPROGRAMCODE_T3000:
		entitysize = program_code_length[start_instance];
		
		//m_Program_data.at(program_list_line).bytes = my_lengthcode -7;
		//entitysize = my_lengthcode;
		if((entitysize<0)||(entitysize>400))
			entitysize = 0;
		break;
	case WRITEVARIABLE_T3000:
		entitysize = sizeof(Str_variable_point);
		break;
	case  WRITEOUTPUT_T3000:
		entitysize = sizeof(Str_out_point);
		break;
	case WRITEWEEKLYROUTINE_T3000:
		entitysize = sizeof(Str_weekly_routine_point);
		break;
	case WRITEANNUALROUTINE_T3000:
		entitysize = sizeof(Str_annual_routine_point);
		break;
	case WRITETIMESCHEDULE_T3000:
		entitysize =WEEKLY_SCHEDULE_SIZE;// sizeof(Str_schedual_time_point);
		break;
	case WRITEANNUALSCHEDULE_T3000:
		entitysize = 48;
		break;
	case RESTARTMINI_COMMAND:
		entitysize = sizeof(Time_block_mini);
		break;
	case WRITE_SETTING_COMMAND:
		entitysize = sizeof(Str_Setting_Info);
		break;
	case WRITECONTROLLER_T3000:
		entitysize = sizeof(Str_controller_point);
		break;
	case WRITESCREEN_T3000:
		entitysize = sizeof(Control_group_point);
		break;
	case WRITEMONITOR_T3000:
		entitysize = sizeof(Str_monitor_point);
		break;

	case  WRITEALARM_T3000:
		entitysize = sizeof(Alarm_point);
		break;
	case WRITETSTAT_T3000:
		entitysize = sizeof(Str_TstatInfo_point);
		break;
	case WRITE_SUB_ID_BY_HAND:
		entitysize = 254;
		break;
	default:
		{
			//AfxMessageBox(_T("Entitysize length error!"));
			TRACE(_T("Entitysize length error!"));
			break;
		}
		
	}
	Str_user_data_header private_data_chunk;
	private_data_chunk.total_length = PRIVATE_HEAD_LENGTH + (end_instance - start_instance + 1)*entitysize;
	private_data_chunk.command = command;
	private_data_chunk.point_start_instance = start_instance;
	private_data_chunk.point_end_instance = end_instance;
	private_data_chunk.entitysize=entitysize;

	char SendBuffer[1000];
	memset(SendBuffer,0,1000);
	char * temp_buffer = SendBuffer;
	memcpy_s(SendBuffer,PRIVATE_HEAD_LENGTH ,&private_data_chunk,PRIVATE_HEAD_LENGTH );

	switch(command)
	{
	case WRITEINPUT_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_in_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_in_point),&m_Input_data.at(i + start_instance),sizeof(Str_in_point));
		}
		break;
	case WRITEPROGRAM_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_program_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_program_point),&m_Program_data.at(i + start_instance),sizeof(Str_program_point));
		}

		break;
	case  WRITEUNIT_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_Units_element) + PRIVATE_HEAD_LENGTH,sizeof(Str_Units_element),&m_customer_unit_data.at(i + start_instance),sizeof(Str_Units_element));
		}
		break;
	case WRITEPROGRAMCODE_T3000:
 		//memcpy_s(SendBuffer + PRIVATE_HEAD_LENGTH,entitysize,mycode,my_lengthcode);
		memcpy_s(SendBuffer + PRIVATE_HEAD_LENGTH,entitysize,program_code[start_instance],entitysize);
		break;
	case WRITEVARIABLE_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_variable_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_variable_point),&m_Variable_data.at(i + start_instance),sizeof(Str_variable_point));
		}
		break;
	case  WRITEOUTPUT_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_out_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_out_point),&m_Output_data.at(i + start_instance),sizeof(Str_out_point));
		}
		break;
	case WRITEWEEKLYROUTINE_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_weekly_routine_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_weekly_routine_point),&m_Weekly_data.at(i + start_instance),sizeof(Str_weekly_routine_point));
		}
		break;
	case WRITETIMESCHEDULE_T3000:
		temp_buffer = temp_buffer + PRIVATE_HEAD_LENGTH;
		for (int j=0;j<9;j++)
		{
			for (int i=0;i<8;i++)
			{
				*(temp_buffer++) = m_Schedual_Time_data.at(start_instance).Schedual_Day_Time[i][j].time_minutes;// = *(my_temp_point ++);
				*(temp_buffer++) = m_Schedual_Time_data.at(start_instance).Schedual_Day_Time[i][j].time_hours;// = *(my_temp_point ++);
			}
		}
		
		break;
	case  WRITEANNUALROUTINE_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_annual_routine_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_annual_routine_point),&m_Annual_data.at(i + start_instance),sizeof(Str_annual_routine_point));
		}
		break;
	case WRITEANNUALSCHEDULE_T3000:
		memcpy_s(SendBuffer + PRIVATE_HEAD_LENGTH,48,&g_DayState[start_instance],48);

		//memcpy_s(g_DayState[annual_list_line],block_length,my_temp_point,block_length);
		break;
	case RESTARTMINI_COMMAND:
		{
			memcpy_s(SendBuffer + PRIVATE_HEAD_LENGTH,sizeof(Time_block_mini),&Device_time,sizeof(Time_block_mini));
		}
		break;
	case WRITE_SETTING_COMMAND:
		{
			memcpy_s(SendBuffer + PRIVATE_HEAD_LENGTH,sizeof(Str_Setting_Info),&Device_Basic_Setting,sizeof(Str_Setting_Info));
		}
		break;
	case WRITECONTROLLER_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_controller_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_controller_point),&m_controller_data.at(i + start_instance),sizeof(Str_controller_point));
		}
		break;
	case WRITESCREEN_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Control_group_point) + PRIVATE_HEAD_LENGTH,sizeof(Control_group_point),&m_screen_data.at(i + start_instance),sizeof(Control_group_point));
		}
		break;
	case WRITEMONITOR_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_monitor_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_monitor_point),&m_monitor_data.at(i + start_instance),sizeof(Str_monitor_point));
		}
		break;
	case WRITETSTAT_T3000:
		for (int i=0;i<(end_instance-start_instance + 1);i++)
		{
			memcpy_s(SendBuffer + i*sizeof(Str_TstatInfo_point) + PRIVATE_HEAD_LENGTH,sizeof(Str_TstatInfo_point),&m_Tstat_data.at(i + start_instance),sizeof(Str_TstatInfo_point));
		}
		break;

	case  WRITEALARM_T3000:
		memcpy_s(SendBuffer + PRIVATE_HEAD_LENGTH,sizeof(Alarm_point),&m_alarmlog_data.at(start_instance),sizeof(Alarm_point));
		break;
	case  WRITE_SUB_ID_BY_HAND:
		memcpy_s(SendBuffer + PRIVATE_HEAD_LENGTH,254,bacnet_add_id,254);
		break;
	default:
		{
			AfxMessageBox(_T("Command not match!Please Check it!"));
			return -1;
		}
		break;
	}


	Set_transfer_length(private_data_chunk.total_length);
	//transfer_len=6;

	status =bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,(char *)&SendBuffer, &data_value);
	//ct_test(pTest, status == true);
	private_data_len =	bacapp_encode_application_data(&test_value[0], &data_value);
	private_data.serviceParameters = &test_value[0];
	private_data.serviceParametersLen = private_data_len;

	BACNET_ADDRESS dest = { 0 };
	status = address_get_by_device(deviceid, &max_apdu, &dest);
	if (status) 
	{
		return Send_ConfirmedPrivateTransfer(&dest,&private_data);
	}
	return -2;
}



/************************************************************************/
/*
Author: Fance
Get Bacnet Private Data
<param name="deviceid">Bacnet Device ID
<param name="command">Bacnet command
<param name="start_instance">start point
<param name="end_instance">end point
<param name="entitysize">Block size of read
*/
/************************************************************************/
int GetPrivateData(uint32_t deviceid,int8_t command,int8_t start_instance,int8_t end_instance,int16_t entitysize)
{
	// TODO: Add your control notification handler code here

	uint8_t apdu[480] = { 0 };
	uint8_t test_value[480] = { 0 };
	int apdu_len = 0;
	int private_data_len = 0;	
	unsigned max_apdu = 0;
	BACNET_APPLICATION_DATA_VALUE data_value = { 0 };
//	BACNET_APPLICATION_DATA_VALUE test_data_value = { 0 };
	BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
//	BACNET_PRIVATE_TRANSFER_DATA test_data = { 0 };
	bool status = false;

	private_data.vendorID = BACNET_VENDOR_ID;
	private_data.serviceNumber = 1;



	Str_user_data_header private_data_chunk;
	private_data_chunk.total_length=PRIVATE_HEAD_LENGTH;
	private_data_chunk.command = command;
	private_data_chunk.point_start_instance=start_instance;
	private_data_chunk.point_end_instance=end_instance;
	private_data_chunk.entitysize=entitysize;
	// char private_data_chunk[33] = { "3031323334353637383940" };
	Set_transfer_length(PRIVATE_HEAD_LENGTH);
	//transfer_len=6;

	status =bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,(char *)&private_data_chunk, &data_value);
	//ct_test(pTest, status == true);
	private_data_len =	bacapp_encode_application_data(&test_value[0], &data_value);
	private_data.serviceParameters = &test_value[0];
	private_data.serviceParametersLen = private_data_len;

	BACNET_ADDRESS dest = { 0 };

	status = address_get_by_device(deviceid, &max_apdu, &dest);
	if (status) 
	//if (1) 
	{
		
		return Send_ConfirmedPrivateTransfer(&dest,&private_data);
		//return g_invoke_id;
	}
	else
		return -2;

}



/************************************************************************/
/*
Author: Fance Du
Get Bacnet Monitor Private Data
<param name="deviceid">Bacnet Device ID
<param name="command">Bacnet command
<param name="index"> read which the item of the monitor list
<param name="nspecial">if the first read, the special should be zero ,otherwise it must be 1;
<param name="MonitorUpdateData"> this structure means the data size ,data start time and end time;
<param name="ntype" > Analog data or digital data
*/
/************************************************************************/
int GetMonitorBlockData(uint32_t deviceid,int8_t command,int8_t nIndex,int8_t ntype_ad, int8_t ntotal_seg,int8_t nseg_index,MonitorUpdateData* up_data)
{
	// TODO: Add your control notification handler code here

	uint8_t apdu[480] = { 0 };
	uint8_t test_value[480] = { 0 };
	int apdu_len = 0;
	int private_data_len = 0;	
	unsigned max_apdu = 0;
	BACNET_APPLICATION_DATA_VALUE data_value = { 0 };
	//	BACNET_APPLICATION_DATA_VALUE test_data_value = { 0 };
	BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
	//	BACNET_PRIVATE_TRANSFER_DATA test_data = { 0 };
	bool status = false;

	private_data.vendorID = BACNET_VENDOR_ID;
	private_data.serviceNumber = 1;

	Str_Monitor_data_header private_data_chunk;
	private_data_chunk.total_length = 0;
	private_data_chunk.command = command;
	private_data_chunk.index = nIndex;
	private_data_chunk.conm_args.nsize = up_data->nsize;
	private_data_chunk.conm_args.oldest_time = up_data->oldest_time;
	private_data_chunk.conm_args.most_recent_time = up_data->most_recent_time;
	private_data_chunk.type = ntype_ad;
	private_data_chunk.special = 0;
	private_data_chunk.total_seg = ntotal_seg;
	private_data_chunk.seg_index = nseg_index;
	Set_transfer_length(PRIVATE_MONITOR_HEAD_LENGTH);


	status =bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,(char *)&private_data_chunk, &data_value);
	//ct_test(pTest, status == true);
	private_data_len =	bacapp_encode_application_data(&test_value[0], &data_value);
	private_data.serviceParameters = &test_value[0];
	private_data.serviceParametersLen = private_data_len;

	BACNET_ADDRESS dest = { 0 };
	status = address_get_by_device(deviceid, &max_apdu, &dest);
	if (status) 
	{
		return Send_ConfirmedPrivateTransfer(&dest,&private_data);
		//return g_invoke_id;
	}
	else
		return -2;

}

/***************************************************
**
** Receive Bacnet private data from device , and handle the data.
** Add by Fance
****************************************************/

int CM5ProcessPTA(	BACNET_PRIVATE_TRANSFER_DATA * data,bool &end_flag)
{
	int i;
	int block_length;
	char *my_temp_point;
	int temp_struct_value;


	int iLen;   /* Index to current location in data */
//	uint32_t uiErrorCode;
//	char cBlockNumber;
//	uint32_t ulTemp;
	int tag_len;
	uint8_t tag_number;
	uint32_t len_value_type;
	BACNET_OCTET_STRING Temp_CS;
	iLen = 0;
	int command_type;

	/* Error code is returned for read and write operations */

	tag_len =  decode_tag_number_and_value(&data->serviceParameters[iLen],   &tag_number, &len_value_type);
	iLen += tag_len;
	if (tag_number != BACNET_APPLICATION_TAG_OCTET_STRING) 
	{
		/* if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {*/
#if PRINT_ENABLED
		printf("CPTA: Bad Encoding!\n");
#endif
		return 0;
	}
	//iLen +=
	//    decode_unsigned(&data->serviceParameters[iLen], len_value_type,
	//    &uiErrorCode);
	decode_octet_string(&data->serviceParameters[iLen], len_value_type,&Temp_CS);
	command_type = Temp_CS.value[2];



	int start_instance=0;
	int end_instance = 0;
	///////////////////////////////
	switch(command_type)
	{
	case READOUTPUT_T3000:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_out_point))!=0)
				return -1;	//得到的结构长度错误;

			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_out_point);
			//m_Input_data_length = block_length;
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			if(end_instance == (BAC_OUTPUT_ITEM_COUNT - 1))
				end_flag = true;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			if(start_instance >= BAC_OUTPUT_ITEM_COUNT)
				return -1;//超过长度了;

			for (i=start_instance;i<=end_instance;i++)
			{
				if(strlen(my_temp_point)>STR_OUT_DESCRIPTION_LENGTH)
					memset(m_Output_data.at(i).description,0,STR_OUT_DESCRIPTION_LENGTH);
				else
				memcpy_s( m_Output_data.at(i).description,STR_OUT_DESCRIPTION_LENGTH,my_temp_point,STR_OUT_DESCRIPTION_LENGTH);
				my_temp_point=my_temp_point + STR_OUT_DESCRIPTION_LENGTH;
				if(strlen(my_temp_point)>STR_OUT_LABEL)
					memset(m_Output_data.at(i).label,0,STR_OUT_LABEL);
				else
					memcpy_s(m_Output_data.at(i).label,STR_OUT_LABEL ,my_temp_point,STR_OUT_LABEL );
				my_temp_point=my_temp_point + STR_OUT_LABEL ;

				temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
				m_Output_data.at(i).value = temp_struct_value;
				//memcpy_s(Private_data[i].value,4,temp_struct_value,4);
				my_temp_point=my_temp_point+4;

				m_Output_data.at(i).auto_manual = *(my_temp_point++);
				m_Output_data.at(i).digital_analog = *(my_temp_point++);
				m_Output_data.at(i).hw_switch_status = *(my_temp_point++);
				m_Output_data.at(i).control = *(my_temp_point++);
				m_Output_data.at(i).digital_control = *(my_temp_point++);
				m_Output_data.at(i).decom	= *(my_temp_point++);
				m_Output_data.at(i).range = *(my_temp_point++);
				m_Output_data.at(i).m_del_low = *(my_temp_point++);
				m_Output_data.at(i).s_del_high = *(my_temp_point++);
				//temp_out.delay_timer = *(my_temp_point++);  Output 这个Delay time先不管 清0
				m_Output_data.at(i).delay_timer = 0;
				my_temp_point = my_temp_point + 2;

				//m_Output_data.push_back(temp_out);
			}
			return READOUTPUT_T3000;
		}
		break;

	case READINPUT_T3000:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_in_point))!=0)
				return -1;	//得到的结构长度错误;
			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_in_point);
			//m_Input_data_length = block_length;
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;
			if(end_instance == (BAC_INPUT_ITEM_COUNT - 1))
				end_flag = true;

			if(start_instance >= BAC_INPUT_ITEM_COUNT)
				return -1;//超过长度了;
			//my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
			//m_Input_data.clear();
			for (i=start_instance;i<=end_instance;i++)
			{
			//	Str_in_point temp_in;
				if(strlen(my_temp_point) > STR_IN_DESCRIPTION_LENGTH)
					memset(m_Input_data.at(i).description,0,STR_IN_DESCRIPTION_LENGTH);
				else
					memcpy_s( m_Input_data.at(i).description,STR_IN_DESCRIPTION_LENGTH,my_temp_point,STR_IN_DESCRIPTION_LENGTH);
				my_temp_point=my_temp_point + STR_IN_DESCRIPTION_LENGTH;
				if(strlen(my_temp_point) > STR_IN_LABEL)
					memset(m_Input_data.at(i).label,0,STR_IN_LABEL);
				else
					memcpy_s(m_Input_data.at(i).label,STR_IN_LABEL ,my_temp_point,STR_IN_LABEL );
				my_temp_point=my_temp_point + STR_IN_LABEL ;

				temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
				m_Input_data.at(i).value = temp_struct_value;
				//memcpy_s(Private_data[i].value,4,temp_struct_value,4);
				my_temp_point=my_temp_point+4;
				m_Input_data.at(i).filter = *(my_temp_point++);
				m_Input_data.at(i).decom	= *(my_temp_point++);
				m_Input_data.at(i).sen_on	= *(my_temp_point++);
				m_Input_data.at(i).sen_off = *(my_temp_point++);
				m_Input_data.at(i).control = *(my_temp_point++);
				m_Input_data.at(i).auto_manual = *(my_temp_point++);
				m_Input_data.at(i).digital_analog = *(my_temp_point++);
				m_Input_data.at(i).calibration_sign = *(my_temp_point++);
				m_Input_data.at(i).calibration_increment = *(my_temp_point++);
				m_Input_data.at(i).unused = *(my_temp_point++);
				m_Input_data.at(i).calibration = *(my_temp_point++);
				m_Input_data.at(i).range = *(my_temp_point++);
				//m_Input_data.push_back(temp_in);
			}
			return READINPUT_T3000;
		}
		break;
	case READVARIABLE_T3000   :
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_variable_point))!=0)
				return -1;	//得到的结构长度错误;

			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_variable_point);
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;
			if(end_instance == (BAC_VARIABLE_ITEM_COUNT - 1))
				end_flag = true;
			if(start_instance >= BAC_VARIABLE_ITEM_COUNT)
				return -1;//超过长度了;

			//m_Input_data_length = block_length;
			//my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
			//m_Variable_data.clear();
			for (i=start_instance;i<=end_instance;i++)
			{
				//Str_variable_point temp_variable;
				if(strlen(my_temp_point) > STR_VARIABLE_DESCRIPTION_LENGTH)
					memset(m_Variable_data.at(i).description,0,STR_VARIABLE_DESCRIPTION_LENGTH);
				else
					memcpy_s( m_Variable_data.at(i).description,STR_VARIABLE_DESCRIPTION_LENGTH,my_temp_point,STR_VARIABLE_DESCRIPTION_LENGTH);
				my_temp_point=my_temp_point + STR_VARIABLE_DESCRIPTION_LENGTH;
				if(strlen(my_temp_point) > STR_VARIABLE_LABEL)
					memset(m_Variable_data.at(i).label,0,STR_VARIABLE_LABEL);
				else
					memcpy_s(m_Variable_data.at(i).label,STR_VARIABLE_LABEL ,my_temp_point,STR_VARIABLE_LABEL );
				my_temp_point=my_temp_point + STR_VARIABLE_LABEL ;

				temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
				m_Variable_data.at(i).value = temp_struct_value;
				//memcpy_s(Private_data[i].value,4,temp_struct_value,4);
				my_temp_point=my_temp_point+4;
				
				
				m_Variable_data.at(i).auto_manual = *(my_temp_point++);
				m_Variable_data.at(i).digital_analog = *(my_temp_point++);
				m_Variable_data.at(i).control = *(my_temp_point++);
				m_Variable_data.at(i).unused = *(my_temp_point++);
				m_Variable_data.at(i).range = *(my_temp_point++);

				//m_Variable_data.push_back(temp_variable);
			}
			return READVARIABLE_T3000;
		}
		break;
	case READWEEKLYROUTINE_T3000  :
		{
			int aaaa = sizeof(Str_weekly_routine_point);
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_weekly_routine_point))!=0)
				return -1;	
			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_weekly_routine_point);

			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			if(start_instance >= BAC_WEEKLY_ROUTINES_COUNT)
				return -1;//超过长度了;

			for (i=start_instance;i<=end_instance;i++)
			{
				//Str_program_point temp_in;
				if(strlen(my_temp_point) > STR_WEEKLY_DESCRIPTION_LENGTH)
					memset(m_Weekly_data.at(i).description,0,STR_WEEKLY_DESCRIPTION_LENGTH);
				else
					memcpy_s( m_Weekly_data.at(i).description,STR_WEEKLY_DESCRIPTION_LENGTH,my_temp_point,STR_WEEKLY_DESCRIPTION_LENGTH);
				my_temp_point=my_temp_point + STR_WEEKLY_DESCRIPTION_LENGTH;

				if(strlen(my_temp_point) > STR_WEEKLY_LABEL_LENGTH)
					memset(m_Weekly_data.at(i).label,0,STR_WEEKLY_LABEL_LENGTH);
				else
					memcpy_s( m_Weekly_data.at(i).label,STR_WEEKLY_LABEL_LENGTH ,my_temp_point,STR_WEEKLY_LABEL_LENGTH );
				my_temp_point=my_temp_point + STR_WEEKLY_LABEL_LENGTH ;


				m_Weekly_data.at(i).value = (unsigned char)(*(my_temp_point++));
				m_Weekly_data.at(i).auto_manual = (unsigned char)(*(my_temp_point++));
				m_Weekly_data.at(i).override_1_value =  (unsigned char)(*(my_temp_point++));
				m_Weekly_data.at(i).override_2_value =  (unsigned char)(*(my_temp_point++));
				m_Weekly_data.at(i).off =  (unsigned char)(*(my_temp_point++));
				m_Weekly_data.at(i).unused = (unsigned char)(*(my_temp_point++));

				my_temp_point = my_temp_point + 2*sizeof(Point_T3000);
			}
			return READWEEKLYROUTINE_T3000;
		}
		break;
	case READANNUALROUTINE_T3000  :
		{
		if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_annual_routine_point))!=0)
			return -1;	//得到的结构长度错误;
		block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_annual_routine_point);

		my_temp_point = (char *)Temp_CS.value + 3;
		start_instance = *my_temp_point;
		my_temp_point++;
		end_instance = *my_temp_point;
		my_temp_point++;
		my_temp_point = my_temp_point + 2;

		if(start_instance >= BAC_ANNUAL_ROUTINES_COUNT)
			return -1;//超过长度了;

		for (i=start_instance;i<=end_instance;i++)
		{
			//Str_program_point temp_in;
			if(strlen(my_temp_point) > STR_ANNUAL_DESCRIPTION_LENGTH)
				memset(m_Annual_data.at(i).description,0,STR_ANNUAL_DESCRIPTION_LENGTH);
			else
				memcpy_s( m_Annual_data.at(i).description,STR_ANNUAL_DESCRIPTION_LENGTH,my_temp_point,STR_ANNUAL_DESCRIPTION_LENGTH);
			my_temp_point=my_temp_point + STR_ANNUAL_DESCRIPTION_LENGTH;

			if(strlen(my_temp_point) > STR_ANNUAL_DESCRIPTION_LENGTH)
				memset(m_Annual_data.at(i).label,0,STR_ANNUAL_DESCRIPTION_LENGTH);
			else
				memcpy_s( m_Annual_data.at(i).label,STR_ANNUAL_LABEL_LENGTH ,my_temp_point,STR_ANNUAL_LABEL_LENGTH );
			my_temp_point=my_temp_point + STR_ANNUAL_LABEL_LENGTH ;


			m_Annual_data.at(i).value = (unsigned char)(*(my_temp_point++));
			m_Annual_data.at(i).auto_manual = (unsigned char)(*(my_temp_point++));
			my_temp_point++;
		}
		return READANNUALROUTINE_T3000;
		}
		break;
	case READPROGRAM_T3000:
		{

			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_program_point))!=0)
				return -1;	//得到的结构长度错误;
			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_program_point);
			//m_Input_data_length = block_length;
			//my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
			//m_Program_data.clear();
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			if(start_instance >= BAC_PROGRAM_ITEM_COUNT)
				return -1;//超过长度了;

			for (i=start_instance;i<=end_instance;i++)
			{
				//Str_program_point temp_in;
				if(strlen(my_temp_point) > STR_PROGRAM_DESCRIPTION_LENGTH)
					memset(m_Program_data.at(i).description,0,STR_PROGRAM_DESCRIPTION_LENGTH);
				else
					memcpy_s( m_Program_data.at(i).description,STR_PROGRAM_DESCRIPTION_LENGTH,my_temp_point,STR_PROGRAM_DESCRIPTION_LENGTH);
				my_temp_point=my_temp_point + STR_PROGRAM_DESCRIPTION_LENGTH;
				if(strlen(my_temp_point) > STR_PROGRAM_LABEL_LENGTH)
					memset(m_Program_data.at(i).label,0,STR_PROGRAM_LABEL_LENGTH);
				else
					memcpy_s( m_Program_data.at(i).label,STR_PROGRAM_LABEL_LENGTH ,my_temp_point,STR_PROGRAM_LABEL_LENGTH );
				my_temp_point=my_temp_point + STR_PROGRAM_LABEL_LENGTH ;
				if(i==14)
					Sleep(1);
				 m_Program_data.at(i).bytes	= ((unsigned char)my_temp_point[1]<<8) | ((unsigned char)my_temp_point[0]);
				 my_temp_point = my_temp_point + 2;
				 m_Program_data.at(i).on_off = *(my_temp_point++);
				 m_Program_data.at(i).auto_manual = *(my_temp_point++);
				 m_Program_data.at(i).com_prg = *(my_temp_point++);
				 m_Program_data.at(i).errcode = *(my_temp_point++);
				 m_Program_data.at(i).unused = *(my_temp_point++);
				//m_Program_data.push_back(temp_in);
			}
			return READPROGRAM_T3000;
		}
		break;
	case READPROGRAMCODE_T3000://Fance 将program code 存至Buf 等待发送消息后使用解码函数
		{
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			if(start_instance >= BAC_PROGRAMCODE_ITEM_COUNT)
				return -1;//超过长度了;

			block_length = len_value_type - PRIVATE_HEAD_LENGTH;//Program code length  =  total -  head;
		//	int code_length = (unsigned char)(my_temp_point[1]*256) + (unsigned char)my_temp_point[0];
			int code_length = ((unsigned char)my_temp_point[1]<<8) | ((unsigned char)my_temp_point[0]);
			//my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
			memset(mycode,0,1024);

				memcpy_s(mycode,block_length,my_temp_point,block_length);
				memcpy_s(program_code[start_instance],block_length,my_temp_point,block_length);
				program_code_length[start_instance] = block_length;

			//if((code_length!=0) && (code_length <=400) && (code_length >0))
			//{
			//	memcpy_s(mycode,block_length+ 10,my_temp_point,block_length+ 10);
			//	memcpy_s(program_code[start_instance],block_length + 10,my_temp_point,block_length + 10);
			//	program_code_length[start_instance] = block_length + 10;
			//}
			//else
			//{
			//	memcpy_s(mycode,block_length+ 10,my_temp_point,block_length+ 10);
			//	memcpy_s(program_code[start_instance],block_length + 10,my_temp_point,block_length + 10);
			//	program_code_length[start_instance] = block_length;
			//}

			
		return READPROGRAMCODE_T3000;
		}
		break;
	case  READTIMESCHEDULE_T3000:
		{
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			block_length = len_value_type - PRIVATE_HEAD_LENGTH;//Program code length  =  total -  head;
			my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
			if(block_length!=(WEEKLY_SCHEDULE_SIZE))
				return -1;
			memset(weeklt_time_schedule[start_instance],0,WEEKLY_SCHEDULE_SIZE);
			memcpy_s(weeklt_time_schedule[start_instance],WEEKLY_SCHEDULE_SIZE,my_temp_point,WEEKLY_SCHEDULE_SIZE);

			//copy the schedule day time to my own buffer.
			for (int j=0;j<9;j++)
			{
				for (int i=0;i<8;i++)
				{
					m_Schedual_Time_data.at(start_instance).Schedual_Day_Time[i][j].time_minutes = *(my_temp_point ++);
					m_Schedual_Time_data.at(start_instance).Schedual_Day_Time[i][j].time_hours = *(my_temp_point ++);
				}
			}

			return READTIMESCHEDULE_T3000;
		}
		break;
	case READANNUALSCHEDULE_T3000:
		{
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			block_length = len_value_type - PRIVATE_HEAD_LENGTH;//Program code length  =  total -  head;
			my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
			if(block_length!=ANNUAL_CODE_SIZE)
				return -1;
			memset(&g_DayState[start_instance],0,ANNUAL_CODE_SIZE);
			memcpy_s(&g_DayState[start_instance],block_length,my_temp_point,block_length);


			return READANNUALSCHEDULE_T3000;
		}
		break;
	case  TIME_COMMAND:
		{
		block_length = len_value_type - PRIVATE_HEAD_LENGTH;//Program code length  =  total -  head;
		my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
		if(block_length!=sizeof(Time_block_mini))
			return -1;
		Device_time.ti_sec = *(my_temp_point ++);
		Device_time.ti_min = *(my_temp_point ++);
		Device_time.ti_hour = *(my_temp_point ++);
		Device_time.dayofmonth = *(my_temp_point ++);
		Device_time.dayofweek = *(my_temp_point ++);
		Device_time.month = *(my_temp_point ++);
		Device_time.year = *(my_temp_point ++);
		
		

		//temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
		//Device_time.dayofyear = temp_struct_value;
		//my_temp_point = my_temp_point + 4;

		temp_struct_value = ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
		Device_time.dayofyear = temp_struct_value;
		my_temp_point = my_temp_point + 2;

		Device_time.isdst = *(my_temp_point ++);

		if(Device_time.ti_sec>=60)
			Device_time.ti_sec=0;
		if(Device_time.ti_min>=60)
			Device_time.ti_min=0;
		if(Device_time.ti_hour>=24)
			Device_time.ti_hour=0;
		if((Device_time.dayofmonth>=32)||(Device_time.dayofmonth==0))
			Device_time.dayofmonth=1;
		if((Device_time.month>12) || (Device_time.month == 0))
			Device_time.month = 1;
		if((Device_time.year>50))
			Device_time.year = 13;
		if((Device_time.dayofweek >7) || (Device_time.dayofweek == 0))
			Device_time.dayofweek = 1;
		if((Device_time.dayofyear >366) || (Device_time.dayofyear == 0))
			Device_time.dayofyear = 1;
		//::PostMessage(BacNet_hwd,WM_FRESH_CM_LIST,NULL,NULL);
		//byte  ti_min;         // 0-59
		//byte  ti_hour;           // 0-23
		//byte  dayofmonth;   // 1-31
		//byte  month;          // 0-11
		//byte  year;           // year - 1900
		//byte  dayofweek;        // 0-6 ; 0=Sunday
		//int   dayofyear;    // 0-365 gmtime
		//signed char isdst;


		return TIME_COMMAND;
		}
		break;
	case READCONTROLLER_T3000:
		{
		if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_controller_point))!=0)
			return -1;	//得到的结构长度错误;
		block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_controller_point);

		my_temp_point = (char *)Temp_CS.value + 3;
		start_instance = *my_temp_point;
		my_temp_point++;
		end_instance = *my_temp_point;
		my_temp_point++;
		my_temp_point = my_temp_point + 2;

		if(start_instance >= BAC_CONTROLLER_COUNT)
			return -1;//超过长度了;

		for (i=start_instance;i<=end_instance;i++)
		{
			m_controller_data.at(i).input.number = *(my_temp_point++);
			m_controller_data.at(i).input.point_type = *(my_temp_point++);
			m_controller_data.at(i).input.panel = *(my_temp_point++);

			//这里先加卡关条件，目前暂时不支持 其他panel的Input
			//if(m_controller_data.at(i).input.number>=BAC_INPUT_ITEM_COUNT)
			//	m_controller_data.at(i).input.number = 0;
			//if(m_controller_data.at(i).input.panel != bac_gloab_panel )
			//	m_controller_data.at(i).input.panel = bac_gloab_panel;

			temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
			m_controller_data.at(i).input_value = temp_struct_value;

			my_temp_point=my_temp_point+4;
			temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
			m_controller_data.at(i).value = temp_struct_value;
			my_temp_point=my_temp_point+4;

			m_controller_data.at(i).setpoint.number = *(my_temp_point++);
			m_controller_data.at(i).setpoint.point_type = *(my_temp_point++);
			m_controller_data.at(i).setpoint.panel = *(my_temp_point++);

			temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
			m_controller_data.at(i).setpoint_value = temp_struct_value;
			my_temp_point=my_temp_point+4;

			m_controller_data.at(i).units = *(my_temp_point++);
			m_controller_data.at(i).auto_manual = *(my_temp_point++);
			m_controller_data.at(i).action = *(my_temp_point++);
			m_controller_data.at(i).repeats_per_min = *(my_temp_point++);
			m_controller_data.at(i).unused = *(my_temp_point++);
			m_controller_data.at(i).prop_high = *(my_temp_point++);
			m_controller_data.at(i).proportional = *(my_temp_point++);
			m_controller_data.at(i).reset = *(my_temp_point++);
			m_controller_data.at(i).bias = *(my_temp_point++);
			m_controller_data.at(i).rate = *(my_temp_point++);
		}



		return READCONTROLLER_T3000;
		}
		break;
	case READSCREEN_T3000:
		{
		if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Control_group_point))!=0)
			return -1;	
		block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Control_group_point);

		my_temp_point = (char *)Temp_CS.value + 3;
		start_instance = *my_temp_point;
		my_temp_point++;
		end_instance = *my_temp_point;
		my_temp_point++;
		my_temp_point = my_temp_point + 2;

		if(start_instance >= BAC_SCREEN_COUNT)
			return -1;//超过长度了;

		for (i=start_instance;i<=end_instance;i++)
		{
			if(strlen(my_temp_point) > STR_SCREEN_DESCRIPTION_LENGTH)
				memset(m_screen_data.at(i).description,0,STR_SCREEN_DESCRIPTION_LENGTH);
			else
				memcpy_s( m_screen_data.at(i).description,STR_SCREEN_DESCRIPTION_LENGTH,my_temp_point,STR_SCREEN_DESCRIPTION_LENGTH);
			my_temp_point=my_temp_point + STR_SCREEN_DESCRIPTION_LENGTH;

			if(strlen(my_temp_point) > STR_SCREEN_LABLE_LENGTH)
				memset(m_screen_data.at(i).label,0,STR_SCREEN_LABLE_LENGTH);
			else
				memcpy_s( m_screen_data.at(i).label,STR_SCREEN_LABLE_LENGTH ,my_temp_point,STR_SCREEN_LABLE_LENGTH );
			my_temp_point=my_temp_point + STR_SCREEN_LABLE_LENGTH ;

			if(strlen(my_temp_point) > STR_SCREEN_PIC_FILE_LENGTH)
				memset(m_screen_data.at(i).picture_file,0,STR_SCREEN_PIC_FILE_LENGTH);
			else
				memcpy_s( m_screen_data.at(i).picture_file,STR_SCREEN_PIC_FILE_LENGTH ,my_temp_point,STR_SCREEN_PIC_FILE_LENGTH );
			my_temp_point=my_temp_point + STR_SCREEN_PIC_FILE_LENGTH ;

			m_screen_data.at(i).update = *(my_temp_point++);
			m_screen_data.at(i).mode = *(my_temp_point++);
			m_screen_data.at(i).xcur_grp = *(my_temp_point++);
			unsigned short temp_ycur_grp;
			temp_ycur_grp = ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
			m_screen_data.at(i).ycur_grp = temp_ycur_grp;
			my_temp_point = my_temp_point + 2;
		}
		return READSCREEN_T3000;
		}
		break;
	case READMONITOR_T3000:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_monitor_point))!=0)
				return -1;	
			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_monitor_point);

			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;
			if(start_instance >= BAC_MONITOR_COUNT)
				return -1;//超过长度了;

			for (i=start_instance;i<=end_instance;i++)
			{
				if(strlen(my_temp_point) > STR_MONITOR_LABEL_LENGTH)
					memset(m_monitor_data.at(i).label,0,STR_MONITOR_LABEL_LENGTH);
				else
					memcpy_s( m_monitor_data.at(i).label,STR_MONITOR_LABEL_LENGTH,my_temp_point,STR_MONITOR_LABEL_LENGTH);
				my_temp_point=my_temp_point + STR_MONITOR_LABEL_LENGTH;

				for (int j=0;j<MAX_POINTS_IN_MONITOR;j++)
				{
					m_monitor_data.at(i).inputs[j].number = *(my_temp_point++);
					m_monitor_data.at(i).inputs[j].point_type = *(my_temp_point++);
					m_monitor_data.at(i).inputs[j].panel = *(my_temp_point++);
					m_monitor_data.at(i).inputs[j].network = ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
					my_temp_point = my_temp_point + 2;
				}
				for (int k=0;k<MAX_POINTS_IN_MONITOR;k++)
				{
					m_monitor_data.at(i).range[k] = *(my_temp_point++);
				}
				m_monitor_data.at(i).second_interval_time = *(my_temp_point++);
				m_monitor_data.at(i).minute_interval_time = *(my_temp_point++);
				m_monitor_data.at(i).hour_interval_time   = *(my_temp_point++);
				m_monitor_data.at(i).max_time_length = *(my_temp_point++);
				m_monitor_data.at(i).num_inputs = *(my_temp_point++);
				m_monitor_data.at(i).an_inputs = *(my_temp_point++);
				m_monitor_data.at(i).unit = *(my_temp_point++);
				m_monitor_data.at(i).wrap_flag = *(my_temp_point++);
				m_monitor_data.at(i).status= *(my_temp_point++);
				m_monitor_data.at(i).reset_flag= *(my_temp_point++);
				m_monitor_data.at(i).double_flag= *(my_temp_point++);
			}
		}
		return READMONITOR_T3000;
		break;
	case  READALARM_T3000:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Alarm_point))!=0)
				return -1;	//得到的结构长度错误;
			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Alarm_point);

			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			if(end_instance == (BAC_ALARMLOG_COUNT - 1))
				end_flag = true;

			if(start_instance >= BAC_ALARMLOG_COUNT)
				return -1;//超过长度了;
			m_alarmlog_data.at(start_instance).point.number = *(my_temp_point++);
			m_alarmlog_data.at(start_instance).point.point_type = *(my_temp_point++);
			m_alarmlog_data.at(start_instance).point.panel = *(my_temp_point++);
			temp_struct_value = (unsigned char)my_temp_point[1]<<8 | (unsigned char)my_temp_point[0];
			m_alarmlog_data.at(start_instance).point.network = temp_struct_value;
			my_temp_point = my_temp_point + 2;
			m_alarmlog_data.at(start_instance).modem = *(my_temp_point++);
			m_alarmlog_data.at(start_instance).printer = *(my_temp_point++);
			m_alarmlog_data.at(start_instance).alarm =  *(my_temp_point++);

			//if one of the alarm is not zero ,show the alarm window.
			bac_show_alarm_window = bac_show_alarm_window || m_alarmlog_data.at(start_instance).alarm;

			m_alarmlog_data.at(start_instance).restored =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).acknowledged =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).ddelete =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).type =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).cond_type =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).level =  *(my_temp_point++);

			temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
			m_alarmlog_data.at(start_instance).alarm_time =(unsigned int) temp_struct_value;
			my_temp_point = my_temp_point + 4;
			m_alarmlog_data.at(start_instance).alarm_count =  *(my_temp_point++);


			if(strlen(my_temp_point) > ALARM_MESSAGE_SIZE)
				memset(m_alarmlog_data.at(start_instance).alarm_message,0,ALARM_MESSAGE_SIZE + 1);
			else
				memcpy_s( m_alarmlog_data.at(start_instance).alarm_message,ALARM_MESSAGE_SIZE + 1,my_temp_point,ALARM_MESSAGE_SIZE + 1);
			my_temp_point=my_temp_point + ALARM_MESSAGE_SIZE + 1;

			my_temp_point = my_temp_point + 5;//ignore char  none[5];

			m_alarmlog_data.at(start_instance).panel_type =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).dest_panel_type =  *(my_temp_point++);

			temp_struct_value = (unsigned char)my_temp_point[1]<<8 | (unsigned char)my_temp_point[0];
			m_alarmlog_data.at(start_instance).alarm_id = (unsigned short)temp_struct_value;
			my_temp_point = my_temp_point + 2;
			m_alarmlog_data.at(start_instance).prg =  *(my_temp_point++);


			m_alarmlog_data.at(start_instance).alarm_panel =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where1 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where2 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where3 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where4 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where5 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where_state1 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where_state2 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where_state3 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where_state4 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).where_state5 =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).change_flag =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).original =  *(my_temp_point++);
			m_alarmlog_data.at(start_instance).no =  *(my_temp_point++);

		}
		return READALARM_T3000;
		break;
	case READ_SETTING_COMMAND:
		{
			block_length = len_value_type - PRIVATE_HEAD_LENGTH;//Program code length  =  total -  head;
			my_temp_point = (char *)Temp_CS.value + PRIVATE_HEAD_LENGTH;
			if(block_length!=sizeof(Str_Setting_Info))
				return -1;

			memcpy_s(Device_Basic_Setting.reg.ip_addr,4,my_temp_point,4);
			my_temp_point = my_temp_point + 4;
			memcpy_s(Device_Basic_Setting.reg.subnet,4,my_temp_point,4);
			my_temp_point = my_temp_point + 4;
			memcpy_s(Device_Basic_Setting.reg.gate_addr,4,my_temp_point,4);
			my_temp_point = my_temp_point + 4;
			memcpy_s(Device_Basic_Setting.reg.mac_addr,6,my_temp_point,6);
			my_temp_point = my_temp_point + 6;
			Device_Basic_Setting.reg.tcp_type = *(my_temp_point++);
			Device_Basic_Setting.reg.mini_type = *(my_temp_point++);
			if(Device_Basic_Setting.reg.mini_type == BIG_MINIPANEL)
				bacnet_device_type = BIG_MINIPANEL;
			else if(Device_Basic_Setting.reg.mini_type == SMALL_MINIPANEL)
				bacnet_device_type = SMALL_MINIPANEL;
			else
				bacnet_device_type = PRODUCT_CM5;
			my_temp_point = my_temp_point + 1;	//中间 minitype  和 debug  没什么用;
			Device_Basic_Setting.reg.pro_info.harware_rev = *(my_temp_point++);
			Device_Basic_Setting.reg.pro_info.firmware0_rev_main = *(my_temp_point++);
			Device_Basic_Setting.reg.pro_info.firmware0_rev_sub = *(my_temp_point++);

			Device_Basic_Setting.reg.pro_info.frimware1_rev = *(my_temp_point++);
			Device_Basic_Setting.reg.pro_info.frimware2_rev = *(my_temp_point++);
			Device_Basic_Setting.reg.pro_info.frimware3_rev = *(my_temp_point++);
			Device_Basic_Setting.reg.pro_info.bootloader_rev = *(my_temp_point++);
			my_temp_point = my_temp_point + 10;
			Device_Basic_Setting.reg.com0_config = *(my_temp_point++);
			Device_Basic_Setting.reg.com1_config = *(my_temp_point++);
			Device_Basic_Setting.reg.com2_config = *(my_temp_point++);
			Device_Basic_Setting.reg.refresh_flash_timer =  *(my_temp_point++);
			Device_Basic_Setting.reg.en_plug_n_play =  *(my_temp_point++);
			Device_Basic_Setting.reg.reset_default = *(my_temp_point++);

			Device_Basic_Setting.reg.com_baudrate0 = *(my_temp_point++);
			Device_Basic_Setting.reg.com_baudrate1 = *(my_temp_point++);
			Device_Basic_Setting.reg.com_baudrate2 = *(my_temp_point++);

			return READ_SETTING_COMMAND;
		}
		break;
	case READMONITORDATA_T3000:
		{
			handle_read_monitordata((char *)Temp_CS.value,len_value_type);
			return READMONITORDATA_T3000;
		}
		break;
	case READ_REMOTE_DEVICE_DB:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_Remote_TstDB))!=0)
				return -1;
			m_remote_device_db.clear();

			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_Remote_TstDB);
			if(block_length == 0)
				break;
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			for (int x=0;x<block_length;x++)
			{
				Str_Remote_TstDB temp;
				m_remote_device_db.push_back(temp);
				m_remote_device_db.at(x).sn = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
				my_temp_point = my_temp_point + 4;
				m_remote_device_db.at(x).product_type = *(my_temp_point++);
				m_remote_device_db.at(x).modbus_id = *(my_temp_point++);
				memcpy_s(m_remote_device_db.at(x).ip_addr,4,my_temp_point,4);
				my_temp_point = my_temp_point + 4;
				m_remote_device_db.at(x).port =  ((unsigned char)my_temp_point[0]<<8) | ((unsigned char)my_temp_point[1]);
				my_temp_point = my_temp_point + 2;
				memcpy_s(m_remote_device_db.at(x).reserved,10,my_temp_point,10);
				my_temp_point = my_temp_point + 10;
			}
			Sleep(1);
		}
		break;
	case GETSERIALNUMBERINFO:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_Serial_info))!=0)
				return -1;


			_Bac_Scan_results_Info temp_struct;
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;
			temp_struct.device_id	= ((unsigned char)my_temp_point[1]<<8) | ((unsigned char)my_temp_point[0]);
			
			 my_temp_point = my_temp_point +2;
			 memcpy_s(temp_struct.ipaddress,6,my_temp_point,6);
			 
			//  temp_struct.panel_number = *(my_temp_point + 3);//Notice
			// temp_struct.macaddress = *my_temp_point;
			 my_temp_point = my_temp_point + 6;
			 temp_struct.serialnumber = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
			 my_temp_point = my_temp_point + 4;
			 temp_struct.modbus_addr =  *(my_temp_point++);
			 temp_struct.product_type =  *(my_temp_point++);
			  temp_struct.panel_number =  *(my_temp_point++);
			  temp_struct.modbus_port = ((unsigned char)my_temp_point[1]<<8) | ((unsigned char)my_temp_point[0]);
			  my_temp_point = my_temp_point + 2;
			  temp_struct.software_version = ((unsigned char)my_temp_point[1]<<8) | ((unsigned char)my_temp_point[0]);
			  my_temp_point = my_temp_point + 2;
			  temp_struct.hardware_version =  *(my_temp_point++);
			
			 int find_exsit = false;
			 TRACE(_T("serialnumber = %d ,modbus_addr = %d , product_type = %d ,ip = %u.%u.%u.%u , instance = %d\r\n"),temp_struct.serialnumber,
				 temp_struct.modbus_addr,temp_struct.product_type,temp_struct.ipaddress[0],temp_struct.ipaddress[1] ,
				 temp_struct.ipaddress[2],temp_struct.ipaddress[3],temp_struct.device_id);
			 for (int x=0;x<(int)m_bac_scan_result_data.size();x++)
			 {
				 if(temp_struct.serialnumber == m_bac_scan_result_data.at(x).serialnumber)
					 find_exsit = true;
			 }
			 if(!find_exsit)
			 {
				 m_bac_scan_result_data.push_back(temp_struct);
				 //CTStat_Dev* pTemp = new CTStat_Dev;			
				 //_ComDeviceInfo* pInfo = new _ComDeviceInfo;
				 //pInfo->m_pDev = pTemp;

				 //pTemp->SetSerialID(temp_struct.serialnumber);
				 //pTemp->SetDevID(temp_struct.modbus_addr);
				 //pTemp->SetProductType(temp_struct.product_type);
			 }
		}
		break;
	case READTSTAT_T3000:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_TstatInfo_point))!=0)
				return -1;	//得到的结构长度错误;
			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_TstatInfo_point);
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			if(start_instance >= BAC_TSTAT_COUNT)
				return -1;//超过长度了;

			if(end_instance == (BAC_TSTAT_COUNT - 1))
				end_flag = true;
			for (i=start_instance;i<=end_instance;i++)
			{
				m_Tstat_data.at(i).product_model = *(my_temp_point++);
				m_Tstat_data.at(i).temperature = ((unsigned char)my_temp_point[0]<<8) | ((unsigned char)my_temp_point[1]);
				my_temp_point = my_temp_point + 2;
				m_Tstat_data.at(i).mode = *(my_temp_point++);
				m_Tstat_data.at(i).cool_heat_mode = *(my_temp_point++);
				m_Tstat_data.at(i).setpoint =  ((unsigned char)my_temp_point[0]<<8) | ((unsigned char)my_temp_point[1]);
				my_temp_point = my_temp_point + 2;
				m_Tstat_data.at(i).cool_setpoint =  ((unsigned char)my_temp_point[0]<<8) | ((unsigned char)my_temp_point[1]);
				my_temp_point = my_temp_point + 2;
				m_Tstat_data.at(i).heat_setpoint =  ((unsigned char)my_temp_point[0]<<8) | ((unsigned char)my_temp_point[1]);
				my_temp_point = my_temp_point + 2;
				m_Tstat_data.at(i).occupied = *(my_temp_point++);
				m_Tstat_data.at(i).output_state = *(my_temp_point++);

				m_Tstat_data.at(i).night_heat_db = *(my_temp_point++);
				m_Tstat_data.at(i).night_cool_db = *(my_temp_point++);
				m_Tstat_data.at(i).night_heat_sp = *(my_temp_point++);
				m_Tstat_data.at(i).night_cool_sp = *(my_temp_point++);
				m_Tstat_data.at(i).over_ride = *(my_temp_point++);
				m_Tstat_data.at(i).tst_db.id = *(my_temp_point++);
				m_Tstat_data.at(i).tst_db.sn = ((unsigned char)my_temp_point[0])<<24 | ((unsigned char)my_temp_point[1]<<16) | ((unsigned char)my_temp_point[2])<<8 | ((unsigned char)my_temp_point[3]);
				my_temp_point = my_temp_point + 4;
				m_Tstat_data.at(i).tst_db.port = *(my_temp_point++);
				m_Tstat_data.at(i).type = *(my_temp_point++);
			}

		}
		break;
	case READUNIT_T3000:
		{
			if((len_value_type - PRIVATE_HEAD_LENGTH)%(sizeof(Str_Units_element))!=0)
				return -1;	//得到的结构长度错误;
			block_length=(len_value_type - PRIVATE_HEAD_LENGTH)/sizeof(Str_Units_element);
			my_temp_point = (char *)Temp_CS.value + 3;
			start_instance = *my_temp_point;
			my_temp_point++;
			end_instance = *my_temp_point;
			my_temp_point++;
			my_temp_point = my_temp_point + 2;

			if(start_instance >= BAC_CUSTOMER_UNITS_COUNT)
				return -1;//超过长度了;
			if(end_instance == (BAC_CUSTOMER_UNITS_COUNT - 1))
			{
				end_flag = true;
				receive_customer_unit = true;
			}
			for (i=start_instance;i<=end_instance;i++)
			{
				m_customer_unit_data.at(i).direct = *(my_temp_point++);
				memcpy_s(m_customer_unit_data.at(i).digital_units_off,12,my_temp_point,12);
				my_temp_point = my_temp_point + 12;
				memcpy_s(m_customer_unit_data.at(i).digital_units_on,12,my_temp_point,12);
				my_temp_point = my_temp_point + 12;


				MultiByteToWideChar( CP_ACP, 0, (char *)m_customer_unit_data.at(i).digital_units_off, (int)strlen((char *)m_customer_unit_data.at(i).digital_units_off)+1, 
					temp_off[i].GetBuffer(MAX_PATH), MAX_PATH );
				temp_off[i].ReleaseBuffer();
				if(temp_off[i].GetLength() >= 12)
					temp_off[i].Empty();

				MultiByteToWideChar( CP_ACP, 0, (char *)m_customer_unit_data.at(i).digital_units_on, (int)strlen((char *)m_customer_unit_data.at(i).digital_units_on)+1, 
					temp_on[i].GetBuffer(MAX_PATH), MAX_PATH );
				temp_on[i].ReleaseBuffer();
				if(temp_on[i].GetLength() >= 12)
					temp_on[i].Empty();

				temp_unit_no_index[i] = temp_off[i] + _T("/") + temp_on[i];

			}

		}
		break;
	}
	return 1;
}

void new_temp_analog_data_block(unsigned char nmonitor_count,unsigned int receive_length)
{
	int	temp_receive_data_count = receive_length / 4;
	int	temp_new_each_data_count = temp_receive_data_count / nmonitor_count;	//得到那14个input 平均每组要new多少个;
	for (int i=0;i<nmonitor_count;i++)		//new 每个的结构;
	{
		if(temp_new_each_data_count == 0 )
			return;
		temp_analog_data[i] = new Data_Time_Match[temp_new_each_data_count];
		//memset(temp_analog_data[i],0,temp_new_each_data_count * sizeof(Data_Time_Match));
	}	
}

//This function code by Fance du for receive the monitor data;
int handle_read_monitordata(char *npoint,int nlength)
{
	char * my_temp_point = npoint;
	char * temp_print = my_temp_point;
	
	unsigned short temp_send_length = 0;
	int temp_struct_value = 0;
	CString temp_cs;
	CString temp_char;
	static unsigned int receive_length = 0;						//需要接收多少个字节的数据;
	static unsigned int already_receive_length = 0;				//已经接收了多少字节；
	static unsigned int receive_all_data_count = 0 ;            //所有完整的里面有多少数据;
	static unsigned int receive_data_count = 0;					//多少笔数据;
	static unsigned int already_receive_count = 0;				//已经接收了多少笔数据;
	static unsigned int each_count_input_data = 0;				//这写数据中 每笔input 含多少笔;

	static unsigned int time_offset_count = 0;						//时间与原始的值偏移了多少次;
	static unsigned char receive_monitor_count = 0;				//有几个模拟量 或数字量;
	static bool flag_data_receive_finished = false;				//标记 是否带头的 数据 接收完毕，例如1000，那没帧 250 ，要4帧 不带头的;
	static unsigned short Filledata_count = 0;					//用来记录该把数据填到那个结构里面去了;
	static unsigned int interval_time = 0;						//每笔数据的间隔时间;
	static unsigned int last_start_time = 0;					//上一帧数据的last time的值，Chealse 专用，用来判断最后一帧是不是多余的;
	static bool ignore_left_data = false;						//如果收到相同的时间久忽略后面的 包;
	if(nlength <= sizeof(Str_Monitor_data_header))
	{
		g_Print.Format(_T("Monitor_data_header = %d"),nlength);
		DFTrace(g_Print);
		return 0;
	}
#if 0
	CString CS_Value;
	CString Write_Position;
	Write_Position.Format(_T("D:\\Test.txt"));
	CS_Value.Empty();
	for (int i =0; i< nlength ; i++)
	{
		temp_char.Format(_T("%02x"),(unsigned char)*temp_print);
		temp_char.MakeUpper();
		temp_print ++;
		temp_cs = temp_cs + temp_char + _T(" ");
	}
	CS_Value = temp_cs + _T("\r\n \r\n");

	CFile file(Write_Position,CFile::modeCreate |CFile::modeReadWrite |CFile::modeNoTruncate);
	file.SeekToEnd();
	int write_length = sizeof(TCHAR)*wcslen(CS_Value.GetBuffer());
	file.Write(CS_Value,write_length);
	file.Flush();
	CS_Value.ReleaseBuffer();
	file.Close();
#endif
	temp_print = my_temp_point;


	m_monitor_head.total_length =  (unsigned char)my_temp_point[1]<<8 | (unsigned char)my_temp_point[0];
	my_temp_point = my_temp_point + 2;
	m_monitor_head.command = *(my_temp_point++);
	m_monitor_head.index = *(my_temp_point++);
	m_monitor_head.type = *(my_temp_point++);
	m_monitor_head.conm_args.nsize = ((unsigned char)my_temp_point[0])<<24 | ((unsigned char)my_temp_point[1]<<16) | ((unsigned char)my_temp_point[2])<<8 | ((unsigned char)my_temp_point[3]);
	my_temp_point = my_temp_point + 4;
	m_monitor_head.conm_args.oldest_time = ((unsigned char)my_temp_point[0])<<24 | ((unsigned char)my_temp_point[1]<<16) | ((unsigned char)my_temp_point[2])<<8 | ((unsigned char)my_temp_point[3]);
	my_temp_point = my_temp_point + 4;
	m_monitor_head.conm_args.most_recent_time = ((unsigned char)my_temp_point[0])<<24 | ((unsigned char)my_temp_point[1]<<16) | ((unsigned char)my_temp_point[2])<<8 | ((unsigned char)my_temp_point[3]);
	my_temp_point = my_temp_point + 4;
	m_monitor_head.special = *(my_temp_point++);
	m_monitor_head.total_seg = *(my_temp_point++);
	m_monitor_head.seg_index = *(my_temp_point++); 

	//	return 0;

	if(m_monitor_head.seg_index == 0)
	{			
		receive_length = 0;					
		already_receive_length = 0;				
		receive_data_count = 0;				
		already_receive_count = 0;				
		time_offset_count = 0;					
		receive_monitor_count = 0;				
	
		flag_data_receive_finished = false;				
		Filledata_count = 0;					
		interval_time = 0;	
		receive_all_data_count = 0;
		each_count_input_data = 0;
		ignore_left_data = false;
	}
	else
	{
		if(flag_data_receive_finished == false) //上一笔的数据还没有传输完毕，这一笔直接继续 20头 + 数据;
		{
			goto fillindata_head;
		}
	}
	g_Print.Format(_T("total_seg = %d , seg_index = %d"),m_monitor_head.total_seg,m_monitor_head.seg_index);
	DFTrace(g_Print);
	if(m_monitor_head.seg_index == 0)	//如果是第一帧数据，必须包含头;
	{
		if(nlength <= (sizeof(Str_Monitor_data_header) + sizeof(Monitor_Block_Header)))
		{
			g_Print.Format(_T("length is = %d"),nlength);
			DFTrace(g_Print);
			return 0;
		}
	}

	//if((m_monitor_head.seg_index == 0) || (m_monitor_head.seg_index == 1))
	if(m_monitor_head.seg_index < 10)
	{
		g_Print.Format(_T("Revice length = %d"),nlength);
		DFTrace(g_Print);
		int split_part;
		int part_value = 40;
		split_part = nlength/40;
		for (int x=0;x<split_part;x++)
		{
			temp_cs.Format(_T("Seg_Index %02d Part%02d :"),m_monitor_head.seg_index,x+1);
			for (int i = part_value*x; i< (part_value)*(x+1) ; i++)
			{
				temp_char.Format(_T("%02x"),(unsigned char)*temp_print);
				temp_char.MakeUpper();
				temp_print ++;
				temp_cs = temp_cs + temp_char + _T(" ");
			}
			g_Print = temp_cs;
			DFTrace(g_Print);
		}

	}

	if(ignore_left_data)	//只要收到相同时间的 包，就说明下位机发来的数据后面的都是以前发过的，忽略它;
		return 1;

reveive_head:
	//my_temp_point = my_temp_point + PRIVATE_MONITOR_HEAD_LENGTH;
	my_temp_point = my_temp_point + MAX_POINTS_IN_MONITOR * 5 ;
	m_monitor_block.monitor =  *(my_temp_point++);
	m_monitor_block.no_points =  *(my_temp_point++);
	m_monitor_block.second_interval_time =  *(my_temp_point++);
	m_monitor_block.minute_interval_time =  *(my_temp_point++);
	m_monitor_block.hour_interval_time =  *(my_temp_point++);
	m_monitor_block.priority =  *(my_temp_point++);
	m_monitor_block.first_block =  *(my_temp_point++);
	m_monitor_block.last_block =  *(my_temp_point++);
	m_monitor_block.analog_digital =  *(my_temp_point++);
	m_monitor_block.block_state =  *(my_temp_point++);
	m_monitor_block.fast_sampling =  *(my_temp_point++);
	m_monitor_block.wrap_around =  *(my_temp_point++);

	temp_send_length = (unsigned char)my_temp_point[0]<<8 | (unsigned char)my_temp_point[1]; 
	//下面的是正确的，等chelsea改完就用下面的;
	//temp_send_length = (unsigned char)my_temp_point[1]<<8 | (unsigned char)my_temp_point[0]; 
	m_monitor_block.send_lenght = temp_send_length;
	my_temp_point = my_temp_point + 8;//这里是长度和 nouse;

	//temp_struct_value = ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
	temp_struct_value = ((unsigned char)my_temp_point[0])<<24 | ((unsigned char)my_temp_point[1]<<16) | ((unsigned char)my_temp_point[2])<<8 | ((unsigned char)my_temp_point[3]);
	m_monitor_block.start_time =temp_struct_value; 
	my_temp_point = my_temp_point + 4;

	if(last_start_time != m_monitor_block.start_time)
	{
		last_start_time = m_monitor_block.start_time;
	}
	else
	{
		ignore_left_data = true;
		TRACE(_T("last_start_time = start_time\r\n"));
		return 1;
	}

	temp_struct_value = (unsigned char)my_temp_point[0]<<8 | (unsigned char)my_temp_point[1];
	m_monitor_block.index =temp_struct_value; 
	my_temp_point = my_temp_point + 2;


	m_monitor_block.send_lenght = m_monitor_block.index * 4;

	m_monitor_block.next_block =  *(my_temp_point++);
	m_monitor_block.block_no =  *(my_temp_point++);
	m_monitor_block.last_digital_state =  *(my_temp_point++);
	my_temp_point = my_temp_point + 1; // uint8_t     reserved;

	receive_length = m_monitor_block.send_lenght;
	receive_data_count = receive_length / 4 ;	//模拟量才是除以4;每笔占用4个字节;

	receive_monitor_count = m_monitor_block.no_points;
	interval_time = m_monitor_block.hour_interval_time*3600 + m_monitor_block.minute_interval_time*60 + m_monitor_block.second_interval_time;
	time_offset_count = 0; //每次新发头的时候 都要把偏移量清零;
	already_receive_count = 0; //已经接收了多少 也要清零;
	Filledata_count = 0; //填值的也要清零;
	if(receive_monitor_count != 0)
		each_count_input_data = receive_data_count / receive_monitor_count ;
	else
	{
		each_count_input_data = 0;
		flag_data_receive_finished = true ;
		ignore_left_data = true;
		return 2;
	}
	if(m_monitor_block.analog_digital == 0)	//如果这80几个字节的头里面是 传的模拟量 就new int;
	{
		new_temp_analog_data_block(receive_monitor_count,receive_length);
	}
	//if(m_monitor_block.analog_digital == 0)	//如果这80几个字节的头里面是 传的模拟量 就new int;
	//{
	//	receive_data_count = receive_length / 4;
	//	new_each_data_count = receive_data_count / receive_monitor_count;	//得到那14个input 平均每组要new多少个;
	//	for (int i=0;i<receive_monitor_count;i++)		//new 每个的结构;
	//	{
	//		temp_analog_data[i] = new Data_Time_Match[new_each_data_count];
	//		memset(temp_analog_data[i],0,new_each_data_count * sizeof(Data_Time_Match));
	//	}	
	//}
		 
	//analog_data_point
	if(m_monitor_head.seg_index == 0)
	{
		g_Print.Format(_T("(%d) monitor = %d ,no_points = %d , length = %d"),m_monitor_head.seg_index,m_monitor_block.monitor,m_monitor_block.no_points ,m_monitor_block.send_lenght);
		DFTrace(g_Print);
		g_Print.Format(_T("(%d)Interval time %d:%d:%d"),m_monitor_head.seg_index,m_monitor_block.hour_interval_time,m_monitor_block.minute_interval_time,m_monitor_block.second_interval_time);
		DFTrace(g_Print);
		g_Print.Format(_T("(%d)priority = %d ,first_block = %d ,last_block = %d ,analog_digital = %d"),m_monitor_head.seg_index,m_monitor_block.priority,
			m_monitor_block.first_block,m_monitor_block.last_block,m_monitor_block.analog_digital);
		DFTrace(g_Print);
		g_Print.Format(_T("(%d)block_state = %d , fast_sampling = %d , wrap_around = %d"),m_monitor_head.seg_index,m_monitor_block.block_state,m_monitor_block.fast_sampling,m_monitor_block.wrap_around);
		DFTrace(g_Print);
	}

	char *data_temp_point;
	data_temp_point = my_temp_point;
	int left_part_length =0; //此帧数据剩余的字节数;
	int delta_pass_1 = my_temp_point - npoint;
	left_part_length = nlength - delta_pass_1;


	if((receive_length == 0) && (left_part_length > sizeof(Monitor_Block_Header)))	//发两个头在一帧里面 需要判断 够发两个头的数据;
		goto reveive_head;
	//already_receive_length


fillindata_head:
	if(each_count_input_data == 0)
	{
		flag_data_receive_finished = true ;
		return 1;
	}
	//这里要重新计算还能继续取多少个数据;
	int delta_pass = my_temp_point - npoint;
	left_part_length = nlength - delta_pass;
	int this_block_of_data_receive_count = 0;
	
	do 
	{
		if(temp_analog_data[Filledata_count] == NULL)
		{
			ignore_left_data = true;	
			//DFTrace(_T("Reveive error"));
			return 2;
		}
		temp_analog_data[Filledata_count][time_offset_count].loggingtime =  m_monitor_block.start_time + time_offset_count * interval_time;
		temp_analog_data[Filledata_count][time_offset_count].analogdata  =  ((unsigned char)my_temp_point[0])<<24 | ((unsigned char)my_temp_point[1]<<16) | ((unsigned char)my_temp_point[2])<<8 | ((unsigned char)my_temp_point[3]);
		if(temp_analog_data[Filledata_count][time_offset_count].analogdata > 100000)
			Sleep(1);
		//temp_analog_data[Filledata_count][time_offset_count].analogdata  =  ((unsigned char)my_temp_point[3])<<24 | ((unsigned char)my_temp_point[2]<<16) | ((unsigned char)my_temp_point[1])<<8 | ((unsigned char)my_temp_point[0]);
		my_temp_point = my_temp_point + 4;
		Filledata_count ++;
		Filledata_count = Filledata_count % receive_monitor_count;
		if(Filledata_count == 0)
			time_offset_count ++;
		this_block_of_data_receive_count ++;
		already_receive_count ++;	//已经接收了多少笔  ++;
	} while ((this_block_of_data_receive_count*4 < left_part_length) && (already_receive_count < receive_data_count ));
	
	
		if(already_receive_count < receive_data_count )
		{
			flag_data_receive_finished = false ;
		}
		else
		{
			flag_data_receive_finished = true ;
			//total_new_how_long = total_new_how_long + time_offset_count;
			receive_all_data_count = receive_all_data_count + already_receive_count;
			for (int i=0;i<m_monitor_block.no_points;i++)		//new 每个的结构;
			{
				if(analog_data_point[i] == NULL)
					analog_data_point[i] = new Data_Time_Match[10000];

				analog_data_count[i] = analog_data_count[i] + each_count_input_data;
				memcpy_s(analog_data_point[i] + analog_data_count[i] - each_count_input_data,each_count_input_data * sizeof(Data_Time_Match),
					temp_analog_data[i],each_count_input_data * sizeof(Data_Time_Match));
				if(temp_analog_data[i])
				{
					delete  temp_analog_data[i];
					temp_analog_data[i] = NULL;
				}
			}	
			get_data_count = m_monitor_block.no_points;


			//这里还要判断 这一帧数据里面 还有没有头和数据;不能直接就退出再下一帧;
			int  left_data =  my_temp_point - npoint;	//两个指针相减，得出长度;
			if(left_data < nlength)	//说明后面还有数据;不要直接return;在去重新接受头;
				goto reveive_head;

		}
	


	return 1;
}

extern void copy_data_to_ptrpanel(int Data_type);//Used for copy the structure to the ptrpanel.
void local_handler_conf_private_trans_ack(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    BACNET_PRIVATE_TRANSFER_DATA data;
    int len;

/*
 * Note:
 * We currently don't look at the source address and service data
 * but we probably should to verify that the ack is oneit is what
 * we were expecting. But this is just to silence some compiler
 * warnings from Borland.
 */
    src = src;
    service_data = service_data;

    len = 0;
#if PRINT_ENABLED
    printf("Received Confirmed Private Transfer Ack!\n");
#endif

    len = ptransfer_decode_service_request(service_request, service_len, &data);        /* Same decode for ack as for service request! */
    if (len < 0) {
#if PRINT_ENABLED
        printf("cpta: Bad Encoding!\n");
#endif
    }
	int receive_data_type;
	bool each_end_flag = false;
	receive_data_type = CM5ProcessPTA(&data,each_end_flag);
	switch(receive_data_type)
	{
	case READINPUT_T3000:
		if(each_end_flag)
			::PostMessage(m_input_dlg_hwnd,WM_REFRESH_BAC_INPUT_LIST,NULL,NULL);
		copy_data_to_ptrpanel(TYPE_INPUT);
		break;
	case READPROGRAM_T3000:
		::PostMessage(m_pragram_dlg_hwnd,WM_REFRESH_BAC_PROGRAM_LIST,NULL,NULL);
		copy_data_to_ptrpanel(TYPE_ALL);
		break;
	case READPROGRAMCODE_T3000:
		::PostMessage(m_program_edit_hwnd,WM_REFRESH_BAC_PROGRAM_RICHEDIT,NULL,NULL);
		break;
	case READVARIABLE_T3000:
		if(each_end_flag)
			::PostMessage(m_variable_dlg_hwnd,WM_REFRESH_BAC_VARIABLE_LIST,NULL,NULL);
		copy_data_to_ptrpanel(TYPE_VARIABLE);
		break;
	case READOUTPUT_T3000:
		if(each_end_flag)
			::PostMessage(m_output_dlg_hwnd,WM_REFRESH_BAC_OUTPUT_LIST,NULL,NULL);
		copy_data_to_ptrpanel(TYPE_OUTPUT);
		break;
	case READWEEKLYROUTINE_T3000:
		::PostMessage(m_weekly_dlg_hwnd,WM_REFRESH_BAC_WEEKLY_LIST,NULL,NULL);
		copy_data_to_ptrpanel(TYPE_WEEKLY);
		break;
	case READANNUALROUTINE_T3000:
		::PostMessage(m_annual_dlg_hwnd,WM_REFRESH_BAC_ANNUAL_LIST,NULL,NULL);
		copy_data_to_ptrpanel(TYPE_ANNUAL);
		break;
	case READTIMESCHEDULE_T3000:
		::PostMessage(m_schedule_time_dlg_hwnd,WM_REFRESH_BAC_SCHEDULE_LIST,NULL,NULL);
		break;
	case TIME_COMMAND:
		::PostMessage(m_setting_dlg_hwnd,WM_FRESH_SETTING_UI,TIME_COMMAND,NULL);
		break;
	case READANNUALSCHEDULE_T3000:
		::PostMessage(m_schedule_day_dlg_hwnd,WM_REFRESH_BAC_DAY_CAL,NULL,NULL);
		break;
	case READCONTROLLER_T3000:
		::PostMessage(m_controller_dlg_hwnd,WM_REFRESH_BAC_CONTROLLER_LIST,NULL,NULL);
		copy_data_to_ptrpanel(TYPE_ALL);
		break;
	case READSCREEN_T3000:
		copy_data_to_ptrpanel(TYPE_ALL);
		break;
	case READALARM_T3000:
		if(each_end_flag)
			::PostMessage(m_alarmlog_dlg_hwnd,WM_REFRESH_BAC_ALARMLOG_LIST,NULL,NULL);
		break;
	case READ_SETTING_COMMAND:
		::PostMessage(m_setting_dlg_hwnd,WM_FRESH_SETTING_UI,READ_SETTING_COMMAND,NULL);	
		break;
	case READTSTAT_T3000:
		if(each_end_flag)
			::PostMessage(m_tstat_dlg_hwnd,WM_REFRESH_BAC_TSTAT_LIST,NULL,NULL);
		break;
	default:
		break;
	}

    return;
}

//This function coded by Fance,used to split the cstring to each part.
void SplitCStringA(CStringArray &saArray, CString sSource, CString sToken)
{
	CString sTempSource, sTempSplitted;

	sTempSource = sSource;

	int nPos = sTempSource.Find(sToken);

	//--if there are no token in the string, then add itself and return.
	if(nPos == -1)
		saArray.Add(sTempSource);
	else
	{
		while(sTempSource.GetLength() > 0)
		{
			nPos = sTempSource.Find(sToken);
			if(nPos == -1)
			{
				saArray.Add(sTempSource.Trim());
				break;
			}
			else if(nPos == 0)
			{
				sTempSource = sTempSource.Mid(sToken.GetLength(), sTempSource.GetLength());
				continue;
			}
			else
			{
				sTempSplitted = sTempSource.Mid(0, nPos);
				saArray.Add(sTempSplitted.Trim());
				sTempSource = sTempSource.Mid(nPos + sToken.GetLength(), sTempSource.GetLength());
			}
		}
	}

}


CString GetProductName(int ModelID)
{
	CString strProductName;
	switch(ModelID)
	{
	case PM_TSTAT5A:
		strProductName="TStat5A";
		break;
	case PM_TSTAT5B:
		strProductName="TStat5B";
		break;
	case PM_TSTAT5B2:
		strProductName="TStat5B2";
		break;
	case PM_TSTAT5C:
		strProductName="TStat5C";
		break;
	case PM_TSTAT5D:
		strProductName="TStat5D";
		break;
	case PM_TSTAT5E:
		strProductName="TStat5E";
		break;
	case PM_TSTAT5F:
		strProductName="TStat5F";
		break;
	case PM_TSTAT5G:
		strProductName="TStat5G";
		break;
	case PM_TSTAT5H:
		strProductName="TStat5H";
		break;
    case PM_TSTAT6:
        strProductName="TStat6";
        break;
    case PM_TSTAT5i:
        strProductName="TStat5i";
        break;
	case PM_TSTAT7:
		strProductName="TStat7";
		break;
	case PM_NC:
		strProductName="NC";
		break;
	case PM_CM5:
		strProductName ="CM5";
		break;
		//20120424
	case PM_LightingController:
		strProductName = "LC";
		break;
	case  PM_CO2_NET:
		strProductName = "CO2 Net";
		break;
	case  PM_CO2_RS485:
		strProductName = "CO2";
		break;
	case PM_TSTAT6_HUM_Chamber:
		strProductName =g_strHumChamber;
		break;

	case PM_T3PT10 :
		strProductName="T3-PT10";
		break;
	case PM_T3IOA :
		strProductName="T3-8IOA";
		break;
	case PM_T332AI :
		strProductName="T3-32AI";
		break;
	case PM_T3AI16O :
		strProductName="T3-8AI160";
		break;
	case PM_T38I13O :
		strProductName="T3-8I13O";
		break;
	case PM_T3PERFORMANCE :
		strProductName="T3-Performance";
		break;
	case PM_T34AO :
		strProductName="T3-4AO";
		break;
	case PM_T36CT :
		strProductName="T3-6CT";
		break;
	case PM_MINIPANEL:
		strProductName="MiniPanel";
		break;
	case PM_PRESSURE:
		strProductName="Pressure Sensor";
		break;
	default:
		strProductName="TStat";
		break;
	}
	return strProductName;
}


CString Get_Table_Name(int SerialNo,CString Type ,int Row){
	CADO ado;
	CString Table_Name;
	ado.OnInitADOConn();
	if (ado.IsHaveTable(ado,_T("IONAME_CONFIG")))//有Version表
	{
		CString sql;
		sql.Format(_T("Select * from IONAME_CONFIG where Type='%s' and  Row=%d and SerialNo=%d"),Type.GetBuffer(),Row,SerialNo);
		ado.m_pRecordset=ado.OpenRecordset(sql);
		if (!ado.m_pRecordset->EndOfFile)//有表但是没有对应序列号的值
		{
			ado.m_pRecordset->MoveFirst();
			while (!ado.m_pRecordset->EndOfFile)
			{
				Table_Name=ado.m_pRecordset->GetCollect(_T("InOutName"));
				ado.m_pRecordset->MoveNext();
			}
		}
		else
		{
		  Table_Name.Format(_T("%s%d"),Type.GetBuffer(),Row);
		}
	}
	else
	{
	    Table_Name.Format(_T("%s%d"),Type.GetBuffer(),Row);
	}
	ado.CloseRecordset();
	ado.CloseConn();
	return Table_Name;
}
void    Insert_Update_Table_Name(int SerialNo,CString Type,int Row,CString TableName){
	CADO ado;
	ado.OnInitADOConn();
	CString sql;
	sql.Format(_T("Select * from IONAME_CONFIG where Type='%s' and  Row=%d and SerialNo=%d"),Type.GetBuffer(),Row,SerialNo);
	ado.m_pRecordset=ado.OpenRecordset(sql);

	if (!ado.m_pRecordset->EndOfFile)//有表但是没有对应序列号的值
	{

		sql.Format(_T("update IONAME_CONFIG set InOutName = '%s' where Type='%s' and  Row=%d and SerialNo=%d "),TableName.GetBuffer(),Type.GetBuffer(),Row,SerialNo);
		ado.m_pConnection->Execute(sql.GetString(),NULL,adCmdText);
	}
	else
	{
			ado.CloseRecordset();
		sql.Format(_T("Insert into IONAME_CONFIG(InOutName,Type,Row,SerialNo) values('%s','%s','%d','%d')"),TableName.GetBuffer(),Type.GetBuffer(),Row,SerialNo);
		ado.m_pConnection->Execute(sql.GetString(),NULL,adCmdText);
	}

	ado.CloseConn();
}

int Get_Unit_Process(CString Unit)
{
	int ret_Value=1;
	if (Unit.CompareNoCase(_T("RAW DATA"))==0)
	{
		ret_Value=1;
	}
	else if (Unit.CompareNoCase(_T("TYPE2 10K C"))==0)
	{ret_Value=10;
	}
	else if (Unit.CompareNoCase(_T("TYPE2 10K F"))==0)
	{
		ret_Value=10;
	}
	else if (Unit.CompareNoCase(_T("0-100%"))==0)
	{
		ret_Value=1;
	}
	else if (Unit.CompareNoCase(_T("ON/OFF"))==0)
	{
		ret_Value=1;
	}

	else if (Unit.CompareNoCase(_T("OFF/ON"))==0)
	{
		ret_Value=1;
	}
	else if (Unit.CompareNoCase(_T("Pulse Input"))==0)
	{
		ret_Value=1;
	}
	else if (Unit.CompareNoCase(_T("Lighting Control"))==0)
	{
		ret_Value=1;
	}
	else if (Unit.CompareNoCase(_T("TYPE3 10K C"))==0)
	{
		ret_Value=10;
	}
	else if (Unit.CompareNoCase(_T("TYPE3 10K F"))==0)
	{
		ret_Value=10;
	}
	else if (Unit.CompareNoCase(_T("NO USE"))==0)
	{
		ret_Value=1;
	}
	else if (Unit.CompareNoCase(_T("0-5V"))==0)
	{
		ret_Value=1000;
	}
	else if (Unit.CompareNoCase(_T("0-10V"))==0)
	{
		ret_Value=1000;
	}
	else if (Unit.CompareNoCase(_T("0-20ma"))==0)
	{
		ret_Value=1000;
	}



	return ret_Value;
}


BOOL Get_Bit_FromRegister(unsigned short RegisterValue,unsigned short Position){
 
int postionvalue=1;
  postionvalue=postionvalue<<(Position-1);
  postionvalue= RegisterValue&postionvalue;
  BOOL ret=postionvalue>>(Position-1);
  return ret;
}



char * intervaltotext(char *textbuf, long seconds , unsigned minutes , unsigned hours, char *c)
{
	char buf[12], *textbuffer;
	char *separator = c ;
	textbuffer = buf;
	if( seconds < 0 )
	{
		seconds = -seconds;
		strcpy(textbuffer++, "-" ) ;        /* add the '-' */
	}
	if(*c!='-')
	{
		hours += seconds/3600;
		minutes += (unsigned)(seconds%3600)/60;
		seconds = (unsigned)(seconds%3600)%60;
	}
	if( hours < 10 ) {
		strcpy(textbuffer++, "0" ) ;        /* add the leading zero 0#:##:## */
	}
	itoa(hours,textbuffer,10) ;
	textbuffer += strlen(textbuffer);
	strcpy(textbuffer++, separator ) ;        /* add the ":" separator*/

	if( minutes < 10 ) {
		strcpy(textbuffer++, "0" ) ;        /* add the leading zero ##:0#:## */
	}
	itoa(minutes,textbuffer,10) ;
	textbuffer += strlen(textbuffer);
	//strcpy(textbuffer++, separator ) ;        /* add the ":" separator*/
	//if( seconds < 10 ) {
	//	strcpy(textbuffer++, "0" ) ;        /* add the leading zero ##:##:0# */
	//}
	//itoa(seconds,textbuffer,10)  ;

	if(textbuf) strcpy(textbuf, buf);
	//return( buf ) ;
	return NULL;
}

char * intervaltotextfull(char *textbuf, long seconds , unsigned minutes , unsigned hours, char *c)
{
	char buf[12], *textbuffer;
	char *separator = c ;
	textbuffer = buf;
	if( seconds < 0 )
	{
		seconds = -seconds;
		strcpy(textbuffer++, "-" ) ;        /* add the '-' */
	}
	if(*c!='-')
	{
		hours += seconds/3600;
		minutes += (unsigned)(seconds%3600)/60;
		seconds = (unsigned)(seconds%3600)%60;
	}
	if( hours < 10 ) {
		strcpy(textbuffer++, "0" ) ;        /* add the leading zero 0#:##:## */
	}
	itoa(hours,textbuffer,10) ;
	textbuffer += strlen(textbuffer);
	strcpy(textbuffer++, separator ) ;        /* add the ":" separator*/

	if( minutes < 10 ) {
		strcpy(textbuffer++, "0" ) ;        /* add the leading zero ##:0#:## */
	}
	itoa(minutes,textbuffer,10) ;
	textbuffer += strlen(textbuffer);
	strcpy(textbuffer++, separator ) ;        /* add the ":" separator*/
	if( seconds < 10 ) {
		strcpy(textbuffer++, "0" ) ;        /* add the leading zero ##:##:0# */
	}
	itoa(seconds,textbuffer,10)  ;

	if(textbuf) strcpy(textbuf, buf);
	return( buf ) ;
}

void LocalIAmHandler(	uint8_t * service_request,	uint16_t service_len,	BACNET_ADDRESS * src)
{

	int len = 0;
	uint32_t device_id = 0;
	unsigned max_apdu = 0;
	int segmentation = 0;
	uint16_t vendor_id = 0;

	(void) src;
	(void) service_len;
	len =  iam_decode_service_request(service_request, &device_id, &max_apdu,
		&segmentation, &vendor_id);



#if 0
	fprintf(stderr, "Received I-Am Request");
	if (len != -1) 
	{
		fprintf(stderr, " from %u!\n", device_id);
		address_add(device_id, max_apdu, src);
	} else
		fprintf(stderr, "!\n");
#endif
	address_add(device_id, max_apdu, src);



	//g_bac_instance =device_id;
#if 1
	if(src->mac_len==6)
		bac_cs_mac.Format(_T("%d"),src->mac[3]);
	else if(src->mac_len==1)
		bac_cs_mac.Format(_T("%d"),src->mac[0]);
	else
		return;
#endif
//	bac_cs_mac.Format(_T("%d"),vendor_id);

	bac_cs_device_id.Format(_T("%d"),device_id);


//	bac_cs_vendor_id.Format(_T("%d"),vendor_id);
	//g_bac_instance =device_id;
	//g_mac = _wtoi(bac_cs_mac);

	TRACE(_T("Find ") + bac_cs_device_id +_T("  ") + bac_cs_mac + _T("\r\n"));

	g_Print = _T("Globle Who is Find ") + bac_cs_device_id +_T("  ") + bac_cs_mac;
	DFTrace(g_Print);
	_Bac_Scan_Com_Info temp_1;
	temp_1.device_id = device_id;
//	temp_1.vendor_id = vendor_id;
	temp_1.macaddress = _wtoi(bac_cs_mac);

	int find_exsit = false;
	for (int i=0;i<(int)m_bac_scan_com_data.size();i++)
	{
		if((m_bac_scan_com_data.at(i).device_id == temp_1.device_id)
			&& (m_bac_scan_com_data.at(i).macaddress == temp_1.macaddress))
		{
			find_exsit = true;
		}
	}

	if(!find_exsit)
	{
		m_bac_scan_com_data.push_back(temp_1);
	}

	::PostMessage(BacNet_hwd,WM_FRESH_CM_LIST,WM_COMMAND_WHO_IS,NULL);
	return;

}

SOCKET my_sokect;
extern void  init_info_table( void );
extern void Init_table_bank();
void Initial_bac(int comport)
{

	BACNET_ADDRESS src = {
		0
	};  /* address where message came from */
	uint16_t pdu_len = 0;
	unsigned timeout = 100;     /* milliseconds */
	BACNET_ADDRESS my_address, broadcast_address;
	char my_port[50];
	
	bac_program_pool_size = 26624;
	bac_program_size = 0;
	bac_free_memory = 26624;
	//Device_Set_Object_Instance_Number(4194300);
	srand((unsigned)time(NULL)); 
	unsigned int temp_value;
	temp_value = rand()%(0x3FFFFF);
	g_Print.Format(_T("The initial T3000 Object Instance value is %d"),temp_value);
	DFTrace(g_Print);
	Device_Set_Object_Instance_Number(temp_value);
	address_init();
	Init_Service_Handlers();
	


#ifdef OPEN_PTP
	CString temp_cs11;
	//temp_cs.Format(_T("COM%d"),g_com);
	temp_cs11.Format(_T("COM%d"),5);
	char cTemp11[255];
	memset(cTemp11,0,255);
	WideCharToMultiByte( CP_ACP, 0, temp_cs11.GetBuffer(), -1, cTemp11, 255, NULL, NULL );
	temp_cs11.ReleaseBuffer();
	sprintf(my_port,cTemp11);


	dl_ptp_init(my_port);
#endif

#ifndef test_ptp

#if 0
	if(comport == 0)	//如果是 0 的话就要初始化网络的socket,否则 就是串口的mstp 或者ptp
	{
#endif	
		int ret_1 = Open_bacnetSocket2(_T("192.168.0.130"),BACNETIP_PORT,my_sokect);
		//	Open_Socket2(_T("127.0.0.1"),6002);
		//	 = (int)GetCommunicationHandle();
		bip_set_socket(my_sokect);
		bip_set_port(49338);
		//	int test_port = bip_get_port();
#if 1
		static in_addr BIP_Broadcast_Address;
		BIP_Broadcast_Address.S_un.S_addr =  inet_addr("255.255.255.255");
		//BIP_Broadcast_Address.S_un.S_addr =  inet_addr("192.168.0.177");
		bip_set_broadcast_addr((uint32_t)BIP_Broadcast_Address.S_un.S_addr);
#endif
		PHOSTENT  hostinfo;  
		char  name[255]; 
		CString  cs_myip; 
		if(  gethostname  (  name,  sizeof(name))  ==  0)  
		{  
			if((hostinfo  =  gethostbyname(name))  !=  NULL)  
			{  
				cs_myip  =  inet_ntoa  (*(struct  in_addr  *)*hostinfo->h_addr_list);  
			}  
		}  
		char cTemp1[255];
		memset(cTemp1,0,255);
		WideCharToMultiByte( CP_ACP, 0, cs_myip.GetBuffer(), -1, cTemp1, 255, NULL, NULL );

#if 1
		static in_addr BIP_Address;
		BIP_Address.S_un.S_addr =  inet_addr(cTemp1);
		bip_set_addr((uint32_t)BIP_Address.S_un.S_addr);
#endif	

		set_datalink_protocol(PROTOCOL_BACNET_IP);
		datalink_get_broadcast_address(&broadcast_address);
		//    print_address("Broadcast", &broadcast_address);
		datalink_get_my_address(&my_address);
		//		print_address("Address", &my_address);
		//int * comport_parameter = new int;
		//*comport_parameter = PROTOCOL_BACNET_IP;
		CM5_hThread =CreateThread(NULL,NULL,MSTP_Receive,NULL,NULL, &nThreadID_x);

#if 0
	}

	else
	{
		HANDLE temphandle;
		temphandle = Get_RS485_Handle();
		if(temphandle !=NULL)
		{
			TerminateThread((HANDLE)Get_Thread1(),0);
			TerminateThread((HANDLE)Get_Thread2(),0);

			CloseHandle(temphandle);
			Set_RS485_Handle(NULL);
		}

		Sleep(10000);

				dlmstp_set_baud_rate(19200);
		dlmstp_set_mac_address(0);
		dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
		dlmstp_set_max_master(DEFAULT_MAX_MASTER);
		memset(my_port,0,50);

		//CString Program_Path,Program_ConfigFile_Path;
		//int g_com=0;
		//GetModuleFileName(NULL,Program_Path.GetBuffer(MAX_PATH),MAX_PATH);  
		//PathRemoveFileSpec(Program_Path.GetBuffer(MAX_PATH) );           
		//Program_Path.ReleaseBuffer();
		//Program_ConfigFile_Path = Program_Path + _T("\\MyConfig.ini");

		/*CFileFind fFind;
		if(!fFind.FindFile(Program_ConfigFile_Path))
		{
			WritePrivateProfileStringW(_T("Setting"),_T("ComPort"),_T("1"),Program_ConfigFile_Path);
		}
		g_com = GetPrivateProfileInt(_T("Setting"),_T("ComPort"),1,Program_ConfigFile_Path);*/
		CString temp_cs;
		//temp_cs.Format(_T("COM%d"),g_com);
		temp_cs.Format(_T("COM%d"),comport);
		char cTemp1[255];
		memset(cTemp1,0,255);
		WideCharToMultiByte( CP_ACP, 0, temp_cs.GetBuffer(), -1, cTemp1, 255, NULL, NULL );
		temp_cs.ReleaseBuffer();


		sprintf(my_port,cTemp1);
		dlmstp_init(my_port);

		set_datalink_protocol(MODBUS_BACNET_MSTP);
		datalink_get_broadcast_address(&broadcast_address);
		//    print_address("Broadcast", &broadcast_address);
		datalink_get_my_address(&my_address);
		//		print_address("Address", &my_address);
		int * comport_parameter = new int;
		*comport_parameter = MODBUS_BACNET_MSTP;
		CM5_hThread =CreateThread(NULL,NULL,MSTP_Receive,comport_parameter,NULL, &nThreadID_x);
	}
#endif

#endif //nodef ptp

	if(!bac_net_initial_once)
	{
		bac_net_initial_once =true;
		timesec1970 = (unsigned long)time(NULL);
		timestart = 0;
		init_info_table();
		Init_table_bank();
	}
}
//#include "datalink.h"
DWORD WINAPI   MSTP_Receive(LPVOID lpVoid)
{
	BACNET_ADDRESS src = {0};
	uint16_t pdu_len;
	//int *mparent = (int *)lpVoid;
	//int protocol_new = *mparent;

	uint8_t Rx_Buf[MAX_MPDU] = { 0 };
	//while(mparent->m_MSTP_THREAD)
	  g_mstp_flag=true;
	while(g_mstp_flag)
	{
		//datalink_set("bip");
		
		pdu_len = datalink_receive(&src,&Rx_Buf[0],MAX_MPDU,INFINITE);
	//	pdu_len =  bip_receive(&src,&Rx_Buf[0],MAX_MPDU, INFINITE);
		//pdu_len = dlmstp_receive(&src, &Rx_Buf[0], MAX_MPDU, INFINITE);
		if(pdu_len==0)
			continue;
		npdu_handler(&src, &Rx_Buf[0], pdu_len);
		//CString TEMP1;
		//TEMP1.Format("%s",Rx_Buf);
		//	AfxMessageBox(TEMP1);
	}
	return 0;
}

void Init_Service_Handlers(	void)
{
	Device_Init(NULL);

	/* we need to handle who-is to support dynamic device binding */
	apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
	apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, LocalIAmHandler);



	apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_PRIVATE_TRANSFER,local_handler_conf_private_trans_ack);
	//apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,Read_Property_feed_back);

	apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,	Localhandler_read_property_ack);
	/* set the handler for all the services we don't implement */
	/* It is required to send the proper reject message... */
	apdu_set_unrecognized_service_handler_handler
		(handler_unrecognized_service);
	/* we must implement read property - it's required! */
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
		handler_read_property);
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
		handler_read_property_multiple);
	/* handle the data coming back from confirmed requests */
	//   apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,handler_read_property_ack);
#if defined(BACFILE)
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_READ_FILE,
		handler_atomic_read_file);
#endif
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV,
		handler_cov_subscribe);

	////#if 0
	////	/* Adding these handlers require the project(s) to change. */
	////#if defined(BACFILE)
	////	apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE,
	////		handler_atomic_write_file);
	////#endif
	////	apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_RANGE,
	////		handler_read_range);
	////	apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
	////		handler_reinitialize_device);
	////	apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
	////		handler_timesync_utc);
	////	apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
	////		handler_timesync);
	////	apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_COV_NOTIFICATION,
	////		handler_ucov_notification);
	////	/* handle communication so we can shutup when asked */
	////	apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
	////		handler_device_communication_control);
	////#endif
}

#define  PRINT_ENABLED 1
void local_rp_ack_print_data(	BACNET_READ_PROPERTY_DATA * data)
{
	//BACNET_OBJECT_PROPERTY_VALUE object_value;  /* for bacapp printing */
	BACNET_APPLICATION_DATA_VALUE value;        /* for decode value data */

	int len = 0;
	uint8_t *application_data;
	int application_data_len;
	bool first_value = true;
	bool print_brace = false;



	if (data) 
	{
		application_data = data->application_data;
		application_data_len = data->application_data_len;
		/* FIXME: what if application_data_len is bigger than 255? */
		/* value? need to loop until all of the len is gone... */
		for (;;) 
		{
			//BACnet_Object_Property_Value_Own object_value;  /* for bacapp printing */
			BACNET_APPLICATION_DATA_VALUE value;        /* for decode value data */
			len = bacapp_decode_application_data(application_data,(uint8_t) application_data_len, &value);
			if (first_value && (len < application_data_len)) 
			{
				first_value = false;
#if PRINT_ENABLED
				fprintf(stdout, "{");
#endif
				print_brace = true;
			}
			receive_object_value.object_type = data->object_type;
			receive_object_value.object_instance = data->object_instance;
			receive_object_value.object_property = data->object_property;
			receive_object_value.array_index = data->array_index;
			receive_object_value.value = &value;

			if (len > 0) 
			{
				if (len < application_data_len) 
				{
					application_data += len;
					application_data_len -= len;
					/* there's more! */

#if PRINT_ENABLED
					fprintf(stdout, ",");
#endif
				} 
				else 
				{
					break;
				}
			} 
			else 
			{
				break;
			}
		}
#if PRINT_ENABLED
		if (print_brace)
			fprintf(stdout, "}");
		fprintf(stdout, "\r\n");
#endif
	}
}
void Localhandler_read_property_ack(
	uint8_t * service_request,
	uint16_t service_len,
	BACNET_ADDRESS * src,
	BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
	int len = 0;
	BACNET_READ_PROPERTY_DATA data;

	(void) src;
	(void) service_data;        /* we could use these... */
	len = rp_ack_decode_service_request(service_request, service_len, &data);
	//char my_pro_name[100];
	//char * temp = get_prop_name();
	//strcpy_s(my_pro_name,100,temp);
	
#if 0
	fprintf(stderr, "Received Read-Property Ack!\n");
#endif
	if (len > 0)
	{
		local_rp_ack_print_data(&data);
	//	::PostMessage(BacNet_hwd,WM_FRESH_CM_LIST,WM_COMMAND_WHO_IS,NULL);
	}
}


//This function add by Fance Du, used for changed the CString to hex
//2013 12 02
//Ex: "0F" -> 15
unsigned char Str_to_Byte(CString need_conver)
{
	int the_first=0;
	int the_second=0;
	switch (need_conver.GetAt(0))
	{
	case 0x30:	the_first=0;		break;
	case 0x31:	the_first=1;		break;
	case 0x32:	the_first=2;		break;
	case 0x33:	the_first=3;		break;
	case 0x34:	the_first=4;		break;
	case 0x35:	the_first=5;		break;
	case 0x36:	the_first=6;		break;
	case 0x37:  the_first=7;		break;
	case 0x38:  the_first=8;		break;
	case 0x39:  the_first=9;		break;
	case 0x41:  the_first=10;		break;
	case 0x42:  the_first=11;		break;
	case 0x43:  the_first=12;		break;
	case 0x44:  the_first=13;		break;
	case 0x45:  the_first=14;		break;
	case 0x46:  the_first=15;		break;
	}
	switch (need_conver.GetAt(1))
	{
	case 0x30:	the_second=0;		break;
	case 0x31:	the_second=1;		break;
	case 0x32:	the_second=2;		break;
	case 0x33:	the_second=3;		break;
	case 0x34:	the_second=4;		break;
	case 0x35:	the_second=5;		break;
	case 0x36:	the_second=6;		break;
	case 0x37:  the_second=7;		break;
	case 0x38:  the_second=8;		break;
	case 0x39:  the_second=9;		break;
	case 0x41:  the_second=10;		break;
	case 0x42:  the_second=11;		break;
	case 0x43:  the_second=12;		break;
	case 0x44:  the_second=13;		break;
	case 0x45:  the_second=14;		break;
	case 0x46:  the_second=15;		break;
	}
	return (the_first*16+the_second);
}


extern char local_network_ip[255];
extern CString local_enthernet_ip;
//socket dll.
bool Open_bacnetSocket2(CString strIPAdress,short nPort,SOCKET &mysocket)
{

	int nNetTimeout=3000;//1 second.
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);

	//if (m_hSocket!=INVALID_SOCKET)
	//{
	//	::closesocket(m_hSocket);
	//	m_hSocket=NULL;
	//}

	if(::WSAStartup(sockVersion, &wsaData) != 0)
	{
		//AfxMessageBox(_T("Init Socket failed!"));
	//	m_hSocket=NULL;
		return FALSE;
	}

//	mysocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	mysocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(mysocket == INVALID_SOCKET)
	{
		//	AfxMessageBox(_T("Create socket failed!"));
		mysocket=NULL;
		return FALSE;
	}
	sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(nPort);
	if(local_enthernet_ip.IsEmpty())
		servAddr.sin_addr.s_addr = INADDR_ANY; 
	else
		servAddr.sin_addr.s_addr =  inet_addr(local_network_ip);

	USES_CONVERSION;   


	int bind_ret =	bind(mysocket, (struct sockaddr*)&servAddr, sizeof(servAddr));
	//if(bind_ret<0)
	//{
	//	//AfxMessageBox(_T("Locol port 47808 is not valiable"));

	//}


	//char pTemp[20];
	//pTemp=W2A(strIPAdress);     


	//servAddr.sin_addr.S_un.S_addr =inet_addr("192.168.0.28");
	//	servAddr.sin_addr.S_un.S_addr =inet_addr((LPSTR)(LPCTSTR)strIPAdress);
	servAddr.sin_addr.S_un.S_addr = (inet_addr(W2A(strIPAdress)));
	//	u_long ul=1;
	//	ioctlsocket(m_hSocket,FIONBIO,(u_long*)&ul);

	setsockopt(mysocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&nNetTimeout,sizeof(int));

	setsockopt(mysocket,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int));


	BOOL bBroadcast=TRUE;
	setsockopt(mysocket,SOL_SOCKET,SO_BROADCAST,(char*)&bBroadcast,sizeof(BOOL));


	//char ABC[10];
	//ABC[0]=0X11;
	//ABC[1]=0X22;
	//sendto(mysocket,ABC,2,NULL,(struct sockaddr *) &servAddr,sizeof(sockaddr));
	//if(::connect(mysocket,(sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
	//{
	//	DWORD dwErr = WSAGetLastError();
	//	//AfxMessageBox(_T(" Failed connect() \n"));
	//	::closesocket(mysocket);
	//	mysocket=NULL;
	//	return FALSE;
	//}
	return TRUE;
}

#define MAX_STRING	100	// should be enough for everone (-:

//********************************************************
//*	FUNCTION: CheckForUpdate
//*
//*	DESCRIPTION:
//*		Connects to the specified Ftp server, and looks
//*		for the specified file (szFtpFilename) and reads
//*		just one string from it. Compares the string with
//*		the szCurrentVersion and if there is a difference,
//*		assumes there is a new app version.
//*
//* PARAMS:
//*		szFtpServer:	FTP server to access
//*		szFtpUsername:	FTP account name
//*		szFtpPassword:	appropriate password
//*		szFtpFilename:	FTP file which holds the
//*						version info
//*		szCurrentVersion:	version of the app calling
//*							this function
//*		szLastVersion:	version retrieved from FTP
//*						valid only if no error occurs
//*
//*	ASSUMES:
//*		Existance of a valid internet connection.
//*		AfxSocketInit() has already been called.
//*		Brains (optional).
//*
//*	RETURNS:
//*		TRUE only if new version is found
//*		FALSE if there was an error OR no new version
//*
//*	AUTHOR: T1TAN <t1tan@cmar-net.org>
//*
//*	COPYRIGHT:	Copyleft (C) T1TAN 2004 - 3827
//*				Copyleft (C) SprdSoft Inc. 2004 - 3827
//*				FREE for (ab)use in any form.
//*				(Leave the headers be)
//*
//*	VERSIONS:
//*		VERSION	AUTHOR	DATE		NOTES
//*		-------	------	----------	------------------
//*		1.0		T1TAN	07/05/2004	initial version
//********************************************************

BOOL CheckForUpdate(
		LPCTSTR szFtpServer,
		LPCTSTR szFtpUsername,
		LPCTSTR szFtpPassword,
		LPCTSTR szFtpFilename,
		LPCTSTR szCurrentVersion,
		LPTSTR szLastVersion )
{
	CWaitCursor wait;
	// zero the last anyway..
	ZeroMemory( szLastVersion, sizeof(szLastVersion) );
	// get a session
	CInternetSession* pFtpSession = new CInternetSession();
	CFtpConnection* pFtpConnection = NULL;

	if ( pFtpSession == NULL )
	{	// DAMN!
		MessageBox( GetDesktopWindow(),
			_T("Could not get internet session."),
			_T("Error"), MB_OK|MB_ICONSTOP );
		return FALSE;
	}

	try {
		pFtpConnection = pFtpSession->GetFtpConnection
			( szFtpServer, szFtpUsername, szFtpPassword,21);
	}
	catch ( CInternetException *err )
	{	// no luck today...
		err->ReportError( MB_OK|MB_ICONSTOP );
		err->Delete();
	}

	if ( pFtpConnection == NULL )
	{	// DAMN AGAIN!!
		// cleanup
		pFtpSession->Close();
		delete pFtpSession;
		return FALSE;
	}

	CFtpFileFind ffind( pFtpConnection );

	BOOL isFound = ffind.FindFile( szFtpFilename );

	if ( isFound == FALSE )
	{	// CRAP!! WHERE IS OUR FILE?!?!
		ffind.Close();
		pFtpConnection->Close();
		pFtpSession->Close();
		delete pFtpConnection;
		delete pFtpSession;
		MessageBox( GetDesktopWindow(),
			_T("Could not get version information."),
			_T("Error"), MB_OK|MB_ICONSTOP );
		return FALSE;
	}
	CString versionfilename=g_strExePth+_T("version.txt");

	BOOL bResult = pFtpConnection->GetFile
		( szFtpFilename, versionfilename, FALSE );

	if ( bResult == 0 )
	{	// DAMN ERRORS
		ffind.Close();
		pFtpConnection->Close();
		pFtpSession->Close();
		delete pFtpConnection;
		delete pFtpSession;
		MessageBox( GetDesktopWindow(),
			_T("Could not get version information."),
			_T("Error"), MB_OK|MB_ICONSTOP );
		return FALSE;
	}

	CStdioFile verFile;
	//CFile verFile;
	CFileException error;

	bResult = verFile.Open( versionfilename,
		CFile::modeRead, &error );

	if ( bResult == 0 )
	{	// WHATTA HECK?!?
		ffind.Close();
		pFtpConnection->Close();
		pFtpSession->Close();
		delete pFtpConnection;
		delete pFtpSession;
		MessageBox( GetDesktopWindow(),
			_T("Error opening local file."),
			_T("Error"), MB_OK|MB_ICONSTOP );
		// just in case...
		DeleteFile(versionfilename);
		return FALSE;
	}
	//verFile.SeekToBegin();
	TCHAR buffer[MAX_STRING];
	ZeroMemory( buffer, sizeof(buffer) );
	//verFile.Read( buffer, MAX_STRING );
	CString The_CurentVersion;
	verFile.ReadString(The_CurentVersion);
	//buffer=The_CurentVersion.GetBuffer()
	//if ( _tcscmp( buffer, szCurrentVersion ) != 0 )
	//{	// new version available!
	//	_tcscpy( szLastVersion, buffer );
	//	// cleanup..
	//	// (i am sometimes impressed with comments
	//	// like this one.. "cleanup.." OH REALLY,
	//	// and i thought it's an airplane!!)
	//	verFile.Close();
	//	ffind.Close();
	//	pFtpConnection->Close();
	//	pFtpSession->Close();
	//	delete pFtpConnection;
	//	delete pFtpSession;

	//	//DeleteFile( versionfilename );
	//	// ok..
	//	return TRUE;
	//}

	// obviously nothing new here...
	// copy the current version to last version
	// so that the caller knows no error occured
	_tcscpy( szLastVersion, The_CurentVersion.GetBuffer() );
	// cleanup.. (again...)
	verFile.Close();
	ffind.Close();
	pFtpConnection->Close();
	pFtpSession->Close();
	delete pFtpConnection;
	delete pFtpSession;
	//DeleteFile( versionfilename );
	return TRUE;
}




int AddNetDeviceForRefreshList(BYTE* buffer, int nBufLen,  sockaddr_in& siBind)
{
	int nLen=buffer[2]+buffer[3]*256;
	//int n =sizeof(char)+sizeof(unsigned char)+sizeof( unsigned short)*9;
	unsigned short usDataPackage[16]={0};
	if(nLen>=0)
	{
		refresh_net_device temp;
		memcpy(usDataPackage,buffer+4,nLen*sizeof(unsigned short));

		DWORD nSerial=usDataPackage[0]+usDataPackage[1]*256+usDataPackage[2]*256*256+usDataPackage[3]*256*256*256;
		int nproduct_id = usDataPackage[4];
		CString nproduct_name = GetProductName(nproduct_id);
		if(nproduct_name.CompareNoCase(_T("TStat")) == 0)	//如果产品号 没定义过，不认识这个产品 就exit;
			return m_refresh_net_device_data.size();
		
		CString nip_address;
		nip_address.Format(_T("%d.%d.%d.%d"),usDataPackage[6],usDataPackage[7],usDataPackage[8],usDataPackage[9]);

		int nport = usDataPackage[10];
		int nsw_version = usDataPackage[11];
		int nhw_version = usDataPackage[12];
			
		int modbusID=usDataPackage[5];
		//TRACE(_T("Serial = %u     ID = %d\r\n"),nSerial,modbusID);
		g_Print.Format(_T("Refresh list :Serial = %u     ID = %d ,ip = %s  , Product name : %s"),nSerial,modbusID,nip_address ,nproduct_name);
		DFTrace(g_Print);
		temp.nport = nport;
		temp.sw_version = nsw_version;
		temp.hw_version = nhw_version;
		temp.ip_address = nip_address;
		temp.product_id = nproduct_id;
		temp.modbusID = modbusID;
		temp.nSerial = nSerial;

		bool find_exsit = false;

		for (int i=0;i<(int)m_refresh_net_device_data.size();i++)
		{
			if(m_refresh_net_device_data.at(i).nSerial == nSerial)
			{
				find_exsit = true;
				break;
			}
		}

		if(!find_exsit)
		{
			m_refresh_net_device_data.push_back(temp);
		}
	}


	return m_refresh_net_device_data.size();
}

int GetHostAdaptersInfo(CString &IP_address_local)
{
	CString szAdaptersInfo;
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	/* variables used to print DHCP time info */

	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen); 
		if (pAdapterInfo == NULL) 
		{
			return -1;
		}
	}
	int i_CntAdapters = 0;
	CString szTmp;
	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR) 
	{
		pAdapter = pAdapterInfo;
		while (pAdapter) 
		{            
			szTmp.Format(_T("adapter name: %s "),pAdapter->AdapterName);
			szAdaptersInfo += szTmp;
			szTmp.Format(_T("adapter description: %s "),pAdapter->Description);
			szAdaptersInfo += szTmp;
			szTmp.Format(_T("adapter address: "));
			szAdaptersInfo += szTmp;
			for (UINT i = 0; i < pAdapter->AddressLength; i++) 
			{
				if (i == (pAdapter->AddressLength - 1))
				{
					szTmp.Format(_T("%.2X "),(int)pAdapter->Address[i]);
					szAdaptersInfo += szTmp;
				}
				else                        
				{
					szTmp.Format(_T("%.2X-"),(int)pAdapter->Address[i]);
					szAdaptersInfo += szTmp;
				}                            
			}                    
			szTmp.Format(_T("ip address: %s "),pAdapter->IpAddressList.IpAddress.String);
			szAdaptersInfo += szTmp;
			szTmp.Format(_T("ip mask: %s "),pAdapter->IpAddressList.IpMask.String);
			szAdaptersInfo += szTmp;
			szTmp.Format(_T("gateway: %s "),pAdapter->GatewayList.IpAddress.String);
			szAdaptersInfo += szTmp;

			szTmp.Format(_T("type: %d "),pAdapter->Type);
			szAdaptersInfo += szTmp;
			if(pAdapter->Type == 6)
			{
				IP_address_local.Empty();
				MultiByteToWideChar( CP_ACP,0,(char *)pAdapter->IpAddressList.IpAddress.String, (int)strlen((char *)pAdapter->IpAddressList.IpAddress.String)+1, 
					IP_address_local.GetBuffer(MAX_PATH), MAX_PATH );
				IP_address_local.ReleaseBuffer();
				break;

				//IP_address_local.fo
			}

			szTmp.Format(_T("index: %d "),pAdapter->Index);
			szAdaptersInfo += szTmp;

			pAdapter = pAdapter->Next;
			i_CntAdapters++;
		}
	}
	else 
	{ 
		if (pAdapterInfo)
			free(pAdapterInfo);
		return -1;
	}
	szTmp.ReleaseBuffer();
	return i_CntAdapters;
}

//UINT RefreshNetWorkDeviceListByUDPFunc(LPVOID pParam)
UINT RefreshNetWorkDeviceListByUDPFunc()
{
	
	int nRet = 0;
	//if (nRet == SOCKET_ERROR)
	//{	  
	//	/*goto END_SCAN;
	//	return 0;*/
	//}
	short nmsgType=UPD_BROADCAST_QRY_MSG;

	//////////////////////////////////////////////////////////////////////////
	const DWORD END_FLAG = 0x00000000;
	TIMEVAL time;
	time.tv_sec =3;
	time.tv_usec = 1000;

	fd_set fdSocket;
	BYTE buffer[512] = {0};

	BYTE pSendBuf[1024];
	ZeroMemory(pSendBuf, 255);
	pSendBuf[0] = 100;
	memcpy(pSendBuf + 1, (BYTE*)&END_FLAG, 4);
	int nSendLen = 5;

	int time_out=0;
	BOOL bTimeOut = FALSE;
	while(!bTimeOut)//!pScanner->m_bNetScanFinish)  // 超时结束
	{
		time_out++;
		if(time_out>5)
			bTimeOut = TRUE;

		FD_ZERO(&fdSocket);	
		FD_SET(h_Broad, &fdSocket);

		nRet = ::sendto(h_Broad,(char*)pSendBuf,nSendLen,0,(sockaddr*)&h_bcast,sizeof(h_bcast));
		if (nRet == SOCKET_ERROR)
		{
			int  nError = WSAGetLastError();
			goto END_SCAN;
			return 0;
		}
		int nLen = sizeof(h_siBind);
		fd_set fdRead = fdSocket;
		int nSelRet = ::select(0, &fdRead, NULL, NULL, &time);//TRACE("recv nc info == %d\n", nSelRet);
		if (nSelRet == SOCKET_ERROR)
		{
			int nError = WSAGetLastError();
			goto END_SCAN;
			return 0;
		}

		if(nSelRet > 0)
		{
			ZeroMemory(buffer, 512);
			int nRet ;
			int nSelRet;
			do 
			{
				 nRet = ::recvfrom(h_Broad,(char*)buffer, 512, 0, (sockaddr*)&h_siBind, &nLen);

				 BYTE szIPAddr[4] = {0};
				 if(nRet > 0)
				 {		
					 FD_ZERO(&fdSocket);
					 if(buffer[0]==RESPONSE_MSG)
					 {	
						 nLen=buffer[2]+buffer[3]*256;
						 unsigned short dataPackage[32]={0};
						 memcpy(dataPackage,buffer+2,nLen*sizeof(unsigned short));
						 szIPAddr[0]= (BYTE)dataPackage[7];
						 szIPAddr[1]= (BYTE)dataPackage[8];
						 szIPAddr[2]= (BYTE)dataPackage[9];
						 szIPAddr[3]= (BYTE)dataPackage[10];

						 int n = 1;
						 BOOL bFlag=FALSE;
						 //////////////////////////////////////////////////////////////////////////
						 // 检测IP重复
						 DWORD dwValidIP = 0;
						 memcpy((BYTE*)&dwValidIP, pSendBuf+n, 4);
						 while(dwValidIP != END_FLAG)
						 {	
							 DWORD dwRecvIP=0;
							 memcpy((BYTE*)&dwRecvIP, szIPAddr, 4);
							 memcpy((BYTE*)&dwValidIP, pSendBuf+n, 4);
							 if(dwRecvIP == dwValidIP)
							 {
								 bFlag = TRUE;
								 break;
							 }
							 n+=4;
						 }
						 //////////////////////////////////////////////////////////////////////////
						 if (!bFlag)
						 {
							 AddNetDeviceForRefreshList(buffer, nRet, h_siBind);

							 //pSendBuf[nSendLen-1] = (BYTE)(modbusID);
							 pSendBuf[nSendLen-4] = szIPAddr[0];
							 pSendBuf[nSendLen-3] = szIPAddr[1];
							 pSendBuf[nSendLen-2] = szIPAddr[2];
							 pSendBuf[nSendLen-1] = szIPAddr[3];
							 memcpy(pSendBuf + nSendLen, (BYTE*)&END_FLAG, 4);
							 //////////////////////////////////////////////////////////////////////////

							 //pSendBuf[nSendLen+3] = 0xFF;
							 nSendLen+=4;
						 }
						 else
						 {
							 AddNetDeviceForRefreshList(buffer, nRet, h_siBind);
						 }
					 }	
				 }
				 else
				 {
					 break;
				 }


				 FD_ZERO(&fdSocket);	
				 FD_SET(h_Broad, &fdSocket);
				 nLen = sizeof(h_siBind);
				 fdRead = fdSocket;
				 nSelRet = ::select(0, &fdRead, NULL, NULL, &time);//TRACE("recv nc info == %d\n", nSelRet);
			} while (nSelRet);
			//int nRet = ::recvfrom(h_Broad,(char*)buffer, 512, 0, (sockaddr*)&h_siBind, &nLen);

		}	
		else
		{
			g_ScnnedNum = 0;
			bTimeOut = TRUE;
		}
	}//end of while
END_SCAN:

	closesocket(h_Broad);
	h_Broad=NULL;
	{

		//SOCKET soAck =::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
		h_Broad=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
		BOOL bBroadcast=TRUE;
		::setsockopt(h_Broad,SOL_SOCKET,SO_BROADCAST,(char*)&bBroadcast,sizeof(BOOL));
		int iMode=1;
		ioctlsocket(h_Broad,FIONBIO, (u_long FAR*) &iMode);

		BOOL bDontLinger = FALSE;
		setsockopt( h_Broad, SOL_SOCKET, SO_DONTLINGER, ( const char* )&bDontLinger, sizeof( BOOL ) );

#if 1
		//SOCKADDR_IN bcast;
		h_bcast.sin_family=AF_INET;
		//bcast.sin_addr.s_addr=nBroadCastIP;
		h_bcast.sin_addr.s_addr=INADDR_BROADCAST;
		h_bcast.sin_port=htons(UDP_BROADCAST_PORT);
#endif

		local_enthernet_ip.Empty();
		GetHostAdaptersInfo(local_enthernet_ip);
		if(!local_enthernet_ip.IsEmpty())
		{

			WideCharToMultiByte( CP_ACP, 0, local_enthernet_ip.GetBuffer(), -1, local_network_ip, 255, NULL, NULL );
			h_siBind.sin_family=AF_INET;
			h_siBind.sin_addr.s_addr =  inet_addr(local_network_ip);
			//h_siBind.sin_addr.s_addr=INADDR_ANY;
			h_siBind.sin_port= htons(57619);
			::bind(h_Broad, (sockaddr*)&h_siBind,sizeof(h_siBind));
		}

#if 0
		//SOCKADDR_IN siBind;
		h_siBind.sin_family=AF_INET;
		h_siBind.sin_addr.s_addr=INADDR_ANY;
		h_siBind.sin_port=htons(RECV_RESPONSE_PORT);
		::bind(h_Broad, (sockaddr*)&h_siBind,sizeof(h_siBind));
#endif
	}
	//pScanner->m_bNetScanFinish = TRUE; // 超时结束

	//g_strScanInfoPrompt = _T("NC by TCP");
	//strlog=_T("UDP scan finished,Time: ")+Get_NowTime()+_T("\n");
	//NET_WriteLogFile(strlog);
	//pScanner->m_eScanNCEnd->SetEvent();
	return 1;
}


extern SOCKET my_sokect;
void Send_WhoIs_remote_ip(CString ipaddress)
{
#if 0
	SOCKET h_temp_sokcet=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	BOOL bBroadcast=TRUE;
	::setsockopt(h_temp_sokcet,SOL_SOCKET,SO_BROADCAST,(char*)&bBroadcast,sizeof(BOOL));
	int iMode=1;
	ioctlsocket(h_temp_sokcet,FIONBIO, (u_long FAR*) &iMode);

	BOOL bDontLinger = FALSE;
	setsockopt( h_temp_sokcet, SOL_SOCKET, SO_DONTLINGER, ( const char* )&bDontLinger, sizeof( BOOL ) );
#endif
	//SOCKADDR_IN bcast;.
	char temp_ip_address[20];
	WideCharToMultiByte( CP_ACP, 0, ipaddress.GetBuffer(), -1, temp_ip_address, 255, NULL, NULL );

	SOCKADDR_IN h_bcast_temp;

	UINT temp_ip = inet_addr(temp_ip_address);
	h_bcast_temp.sin_family=AF_INET;
	h_bcast_temp.sin_addr.s_addr=temp_ip;
	//h_bcast.sin_addr.s_addr=INADDR_BROADCAST;
	h_bcast_temp.sin_port=htons(BACNETIP_PORT);


#if 0
	SOCKADDR_IN h_siBind_test;
	h_siBind_test.sin_family=AF_INET;
	h_siBind_test.sin_addr.s_addr=INADDR_ANY;
	h_siBind_test.sin_port=htons(BACNETIP_PORT);
#endif
	//	::bind(h_temp_sokcet, (sockaddr*)&h_siBind,sizeof(h_siBind));

	//	USES_CONVERSION;   


	//int bind_ret =::bind(h_temp_sokcet, (sockaddr*)&h_siBind_test,sizeof(h_siBind_test));
	//if(bind_ret<0)
	//{
	//	AfxMessageBox(_T("Locol port 47808 is not valiable"));

	//}


	const DWORD END_FLAG = 0x00000000;
	TIMEVAL time;
	time.tv_sec =3;
	time.tv_usec = 1000;

	fd_set fdSocket;
	BYTE buffer[512] = {0};
	//	char pSendBuf[255];
	//	ZeroMemory(pSendBuf, 255);
	BYTE pSendBuf[1024] = {0x81, 0x0B, 0x00, 0x0C, 0x01, 0x20, 0xFF, 0xFF, 0x00, 0xFF, 0x10, 0x08};

	//pSendBuf[0] = 100;
	//memcpy(pSendBuf + 1, (BYTE*)&END_FLAG, 4);
	int nSendLen = 12;

	int time_out=0;
	BOOL bTimeOut = FALSE;
	int nRet = 0;
	Sleep(1);
	Initial_bac(0);
	while(!bTimeOut)//!pScanner->m_bNetScanFinish)  // 超时结束
	{
		time_out++;
		if(time_out>5)
			bTimeOut = TRUE;

		//		FD_ZERO(&fdSocket);	
		//		FD_SET(h_temp_sokcet, &fdSocket);
		//my_sokect
		nRet = ::sendto(my_sokect,(char*)pSendBuf,nSendLen,0,(sockaddr*)&h_bcast_temp,sizeof(h_bcast_temp));
		//nRet = ::sendto(h_temp_sokcet,(char*)pSendBuf,nSendLen,0,(sockaddr*)&h_bcast_temp,sizeof(h_bcast_temp));
		if (nRet == SOCKET_ERROR)
		{
			int  nError = WSAGetLastError();
			//goto END_SCAN;
			return ;
		}
		Sleep(200);
		continue;//Test ///////////////////////////////////////////////////////////////////////////////////

	}//end of while

	//closesocket(h_Broad);


}




int Open_MonitorDataBase(CString DataSource)
{
	//m_pCon.CreateInstance(_T("ADODB.Connection"));
	//m_pCon->Open(g_strDatabasefilepath.GetString(),_T(""),_T(""),adModeUnknown);
	//strSql.Format(_T("update ALL_NODE set Hardware_Ver ='%s' where Serial_ID = '%s' and Bautrate = '%s'"),hw_instance,str_serialid,str_baudrate);
	//m_pCon->Execute(strSql.GetString(),NULL,adCmdText);		
	//strSql.Format(_T("update ALL_NODE set Software_Ver ='%s' where Serial_ID = '%s' and Bautrate = '%s'"),sw_mac,str_serialid,str_baudrate);
	//m_pCon->Execute(strSql.GetString(),NULL,adCmdText);		
	CString locol_path;
	CString m_mdb_path_t3000;
	CString m_application_path;
	m_mdb_path_t3000 = _T("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=");
	GetModuleFileName(NULL, m_application_path.GetBuffer(MAX_PATH), MAX_PATH);
	PathRemoveFileSpec(m_application_path.GetBuffer(MAX_PATH));
	m_application_path.ReleaseBuffer();
	m_mdb_path_t3000 = m_mdb_path_t3000 + m_application_path;
	m_mdb_path_t3000 = m_mdb_path_t3000 + DataSource;

	HRESULT hr;
	m_global_pCon.CreateInstance(_T("ADODB.Connection"));
	hr=m_global_pRs.CreateInstance(_T("ADODB.Recordset"));
	if(FAILED(hr))
	{
		AfxMessageBox(_T("Load msado12.dll erro"));
		return FALSE;
	}
	int ret = m_global_pCon->Open(m_mdb_path_t3000.GetString(),_T(""),_T(""),adModeUnknown);
	return ret;
}

bool Is_Bacnet_Device(unsigned short n_product_class_id)
{
	if((n_product_class_id == PM_CM5) ||
		(n_product_class_id == PM_MINIPANEL))
	{
		return true;
	}

	return false;
}

bool IP_is_Local(LPCTSTR ip_address)
{

	IP_ADAPTER_INFO pAdapterInfo;
	ULONG len = sizeof(pAdapterInfo); 
	if(GetAdaptersInfo(&pAdapterInfo, &len) != ERROR_SUCCESS) 
	{
		return 0;
	}
	long nLocalIP=inet_addr(pAdapterInfo.IpAddressList.IpAddress.String);
	DWORD dw_ip = htonl(nLocalIP);

	CString temp_ip;
	temp_ip.Format(_T("%s"),ip_address);
	char tempchar[200];
	memset(tempchar,0,200);
	WideCharToMultiByte(CP_ACP,0,temp_ip.GetBuffer(),-1,tempchar,200,NULL,NULL);

	DWORD m_dwClientIP = inet_addr((char *)tempchar);

	BYTE byIP[4];
	for (int i = 0, ic = 3; i < 4; i++,ic--)
	{
		byIP[i] = (dw_ip >> ic*8)&0x000000FF;
	}

	BYTE byISPDeviceIP[4];
	DWORD dwClientIP = m_dwClientIP;
	ZeroMemory(byISPDeviceIP,4);
	for (int i = 0, ic = 3; i < 4; i++,ic--)
	{
		byISPDeviceIP[3-i] = (dwClientIP >> ic*8)&0x000000FF;
	}
	if(memcmp(byIP,byISPDeviceIP,3)==0)
	{
		return true;
	}

	//memcpy_s(byIP,3,byISPDeviceIP,3)
	return false;
}

// 执行程序的路径 // 参数  // 执行环境目录   // 最大等待时间, 超过这个时间强行终止;
DWORD WinExecAndWait( LPCTSTR lpszAppPath,LPCTSTR lpParameters,LPCTSTR lpszDirectory, 	DWORD dwMilliseconds)  
{
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize    = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask    = SEE_MASK_NOCLOSEPROCESS;
	//ShExecInfo.fMask    = SEE_MASK_HMONITOR;
	ShExecInfo.hwnd        = NULL;
	ShExecInfo.lpVerb    = NULL;
	ShExecInfo.lpFile    = lpszAppPath;        
	ShExecInfo.lpParameters = lpParameters;    
	ShExecInfo.lpDirectory    = lpszDirectory;
	ShExecInfo.nShow    = SW_HIDE;
	ShExecInfo.hInstApp = NULL;    
	ShellExecuteEx(&ShExecInfo);
	if(dwMilliseconds==0)
	{
		WaitForSingleObject(ShExecInfo.hProcess,INFINITE);  //等待进程退出，才继续运行;
	}
	else if (dwMilliseconds!=0 && WaitForSingleObject(ShExecInfo.hProcess, dwMilliseconds) == WAIT_TIMEOUT)  	// 指定时间没结束;
	{    // 强行杀死进程;
		TRACE(_T("TerminateProcess        %s"),lpszAppPath);
		TerminateProcess(ShExecInfo.hProcess, 0);
		return 0;    //强行终止;
	}

	DWORD dwExitCode;
	BOOL bOK = GetExitCodeProcess(ShExecInfo.hProcess, &dwExitCode);
	ASSERT(bOK);

	return dwExitCode;
}



BOOL DirectoryExist(CString Path)
{
	WIN32_FIND_DATA fd;
	BOOL ret = FALSE;
	HANDLE hFind = FindFirstFile(Path, &fd);
	if ((hFind != INVALID_HANDLE_VALUE) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		//目录存在
		ret = TRUE;

	}
	FindClose(hFind);
	return ret;
}


BOOL Ping(const CString& strIP, CWnd* pWndEcho)
{
	CMyPing* pPing = new CMyPing;

	pPing->SetPingEchoWnd(pWndEcho);
	pPing->TestPing(strIP);


	delete pPing;
	pPing = NULL;
	return FALSE;
}


BOOL CreateDirectory(CString path)
{
	SECURITY_ATTRIBUTES attrib;
	attrib.bInheritHandle = FALSE;
	attrib.lpSecurityDescriptor = NULL;
	attrib.nLength = sizeof(SECURITY_ATTRIBUTES);

	return CreateDirectory( path, &attrib);
} 

bool GetFileNameFromPath(CString ncstring,CString &cs_return)
{
	CStringArray ntemparray;
	SplitCStringA(ntemparray,ncstring,_T("\\"));
	int find_results = 0;
	find_results = ntemparray.GetSize();
	if(find_results>=2)
	{
		cs_return = ntemparray.GetAt(find_results - 1);
		return true;
	}
	return false;
}


void DFTrace(LPCTSTR lpCString)
{
	CString nCString;
	nCString = lpCString;
	static int count = 0;
	CTime print_time=CTime::GetCurrentTime();
	CString str=print_time.Format("%H:%M:%S    ");

	PrintText[count].Empty();
	PrintText[count] =str + nCString;
	PostMessage(h_debug_window,WM_ADD_DEBUG_CSTRING,(WPARAM)PrintText[count].GetBuffer(),NULL);
	count = (count ++) % 900;
	
}
