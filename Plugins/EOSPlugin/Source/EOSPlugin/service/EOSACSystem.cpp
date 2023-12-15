#include "EOSACSystem.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Winsvc.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if USE_EOSAC_SYSTEM 
DEFINE_LOG_CATEGORY(LogEOSAC)

/**
* Callback function to use for EOS SDK log messages
* @param InMsg - A structure representing data for a log message
*/
void EOS_CALL EOSSDKLoggingCallback(const EOS_LogMessage* InMsg)
{
	if (InMsg->Level != EOS_ELogLevel::EOS_LOG_Off)
	{
		if (InMsg->Level == EOS_ELogLevel::EOS_LOG_Error || InMsg->Level == EOS_ELogLevel::EOS_LOG_Fatal)
		{
			FDebugLog::LogError(L"[EOS SDK] %ls: %ls", FStringUtils::Widen(InMsg->Category).c_str(), FStringUtils::Widen(InMsg->Message).c_str());
		}
		else if (InMsg->Level == EOS_ELogLevel::EOS_LOG_Warning)
		{
			FDebugLog::LogWarning(L"[EOS SDK] %ls: %ls", FStringUtils::Widen(InMsg->Category).c_str(), FStringUtils::Widen(InMsg->Message).c_str());
		}
		else
		{
			
		}
	}
}

bool FEOSACSystem::PlatformInit() {
	FDebugLog::Init();
	FDebugLog::AddTarget(FDebugLog::ELogTarget::DebugOutput);
	FDebugLog::AddTarget(FDebugLog::ELogTarget::File);
	FDebugLog::AddTarget(FDebugLog::ELogTarget::All);

	if (!GConfig)
	{
		UE_LOG(LogEOSAC, Error, TEXT("GConfig 为空"));
		FEOSACSystem::GetInstance()->EnDataChannelLog = false;
	}
	else
	{
		bool bEnable = false;
		GConfig->GetBool(TEXT("AntiCheat"), TEXT("DataChannelLog"), bEnable, GGameIni);
		FEOSACSystem::GetInstance()->EnDataChannelLog = bEnable;
	}

	UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 初始化开始"));

	// Init EOS SDK
	EOS_InitializeOptions SDKOptions = {};
	SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	SDKOptions.AllocateMemoryFunction = nullptr;
	SDKOptions.ReallocateMemoryFunction = nullptr;
	SDKOptions.ReleaseMemoryFunction = nullptr;
	SDKOptions.ProductName = "";
	SDKOptions.ProductVersion = "1.0";
	SDKOptions.Reserved = nullptr;
	SDKOptions.SystemInitializeOptions = nullptr;
	SDKOptions.OverrideThreadAffinity = nullptr;

	EOS_EResult InitResult = EOS_Initialize(&SDKOptions);
	if (InitResult == EOS_EResult::EOS_Success)
	{
		FDebugLog::Log(L"[EOS SDK] Init Success!");
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 初始化成功"));
	}
	else
	{
		FDebugLog::Log(L"[EOS SDK] Init Failed!");
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 初始化失败: %d"), InitResult);

		return false;
	}

	EOS_EResult SetLogCallbackResult = EOS_Logging_SetCallback(&EOSSDKLoggingCallback);
	if (SetLogCallbackResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::Log(L"[EOS SDK] 日志设置失败");
	}
	else
	{
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);
	}

	if (GIsServer)
	{
		const bool bCreateSuccess = FPlatform::Create(true);
		if (!bCreateSuccess)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄创建失败"));
			return false;
		}
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 服务器句柄创建成功"));
		if (!Server_Check())
		{
			return false;
		}
	}
	else
	{
		const bool bCreateSuccess = FPlatform::Create(false);
		if (!bCreateSuccess)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端句柄创建失败"));
			return false;
		}
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] EOS_PlatformInit 客户端创建成功"));
	}

	return true;
}

void FEOSACSystem::Tick(float DeltaSeconds) {
#ifdef EOS_STEAM_ENABLED
	if (GIsClient)
	{
		FSteamManager::GetInstance().Update();
	}
#endif

	if (GIsClient)
	{
		ClientTickCount += 1;

		if (ClientTickCount == 1000)
		{
			PollStatus();
			ClientTickCount = 0;
		}
	}

	EOS_Platform_Tick(FPlatform::GetPlatformHandle());
}


