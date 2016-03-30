/*
 * File:   Drv_Modem_WismoHL.h
 * Author: Eng. Juan Camilo G�mez Cadavid
 * A generalized AT-Based Driver for Sierra Wireless Modems : Wismo and HL devices
 * Requirements
 *  - A 1ms tick timer interrupt
 *  - A Receiver Interrupt (RX)
 * Revision: 4
 */
#include "DrvModem.h"

/*UNDER CONSTRUCTION!, BE CAREFUL WITH THIS LIBRARY*/
volatile char BufferCellularFrame[MAX_CELLULAR_SIZE_FRAME]={0};
volatile ModemDrv_data_t ModemDrv;
volatile ModemDrv_StateFlags_t ModemState;
ModemDrv_gStrIncomingMessage_t ModemSMSData;
ModemDrv_OperatorInfo_t OperatorsInfo;
/*=================================================================================================================================================*/
char* CellularModem_Tool_trimwhitespace(char *str){
    char *end;
    while(isspace(*str)) str++;
    if(*str == 0)
        return str;
    end = str + strlen(str) - 1;
    while( end > str && isspace(*end)) end--;
    *(end+1) = 0;
    return str;
}
/*============================================================================*/
void CellularModem_Tool_strtolower(char *str){
    char *pstr=str;
    while(*pstr++)
        *pstr = (char)tolower(*pstr);
}
/*============================================================================*/
int CellularModem_addOperatorInfo(ModemDrv_OperatorInfo_t *obj, const char *service_operator, const char *apn, const char* username, const char* passwd){
    if(obj->index < 0) obj->index=0;
    if(obj->index > MAX_CELL_OPERATORS_DETECTION) return OPERATION_INCOMPLETE;
    obj->Names[obj->index] = (unsigned char*)service_operator;
    obj->APN[obj->index] = (unsigned char*)apn;
    obj->UserName[obj->index] = (unsigned char*)username;
    obj->Password[obj->index] = (unsigned char*)passwd;
    obj->index++;
    return OK_CELLULAR_RESPONSE;
}
/*============================================================================*/
void CellularModem_InitDrv(void (*OutFcn)(char), void (*DebugFcn)(char), void(*PWRCtrlFcn)(unsigned char), void(*RSTCtrlFcn)(unsigned char)){
    CellularModem_Waitms(1500);
    CellularModem_FlushInputBuffer();
    ModemDrv.u8contCellularframe = 0;
    ModemDrv.u8contMatch = 0;
    ModemDrv.ptrBufferToMatchResponse = (unsigned char *)BufferCellularFrame;
    ModemDrv.flgCELLULAR_MODEM = NO_CELLULAR_FLAG;
    ModemDrv.ModemSend = OutFcn;
    ModemDrv.ModemDebug = DebugFcn;
    ModemState.All = 0x0000;
    ModemDrv.retry_CELLULAR_HARDWARE = 0;
    ModemDrv.retry_CELLULAR_IP_STACK = 0;
    ModemDrv.retry_CELLULAR_GSM_STACK = 0;
    ModemDrv.PWRCtrlFcn = PWRCtrlFcn;
    ModemDrv.RSTCtrlFcn = RSTCtrlFcn;
    OperatorsInfo.index = 0;
    CellularModem_SendString("\r\n[DBG:ModemDrv]: Driver Initialized.",CM_DRV_DEBUG_ONLY);
}
/*============================================================================*/
int CellularModem_TurnOn(void){
    if (ModemState.ModemON == 0){ //
    	CellularModem_SendString("\r\n[DBG:ModemDrv]: Turning ON Modem...\r\n",CM_DRV_DEBUG_ONLY);
        ModemState.All  = 0x0000;
        CellularModem_OnOff(CELLULAR_MODEM_ON);
        ModemState.ModemON  = 1;
    }
    return (int)ModemState.ModemON;
}
/*============================================================================*/
int CellularModem_StartSetup(const unsigned char No_Retries){
    if(ModemState.Configured == 0 && ModemState.ModemON == 1){
    	CellularModem_SendString("\r\n[DBG:ModemDrv]: Setting up Modem...\r\n",CM_DRV_DEBUG_ONLY);
        if(CellularModem_Setup()== OK_CELLULAR_RESPONSE){
            ModemState.Configured = 1;
        }
        else if ((++ModemDrv.retry_CELLULAR_HARDWARE) >= No_Retries){
          ModemDrv.retry_CELLULAR_HARDWARE = 0;
          CellularModem_SendString("\r\n[DBG:ModemDrv]: {Number of Retries Reached} Device Configuration\r\n",CM_DRV_DEBUG_ONLY);
          CellularModem_OnOff(CELLULAR_MODEM_OFF); //Turn Off
          ModemState.ModemON = 0;               //release previous flag - CellularModem_TurnOn 
        }
    }
    return (int)ModemState.Configured;
}
/*============================================================================*/
int CellularModem_ConnectToCellularNetwork(const unsigned char No_Retries){  
    if(ModemState.NetworkConnection == CELLULAR_REGISTRATION_NONE && ModemState.Configured == 1 ){
    	CellularModem_SendString("\r\n[DBG:ModemDrv]: Connecting to Network...\r\n",CM_DRV_DEBUG_ONLY);
        CellularModem_SwitchToATMode();
        CellularModem_SendString("[DBG:ModemDrv]: {CHECK} Checking Network registration...\r\n",CM_DRV_DEBUG_ONLY);
        if(CellularModem_CheckCellularConnection() == OK_CELLULAR_RESPONSE){
            ModemDrv.retry_CELLULAR_GSM_STACK = 0;
            ModemDrv.retry_CELLULAR_IP_STACK = 0;
            //ModemState.NetworkConnection  = 1;
            CellularModem_SendString("\r\n[DBG:ModemDrv]: <Registered on Network>\r\n",CM_DRV_DEBUG_ONLY);
            CellularModem_SendString("[DBG:ModemDrv]: {CHECK} Detecting service operator...\r\n",CM_DRV_DEBUG_ONLY);
            if (CellularModem_GetOperator()==OK_CELLULAR_RESPONSE){
            	CellularModem_SendString("[DBG:ModemDrv]: Operator: ",CM_DRV_DEBUG_ONLY);
            	CellularModem_SendString(OperatorsInfo.Names[OperatorsInfo.DetectedOperatorIndex], CM_DRV_DEBUG_ONLY );
            	CellularModem_SendString("\r\n",CM_DRV_DEBUG_ONLY);
            }
        }
        else if ((++ModemDrv.retry_CELLULAR_GSM_STACK) >= No_Retries){
            ModemDrv.retry_CELLULAR_GSM_STACK = 0;
            ModemState.NetworkConnection = CELLULAR_REGISTRATION_NONE;
            ModemState.Configured = 0; //release previous flag
            CellularModem_SendString("\r\n[DBG:ModemDrv]: {Number of Retries Reached} Network Connection\r\n",CM_DRV_DEBUG_ONLY);
            CellularModem_LetUsKnowWhenThis("+\r\n");
        }
    }
    return (int)ModemState.NetworkConnection;
}
/*============================================================================*/
int CellularModem_ConnectToInternet(unsigned char *APNName, unsigned char *UserName, unsigned char *APNPassword, const unsigned char No_Retries){
    if(ModemState.InternetConnection == CELLULAR_INTERNET_NONE && ModemState.NetworkConnection > 0){
        CellularModem_SendString("\r\n[DBG:ModemDrv]: Conected to Internet...\r\n",CM_DRV_DEBUG_ONLY);
        if(ModemState.InternetConnection == 0 && ModemState.NetworkConnection > 0 ){
            if(CellularModem_InternetConnection(APNName, UserName, APNPassword) == OK_CELLULAR_RESPONSE){
                ModemDrv.retry_CELLULAR_IP_STACK = 0;
                ModemState.InternetConnection = 1;
                CellularModem_GetDeviceIP();
            }
            else if ((++ModemDrv.retry_CELLULAR_GSM_STACK) >= No_Retries){
                ModemDrv.retry_CELLULAR_IP_STACK = 0;
                ModemState.NetworkConnection = CELLULAR_REGISTRATION_NONE; //release previous flag
                ModemState.InternetConnection = 0;
                CellularModem_SendString("\r\n[DBG:ModemDrv]: {Number of Retries Reached} Internet Connection\r\n",CM_DRV_DEBUG_ONLY);
            }
        }
    }
    return (int)ModemState.InternetConnection;
}
/*============================================================================*/
void CellularModem_ModemOff(void){
    CellularModem_SendString("[DBG:ModemDrv]: Device Off...\r\n",CM_DRV_DEBUG_ONLY);
    ModemDrv.PWRCtrlFcn(NO_ON_MODEM); 
}
/*============================================================================*/
void CellularModem_ModemOn(void){
    CellularModem_SendString("[DBG:ModemDrv]: Device ON...\r\n",CM_DRV_DEBUG_ONLY);
    ModemDrv.RSTCtrlFcn(NO_RESET_MODEM);
    ModemDrv.PWRCtrlFcn(YES_ON_MODEM);
}
/*============================================================================*/
int CellularModem_SleepMode(unsigned char *mode){
	CellularModem_FlushInputBuffer();
    CellularModem_SendString("[DBG:ModemDrv]: Disabling Sleep Mode...\r\n",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("AT+KSLEEP=",CM_DRV_BOTH);
    CellularModem_SendString(mode,CM_DRV_BOTH);
    CellularModem_SendString("\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n"));
}
/*============================================================================*/
void CellularModem_ModemReset(void){
    CellularModem_SendString("[DBG:ModemDrv]: Device Reset...\r\n",CM_DRV_DEBUG_ONLY);
    ModemDrv.RSTCtrlFcn(YES_RESET_MODEM);
}
/*============================================================================*/
int CellularModem_OnOff(unsigned char action){
    switch (action){
         /*******************************************************
         *                         ____________
         * OnOffpin_______________|
         * Command   "AT+CPOF\r\n"
         ********************************************************/
        case CELLULAR_MODEM_OFF:
            CellularModem_SendString("AT+CPOF\r\n",CM_DRV_BOTH);
            CellularModem_ModemOff();                 // TurnOff Modem by Hardware (High level)
            CellularModem_InitTIMEOUT(4000);
            while(CellularModem_GetFlagTIMEOUT()!= TIME_OUT_TRUE){}
            CellularModem_ModemReset();               // RESET Modem by Hardware
            CellularModem_InitTIMEOUT(4000);
            while(CellularModem_GetFlagTIMEOUT()!= TIME_OUT_TRUE){}
            return OK_CELLULAR_RESPONSE;
        //break;
        /*******************************************************
         * A <LOW level> signal has to be provided on the ON/~OFF
         * pin to switch the AirPrime WISMO228 ON
         ********************************************************/
        case CELLULAR_MODEM_ON:
            CellularModem_ModemOn();                 // TurnOn WISMO by Hardware (LOW level)
            return OK_CELLULAR_RESPONSE;
        //break;
        case CELLULAR_MODEM_RESET:
            return OK_CELLULAR_RESPONSE;
        //break;
        case CELLULAR_MODEM_GO_TO_SLEEP:
        break;
        case CELLULAR_MODEM_WAKEUP:
        break;
        default:
        break;
    }
    return ERROR_CELLULAR_RESPONSE;
}
/*============================================================================*/
void CellularModem_SendString(const char *s, unsigned char mode){
    unsigned char i=0;
    switch(mode){
        case 0:
            while(s[i]) ModemDrv.ModemSend(s[i++]);
            break;
        case 1:
            if(ModemDrv.ModemDebug!=NULL){
                while(s[i]) ModemDrv.ModemDebug(s[i++]);
            }
            break;
        case 2:
            if(ModemDrv.ModemDebug!=NULL){
                while(s[i]) ModemDrv.ModemDebug(s[i++]);
            }
            i=0;
            while(s[i]) ModemDrv.ModemSend(s[i++]);
            break;
        default:
            break;
    }
}
/*============================================================================*/
void CellularModemISR_1msTimeOutHandler(void){
    ModemDrv.miliSecondsCellularModem++;
}
/*============================================================================*/
int CellularModemISR_UARTHandler(unsigned char charRxFromCellular){
    if(ModemDrv.u8contCellularframe < (MAX_CELLULAR_SIZE_FRAME-1)){
        BufferCellularFrame[ModemDrv.u8contCellularframe] = charRxFromCellular;
        BufferCellularFrame[++ModemDrv.u8contCellularframe] = '\0';
        if(ModemDrv.ptrBufferToMatchResponse[ModemDrv.u8contMatch] == charRxFromCellular){
            ModemDrv.u8contMatch++;
            if(ModemDrv.u8contMatch == ModemDrv.lengthBufferToMatch){
                ModemDrv.flgCELLULAR_MODEM = OK_CELLULAR_FRAME;
                return ModemDrv.flgCELLULAR_MODEM; // notification to send event
            }
        }
    }
    else{
        ModemDrv.flgCELLULAR_MODEM = ERROR_CELLULAR_FRAME;
        ModemDrv.u8contCellularframe = 0;
        ModemDrv.u8contMatch = 0;
    }
    return 0;
}
/*============================================================================*/
void CellularModem_InitTIMEOUT(unsigned int time_out){
    ModemDrv.gDelay = time_out;
    ModemDrv.miliSecondsCellularModem = 0;
}
/*============================================================================*/
void CellularModem_ClearFlags(void){
    ModemDrv.flgCELLULAR_MODEM = NO_CELLULAR_FLAG;
    ModemDrv.u8contMatch = 0;
    ModemDrv.u8contCellularframe = 0;
    ModemDrv.miliSecondsCellularModem;
}
/*============================================================================*/
int CellularModem_GetFlagTIMEOUT(void){
    if(ModemDrv.miliSecondsCellularModem < ModemDrv.gDelay){
        return MODEMDRV_TIME_OUT_FALSE;
    }
    else return MODEMDRV_TIME_OUT_TRUE;
}
/*============================================================================*/
int CellularModem_Waitms(unsigned int timems){
	  CellularModem_InitTIMEOUT(timems);
	  do{
	     #ifdef NOP
	         NOP();
	     #endif
	  }while( CellularModem_GetFlagTIMEOUT()!=MODEMDRV_TIME_OUT_TRUE );
}
/*============================================================================*/
void CellularModem_LetUsKnowWhenThis(const char *ptResponse){
    ModemDrv.ptrBufferToMatchResponse =  (unsigned char*) ptResponse;
    ModemDrv.lengthBufferToMatch = (unsigned char) strlen((const char*)ModemDrv.ptrBufferToMatchResponse);
    ModemDrv.flgCELLULAR_MODEM = NO_CELLULAR_FLAG;
    ModemDrv.u8contMatch = 0;
    ModemDrv.u8contCellularframe = 0;
}
/*============================================================================*/
int CellularModem_WaitForTimeExpirationOrResponse(unsigned int timeout_mseg, const char *ptResponse){
    //CellularModem_FlushInputBuffer();
    CellularModem_LetUsKnowWhenThis(ptResponse);
    CellularModem_InitTIMEOUT(timeout_mseg);
    do{
       #ifdef NOP
           NOP();
       #endif
    }while( (ModemDrv.flgCELLULAR_MODEM != OK_CELLULAR_FRAME) && (CellularModem_GetFlagTIMEOUT()!=MODEMDRV_TIME_OUT_TRUE) );
    if(CellularModem_GetFlagTIMEOUT() == MODEMDRV_TIME_OUT_TRUE){
        CellularModem_SendString("[DBG:ModemDrv]: {ERROR} TimeOut\r\n",CM_DRV_DEBUG_ONLY);
        return ERROR_CELLULAR_RESPONSE;
    }
    else{
        return OK_CELLULAR_RESPONSE;
    }
}
/*============================================================================*/
int CellularModem_Setup(void){    
    #ifdef HW_HL_MODULE
    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Disabling Sleep Mode...\r\n", CM_DRV_DEBUG_ONLY);
    if (CellularModem_SleepMode((unsigned char *)"2") != OK_CELLULAR_RESPONSE)
        return (ERROR_CELLULAR_RESPONSE);
    #endif
    
    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Setting Flow Control...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_LocalFlowControl() != OK_CELLULAR_RESPONSE )          // Set the flow control to NONE or CTS,RTS
        return (ERROR_CELLULAR_RESPONSE);

    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Setting Local ECHO...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_Echo(EchoOFF) != OK_CELLULAR_RESPONSE )               // Echo off
        return (ERROR_CELLULAR_RESPONSE);

    #ifdef HW_HL_MODULE
    //CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Selecting SIM...\r\n", CM_DRV_DEBUG_ONLY);
    //if(CellularModem_SelectSIM("1") != OK_CELLULAR_RESPONSE )               // SIM 1
    //    return (ERROR_CELLULAR_RESPONSE);
    #endif

    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Enabling CLIP...\r\n", CM_DRV_DEBUG_ONLY);
    if (CellularModem_EnableCallLineIdentification()== OK_CELLULAR_RESPONSE){
        CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Call Line Identification Enabled.\r\n", CM_DRV_DEBUG_ONLY);
    }

    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Getting Device IMEI...\r\n", CM_DRV_DEBUG_ONLY);
    if (CellularModem_GetIMEI() == OK_CELLULAR_RESPONSE){
        memset((void*)ModemDrv.IMEI, 0x00, sizeof(ModemDrv.IMEI));
        memcpy((void*)ModemDrv.IMEI, (void*)BufferCellularFrame+2, 15);       
        CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: Device IMEI:", CM_DRV_DEBUG_ONLY);
        CellularModem_SendString( (unsigned char *)ModemDrv.IMEI, CM_DRV_DEBUG_ONLY);
        CellularModem_SendString( (unsigned char *)"\r\n", CM_DRV_DEBUG_ONLY);
    }
    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Checking SIM...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_PIN() != OK_CELLULAR_RESPONSE )                       // SimCard Ready
        return (ERROR_CELLULAR_RESPONSE);
    
    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Setting Message Format...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_MessageFormat(Textmode) != OK_CELLULAR_RESPONSE )     // Text mode
        return (ERROR_CELLULAR_RESPONSE);

    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Setting New Messague Indication...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_GetNewMessagesIndication() != OK_CELLULAR_RESPONSE )  //
        return (ERROR_CELLULAR_RESPONSE);

    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Enabling Cellular Connection...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_EnableCellularConnection() != OK_CELLULAR_RESPONSE)
        return (ERROR_CELLULAR_RESPONSE);

    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Setting Operation bands...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_OperationBand(GSM_850 + GSM_900 + DCS_1800 + E_GSM + PCS_1900) != OK_CELLULAR_RESPONSE) //GSM_850 + GSM_900 + DCS_1800 + E_GSM + PCS_1900
            return (ERROR_CELLULAR_RESPONSE);
    
    CellularModem_SendString( (unsigned char *)"\r\n[DBG:ModemDrv]: {SETUP} Saving Modem Parameters...\r\n", CM_DRV_DEBUG_ONLY);
    if(CellularModem_SaveParams() != OK_CELLULAR_RESPONSE)
        return (ERROR_CELLULAR_RESPONSE);

    return(OK_CELLULAR_RESPONSE);
}
/*============================================================================*/
int CellularModem_ATOK(void){
    CellularModem_SendString("AT\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(500,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_LocalFlowControl(void){
	CellularModem_FlushInputBuffer();
    #ifdef  HW_HL_MODULE
        CellularModem_SendString("AT&K0\r",CM_DRV_BOTH);
    #else
        CellularModem_SendString("AT+IFC=0,0\r",CM_DRV_BOTH);
    #endif
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(2000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_Echo(unsigned char *mode){
	CellularModem_FlushInputBuffer();
    CellularModem_SendString("ATE",CM_DRV_BOTH);
    CellularModem_SendString(mode,CM_DRV_BOTH);
    CellularModem_SendString("\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_PIN(void){
    CellularModem_SendString("AT+CPIN?\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(4000,"READY\r\n"));
}
/*============================================================================*/
int CellularModem_SelectSIM(const unsigned char *sim){ //AT+KSDS=1 
    CellularModem_FlushInputBuffer();
    CellularModem_SendString("AT+KSDS=",CM_DRV_BOTH);
    CellularModem_SendString(sim,CM_DRV_BOTH);
    CellularModem_SendString("\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(4000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_MessageFormat(const unsigned char *mode){
    CellularModem_SendString("AT+CMGF=",CM_DRV_BOTH);
    CellularModem_SendString(mode,CM_DRV_BOTH);
    CellularModem_SendString("\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_GetNewMessagesIndication(void){
    CellularModem_SendString("AT+CNMI=1,1,,,1\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_Tool_findcharindex(int offset, char *str, char c){
    while(str[offset]!=c && str[offset]!='\0') offset++;
    return offset;
}
/*============================================================================*/
int CellularModem_ReadMessage(unsigned char *index){
    char *ptr1=NULL;
    char *ptr2=NULL;
    //CellularModem_FlushInputBuffer();
    CellularModem_SendString("AT+CMGR=",CM_DRV_BOTH);
    CellularModem_SendString(index,CM_DRV_BOTH);
    CellularModem_SendString("\r",CM_DRV_BOTH);    
    if (CellularModem_WaitForTimeExpirationOrResponse(2000,"OK\r\n") != OK_CELLULAR_RESPONSE){
        return ERROR_CELLULAR_RESPONSE;
    }   
    //CellularModem_SendString(BufferCellularFrame,CM_DRV_DEBUG_ONLY);
    
    ptr1=strstr((const char*)BufferCellularFrame,"\"");
    ModemSMSData.pStatus = ptr1+1;
    ptr2=strstr(ptr1+1,"\"");
    ptr2[0]='\0';
    
    ptr1=strstr(ptr2+1,",\"");
    ModemSMSData.pCellPhone = ptr1+2;
    ptr2=strstr(ptr1+2,"\"");
    ptr2[0]='\0';
    
    ptr1=strstr(ptr2+1,",\"");
    ModemSMSData.pContact = ptr1+2;
    ptr2=strstr(ptr1+2,"\"");
    ptr2[0]='\0';
    
    ptr1=strstr(ptr2+1,",\"");
    ModemSMSData.pDate = ptr1+2;
    ptr2=strstr(ptr1+2,"\"");
    ptr2[0]='\0';
    
    ptr1=strstr(ptr2+1,"\r\n");
    ModemSMSData.pMessage = ptr1+2;
    ptr2=strstr(ptr1+2,"\r\n");
    ptr2[0]='\0';
    CellularModem_SendString("[DBG:ModemDrv]: Incoming SMS\r\n",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("\r\n -Status :",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString(ModemSMSData.pStatus,CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("\r\n -CellPhone :",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString(ModemSMSData.pCellPhone,CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("\r\n -Contact :",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString(ModemSMSData.pContact,CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("\r\n -Date :",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString(ModemSMSData.pDate,CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("\r\n -SMS :",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString(ModemSMSData.pMessage,CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("\r\n\r\n", CM_DRV_DEBUG_ONLY);
    return OK_CELLULAR_RESPONSE;
}
/*============================================================================*/
int CellularModem_SendSMS(const char *pCellularPhoneNumber, const char * pMsg){
    CellularModem_SendString("AT+CMGS=\"",CM_DRV_BOTH);
    CellularModem_SendString(pCellularPhoneNumber,CM_DRV_BOTH);
    CellularModem_SendString("\"\r",CM_DRV_BOTH);
    if (CellularModem_WaitForTimeExpirationOrResponse(2000, ">") != OK_CELLULAR_RESPONSE){
        return (ERROR_CELLULAR_RESPONSE);         // try later
    }
    CellularModem_SendString(pMsg,CM_DRV_BOTH);
    CellularModem_SendString("\r",CM_DRV_DEBUG_ONLY);
    ModemDrv.ModemSend(0x1A);
    return (CellularModem_WaitForTimeExpirationOrResponse(8000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_DeleteMessage(char *index, const char *which_one){
	CellularModem_FlushInputBuffer();
    CellularModem_SendString("AT+CMGD=",CM_DRV_BOTH);
    CellularModem_SendString(index,CM_DRV_BOTH);
    CellularModem_SendString(",",CM_DRV_BOTH);
    CellularModem_SendString(which_one,CM_DRV_BOTH);
    CellularModem_SendString("\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(2000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_EnableCellularConnection(void){
    CellularModem_SendString("AT+CREG=1\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(4000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_CheckCellularConnection(void){
    int ret=ERROR_CELLULAR_RESPONSE;
    if(ModemState.NetworkConnection==CELLULAR_REGISTRATION_ROAMING) goto CM_DRV_ONLY_CHECK_ROAMING;
    CellularModem_SendString("[DBG:ModemDrv]: Verifying HOME...\r\n", CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("AT+CREG?\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    ret = CellularModem_WaitForTimeExpirationOrResponse(8000,"+CREG: 1,1"); //check if modem is home-registered
    if (ret==OK_CELLULAR_RESPONSE){
        ModemState.NetworkConnection = CELLULAR_REGISTRATION_HOME;
        return ret;
    }
    CellularModem_FlushInputBuffer();
    CM_DRV_ONLY_CHECK_ROAMING:
    CellularModem_SendString("[DBG:ModemDrv]: Verifying ROAMING...\r\n", CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("AT+CREG?\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    ret = CellularModem_WaitForTimeExpirationOrResponse(8000,"+CREG: 1,5");  //check if modem is roaming-registered 
    if (ret==OK_CELLULAR_RESPONSE){
        ModemState.NetworkConnection = CELLULAR_REGISTRATION_ROAMING;
        return ret;
    }
    ModemState.NetworkConnection = CELLULAR_REGISTRATION_NONE;
    return ret;
}
/*============================================================================*/
int CellularModem_EnableCallLineIdentification(void){
    CellularModem_FlushInputBuffer();
    CellularModem_SendString("AT+CLIP=1\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(6000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_GetIMEI(void){
    CellularModem_FlushInputBuffer();
    CellularModem_SendString("AT+CGSN\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_GetMyPhoneNumber(void){
    CellularModem_FlushInputBuffer();
    CellularModem_SendString("AT+MDN?\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(4000,"\r\n"));
}
/*============================================================================*/
int CellularModem_GetOperator(void){
    char i;
    CellularModem_FlushInputBuffer();
    CellularModem_SendString("AT+COPS?\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    if(CellularModem_WaitForTimeExpirationOrResponse(500,"OK\r\n")!= OK_CELLULAR_RESPONSE){
        return ERROR_CELLULAR_RESPONSE;
    }
    CellularModem_SendString((unsigned char*)&BufferCellularFrame[3],CM_DRV_DEBUG_ONLY);
    CellularModem_Tool_strtolower((unsigned char*)&BufferCellularFrame[3]);
    OperatorsInfo.DetectedOperatorIndex  = (char)-1;
    for(i=0;i<OperatorsInfo.index;i++){
        if(strstr((const unsigned char*)&BufferCellularFrame[3], OperatorsInfo.Names[i])){
            OperatorsInfo.DetectedOperatorIndex = i;
            return OK_CELLULAR_RESPONSE;
        }
    }
    return OPERATION_INCOMPLETE;
}
/*============================================================================*/
int CellularModem_SaveParams(void){
    CellularModem_SendString("AT&W\r",CM_DRV_BOTH);
    ModemDrv.ModemDebug('\n');
    return (CellularModem_WaitForTimeExpirationOrResponse(4000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_OperationBand(unsigned char gsmBand){
    char cmd[20]={0x00};
    #ifdef HW_QUECTEL_MODULE
	#ifdef HW_QUECTEL_CLASS_1
            strcpy(cmd,"AT+QBAND=\"GSM850_EGSM_DCS_PCS_MODE\"\r\n");
	#endif
	#ifdef HW_QUECTEL_CLASS_2
            strcpy(cmd,"AT+QCFG=\"band\",512,1\r\n");
	#endif
    #else
        sprintf(cmd,"AT*PSRDBS=0,%d\r\n",gsmBand);
    #endif
    CellularModem_SendString(cmd,CM_DRV_BOTH);
    return (CellularModem_WaitForTimeExpirationOrResponse(6000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_SwitchToATMode(void){
    CellularModem_SendString("\r\n[DBG:ModemDrv]: Changing to AT Mode...\r\n",CM_DRV_DEBUG_ONLY);
    CellularModem_Waitms(1000);
    CellularModem_SendString("+++",CM_DRV_BOTH);
    CellularModem_Waitms(1000);
    CellularModem_SendString("\r\n",CM_DRV_DEBUG_ONLY);
    if(CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n")== OK_CELLULAR_RESPONSE){
        return OK_CELLULAR_RESPONSE;
    }
    /*Maybe the modem was not in <exchange data on socket>*/
    CellularModem_SendString("AT\r\nAT\r\n",CM_DRV_BOTH);
    if(CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n")== OK_CELLULAR_RESPONSE){
        return OK_CELLULAR_RESPONSE;
    }
    return (ERROR_CELLULAR_RESPONSE);
}
/*============================================================================*/
int CellularModem_GetDeviceIP(void){
    int j=0,i=0;
    CellularModem_SendString("AT+WIPBR=3,6,15\r\n",CM_DRV_BOTH);
    if(CellularModem_WaitForTimeExpirationOrResponse(500,"OK\r\n")!= OK_CELLULAR_RESPONSE){
        return ERROR_CELLULAR_RESPONSE;
    }
    while( (BufferCellularFrame[j]!='\"') && (i < (int)sizeof(BufferCellularFrame)) )
        j++;
    j++; //Skip the first "
    for (i=0; i<((int)sizeof(ModemDrv.IPAddress)-1);i++,j++){
        if(BufferCellularFrame[j]=='\"')
            break;
        else{
            ModemDrv.IPAddress[i] = BufferCellularFrame[j];
            ModemDrv.IPAddress[i+1] = '\0';
        }
    }
    CellularModem_SendString("\r\n[DBG:ModemDrv]:Device IP: ",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString((unsigned char*)ModemDrv.IPAddress,CM_DRV_DEBUG_ONLY);
    return OK_CELLULAR_RESPONSE;
}
/*============================================================================*/
int CellularModem_InternetConnection(unsigned char * pAPN_Name,unsigned char * pUSER_APN_Name,unsigned char * pAPN_Password){
    if(CellularModem_SwitchToATMode() != OK_CELLULAR_RESPONSE)
        return (ERROR_CELLULAR_RESPONSE);//Try again later

    CellularModem_SendString("\r\n[DBG:ModemDrv]: Stopping IP Stack...\r\n",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("AT+WIPCFG=0\r\n",CM_DRV_BOTH);
    CellularModem_InitTIMEOUT(200);
    while(CellularModem_GetFlagTIMEOUT()!= TIME_OUT_TRUE){}
    CellularModem_SendString("\r\n[DBG:ModemDrv]: Initializing Connection...\r\n",CM_DRV_DEBUG_ONLY);
    CellularModem_SendString("AT+WIPCFG=1\r\n",CM_DRV_BOTH);
    if ( CellularModem_WaitForTimeExpirationOrResponse(300,"OK\r\n") != OK_CELLULAR_RESPONSE ){
        return (ERROR_CELLULAR_RESPONSE);          // Responds OK?
    }
    CellularModem_SendString("AT+WIPBR=1,6\r\n",CM_DRV_BOTH);
    if ( CellularModem_WaitForTimeExpirationOrResponse(500,"OK\r\n") != OK_CELLULAR_RESPONSE ){
        return (ERROR_CELLULAR_RESPONSE);          // Responds OK?
    }
    CellularModem_SendString("AT+WIPBR=2,6,11,\"",CM_DRV_BOTH);
    CellularModem_SendString(pAPN_Name,CM_DRV_BOTH);
    CellularModem_SendString("\"\r\n",CM_DRV_BOTH);    
    if ( CellularModem_WaitForTimeExpirationOrResponse(300,"OK\r\n") != OK_CELLULAR_RESPONSE ){
        return (ERROR_CELLULAR_RESPONSE);          // Responds OK?
    }
    CellularModem_SendString("AT+WIPBR=2,6,0,\"",CM_DRV_BOTH);
    CellularModem_SendString(pUSER_APN_Name,CM_DRV_BOTH);
    CellularModem_SendString("\"\r\n",CM_DRV_BOTH);    
    if ( CellularModem_WaitForTimeExpirationOrResponse(300,"OK\r\n") != OK_CELLULAR_RESPONSE ){
        return (ERROR_CELLULAR_RESPONSE);          // Responds OK?
    }
    CellularModem_SendString("AT+WIPBR=2,6,1,\"",CM_DRV_BOTH);
    CellularModem_SendString(pAPN_Password,CM_DRV_BOTH);
    CellularModem_SendString("\"\r\n",CM_DRV_BOTH);    
    if ( CellularModem_WaitForTimeExpirationOrResponse(300,"OK\r\n") != OK_CELLULAR_RESPONSE ){
        return (ERROR_CELLULAR_RESPONSE);          // Responds OK?
    }
    CellularModem_SendString("AT+WIPBR=4,6,0\r\n",CM_DRV_BOTH);  
    if ( CellularModem_WaitForTimeExpirationOrResponse(3000,"OK\r\n") != OK_CELLULAR_RESPONSE ){
        return (ERROR_CELLULAR_RESPONSE);          // Responds OK?
    }
    return (OK_CELLULAR_RESPONSE);
}
/*============================================================================*/
int CellularModem_CloseSocket(unsigned char protocol,unsigned char index){
    if(CellularModem_SwitchToATMode() != OK_CELLULAR_RESPONSE)
        return (ERROR_CELLULAR_RESPONSE);//Try again later
    CellularModem_SendString("AT+WIPCLOSE=",CM_DRV_BOTH);
    ModemDrv.ModemSend(protocol);ModemDrv.ModemSend(',');ModemDrv.ModemSend(index);
    ModemDrv.ModemDebug(protocol);ModemDrv.ModemDebug(',');ModemDrv.ModemDebug(index);
    CellularModem_SendString("\r\n",CM_DRV_BOTH);
    return(CellularModem_WaitForTimeExpirationOrResponse(1000,"OK\r\n"));// Wait for timeOut or OK
}
/*============================================================================*/
int CellularModem_OpenSocket(unsigned char protocol,unsigned char index, unsigned char *args,...){
    unsigned char wipcreate_data[50];
    va_list ap;
    va_start(ap, args);
    sprintf(wipcreate_data,"AT+WIPCREATE=%c,%c",protocol,index);
    while (args != 0 ){
        (void)strcat(wipcreate_data,",");
        (void)strcat(wipcreate_data,args);
        args = va_arg(ap, unsigned char *);
    }
    (void)strcat(wipcreate_data,"\r\n");
    va_end(ap);
    CellularModem_SendString(wipcreate_data,CM_DRV_BOTH);
    return(CellularModem_WaitForTimeExpirationOrResponse(6000,"OK\r\n"));
}
/*============================================================================*/
int CellularModem_SocketExchangeData(unsigned char protocol,unsigned char index,unsigned char mode){
    unsigned char wipdata_cmd[50];
    sprintf(wipdata_cmd,"AT+WIPDATA=%c,%c,%c\r\n",protocol,index,mode);
    CellularModem_SendString(wipdata_cmd,CM_DRV_BOTH);
    return(CellularModem_WaitForTimeExpirationOrResponse(2000,"CONNECT\r\n"));
}
/*============================================================================*/
