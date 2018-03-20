/************************************************************************
** Includes
*************************************************************************/
#include <string.h>

#include "cfe.h"
#include "ms5611_custom.h"

#include "ms5611_app.h"
#include "ms5611_msg.h"
#include "ms5611_version.h"
#include <math.h>
#include "px4lib.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Instantiate the application object.                             */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
MS5611 oMS5611;


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Default constructor.                                            */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
MS5611::MS5611()
{

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Destructor constructor.                                         */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
MS5611::~MS5611()
{

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Initialize event tables.                                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 MS5611::InitEvent()
{
    int32  iStatus=CFE_SUCCESS;
    int32  ind = 0;
    int32 customEventCount = 0;
    
    CFE_EVS_BinFilter_t   EventTbl[CFE_EVS_MAX_EVENT_FILTERS];

    /* Initialize the event filter table.
     * Note: 0 is the CFE_EVS_NO_FILTER mask and event 0 is reserved (not used) */
    memset(EventTbl, 0x00, sizeof(EventTbl));
    
    /* TODO: Choose the events you want to filter.  CFE_EVS_MAX_EVENT_FILTERS
     * limits the number of filters per app.  An explicit CFE_EVS_NO_FILTER 
     * (the default) has been provided as an example. */
    EventTbl[  ind].EventID = MS5611_RESERVED_EID;
    EventTbl[ind++].Mask    = CFE_EVS_NO_FILTER;
    EventTbl[  ind].EventID = MS5611_READ_ERR_EID;
    EventTbl[ind++].Mask    = CFE_EVS_FIRST_16_STOP;
    
    
    /* Add custom events to the filter table */
    customEventCount = MS5611_Custom_Init_EventFilters(ind, EventTbl);
    
    if(-1 == customEventCount)
    {
        iStatus = CFE_EVS_APP_FILTER_OVERLOAD;
        (void) CFE_ES_WriteToSysLog("Failed to init custom event filters (0x%08X)\n", (unsigned int)iStatus);
        goto end_of_function;
    }

    /* Register the table with CFE */
    iStatus = CFE_EVS_Register(EventTbl, (ind + customEventCount), CFE_EVS_BINARY_FILTER);
    if (iStatus != CFE_SUCCESS)
    {
        (void) CFE_ES_WriteToSysLog("MS5611 - Failed to register with EVS (0x%08lX)\n", iStatus);
    }

end_of_function:

    return iStatus;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Initialize Message Pipes                                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 MS5611::InitPipe()
{
    int32  iStatus=CFE_SUCCESS;

    /* Init schedule pipe and subscribe to wakeup messages */
    iStatus = CFE_SB_CreatePipe(&SchPipeId,
            MS5611_SCH_PIPE_DEPTH,
            MS5611_SCH_PIPE_NAME);
    if (iStatus == CFE_SUCCESS)
    {
        iStatus = CFE_SB_SubscribeEx(MS5611_MEASURE_MID, SchPipeId, CFE_SB_Default_Qos, MS5611_MEASURE_MID_MAX_MSG_COUNT);
        if (iStatus != CFE_SUCCESS)
        {
            (void) CFE_EVS_SendEvent(MS5611_SUBSCRIBE_ERR_EID, CFE_EVS_ERROR,
                    "Sch Pipe failed to subscribe to MS5611_MEASURE_MID. (0x%08lX)",
                    iStatus);
            goto MS5611_InitPipe_Exit_Tag;
        }

        iStatus = CFE_SB_SubscribeEx(MS5611_SEND_HK_MID, SchPipeId, CFE_SB_Default_Qos, MS5611_SEND_HK_MID_MAX_MSG_COUNT);
        if (iStatus != CFE_SUCCESS)
        {
            (void) CFE_EVS_SendEvent(MS5611_SUBSCRIBE_ERR_EID, CFE_EVS_ERROR,
                     "CMD Pipe failed to subscribe to MS5611_SEND_HK_MID. (0x%08X)",
                     (unsigned int)iStatus);
            goto MS5611_InitPipe_Exit_Tag;
        }
    }
    else
    {
        (void) CFE_EVS_SendEvent(MS5611_PIPE_INIT_ERR_EID, CFE_EVS_ERROR,
             "Failed to create SCH pipe (0x%08lX)",
             iStatus);
        goto MS5611_InitPipe_Exit_Tag;
    }

    /* Init command pipe and subscribe to command messages */
    iStatus = CFE_SB_CreatePipe(&CmdPipeId,
            MS5611_CMD_PIPE_DEPTH,
            MS5611_CMD_PIPE_NAME);
    if (iStatus == CFE_SUCCESS)
    {
        /* Subscribe to command messages */
        iStatus = CFE_SB_Subscribe(MS5611_CMD_MID, CmdPipeId);

        if (iStatus != CFE_SUCCESS)
        {
            (void) CFE_EVS_SendEvent(MS5611_SUBSCRIBE_ERR_EID, CFE_EVS_ERROR,
                 "CMD Pipe failed to subscribe to MS5611_CMD_MID. (0x%08lX)",
                 iStatus);
            goto MS5611_InitPipe_Exit_Tag;
        }
    }
    else
    {
        (void) CFE_EVS_SendEvent(MS5611_PIPE_INIT_ERR_EID, CFE_EVS_ERROR,
             "Failed to create CMD pipe (0x%08lX)",
             iStatus);
        goto MS5611_InitPipe_Exit_Tag;
    }

MS5611_InitPipe_Exit_Tag:
    return iStatus;
}
    

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Initialize Global Variables                                     */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void MS5611::InitData()
{
    /* Init housekeeping message. */
    CFE_SB_InitMsg(&HkTlm,
            MS5611_HK_TLM_MID, sizeof(HkTlm), TRUE);
    /* Init output messages */
    CFE_SB_InitMsg(&SensorBaro,
            PX4_SENSOR_BARO_MID, sizeof(PX4_SensorBaroMsg_t), TRUE);
    /* Init diagnostic message */
    CFE_SB_InitMsg(&Diag,
            MS5611_DIAG_TLM_MID, sizeof(MS5611_DiagPacket_t), TRUE);
    /* Init custom data */
    MS5611_Custom_InitData();
    /* Update params from the configuration table */
    UpdateParamsFromTable();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* MS5611 initialization                                           */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 MS5611::InitApp()
{
    int32  iStatus   = CFE_SUCCESS;
    int8   hasEvents = 0;
    boolean returnBool = TRUE;
    int32 i = 0;
    
    iStatus = InitEvent();
    if (iStatus != CFE_SUCCESS)
    {
        (void) CFE_ES_WriteToSysLog("MS5611 - Failed to init events (0x%08lX)\n", iStatus);
        goto MS5611_InitApp_Exit_Tag;
    }
    else
    {
        hasEvents = 1;
    }

    iStatus = InitPipe();
    if (iStatus != CFE_SUCCESS)
    {
        goto MS5611_InitApp_Exit_Tag;
    }

    iStatus = InitConfigTbl();
    if (iStatus != CFE_SUCCESS)
    {
        goto MS5611_InitApp_Exit_Tag;
    }

    InitData();

    returnBool = MS5611_Custom_Init();
    if (FALSE == returnBool)
    {
        iStatus = -1;
        goto MS5611_InitApp_Exit_Tag;
    }

    /* Get calibration coefficients from device PROM */
    for(i = 0; i < MS5611_COEF_SIZE; ++i)
    {
        returnBool = MS5611_ReadPROM(i, &MS5611_Coefficients[i]);
        /* Copy to diagnostic message */
        Diag.Coefficients[i] = MS5611_Coefficients[i];
        if (FALSE == returnBool)
        {
            iStatus = -1;
            goto MS5611_InitApp_Exit_Tag; 
        }
    }
    
    /* Validate the CRC code */
    returnBool = ValidateCRC();
    if (FALSE == returnBool)
    {
        CFE_EVS_SendEvent(MS5611_INIT_ERR_EID, CFE_EVS_ERROR,
                "MS5611 failed CRC check");
        iStatus = -1;
        goto MS5611_InitApp_Exit_Tag;
    }
    
    HkTlm.State = MS5611_INITIALIZED;

    /* Register the cleanup callback */
    iStatus = OS_TaskInstallDeleteHandler(&MS5611_CleanupCallback);
    if (iStatus != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(MS5611_INIT_ERR_EID, CFE_EVS_ERROR,
                                 "Failed to init register cleanup callback (0x%08X)",
                                 (unsigned int)iStatus);
        goto MS5611_InitApp_Exit_Tag;
    }

MS5611_InitApp_Exit_Tag:
    if (iStatus == CFE_SUCCESS)
    {
        (void) CFE_EVS_SendEvent(MS5611_INIT_INF_EID, CFE_EVS_INFORMATION,
                                 "Initialized.  Version %d.%d.%d.%d",
                                 MS5611_MAJOR_VERSION,
                                 MS5611_MINOR_VERSION,
                                 MS5611_REVISION,
                                 MS5611_MISSION_REV);
    }
    else
    {
        if (hasEvents == 1)
        {
            (void) CFE_ES_WriteToSysLog("MS5611 - Application failed to initialize\n");
        }
    }

    return iStatus;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Receive and Process Messages                                    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 MS5611::RcvSchPipeMsg(int32 iBlocking)
{
    int32           iStatus=CFE_SUCCESS;
    CFE_SB_Msg_t*   MsgPtr=NULL;
    CFE_SB_MsgId_t  MsgId;

    /* Stop Performance Log entry */
    CFE_ES_PerfLogExit(MS5611_MAIN_TASK_PERF_ID);

    /* Wait for WakeUp messages from scheduler */
    iStatus = CFE_SB_RcvMsg(&MsgPtr, SchPipeId, iBlocking);

    /* Start Performance Log entry */
    CFE_ES_PerfLogEntry(MS5611_MAIN_TASK_PERF_ID);

    if (iStatus == CFE_SUCCESS)
    {
        MsgId = CFE_SB_GetMsgId(MsgPtr);
        switch (MsgId)
        {
            case MS5611_SEND_HK_MID:
                ProcessCmdPipe();
                ReportHousekeeping();
                break;

            case MS5611_MEASURE_MID:
                ReadDevice();
                break;

            default:
                (void) CFE_EVS_SendEvent(MS5611_MSGID_ERR_EID, CFE_EVS_ERROR,
                        "Recvd invalid SCH msgId (0x%04X)", MsgId);
        }
    }
    else if (iStatus == CFE_SB_NO_MESSAGE)
    {
        /* TODO: If there's no incoming message, you can do something here, or 
         * nothing.  Note, this section is dead code only if the iBlocking arg
         * is CFE_SB_PEND_FOREVER. */
        iStatus = CFE_SUCCESS;
    }
    else if (iStatus == CFE_SB_TIME_OUT)
    {
        /* TODO: If there's no incoming message within a specified time (via the
         * iBlocking arg, you can do something here, or nothing.  
         * Note, this section is dead code only if the iBlocking arg
         * is CFE_SB_PEND_FOREVER. */
        iStatus = CFE_SUCCESS;
    }
    else
    {
        (void) CFE_EVS_SendEvent(MS5611_RCVMSG_ERR_EID, CFE_EVS_ERROR,
                "SCH pipe read error (0x%08lX).", iStatus);
    }

    return iStatus;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Process Incoming Commands                                       */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void MS5611::ProcessCmdPipe()
{
    int32 iStatus = CFE_SUCCESS;
    CFE_SB_Msg_t*   CmdMsgPtr=NULL;
    CFE_SB_MsgId_t  CmdMsgId;

    /* Process command messages until the pipe is empty */
    while (1)
    {
        iStatus = CFE_SB_RcvMsg(&CmdMsgPtr, CmdPipeId, CFE_SB_POLL);
        if(iStatus == CFE_SUCCESS)
        {
            CmdMsgId = CFE_SB_GetMsgId(CmdMsgPtr);
            switch (CmdMsgId)
            {
                case MS5611_CMD_MID:
                    ProcessAppCmds(CmdMsgPtr);
                    break;

                default:
                    /* Bump the command error counter for an unknown command.
                     * (This should only occur if it was subscribed to with this
                     *  pipe, but not handled in this switch-case.) */
                    HkTlm.usCmdErrCnt++;
                    (void) CFE_EVS_SendEvent(MS5611_MSGID_ERR_EID, CFE_EVS_ERROR,
                                "Recvd invalid CMD msgId (0x%04X)", (unsigned short)CmdMsgId);
                    break;
            }
        }
        else if (iStatus == CFE_SB_NO_MESSAGE)
        {
            break;
        }
        else
        {
            (void) CFE_EVS_SendEvent(MS5611_RCVMSG_ERR_EID, CFE_EVS_ERROR,
                  "CMD pipe read error (0x%08lX)", iStatus);
            break;
        }
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Process MS5611 Commands                                         */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void MS5611::ProcessAppCmds(CFE_SB_Msg_t* MsgPtr)
{
    uint32  uiCmdCode=0;

    if (MsgPtr != NULL)
    {
        uint16 ExpectedLength = sizeof(MS5611_NoArgCmd_t); 
        /* Length verification is then the same for all three commands */
        if (VerifyCmdLength(MsgPtr, ExpectedLength))
        {
            uiCmdCode = CFE_SB_GetCmdCode(MsgPtr);
            switch (uiCmdCode)
            {
                case MS5611_NOOP_CC:
                    HkTlm.usCmdCnt++;
                    (void) CFE_EVS_SendEvent(MS5611_CMD_NOOP_EID, CFE_EVS_INFORMATION,
                        "Recvd NOOP. Version %d.%d.%d.%d",
                        MS5611_MAJOR_VERSION,
                        MS5611_MINOR_VERSION,
                        MS5611_REVISION,
                        MS5611_MISSION_REV);
                    break;
    
                case MS5611_RESET_CC:
                    HkTlm.usCmdCnt = 0;
                    HkTlm.usCmdErrCnt = 0;
                    break;

                case MS5611_SEND_DIAG_TLM_CC:
                    HkTlm.usCmdCnt++;
                    ReportDiagnostic();
                    break;
    
                default:
                    HkTlm.usCmdErrCnt++;
                    (void) CFE_EVS_SendEvent(MS5611_CC_ERR_EID, CFE_EVS_ERROR,
                                      "Recvd invalid command code (%u)", (unsigned int)uiCmdCode);
                    break;
            }
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Send MS5611 Housekeeping                                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void MS5611::ReportHousekeeping()
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t*)&HkTlm);
    CFE_SB_SendMsg((CFE_SB_Msg_t*)&HkTlm);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Send MS5611 Diagnostic                                          */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void MS5611::ReportDiagnostic()
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t*)&Diag);
    CFE_SB_SendMsg((CFE_SB_Msg_t*)&Diag);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Verify Command Length                                           */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
boolean MS5611::VerifyCmdLength(CFE_SB_Msg_t* MsgPtr,
                           uint16 usExpectedLen)
{
    boolean bResult  = TRUE;
    uint16  usMsgLen = 0;

    if (MsgPtr != NULL)
    {
        usMsgLen = CFE_SB_GetTotalMsgLength(MsgPtr);

        if (usExpectedLen != usMsgLen)
        {
            bResult = FALSE;
            CFE_SB_MsgId_t MsgId = CFE_SB_GetMsgId(MsgPtr);
            uint16 usCmdCode = CFE_SB_GetCmdCode(MsgPtr);

            (void) CFE_EVS_SendEvent(MS5611_MSGLEN_ERR_EID, CFE_EVS_ERROR,
                              "Rcvd invalid msgLen: msgId=0x%08X, cmdCode=%d, "
                              "msgLen=%d, expectedLen=%d",
                              MsgId, usCmdCode, usMsgLen, usExpectedLen);
            HkTlm.usCmdErrCnt++;
        }
    }

    return bResult;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* MS5611 Application C style main entry point.                    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
extern "C" void MS5611_AppMain()
{
    oMS5611.AppMain();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* MS5611 Application C++ style main entry point.                  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void MS5611::AppMain()
{
    /* Register the application with Executive Services */
    uiRunStatus = CFE_ES_APP_RUN;

    int32 iStatus = CFE_ES_RegisterApp();
    if (iStatus != CFE_SUCCESS)
    {
        (void) CFE_ES_WriteToSysLog("MS5611 - Failed to register the app (0x%08lX)\n", iStatus);
    }

    /* Start Performance Log entry */
    CFE_ES_PerfLogEntry(MS5611_MAIN_TASK_PERF_ID);

    /* Perform application initializations */
    if (iStatus == CFE_SUCCESS)
    {
        iStatus = InitApp();
    }

    if (iStatus == CFE_SUCCESS)
    {
        /* Do not perform performance monitoring on startup sync */
        CFE_ES_PerfLogExit(MS5611_MAIN_TASK_PERF_ID);
        CFE_ES_WaitForStartupSync(MS5611_STARTUP_TIMEOUT_MSEC);
        CFE_ES_PerfLogEntry(MS5611_MAIN_TASK_PERF_ID);
    }
    else
    {
        uiRunStatus = CFE_ES_APP_ERROR;
    }

    /* Application main loop */
    while (CFE_ES_RunLoop(&uiRunStatus) == TRUE)
    {
        RcvSchPipeMsg(MS5611_SCH_PIPE_PEND_TIME);
        iStatus = AcquireConfigPointers();
        if(iStatus != CFE_SUCCESS)
        {
            /* We apparently tried to load a new table but failed.  Terminate the application. */
            uiRunStatus = CFE_ES_APP_ERROR;
        }
    }

    /* Stop Performance Log entry */
    CFE_ES_PerfLogExit(MS5611_MAIN_TASK_PERF_ID);

    /* Exit the application */
    CFE_ES_ExitApp(uiRunStatus);
}


boolean MS5611::GetMeasurement(int32 *Pressure, int32 *Temperature)
{
    uint32 D1 = 0;  //ADC value of the pressure conversion
    /* TODO static */
    static uint32 D2 = 0;  //ADC value of the temperature conversion
    int32 dT = 0;   //difference between actual and measured temp.
    int64 OFF = 0;  //offset at actual temperature
    int64 SENS = 0; //sensitivity at actual temperature
    int32 T2 = 0;
    int64 OFF2 = 0;
    int64 SENS2 = 0;
    int64 TEMP = 0;
    int32 TempValidate = 0;
    int32 PressValidate = 0;
    boolean returnBool = TRUE;

    returnBool = MS5611_D1Conversion();
    if (FALSE == returnBool)
    {
        CFE_EVS_SendEvent(MS5611_READ_ERR_EID, CFE_EVS_ERROR,
                "MS5611 get measurement D1 conversion failed");
        returnBool = FALSE;
        goto end_of_function;
    }

    returnBool = MS5611_ReadADCResult(&D1);
    if (FALSE == returnBool)
    {
        CFE_EVS_SendEvent(MS5611_READ_ERR_EID, CFE_EVS_ERROR,
                "MS5611 read ADC result D1 failed");
        returnBool = FALSE;
        goto end_of_function;
    }

    /* Measurement ratio pressure per temperature measurements */
    /* TODO move this to platform config */
    if(0 == Diag.MeasureCount % 3)
    { 
        if(0xFF == Diag.MeasureCount)
        {
            /* Roll over*/
            Diag.MeasureCount = 0;
        }
        
        returnBool = MS5611_D2Conversion();
        if (FALSE == returnBool)
        {
            CFE_EVS_SendEvent(MS5611_READ_ERR_EID, CFE_EVS_ERROR,
                    "MS5611 get measurement D2 conversion failed");
            returnBool = FALSE;
            goto end_of_function;
        }
        
        returnBool = MS5611_ReadADCResult(&D2);
        if (FALSE == returnBool)
        {
            CFE_EVS_SendEvent(MS5611_READ_ERR_EID, CFE_EVS_ERROR,
                    "MS5611 read ADC result D2 failed");
            returnBool = FALSE;
            goto end_of_function;
        }
    }
    Diag.MeasureCount++;
    /* D2 - C5 * 2^8 */
    dT    = (int32)D2 - ((int32)MS5611_Coefficients[5] << 8);
    /* The following two equations must be solved with 64-bit integers (long long int)
     * or overflow could occur. Note integer-suffix ll. */

    /* C2 * 2^16 + (C4 * dT )/ 2^7 */
    OFF   = ((int64)MS5611_Coefficients[2] << 16) + (((int64)MS5611_Coefficients[4] * dT) >> 7);
    /* C1 * 2^15 + (C3 * dT )/ 2^8 */
    SENS  = ((int64)MS5611_Coefficients[1] << 15) + (((int64)MS5611_Coefficients[3] * dT) >> 8);
    /* 2000 + dT * C6 / 2^23 */
    TempValidate = 2000 + (int32)(((int64)dT * MS5611_Coefficients[6]) >> 23);

    if(TempValidate > MS5611_TEMP_MIN && TempValidate < MS5611_TEMP_MAX)
    {
        *Temperature = TempValidate;
    }
    else
    {
        CFE_EVS_SendEvent(MS5611_READ_ERR_EID, CFE_EVS_ERROR,
                "MS5611 temperature out of range value = %ld", TempValidate);
        returnBool = FALSE;
        goto end_of_function;
    }
    /* Second order temperature compensation */
    if(*Temperature < 2000)
    {
        T2    = (dT * dT) >> 31;
        TEMP  = (int64)*Temperature - 2000;
        OFF2  = (5 *  TEMP * TEMP) >> 1;
        SENS2 = (5 *  TEMP * TEMP) >> 2;

        if(*Temperature < -1500)
        {
            TEMP  = *Temperature + 1500;
            OFF2  += 7 * TEMP * TEMP;
            SENS2 += (11 * TEMP * TEMP) >> 1;
        }

        *Temperature -= T2;
        OFF   -= OFF2;
        SENS  -= SENS2;
    }
    /*  (D1 * SENS / 2^21 - OFF) / 2^15*/
    PressValidate = ((((D1 * SENS) >> 21) - OFF ) >> 15);

    if(PressValidate > MS5611_PRESS_MIN && PressValidate < MS5611_PRESS_MAX)
    {
        *Pressure = PressValidate;
    }
    else
    {
        CFE_EVS_SendEvent(MS5611_READ_ERR_EID, CFE_EVS_ERROR,
                "MS5611 pressure out of range value = %ld", PressValidate);
        returnBool = FALSE;
        goto end_of_function;
    }
    /* Update diagnostic message */
    Diag.RawPressure = *Pressure;
    Diag.RawTemperature = *Temperature;

end_of_function:
    return returnBool;
}


void MS5611::ReadDevice(void)
{
    int32 pressure = 0;
    int32 temperature = 0;
    boolean returnBool = TRUE;

    returnBool = GetMeasurement(&pressure, &temperature);
    if (FALSE == returnBool)
    {
        CFE_EVS_SendEvent(MS5611_READ_ERR_EID, CFE_EVS_ERROR,
                "MS5611 read failure, altitude not updated");
        /* TODO handle error condition */
    }
    else
    {
        SensorBaro.Temperature = temperature / 100.0f;
        /* convert to millibar */
        SensorBaro.Pressure = pressure / 100.0f;
    
        /* tropospheric properties (0-11km) for standard atmosphere */
        /* temperature at base height in Kelvin */
        const double T1 = 15.0 + 273.15;
        /* temperature gradient in degrees per metre */
        const double a  = -6.5 / 1000;
        /* gravity constant in m/s/s */
        const double g  = 9.80665;
        /* ideal gas constant in J/kg/K */
        const double R  = 287.05;
    
        /* current pressure at MSL in kPa */
        //double p1 = 101325 / 1000.0;

        /* measured pressure in kPa */
        double p = static_cast<double>(pressure) / 1000.0;
    
        /*
         * Solve:
         *
         *     /        -(aR / g)     \
         *    | (p / p1)          . T1 | - T1
         *     \                      /
         * h = -------------------------------  + h1
         *                   a
         */
        SensorBaro.Altitude = (((pow((p / m_Params.p1), (-(a * R) / g))) * T1) - T1) / a;
    
        /* Update diagnostic message */
        Diag.Pressure = SensorBaro.Pressure;
        Diag.Temperature = SensorBaro.Temperature;
        Diag.Altitude = SensorBaro.Altitude;

        SensorBaro.Timestamp = PX4LIB_GetPX4TimeUs();

        CFE_SB_TimeStampMsg((CFE_SB_Msg_t*)&SensorBaro);
        CFE_SB_SendMsg((CFE_SB_Msg_t*)&SensorBaro);
    }
}


uint8 MS5611::CRC4(uint16 n_prom[])
{
    int cnt; 
    /* crc remainder */
    uint16 n_rem; 
    /* original crc value */
    uint16 crc_read; 
    uint8 n_bit;
    n_rem = 0x00;

    /* save the original CRC */
    crc_read = n_prom[7];
    /* replace the crc byte with 0 */
    n_prom[7] = (0xFF00 & (n_prom[7])); 
    /* operation is performed on bytes */
    for (cnt = 0; cnt < 16; cnt++) 
    {
        /* choose LSB or MSB */
        if (cnt%2==1) n_rem ^= (uint16) ((n_prom[cnt>>1]) & 0x00FF);
        else n_rem ^= (uint16) (n_prom[cnt>>1]>>8);
        for (n_bit = 8; n_bit > 0; n_bit--)
        {
            if (n_rem & (0x8000))
            {
            n_rem = (n_rem << 1) ^ 0x3000;
            }
            else
            {
                n_rem = (n_rem << 1);
            }
        }
    }
    /* the final 4-bit remainder is the CRC code */
    n_rem = (0x000F & (n_rem >> 12));
    /* Restore the crc_read to its original value */
    n_prom[7] = crc_read;
    return (n_rem ^ 0x00);
}


boolean MS5611::ValidateCRC(void)
{
    unsigned char returnedCRC = 0;
    uint16 promCRC = 0;
    /* CRC code is in the last 4-bits */
    uint16 bitmask = 0x0F;
    /* Get CRC value from PROM */
    promCRC = bitmask & MS5611_Coefficients[7];
    /* Calculate CRC */
    returnedCRC = CRC4(MS5611_Coefficients);

    if(promCRC == returnedCRC)
    {
        return TRUE;
    }
    CFE_EVS_SendEvent(MS5611_INIT_ERR_EID, CFE_EVS_ERROR,
            "MS5611 CRC check failed PROM = %u: CRC = %u",
            (unsigned int)promCRC, (unsigned int) returnedCRC);
    return FALSE;
}

void MS5611::UpdateParamsFromTable(void)
{
    if(0 != ConfigTblPtr)
    {
        m_Params.p1 = ConfigTblPtr->p1;
    }
}


void MS5611_CleanupCallback(void)
{
    oMS5611.HkTlm.State = MS5611_UNINITIALIZED;
    if(MS5611_Custom_Uninit() != TRUE)
    {
        CFE_EVS_SendEvent(MS5611_UNINIT_ERR_EID, CFE_EVS_ERROR,"MS5611_Uninit failed");
        oMS5611.HkTlm.State = MS5611_INITIALIZED;
    }
}


/************************/
/*  End of File Comment */
/************************/