bool FEOSACSystem::Client_Check()
{
	EOS_HAntiCheatClient AntiCheatClient = nullptr;
	if (!FPlatform::IsInitialized())
	{
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK Client] 请先初始化"));
		return false;
	}

	if (FPlatform::GetPlatformHandle() != nullptr)
	{
		AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
	}

	if (AntiCheatClient != nullptr)
	{
		return true;
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK Client] EOS_HAntiCheatClient 为空"));
		return false;
	}
}

void FEOSACSystem::PollStatus() {
	if (Client_Check())
	{
		EOS_HAntiCheatClient AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatClient_PollStatusOptions pollOptions = {};
		pollOptions.ApiVersion = EOS_ANTICHEATCLIENT_POLLSTATUS_API_LATEST;
		pollOptions.OutMessageLength = 256;
		EOS_EAntiCheatClientViolationType ViolationType;
		char OutMessage[256] = { 0 };
		EOS_EResult result = EOS_AntiCheatClient_PollStatus(AntiCheatClient, &pollOptions, &ViolationType, OutMessage);
		if (result == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] EOS_AntiCheatClient_PollStatus, Found Violation, Type: %d, Message=%ls"), ViolationType, OutMessage);
			FDebugLog::Log(L"EOS_AntiCheatClient_PollStatus: ViolationType=%d, Message=%ls", ViolationType, OutMessage);
			// 
			FString reason = FString(OutMessage);
			EacViolationDelegate.ExecuteIfBound((int32)ViolationType, reason);
		}
		else if (result == EOS_EResult::EOS_NotFound)
		{
	
		}
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端句柄为空"));
	}
}

bool FEOSACSystem::Client_IsServiceInstalled() {
#if PLATFORM_WINDOWS
	FString EACServiceName(TEXT("EasyAntiCheat_EOS"));

	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	// Get a handle to the SCM database. 
	SC_HANDLE schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // servicesActive database 
		STANDARD_RIGHTS_READ);  // full access rights 

	if (NULL == schSCManager)
	{
		FString reason = FString::Printf(TEXT("EOS 服务: 服务管理器打开失败:%ld"), GetLastError());
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 服务管理器打开失败:%ld"), GetLastError());
		return false;
	}

	// Get a handle to the service.
	SC_HANDLE schService = OpenService(
		schSCManager,         // SCM database 
		&(EACServiceName.GetCharArray()[0]),    // name of service 
		SERVICE_QUERY_STATUS | SERVICE_START);  // full access 

	if (schService == NULL)
	{
		FString reason = FString::Printf(TEXT("EOS 服务: 打开服务失败:%ld"), GetLastError());

		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 打开服务失败:%ld"), GetLastError());
		CloseServiceHandle(schSCManager);
		return false;
	}

	// Check the status in case the service is not stopped. 

	if (!QueryServiceStatusEx(
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // information level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // size needed if buffer is too small
	{

		FString reason = FString::Printf(TEXT("EOS 服务: 查询服务状态失败:%ld"), GetLastError());
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 查询服务状态失败:%ld"), GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return false;
	}

	// Check if the service is already running. It would be possible 
	// to stop the service here, but for simplicity this example just returns. 

	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		UE_LOG(LogEOSAC, Display, TEXT("EOS 服务: 无法启动该服务，因为它已经在运行"));
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return true;
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	// Wait for the service to stop before attempting to start it.

	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status until the service is no longer stop pending. 

		if (!QueryServiceStatusEx(
			schService,                     // handle to service 
			SC_STATUS_PROCESS_INFO,         // information level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // size needed if buffer is too small
		{
			UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 查询服务状态失败:%ld"), GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return false;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 等待服务停止超时"));
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return false;
			}
		}
	}

	// Attempt to start the service.

	if (!StartService(
		schService,  // handle to service 
		0,           // number of arguments 
		NULL))      // no arguments 
	{
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 启动服务失败:%ld"), GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return false;
	}
	else UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 启动服务挂起"));

	// Check the status until the service is no longer start pending. 

	if (!QueryServiceStatusEx(
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // info level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // if buffer too small
	{
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 查询服务状态失败:%ld"), GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return false;
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth the wait hint, but no less than 1 second and no 
		// more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status again. 

		if (!QueryServiceStatusEx(
			schService,             // handle to service 
			SC_STATUS_PROCESS_INFO, // info level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // if buffer too small
		{
			UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 查询服务状态失败:%ld"), GetLastError());
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				// No progress made within the wait hint.
				break;
			}
		}
	}

	// Determine whether the service is running.
	bool result = false;
	if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	{
		UE_LOG(LogEOSAC, Warning, TEXT("EOS 服务启动成功."));
		result = true;

	}
	else
	{
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 服务未启动"));
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 当前状态: %ld"), ssStatus.dwCurrentState);
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 退出代码: %ld"), ssStatus.dwWin32ExitCode);
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 检查点: %ld"), ssStatus.dwCheckPoint);
		UE_LOG(LogEOSAC, Error, TEXT("EOS 服务: 等待提示: %ld"), ssStatus.dwWaitHint);
		return false;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return result;
#else
	return true;
#endif
}

bool FEOSACSystem::Check_Start_Protected_Game() {
	EOS_HAntiCheatClient AntiCheatClient = nullptr;
	if (!FPlatform::IsInitialized())
	{
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK Client] 请先初始化"));
		return false;
	}

	if (FPlatform::GetPlatformHandle() != nullptr)
	{
		AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
	}

	if (AntiCheatClient != nullptr)
	{
		return true;
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK Client] 客户端句柄为空"));
		return false;
	}
}

FString FEOSACSystem::Client_GetProductUserID() {
	if (StrProductUserId.IsEmpty())
	{
		return "";
	}

	return StrProductUserId;
}

void FEOSACSystem::Add_ClientViolation_Notification(FClientViolationDelegate& EACViolationDelegate){
	EacViolationDelegate = EACViolationDelegate;
}

void FEOSACSystem::Client_CallLogin(FEACLoginDelegate& EACLoginDelegate)
{
#ifdef EOS_STEAM_ENABLED
	UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 登录"));
	EacLoginDelegate = EACLoginDelegate;
	FSteamManager::GetInstance().Init();
#endif
}

void FEOSACSystem::Client_CallLoginCallBackFn(int ResultCode, FString _productID, FString ErrorMsg)
{
	if (GIsClient)
	{
		if (ResultCode == 1)
		{
			UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 获取到 ProductUserID"));
			FEOSACSystem::GetInstance()->StrProductUserId = _productID;
			FEOSACSystem::GetInstance()->GlobalProductUserId = EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*_productID));

			FEOSACSystem::GetInstance()->EacLoginDelegate.ExecuteIfBound(1, _productID, "");
		}
		else
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 获取 ProductUserID 失败 %s "), *ErrorMsg);
			FEOSACSystem::GetInstance()->EacLoginDelegate.ExecuteIfBound(0, "", ErrorMsg);
		}
	}
}


