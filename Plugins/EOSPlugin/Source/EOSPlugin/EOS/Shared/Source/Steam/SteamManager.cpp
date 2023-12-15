// Copyright Epic Games, Inc. All Rights Reserved.

#ifdef EOS_STEAM_ENABLED
#include "SteamManager.h"
#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "Users.h"
#include "CommandLine.h"
#include "steam/steam_api.h"
#include "../service/EOSACSystem.h"
//#include "../../../ResonanceAudio/Source/ResonanceAudio/Private/ResonanceAudioLibrary/resonance_audio/base/integral_types.h"

// Steam Manager Implementation
class FSteamManager::FImpl
{
public:
	FImpl()
	{
		
	}

	~FImpl()
	{
		
	}

	void Init();
	void Update();
	void RetrieveEncryptedAppTicket();
	void OnRequestEncryptedAppTicket(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure);
	void StartLogin();
	void OnGameEvent(const FGameEvent& Event);

private:
	CCallResult<FSteamManager::FImpl, EncryptedAppTicketResponse_t> SteamCallResultEncryptedAppTicket;
	std::wstring EncryptedSteamAppTicket;
	bool bIsInitialized = false;
	/** Authentication component */
	std::shared_ptr<FAuthentication> Authentication;
};

void FSteamManager::FImpl::OnGameEvent(const FGameEvent& Event) {
	if (Authentication)
	{
		FDebugLog::Log(L"FSteamManager call Init() %d",Event.GetType());
		Authentication->OnGameEvent(Event);
	}
}

void FSteamManager::FImpl::Init()
{
	if (!Authentication)
	{
		Authentication = std::make_shared<FAuthentication>();
	}

	FDebugLog::Log(L"FSteamManager call Init()");

	if (SteamAPI_Init())
	{
		FDebugLog::Log(L"Steam - Initialized");

		bIsInitialized = true;

		RetrieveEncryptedAppTicket();
	}
	else
	{
		FDebugLog::LogError(L"Steam must be running to play this game (SteamAPI_Init() failed)");
	}
}

void FSteamManager::FImpl::Update()
{
	if (!bIsInitialized)
	{
		return;
	}

	// Run Steam client callbacks
	SteamAPI_RunCallbacks();
}

void FSteamManager::FImpl::RetrieveEncryptedAppTicket()
{
	if (!bIsInitialized)
	{
		return;
	}

	if (!EncryptedSteamAppTicket.empty())
	{
		StartLogin();
	}
	else
	{
		FDebugLog::LogWarning(L"Steam -  RequestEncryptedAppTicket");
		SteamAPICall_t SteamAPICallHandle = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
		SteamCallResultEncryptedAppTicket.Set(SteamAPICallHandle, this, &FSteamManager::FImpl::OnRequestEncryptedAppTicket);
	}
}

