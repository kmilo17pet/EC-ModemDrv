/* 
 * File:   Drv_Modem_WismoHL.h
 * Author: Eng. Juan Camilo Gómez Cadavid
 * A generalized AT-Based Driver for  Modems
 * Requirements
 *  - A 1ms tick timer interrupt
 *  - A Receiver Interrupt (RX)
 * Revision: 7
 */

#ifndef EC_DRV_MODEM
#define	EC_DRV_MODEM

#ifdef	__cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>



#define MAX_CELLULAR_SIZE_FRAME     (256)
#define CM_DRV_MODEM_ONLY           (0)
#define CM_DRV_DEBUG_ONLY           (1)
#define CM_DRV_BOTH                 (2)


/*On-Off Actions*/
#define CELLULAR_MODEM_OFF          0
#define CELLULAR_MODEM_ON           1
#define CELLULAR_MODEM_RESET        2
#define CELLULAR_MODEM_GO_TO_SLEEP  3
#define CELLULAR_MODEM_WAKEUP       4

typedef enum{
    NO_CELLULAR_FLAG,
    OK_CELLULAR_FRAME,
    OK_CELLULAR_RESPONSE,
    ERROR_CELLULAR_FRAME,
    ERROR_CELLULAR_RESPONSE,
    OPERATION_INCOMPLETE,
}ModemDrv_CellularFlags_t;

#define CELLULAR_REGISTRATION_HOME      0x01
#define CELLULAR_REGISTRATION_ROAMING   0x02
#define CELLULAR_REGISTRATION_NONE      0x00
#define CELLULAR_INTERNET_NONE          0x00
#define CELLULAR_INTERNET_UP            0x01

typedef union{
    struct{
        unsigned ModemON :1;
        unsigned Configured:1;
        unsigned NetworkConnection:2;
        unsigned InternetConnection:1;
        unsigned TCPServerConnection:1;
    };
    unsigned short All;
}ModemDrv_StateFlags_t;

typedef struct{
    short u8contCellularframe;
    unsigned char u8contMatch;
    unsigned char *ptrBufferToMatchResponse;
    unsigned char lengthBufferToMatch;
    ModemDrv_CellularFlags_t flgCELLULAR_MODEM;
    unsigned int gDelay;
    unsigned int miliSecondsCellularModem;
    void (*ModemSend)(char c);
    void (*ModemDebug)(char c);
    unsigned char IMEI[20];
    unsigned char IPAddress[17];
    unsigned char retry_CELLULAR_HARDWARE;
    unsigned char retry_CELLULAR_IP_STACK;
    unsigned char retry_CELLULAR_GSM_STACK;
    void(*PWRCtrlFcn)(unsigned char);
    void(*RSTCtrlFcn)(unsigned char);
}ModemDrv_data_t;

typedef struct{
    unsigned char *pStatus;
    unsigned char *pCellPhone;
    unsigned char *pContact;
    unsigned char *pDate;
    unsigned char *pMessage;
}ModemDrv_gStrIncomingMessage_t;

#define MAX_CELL_OPERATORS_DETECTION    (10)
typedef struct{
    char *Names[MAX_CELL_OPERATORS_DETECTION];
    char *APN[MAX_CELL_OPERATORS_DETECTION];
    char *UserName[MAX_CELL_OPERATORS_DETECTION];
    char *Password[MAX_CELL_OPERATORS_DETECTION];
    short index;
    char DetectedOperatorIndex;
}ModemDrv_OperatorInfo_t;
extern ModemDrv_OperatorInfo_t OperatorsInfo;

extern ModemDrv_gStrIncomingMessage_t ModemSMSData;

#define MODEMDRV_TIME_OUT_FALSE         (1)
#define MODEMDRV_TIME_OUT_TRUE          (2)
#define EchoOFF (unsigned char*)"0"
#define EchoON  (unsigned char*)"1"

#define PDUmode   (unsigned char*)"0"
#define Textmode  (unsigned char*)"1"

#define UDP_CLIENT  "1"
#define TCP_CLIENT  "2"
#define TCP_SERVER  "3"
#define PAD_PROFILE_1   "1"
#define PAD_PROFILE_2   "2"
#define PAD_PROFILE_3   "3"

#define TIME_OUT_FALSE  1
#define TIME_OUT_TRUE   2//2

#define GSM_850     1
#define GSM_900     2
#define E_GSM       4
#define DCS_1800    8
#define PCS_1900    16

#define DELETE_THIS_MESSAGE_ONLY (unsigned char *)"0"
#define DELETE_ALL_READ_MESSAGES (unsigned char *)"3"
#define ALL                      (unsigned char *)"4"

#define MODE_UDP          '1'
#define MODE_TCP_CLIENT   '2'
#define MODE_TCP_SERVER   '3'
#define MODE_FTP          '4'
#define MODE_HTTP_CLIENT  '5'
#define MODE_SMTP_CLIENT  '6'
#define MODE_POP3_CLIENT  '7'
#define MODE_MMS_CLIENT   '8'
#define MODE_UDP_SERVER   '9'