bool FEOSACSystem::Client_BeginSession(FCTSEosAddPacketDelegate& EosAddPacketDelegate, const FString& ProductUserID, UNetConnection* ServerConnection) {
	if (ServerConnection == nullptr || ServerConnection->State == USOCK_Closed)
	{
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 客户端 BeginSession, UNetConnection 为 null 或状态为 USOCK_Closed"));
		return false;
	}

	GServerConnection = ServerConnection;

	EacCtsEosAddPacketDelegate = EosAddPacketDelegate;

	Client_EndSession();
	EOS_ProductUserId ClientUserId = EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*ProductUserID));

	if (!ClientUserId)
	{
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] ProductUserId 为空, 请先登录验证"));
		return false;
	}

	FString Accountid = ProductUserID;

	if (Client_Check())
	{

		if (ClietnNotiMap.Contains(Accountid))
		{
			EOS_NotificationId notiID = ClietnNotiMap[Accountid];
			if (notiID)
			{
				Client_RemoveNotifyMessageToServer(notiID);
			}
		}
		
		EOS_NotificationId newID = Client_AddNotifyMessageToServer(NULL);
		if (newID == EOS_INVALID_NOTIFICATIONID)
		{
			FDebugLog::LogError(L"EOS_INVALID_NOTIFICATIONID");
			return false;
		}
		ClietnNotiMap.Add(Accountid, newID);
		
		EOS_HAntiCheatClient AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatClient_BeginSessionOptions BeginSessionOptions = {};
		BeginSessionOptions.ApiVersion = EOS_ANTICHEATCLIENT_BEGINSESSION_API_LATEST;
		BeginSessionOptions.LocalUserId = ClientUserId;
		BeginSessionOptions.Mode = EOS_EAntiCheatClientMode::EOS_ACCM_ClientServer;

		EOS_EResult result = EOS_AntiCheatClient_BeginSession(AntiCheatClient, &BeginSessionOptions);
		if (result == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 客户端 BeginSession 成功"));
			return true;
		}
		else
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端 BeginSession 失败 %d"), result);
			return false;
		}
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端句柄为空"));
		return false;
	}
}
void FEOSACSystem::Client_EndSession() {
	if (Client_Check())
	{
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 客户端 EndSession"));
		EOS_HAntiCheatClient AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatClient_EndSessionOptions EndSessionOptions = { EOS_ANTICHEATCLIENT_ENDSESSION_API_LATEST };
		EOS_EResult result = EOS_AntiCheatClient_EndSession(AntiCheatClient, &EndSessionOptions);
		if (result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端 EndSession 失败: %d"), result);
		}
		else
		{
			UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 客户端 EndSession 成功"));
		}	
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端句柄为空"));
	}
}

