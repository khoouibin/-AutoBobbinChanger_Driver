// ReceivMsg.h

#ifndef _RECEIVMSG_H_
#define _RECEIVMSG_H_

// #ifdef __cplusplus
// extern "C" {
// #endif

// function.
void *thdProcessUsbMessage(void *ptr);
int Handle_Received_Messages(ONS_BYTE *gbRcv_Message);
void printout_message(void);

void *show_485msg(void *ptr);

void RegisterCommandEcho(int iIndex, int iValue, unsigned char *pBuf);
void SetCommandEcho(int iIndex, unsigned char *pSource, int iByte);
int GetCommandEcho(int iIndex);
int WaitForCommandReply(int iCommandID, int iTimeOutUs);
// #ifdef __cplusplus
// }
// #endif

#endif //_RECEIVMSG_H_
