// ---------------------------------------------------------------------------
//  Filename: Definition.h
//  Created by: Nissim Avichail
//  Date:  01/07/15
//  Orisol (c)
// ---------------------------------------------------------------------------

// 之前的define太過混亂, 應該根據 作業系統/產品/通訊方式/硬體平台 分別制定define, 而不是把4個綁在一起.

// ONS or something
//#define _PRODUCT_ONS
// Linux or Windows
//#define _OS_LINUX64 / _OS_WIN64
// PC or PIC33EP512
//#define _PLATFORM_PC
// Host (usually pc) or Guest 9usually RTC)
//#define _ROLE_HOST


//#define ENABLE_WRITE_ERROR_TO_LOG
//#define ENABLE_PRINT_TO_SCREEN
#define ENABLE_TERMINET_ON_ERROR

#define ONS_HOST_PRG_LINUX

#ifdef ONS_HOST_PRG_LINUX						// Host with LINUX OS with USB communication
#define ONS_PC_PLAT
#define ONS_HOST
#define ONS_HOST_LINUX
#define ONS_LINUX


#else
	#ifdef ONS_UI_PRG_LINUX						// Linux UI Deff
	#define ONS_PC_PLAT
	#define ONS_UI_LINUX
	#define ONS_LINUX
	#else
		#ifdef _MSC_VER   // Windows OS
		#define ONS_PC_PLAT
		#define ONS_WIN

		#ifdef ONS_HOST_IP_WIN						//Host with IP communication
		#define ONS_HOST
		#define ONS_HOST_WIN
		#define ONS_IP_WIN
		#define ONS_UI_WIN_IP
		#define ONS_UI_WIN_HOST
		#else
			#ifdef ONS_HOST_USB_WIN					//Host with USB communication
			#define ONS_USB_WIN
			#define ONS_HOST
			#define ONS_HOST_WIN
			#define ONS_UI_WIN_IP
			#define ONS_UI_WIN_HOST
			#else
				#ifdef ONS_RTC_SIM_IP_WIN			// RTC Simulation with IP communication
				#define ONS_RTC_SIM
				#define ONS_RTC_SIM_WIN    
				#define ONS_IP_WIN
				#else
					#ifdef ONS_RTC_SIM_USB_WIN		// RTC Simulation with USB communication
					#define ONS_USB_WIN
					#define ONS_RTC_SIM
					#define ONS_RTC_SIM_WIN
					#else
						#ifdef ONS_UI_WIN_HOST						// Host with windows OS with UI communication using IP 
						#define ONS_UI_WIN_IP
						#else	
							#ifdef ONS_UI_WIN_UI						// UI with windows OS with UI communication using IP 
							#define ONS_UI_WIN_IP
							#else		
							ERROR NOT VALID SELECTION!!!!!
							#endif
						#endif
					#endif
				#endif
			#endif
		#endif
		#else
			#define ONS_RTC
       		#define RTC_Version	4
		#endif
	#endif				//	
#endif				//	