void FEOSACSystem::Client_ReceiveMessageFromServer(const TArray<uint8>& EosPacket, const uint32 nLength) {
	if (GIsClient)
	{
		if (Client_Check())
		{
			EOS_HAntiCheatClient AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
			EOS_AntiCheatClient_ReceiveMessageFromServerOptions ReceiveMessageFromServerOptions = {};
			ReceiveMessageFromServerOptions.ApiVersion = EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMSERVER_API_LATEST;
			ReceiveMessageFromServerOptions.Data = EosPacket.GetData();
			ReceiveMessageFromServerOptions.DataLengthBytes = nLength;

			EOS_EResult result = EOS_AntiCheatClient_ReceiveMessageFromServer(AntiCheatClient, &ReceiveMessageFromServerOptions);
			if (result == EOS_EResult::EOS_Success)
			{

			}
			else if (result == EOS_EResult::EOS_InvalidParameters)
			{
				UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] EOS_AntiCheatClient_ReceiveMessageFromServer EOS_InvalidParameters"));
			}
			else if (result == EOS_EResult::EOS_InvalidRequest)
			{
				UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] EOS_AntiCheatClient_ReceiveMessageFromServer EOS_InvalidRequest"));
			}
			else if (result == EOS_EResult::EOS_AntiCheat_InvalidMode)
			{

			}
		}
		else {
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端句柄为空"));
		}
	}
}


EOS_NotificationId FEOSACSystem::Client_AddNotifyMessageToServer(void* clienData)
{
	if (Client_Check())
	{
		EOS_HAntiCheatClient AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatClient_AddNotifyMessageToServerOptions Options = {};
		Options.ApiVersion = EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOSERVER_API_LATEST;
		return EOS_AntiCheatClient_AddNotifyMessageToServer(AntiCheatClient, &Options, NULL, FEOSACSystem::Client_OnMessageToServerCallbackFn);
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端句柄为空"));
		return 0;
	}
}

void FEOSACSystem::Client_OnMessageToServerCallbackFn(const EOS_AntiCheatClient_OnMessageToServerCallbackInfo* Data)
{
	UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 客户端收到服务器数据"));
	if (!FEOSACSystem::GetInstance()->GServerConnection.IsValid())
	{
		UE_LOG(LogEOSAC, Error, TEXT("无效服务器连接"));
		return;
	}

	TArray<uint8> EosPacket;
	EosPacket.Append((uint8*)Data->MessageData, Data->MessageDataSizeBytes);      
	FEOSACSystem::GetInstance()->EacCtsEosAddPacketDelegate.ExecuteIfBound(EosPacket, Data->MessageDataSizeBytes, FEOSACSystem::GetInstance()->GServerConnection.Get());
}

void FEOSACSystem::Client_RemoveNotifyMessageToServer(EOS_NotificationId NotificationId)
{
	if (Client_Check())
	{
		EOS_HAntiCheatClient AntiCheatClient = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatClient_RemoveNotifyMessageToServer(AntiCheatClient, NotificationId);
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 客户端句柄为空"));
	}
}

//Server
bool FEOSACSystem::Server_Check()
{
	EOS_HAntiCheatServer AntiCheatServer = nullptr;
	if (!FPlatform::IsInitialized())
	{
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK Server] 服务器请先初始化"));
		return false;
	}

	if (FPlatform::GetPlatformHandle() != nullptr)
	{
		AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
	}

	if (AntiCheatServer != nullptr)
	{
		return true;
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK Server] FPlatform::GetPlatformHandle 为空 "));
		return false;
	}
}