#define INDEX_1           '1'
#define INDEX_2           '2'
#define INDEX_3           '3'
#define INDEX_4           '4'
#define INDEX_5           '5'
#define INDEX_6           '6'
#define INDEX_7           '7'
#define INDEX_8           '8'

#define RETRIES_MAX_CELLULAR_IP_STACK       2      //Maximum attempts numbers before resetting IP stack

#define CONTINUOUS_MODE             '1'
#define CONTINUOUS_TRANSPARENT_MODE '2'

#define DOWNLOAD_FILE   '1'
#define UPLOAD_FILE     '2'

#define YES_RESET_MODEM 1
#define NO_RESET_MODEM  0

#define YES_ON_MODEM    1
#define NO_ON_MODEM     0

extern volatile ModemDrv_data_t ModemDrv;
extern volatile char BufferCellularFrame[MAX_CELLULAR_SIZE_FRAME];
extern volatile ModemDrv_StateFlags_t ModemState;
/*Utility*/
char* CellularModem_Tool_trimwhitespace(char *str);
void CellularModem_Tool_strtolower(char *str);
int CellularModem_Tool_findcharindex(int offset, char *str, char c);


#define CellularModem_AddOperator(SERV_OPERATOR, SERV_OPERATOR_APN, SERV_OPERATOR_USER, SERV_OPERATOR_PASS)    CellularModem_addOperatorInfo(&OperatorsInfo, SERV_OPERATOR, SERV_OPERATOR_APN, SERV_OPERATOR_USER, SERV_OPERATOR_PASS)

#define     CellularModem_FlushInputBuffer()    memset((void*)BufferCellularFrame, 0x00, sizeof(BufferCellularFrame))
												
/*Cellular Driver Functions*/
/*============================================================================*/
/**/
int CellularModem_TurnOn(void);
int CellularModem_StartSetup(const unsigned char No_Retries);
int CellularModem_ConnectToCellularNetwork(const unsigned char No_Retries);
int CellularModem_ConnectToInternet(unsigned char *APNName, unsigned char *UserName, unsigned char *APNPassword, const unsigned char No_Retries);
/**/
void CellularModem_ModemOn(void);
void CellularModem_ModemReset(void);
void CellularModem_ModemOff(void);
int CellularModem_OnOff(unsigned char action);
void CellularModem_SendString(const char *s, unsigned char mode);
void CellularModem_InitDrv(void (*OutFcn)(char), void (*DebugFcn)(char), void(*PWRCtrlFcn)(unsigned char), void(*RSTCtrlFcn)(unsigned char));
void CellularModemISR_1msTimeOutHandler(void);
int CellularModemISR_UARTHandler(unsigned char charRxFromCellular);
void CellularModem_InitTIMEOUT(unsigned int time_out);
void CellularModem_ClearFlags(void);
int CellularModem_GetFlagTIMEOUT(void);
int CellularModem_Waitms(unsigned int timems);
void CellularModem_LetUsKnowWhenThis(const char *ptResponse);
int CellularModem_WaitForTimeExpirationOrResponse(unsigned int timeout_mseg, const char *ptResponse);
int CellularModem_Setup(void);
int CellularModem_ATOK(void);
int CellularModem_LocalFlowControl(void);
int CellularModem_Echo(unsigned char *mode);
int CellularModem_PIN(void);
int CellularModem_SelectSIM(const unsigned char *sim);
int CellularModem_MessageFormat(const unsigned char *mode);
int CellularModem_GetNewMessagesIndication(void);
int CellularModem_ReadMessage(unsigned char *index);
int CellularModem_SendSMS(const char *pCellularPhoneNumber, const char * pMsg);
int CellularModem_DeleteMessage(char *index, const char *which_one);
int CellularModem_EnableCellularConnection(void);
int CellularModem_CheckCellularConnection(void);
int CellularModem_EnableCallLineIdentification(void);
int CellularModem_GetIMEI(void);
int CellularModem_GetMyPhoneNumber(void);
int CellularModem_GetOperator(void);
int CellularModem_SaveParams(void);
int CellularModem_OperationBand(unsigned char gsmBand);
int CellularModem_SwitchToATMode(void);
int CellularModem_GetDeviceIP(void);
int CellularModem_InternetConnection(unsigned char * pAPN_Name,unsigned char * pUSER_APN_Name,unsigned char * pAPN_Password);
int CellularModem_SleepMode(unsigned char *mode);
int CellularModem_CloseSocket(unsigned char protocol,unsigned char index);
int CellularModem_OpenSocket(unsigned char protocol,unsigned char index, unsigned char *args,...);
int CellularModem_SocketExchangeData(unsigned char protocol,unsigned char index,unsigned char mode);

int CellularModem_addOperatorInfo(ModemDrv_OperatorInfo_t *obj, const char *service_operator, const char *apn, const char* username, const char* passwd);

#ifdef	__cplusplus
}
#endif

#endif	/* DRV_MODEM_WISMOHL_H */