void FSteamManager::FImpl::OnRequestEncryptedAppTicket(EncryptedAppTicketResponse_t* EncryptedAppTicketResponse, bool bIOFailure)
{
	FDebugLog::Log(L"Steam - OnRequestEncryptedAppTicket Callback");

	if (bIOFailure)
	{
		FDebugLog::LogError(L"Steam - OnRequestEncryptedAppTicket Callback - Failure Requesting Ticket");
		// 回调客户端登失败
#if USE_EOSAC_SYSTEM 
		if (FEOSACSystem::GetInstance())
		{
			FEOSACSystem::GetInstance()->Client_CallLoginCallBackFn(0, "", "Steam - OnRequestEncryptedAppTicket Callback - Failure Requesting Ticket");
		}
#endif
		return;
	}

	if (EncryptedAppTicketResponse->m_eResult == k_EResultOK)
	{
		// Get ticket size
		uint32 TicketSize = 0;
		SteamUser()->GetEncryptedAppTicket(nullptr, 0, &TicketSize);

		// Get encrypted app ticket
		uint32 BufSize = TicketSize;
		uint8* SteamAppTicket = new uint8[BufSize];
		if (!SteamUser()->GetEncryptedAppTicket(SteamAppTicket, BufSize, &TicketSize))
		{
			FDebugLog::LogError(L"Steam App Ticket not available");
			delete[] SteamAppTicket;
#if USE_EOSAC_SYSTEM 
			// 回调客户端登失败
			if (FEOSACSystem::GetInstance())
			{
				FEOSACSystem::GetInstance()->Client_CallLoginCallBackFn(0, "", "Steam App Ticket not available");
			}
			
#endif
			return;
		}

		uint32 StringBufSize = (TicketSize * 2) + 1;
		char* SteamAppTicketString = new char[StringBufSize];
		uint32_t OutLen = StringBufSize;
		EOS_EResult ConvResult = EOS_ByteArray_ToString(SteamAppTicket, BufSize, SteamAppTicketString, &OutLen);
		if (ConvResult != EOS_EResult::EOS_Success)
		{
			delete[] SteamAppTicket;
			delete[] SteamAppTicketString;
			FDebugLog::LogError(L"Steam - OnRequestEncryptedAppTicket Callback - EOS_ByteArray_ToString Failed - Result: %ls", FStringUtils::Widen(EOS_EResult_ToString(ConvResult)).c_str());
#if USE_EOSAC_SYSTEM
			// 回调客户端登失败
			if (FEOSACSystem::GetInstance())
			{
				FEOSACSystem::GetInstance()->Client_CallLoginCallBackFn(0, "", "Steam - OnRequestEncryptedAppTicket Callback - EOS_ByteArray_ToString Failed");
			}
#endif
			return;
		}

		std::string NarrowSteamAppTicketString = SteamAppTicketString;
		EncryptedSteamAppTicket = FStringUtils::Widen(NarrowSteamAppTicketString);

		delete[] SteamAppTicket;
		delete[] SteamAppTicketString;

		// 验证加密票据
		StartLogin();
	}
	else if (EncryptedAppTicketResponse->m_eResult == k_EResultLimitExceeded)
	{
		FDebugLog::LogError(L"Steam - OnRequestEncryptedAppTicket Callback - Calling RequestEncryptedAppTicket more than once per minute returns this error");
#if USE_EOSAC_SYSTEM 
		// 回调客户端登失败
		if (FEOSACSystem::GetInstance())
		{	
			FEOSACSystem::GetInstance()->Client_CallLoginCallBackFn(0, "", "Steam - Calling RequestEncryptedAppTicket more than once per minute returns this error");
		}
#endif
	}
	else if (EncryptedAppTicketResponse->m_eResult == k_EResultDuplicateRequest)
	{
		FDebugLog::LogError(L"Steam - OnRequestEncryptedAppTicket Callback - Calling RequestEncryptedAppTicket while there is already a pending request results in this error");
#if USE_EOSAC_SYSTEM 
		// 回调客户端登失败
		if (FEOSACSystem::GetInstance())
		{
			FEOSACSystem::GetInstance()->Client_CallLoginCallBackFn(0, "", "Steam - Calling RequestEncryptedAppTicket while there is already a pending request results in this error");
		}
#endif
	}
	else if (EncryptedAppTicketResponse->m_eResult == k_EResultNoConnection)
	{
		FDebugLog::LogError(L"Steam - OnRequestEncryptedAppTicket Callback - Calling RequestEncryptedAppTicket while not connected to steam results in this error");
#if USE_EOSAC_SYSTEM 
		// 回调客户端登失败
		if (FEOSACSystem::GetInstance())
		{
			FEOSACSystem::GetInstance()->Client_CallLoginCallBackFn(0, "", "Steam - Calling RequestEncryptedAppTicket while not connected to steam results in this error");
		}
#endif
	}
	else {
		FDebugLog::LogError(L"Steam - OnRequestEncryptedAppTicket Callback - other error %d", EncryptedAppTicketResponse->m_eResult);
#if USE_EOSAC_SYSTEM 
		// 回调客户端登失败
		if (FEOSACSystem::GetInstance())
		{
			FEOSACSystem::GetInstance()->Client_CallLoginCallBackFn(0, "", "Steam - Other Error");
		}
#endif
	}
}

void FSteamManager::FImpl::StartLogin()
{
	FDebugLog::Log(L"Steam - StartLogin");
	if (!EncryptedSteamAppTicket.empty())
	{
		FDebugLog::Log(L"Steam - StartLogin - Ticket: %ls", EncryptedSteamAppTicket.c_str());

		FGameEvent Event(EGameEventType::StartUserLogin, EncryptedSteamAppTicket, (int)ELoginMode::ExternalAuth, (int)ELoginExternalType::Steam);
		OnGameEvent(Event);
	}
	else
	{
		FDebugLog::LogError(L"Steam - StartLogin - Invalid Steam App Ticket");
	}
}



std::unique_ptr<FSteamManager> FSteamManager::Instance;

FSteamManager::FSteamManager()
	: Impl(new FImpl())
{

}

FSteamManager::~FSteamManager()
{

}

FSteamManager& FSteamManager::GetInstance()
{
	if (!Instance)
	{
		Instance = std::unique_ptr<FSteamManager>(new FSteamManager());
	}

	return *Instance;
}

void FSteamManager::ClearInstance()
{
	Instance.reset();
}

void FSteamManager::Init()
{
	Impl->Init();
}

void FSteamManager::Update()
{
	Impl->Update();
}

void FSteamManager::RetrieveEncryptedAppTicket()
{
	Impl->RetrieveEncryptedAppTicket();
}

void FSteamManager::StartLogin()
{
	Impl->StartLogin();
}

void FSteamManager::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserInfoRetrieved)
	{
		FEpicAccountId UserId = Event.GetUserId();

		// Log Steam Display Name
		std::wstring DisplayName = L"";
//		std::wstring DisplayName = FGame::Get().GetUsers()->GetExternalAccountDisplayName(UserId, UserId, EOS_EExternalAccountType::EOS_EAT_STEAM);
		if (!DisplayName.empty())
		{
			FDebugLog::Log(L"[EOS SDK] External Account Display Name: %ls", DisplayName.c_str());
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] External Account Display Name Not Found");
		}
	}
	else
	{
		Impl->OnGameEvent(Event);
	}
}
#endif //EOS_STEAM_ENABLED