void FEOSACSystem::Server_BeginSession(FString serverName, FServerToClientEosAddPacketDelegate& STCEosAddPacketDelegate, FEACServerKickerDelegate& KickerPlayerDelegate) {
	if (Server_Check())
	{
		EacStcEosAddPacketDelegate = STCEosAddPacketDelegate;
		EacKickerDelegate = KickerPlayerDelegate;

		Server_AddNotifyMessageToClient();
		Server_AddNotifyClientActionRequired();

		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_BeginSessionOptions BeginSessionOptions = {};
		BeginSessionOptions.ApiVersion = EOS_ANTICHEATSERVER_BEGINSESSION_API_LATEST;
		BeginSessionOptions.RegisterTimeoutSeconds = EOS_ANTICHEATSERVER_BEGINSESSION_MAX_REGISTERTIMEOUT;
		BeginSessionOptions.bEnableGameplayData = true;
		BeginSessionOptions.LocalUserId = nullptr;
		const char* c_servername = TCHAR_TO_ANSI(*serverName);
		BeginSessionOptions.ServerName = c_servername;
		EOS_EResult result = EOS_AntiCheatServer_BeginSession(AntiCheatServer, &BeginSessionOptions);
		if (result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器 BeginSession 失败: %d"), result);
		}
		else {
			UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 服务器 BeginSession 成功"));
		}

	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
	}
}

void FEOSACSystem::Server_EndSession()
{
	if (Server_Check())
	{
		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_EndSessionOptions EndSessionOptions = { EOS_ANTICHEATSERVER_ENDSESSION_API_LATEST };
		EOS_EResult result = EOS_AntiCheatServer_EndSession(AntiCheatServer, &EndSessionOptions);
		if (result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器 EndSession 失败 "));
		}
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
	}
}

bool FEOSACSystem::Server_RegisterClient(UNetConnection* ClientConnection, FString ProductUserID) {
	if (Server_Check())
	{
		if (ClientConnection == nullptr) {
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 注册客户端失败，无效的连接"));
			return false;
		}

		FString Accountid = ProductUserID;

		if (Accountid.IsEmpty())
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 注册客户端失败 ProductUserId 为空"));
			return false;
		}

		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_RegisterClientOptions RegisterClientOptions = {};
		RegisterClientOptions.ApiVersion = EOS_ANTICHEATSERVER_REGISTERCLIENT_API_LATEST;
		RegisterClientOptions.UserId = EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*Accountid));
		RegisterClientOptions.ClientType = EOS_EAntiCheatCommonClientType::EOS_ACCCT_ProtectedClient;
		RegisterClientOptions.ClientPlatform = EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Windows;
		RegisterClientOptions.ClientHandle = (EOS_AntiCheatCommon_ClientHandle)ClientConnection;
		EOS_EResult RegisterResult = EOS_AntiCheatServer_RegisterClient(AntiCheatServer, &RegisterClientOptions);

		if (RegisterResult == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] EOS_AntiCheatServer_RegisterClient 成功 %s"), *Accountid);
			return true;
		}
		else {
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] EOS_AntiCheatServer_RegisterClient 失败 %d"), RegisterResult);
			return false;
		}
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
		return false;
	}
}

void FEOSACSystem::Server_UnregisterClient(UNetConnection* ClientConnection) {
	if (Server_Check())
	{
		if (ClientConnection == nullptr)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 注销客户端失败，无效的连接"));
			return;
		}
		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_UnregisterClientOptions UnregisterClientOptions = {};
		UnregisterClientOptions.ApiVersion = EOS_ANTICHEATSERVER_UNREGISTERCLIENT_API_LATEST;
		UnregisterClientOptions.ClientHandle = (EOS_AntiCheatCommon_ClientHandle)ClientConnection;
		EOS_EResult result = EOS_AntiCheatServer_UnregisterClient(AntiCheatServer, &UnregisterClientOptions);
		if (result == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] EOS_AntiCheatServer_UnregisterClient 成功"));
		}
		else {
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] EOS_AntiCheatServer_UnregisterClient 失败 %d"), result);
		}
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
	}
}

