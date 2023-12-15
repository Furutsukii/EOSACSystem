#pragma once

#include "Core.h"
#include "Engine/EngineTypes.h"


#if USE_EOSAC_SYSTEM 
#include "../interface/EOSACSystemAPI.h"
#include "../EOS/Include/eos_sdk.h"
#include "../EOS/Include/eos_anticheatclient.h"
#include "../EOS/Include/eos_anticheatserver.h"
#include "../EOS/Include/eos_anticheatclient_types.h"
#include "../EOS/Shared/Source/Core/Platform.h"
#include "../EOS/EosConstants.h"
#include "../EOS/Include/eos_init.h"
#include "../EOS/Include/eos_logging.h"
#include "../EOS/Shared/Source/Utils/DebugLog.h"
#include "../EOS/Shared/Source/Steam/SteamManager.h"
#endif

#if USE_EOSAC_SYSTEM 
class FEOSACSystem : public IEOSACSystem
{

public:
	static FEOSACSystem* GetInstance()
	{
		static FEOSACSystem sFEOSSystem;
		return &sFEOSSystem;
	}

	// 客户端与服务器
	virtual bool PlatformInit();
	virtual void Tick(float DeltaSeconds);
	virtual void PollStatus();

	// 客户端调用
	virtual bool Client_Check();
	virtual bool Client_IsServiceInstalled();
	virtual bool Check_Start_Protected_Game();
	virtual FString Client_GetProductUserID();
	virtual void Add_ClientViolation_Notification(FClientViolationDelegate& EACViolationDelegate);
	virtual void Client_CallLogin(FEACLoginDelegate& EACLoginDelegate);
	virtual bool Client_BeginSession(FCTSEosAddPacketDelegate& EosAddPacketDelegate, const FString& ProductUserID, UNetConnection* ServerConnection);
	virtual void Client_EndSession();
	virtual void Client_ReceiveMessageFromServer(const TArray<uint8>& EosPacket, const uint32 nLength);

	static void Client_CallLoginCallBackFn(int ResultCode, FString _productID, FString ErrorMsg);
	EOS_NotificationId Client_AddNotifyMessageToServer(void* clienData);
	void Client_RemoveNotifyMessageToServer(EOS_NotificationId NotificationId);
	static void Client_OnMessageToServerCallbackFn(const EOS_AntiCheatClient_OnMessageToServerCallbackInfo* Data);

	// 服务器调用
	virtual bool Server_Check();
	virtual void Server_BeginSession(FString serverName, FServerToClientEosAddPacketDelegate& STCEosAddPacketDelegate, FEACServerKickerDelegate& KickerPlayerDelegate);
	virtual void Server_EndSession();
	virtual bool Server_RegisterClient(UNetConnection* ClientConnection, FString ProductUserID);
	virtual void Server_UnregisterClient(UNetConnection* ClientConnection);
	virtual void Server_ReceiveMessageFromClient(const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* ClientConnection);

	void Server_AddNotifyMessageToClient();
	void Server_RemoveNotifyMessageToClient(EOS_NotificationId NotificationId);
	void Server_AddNotifyClientActionRequired();
	void Server_RemoveNotifyClientActionRequired(EOS_NotificationId NotificationId);
	static void Server_OnMessageToClientCallbackFn(const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Data);
	static void Server_OnClientActionRequiredCallbackFn(const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Data);

public:
	TWeakObjectPtr<class UNetConnection> GServerConnection = nullptr;
	TMap<FString, EOS_NotificationId> ClietnNotiMap;
	FProductUserId GlobalProductUserId;
	FString StrProductUserId = "";
	int ClientTickCount = 0;
	int ServerTickCount = 0;
	bool EnDataChannelLog = false;

	// Callback
	FClientViolationDelegate EacViolationDelegate;
	FEACLoginDelegate EacLoginDelegate;
	FCTSEosAddPacketDelegate EacCtsEosAddPacketDelegate;
	FServerToClientEosAddPacketDelegate EacStcEosAddPacketDelegate;
	FEACServerKickerDelegate EacKickerDelegate;
};
#endif

