#pragma once

#include "Core.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEOSAC, Log, All);

// ��¼�ص�
DECLARE_DELEGATE_ThreeParams(FEACLoginDelegate, int32 resultCode, const FString& ProductUserID, const FString& ErrorMsg);

// �ͻ���Υ��ص�
DECLARE_DELEGATE_TwoParams(FClientViolationDelegate, int32 resultCode, const FString& reason);

// �ͻ��˻ص�
DECLARE_DELEGATE_ThreeParams(FCTSEosAddPacketDelegate, const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* Connection);

// �������ص�
DECLARE_DELEGATE_ThreeParams(FServerToClientEosAddPacketDelegate, const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* Connection);

// ���˻ص�
DECLARE_DELEGATE_ThreeParams(FEACServerKickerDelegate, int32 resultCode, const FString& reason, UNetConnection* ClientConnection);


class IEOSACSystem
{
public:
	virtual ~IEOSACSystem() {}

	// ͨ�õĿͻ��˷�������ʼ��
	virtual bool PlatformInit() = 0;

	//����
	virtual void Tick(float DeltaSeconds) = 0;
	virtual void PollStatus() = 0;

	// �ͻ��˵���
	// ���EAC�ͻ��˾���Ƿ�����
	virtual bool Client_Check() = 0;
	// �����Ƿ�����
	virtual bool Client_IsServiceInstalled() = 0;
	// �����������������Ƿ����
	virtual bool Check_Start_Protected_Game() = 0;
	// ��ȡ�û�ID
	virtual FString Client_GetProductUserID() = 0;
	// �ͻ���Υ��ص�
	virtual void Add_ClientViolation_Notification(FClientViolationDelegate& EACViolationDelegate) = 0;
	// �ͻ��˵�¼ 
	virtual void Client_CallLogin(FEACLoginDelegate& EACLoginDelegate) = 0;
	// �ͻ��˿����Ự
	virtual bool Client_BeginSession(FCTSEosAddPacketDelegate& EosAddPacketDelegate, const FString& ProductUserID, UNetConnection* ServerConnection) = 0;
	// �ͻ��˽����Ự
	virtual void Client_EndSession() = 0;
	// ���շ�����������
	virtual void Client_ReceiveMessageFromServer(const TArray<uint8>& EosPacket, const uint32 nLength) = 0;

	// ������
	// ������������Ƿ�����
	virtual bool Server_Check() = 0;
	// �����������Ự
	virtual void Server_BeginSession(FString serverName, FServerToClientEosAddPacketDelegate& STCEosAddPacketDelegate, FEACServerKickerDelegate& KickerPlayerDelegate) = 0;
	// �����������Ự
	virtual void Server_EndSession() = 0;
	// �ͻ��˼����������ע��ͻ���
	virtual bool Server_RegisterClient(UNetConnection* ClientConnection, FString ProductUserID) = 0;
	// �ͻ����뿪�����������ͻ���
	virtual void Server_UnregisterClient(UNetConnection* ClientConnection) = 0;
	// ���տͻ�������
	virtual void Server_ReceiveMessageFromClient(const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* ClientConnection) = 0;
};
