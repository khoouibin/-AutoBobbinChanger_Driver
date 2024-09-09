#ifndef DRIVER_TO_UI_H_
#define DRIVER_TO_UI_H_

#include <iostream>
#include <string>
#include <queue>

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/Observer.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/URI.h"
#include "Poco/JSON/Object.h"
#include "Poco/Exception.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/JSON/Object.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Mutex.h"
#include "Poco/Net/TCPServer.h"
#include "json.hpp"

using namespace std;
using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;
using json = nlohmann::json;

// typedef struct
// {
// 	int              h32_status;
// 	int              ini_status;
// 	VersionInfo_t    h32_vernum;
// 	unsigned short   h32_chksum;
// 	ini_sourcefile_t ini_info;
// }load_h32_resp_t;

// typedef struct
// {
// 	int i_total_msg;
// 	int i_curr_msg; //current message number.
// 	int i_tx_res;
// }tx_h32_resp_t;

// int send_message_to_uiBL( std::string message );
// int msg_driv_to_ui_load_h32( string file_name, int file_status );
// int msg_driv_to_ui_read_h32_gen_chksum( unsigned short chksum, int chksum_status );
// int msg_driv_to_ui_read_h32_aux_chksum( unsigned short chksum, int chksum_status );
// int msg_driv_to_ui_read_h32_version_info( VersionInfo_t verinfo, int verinfo_status );

// int msg_driv_to_ui_load_h32_resp( load_h32_resp_t sour_info );
// int msg_driv_to_ui_tx_h32_resp( tx_h32_resp_t tx_status );
// int msg_driv_to_ui_tx_h32_end( string end_status );

int time_stamp(void);


int tx_msg_to_Bootloader_UI( std::string message );

#endif
