#ifndef _IO_CONTROL_H_
#define _IO_CONTROL_H_

#include <string>
#include "Msg_Prot.h"

using namespace std;

#define MAX_MSG_PROT_ID_NUM 128 // 此數值不可低於Msg_Port.h的MSG_PROT_xxx數量

#define _MAX_ENTITY_MAPPING_NUM 46 // RTC在USB封包的設計上就是每次最多指定46個entity
typedef struct
{
	int iId;		// Entity 編號, 請參考 RTC 的 IO_Entity.h
	string strName; // 要上傳給Host的字串名
} EntityData;

extern bool bOneEntityUpdated;
extern bool bEntityListUpdated;

int SetEntityReplyTableToRtc();
int SetRtcEntityStatusReplyMode(int iMode, int iPeriodMs);
int SetEntity(int iId, int iValue, int iCheckTimeoutMs = 0);
int SetEntityS02(int iId1, int iValue1, int iId2, int iValue2, int iCheckTimeoutMs = 0);
int SetEntityS03(int iId1, int iValue1, int iId2, int iValue2, int iId3, int iValue3);
int GetEntity(int iId);		   // 透過USB問答取得一個entity狀態, 速度很慢, 但是可以取得任意entity的狀態.
char GetEntityStatus(int iID); // 由 entity 改變時, RTC 自動通知的機制, 記錄下的 entity 狀態. 速度很快, 但是只能取得有登記在 gtaMyEntity 的 entity.
int SetOutput(int iId, int iValue);
int GetIoStatus(int iId);
int UpdateEntityList(Rcv_Msg_Data_t aReceivedMessage);
int SetInterestEntity(int iIndex, int iValue);

int SetEntityTableChange(int TableNum);
#endif //_LOGIN_H_