void FEOSACSystem::Server_ReceiveMessageFromClient(const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* ClientConnection) {
	if (GIsServer)
	{
		if (Server_Check())
		{
			if (FEOSACSystem::GetInstance()->EnDataChannelLog)
			{
				UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 服务器收到客户端数据 "));
			}
			EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
			EOS_AntiCheatServer_ReceiveMessageFromClientOptions Options = {};
			Options.ApiVersion = EOS_ANTICHEATSERVER_RECEIVEMESSAGEFROMCLIENT_API_LATEST;
			Options.Data = EosPacket.GetData();
			Options.DataLengthBytes = nLength;
			Options.ClientHandle = (EOS_AntiCheatCommon_ClientHandle)ClientConnection;
			EOS_EResult result = EOS_AntiCheatServer_ReceiveMessageFromClient(AntiCheatServer, &Options);
			if (result != EOS_EResult::EOS_Success)
			{
				if (FEOSACSystem::GetInstance()->EnDataChannelLog)
				{
					UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器收到客户端数据 EOS_EResult:%d"), result);
				}
			}
			else
			{
				
			}
		}
		else {
			if (FEOSACSystem::GetInstance()->EnDataChannelLog)
			{
				UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
			}
		}
	}
}

void FEOSACSystem::Server_AddNotifyMessageToClient()
{
	if (Server_Check())
	{
		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_AddNotifyMessageToClientOptions Options = {};
		Options.ApiVersion = EOS_ANTICHEATSERVER_ADDNOTIFYMESSAGETOCLIENT_API_LATEST;
		EOS_AntiCheatServer_AddNotifyMessageToClient(AntiCheatServer, &Options, NULL, FEOSACSystem::Server_OnMessageToClientCallbackFn);
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
	}
}

void FEOSACSystem::Server_OnMessageToClientCallbackFn(const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Data) {
	if (FEOSACSystem::GetInstance()->EnDataChannelLog)
	{
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] 服务器给客户端发数据"));
	}
	UNetConnection* ClientConnection = (UNetConnection*)Data->ClientHandle;
	if (ClientConnection == nullptr)
	{
		if (FEOSACSystem::GetInstance()->EnDataChannelLog)
		{
			UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器给客户端发数据失败, 无效的连接"));
		}
		return;
	}

	TArray<uint8> EosPacket;
	uint8* vals = (uint8*)Data->MessageData;
	EosPacket.Append(vals, Data->MessageDataSizeBytes);
	FEOSACSystem::GetInstance()->EacStcEosAddPacketDelegate.ExecuteIfBound(EosPacket, Data->MessageDataSizeBytes, ClientConnection);
}

void FEOSACSystem::Server_OnClientActionRequiredCallbackFn(const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Data)
{
	EOS_AntiCheatCommon_ClientHandle clientHandle = Data->ClientHandle;
	EOS_EAntiCheatCommonClientAction action = Data->ClientAction;
	EOS_EAntiCheatCommonClientActionReason reason = Data->ActionReasonCode;
	FString detail = Data->ActionReasonDetailsString;
	UNetConnection* ClientConn = (UNetConnection*)clientHandle;

	if (action == EOS_EAntiCheatCommonClientAction::EOS_ACCCA_RemovePlayer)
	{
		UE_LOG(LogEOSAC, Warning, TEXT("[EOS SDK] EOS_ACCCA_RemovePlayer"));

		FEOSACSystem::GetInstance()->EacKickerDelegate.ExecuteIfBound((int)reason, detail, ClientConn);
	}
}

void FEOSACSystem::Server_RemoveNotifyMessageToClient(EOS_NotificationId NotificationId)
{
	if (Server_Check())
	{
		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_RemoveNotifyMessageToClient(AntiCheatServer, NotificationId);
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
	}
}

void FEOSACSystem::Server_AddNotifyClientActionRequired()
{
	if (Server_Check())
	{
		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_AddNotifyClientActionRequiredOptions Options = { EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTACTIONREQUIRED_API_LATEST };
		EOS_AntiCheatServer_AddNotifyClientActionRequired(AntiCheatServer, &Options, NULL, FEOSACSystem::Server_OnClientActionRequiredCallbackFn);
	}
	else
	{
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
	}
}

void FEOSACSystem::Server_RemoveNotifyClientActionRequired(EOS_NotificationId NotificationId)
{
	if (Server_Check())
	{
		EOS_HAntiCheatServer AntiCheatServer = EOS_Platform_GetAntiCheatServerInterface(FPlatform::GetPlatformHandle());
		EOS_AntiCheatServer_RemoveNotifyClientActionRequired(AntiCheatServer, NotificationId);
	}
	else {
		UE_LOG(LogEOSAC, Error, TEXT("[EOS SDK] 服务器句柄为空"));
	}
}

#endif
