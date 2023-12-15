#pragma once

#include "Core.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEOSAC, Log, All);

// 登录回调
DECLARE_DELEGATE_ThreeParams(FEACLoginDelegate, int32 resultCode, const FString& ProductUserID, const FString& ErrorMsg);

// 客户端违规回调
DECLARE_DELEGATE_TwoParams(FClientViolationDelegate, int32 resultCode, const FString& reason);

// 客户端回调
DECLARE_DELEGATE_ThreeParams(FCTSEosAddPacketDelegate, const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* Connection);

// 服务器回调
DECLARE_DELEGATE_ThreeParams(FServerToClientEosAddPacketDelegate, const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* Connection);

// 踢人回调
DECLARE_DELEGATE_ThreeParams(FEACServerKickerDelegate, int32 resultCode, const FString& reason, UNetConnection* ClientConnection);


class IEOSACSystem
{
public:
	virtual ~IEOSACSystem() {}

	// 通用的客户端服务器初始化
	virtual bool PlatformInit() = 0;

	//心跳
	virtual void Tick(float DeltaSeconds) = 0;
	virtual void PollStatus() = 0;

	// 客户端调用
	// 检查EAC客户端句柄是否正常
	virtual bool Client_Check() = 0;
	// 服务是否正常
	virtual bool Client_IsServiceInstalled() = 0;
	// 反作弊启动加载器是否加载
	virtual bool Check_Start_Protected_Game() = 0;
	// 获取用户ID
	virtual FString Client_GetProductUserID() = 0;
	// 客户端违规回调
	virtual void Add_ClientViolation_Notification(FClientViolationDelegate& EACViolationDelegate) = 0;
	// 客户端登录 
	virtual void Client_CallLogin(FEACLoginDelegate& EACLoginDelegate) = 0;
	// 客户端开启会话
	virtual bool Client_BeginSession(FCTSEosAddPacketDelegate& EosAddPacketDelegate, const FString& ProductUserID, UNetConnection* ServerConnection) = 0;
	// 客户端结束会话
	virtual void Client_EndSession() = 0;
	// 接收服务器的数据
	virtual void Client_ReceiveMessageFromServer(const TArray<uint8>& EosPacket, const uint32 nLength) = 0;

	// 服务器
	// 检查服务器句柄是否正常
	virtual bool Server_Check() = 0;
	// 服务器开启会话
	virtual void Server_BeginSession(FString serverName, FServerToClientEosAddPacketDelegate& STCEosAddPacketDelegate, FEACServerKickerDelegate& KickerPlayerDelegate) = 0;
	// 服务器结束会话
	virtual void Server_EndSession() = 0;
	// 客户端加入服务器，注册客户端
	virtual bool Server_RegisterClient(UNetConnection* ClientConnection, FString ProductUserID) = 0;
	// 客户端离开服务器，解绑客户端
	virtual void Server_UnregisterClient(UNetConnection* ClientConnection) = 0;
	// 接收客户端数据
	virtual void Server_ReceiveMessageFromClient(const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* ClientConnection) = 0;
};
