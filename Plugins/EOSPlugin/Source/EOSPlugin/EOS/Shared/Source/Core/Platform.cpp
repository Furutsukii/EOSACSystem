// Copyright Epic Games, Inc. All Rights Reserved.

#include "Platform.h"
#include "pch.h"
#include "DebugLog.h"
#include "../Utils/CommandLine.h"
#include "StringUtils.h"
#include "EosConstants.h"
#include "GameEvent.h"
#include "../Utils/Utils.h"
#include "eos_init.h"
#include "../service/EOSACSystem.h"

#if ALLOW_RESERVED_PLATFORM_OPTIONS
//#include "ReservedPlatformOptions.h"
#endif

#ifdef _WIN32
#include "Windows/eos_Windows.h"
#endif

constexpr char EosConstants::ProductId[];
constexpr char EosConstants::SandboxId[];
constexpr char EosConstants::DeploymentId[];
constexpr char EosConstants::ClientCredentialsId[];
constexpr char EosConstants::ClientCredentialsSecret[];
constexpr char EosConstants::EncryptionKey[];

EOS_HPlatform FPlatform::PlatformHandle = nullptr;
bool FPlatform::bIsInit = false;
bool FPlatform::bIsShuttingDown = false;
bool FPlatform::bHasShownCreateFailedError = false;
bool FPlatform::bHasShownInvalidParamsErrors = false;
bool FPlatform::bHasInvalidParamProductId = false;
bool FPlatform::bHasInvalidParamSandboxId = false;
bool FPlatform::bHasInvalidParamDeploymentId = false;
bool FPlatform::bHasInvalidParamClientCreds = false;

bool FPlatform::Create(bool isServer)
{
	bIsInit = false;

	// Create platform instance
	EOS_Platform_Options PlatformOptions = {};
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	PlatformOptions.bIsServer = EOS_CommandLine::FCommandLine::Get().HasFlagParam(CommandLineConstants::Server);
	if (isServer)
	{
		PlatformOptions.bIsServer = true;
	}
	else
	{
		PlatformOptions.bIsServer = false;
	}

	PlatformOptions.EncryptionKey = EosConstants::EncryptionKey;
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;
	PlatformOptions.Flags = EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D9 | EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D10 | EOS_PF_WINDOWS_ENABLE_OVERLAY_OPENGL; // Enable overlay support for D3D9/10 and OpenGL. This sample uses D3D11 or SDL.
	PlatformOptions.CacheDirectory = FUtils::GetTempDirectory();

	UE_LOG(LogEOSAC, Log, TEXT("[EOS SDK] FPlatform::Create: isServer %d"), PlatformOptions.bIsServer);

	std::string ProductId = EosConstants::ProductId;
	std::string SandboxId = EosConstants::SandboxId;
	std::string DeploymentId = EosConstants::DeploymentId;

	// Use Command Line vars to populate vars if they exist
	std::wstring CmdProductID = EOS_CommandLine::FCommandLine::Get().GetParamValue(CommandLineConstants::ProductId);
	if (!CmdProductID.empty())
	{
		ProductId = FStringUtils::Narrow(CmdProductID).c_str();
	}
	bHasInvalidParamProductId = ProductId.empty() ? true : false;

	bHasInvalidParamSandboxId = false;
	std::wstring CmdSandboxID = EOS_CommandLine::FCommandLine::Get().GetParamValue(CommandLineConstants::SandboxId);
	if (!CmdSandboxID.empty())
	{
		SandboxId = FStringUtils::Narrow(CmdSandboxID).c_str();
	}
	bHasInvalidParamSandboxId = SandboxId.empty() ? true : false;

	std::wstring CmdDeploymentID = EOS_CommandLine::FCommandLine::Get().GetParamValue(CommandLineConstants::DeploymentId);
	if (!CmdDeploymentID.empty())
	{
		DeploymentId = FStringUtils::Narrow(CmdDeploymentID).c_str();
	}
	bHasInvalidParamDeploymentId = DeploymentId.empty() ? true : false;

	PlatformOptions.ProductId = ProductId.c_str();
	PlatformOptions.SandboxId = SandboxId.c_str();
	PlatformOptions.DeploymentId = DeploymentId.c_str();

	std::string ClientId = EosConstants::ClientCredentialsId;
	std::string ClientSecret = EosConstants::ClientCredentialsSecret;

	// Use Command Line vars to populate vars if they exist
	std::wstring CmdClientID = EOS_CommandLine::FCommandLine::Get().GetParamValue(CommandLineConstants::ClientId);
	if (!CmdClientID.empty())
	{
		ClientId = FStringUtils::Narrow(CmdClientID).c_str();
	}
	std::wstring CmdClientSecret = EOS_CommandLine::FCommandLine::Get().GetParamValue(CommandLineConstants::ClientSecret);
	if (!CmdClientSecret.empty())
	{
		ClientSecret = FStringUtils::Narrow(CmdClientSecret).c_str();
	}

	bHasInvalidParamClientCreds = false;
	if (!ClientId.empty() && !ClientSecret.empty())
	{
		PlatformOptions.ClientCredentials.ClientId = ClientId.c_str();
		PlatformOptions.ClientCredentials.ClientSecret = ClientSecret.c_str();
	}
	else if (!ClientId.empty() || !ClientSecret.empty())
	{
		bHasInvalidParamClientCreds = true;
	}
	else
	{
		PlatformOptions.ClientCredentials.ClientId = nullptr;
		PlatformOptions.ClientCredentials.ClientSecret = nullptr;
	}

	if (bHasInvalidParamProductId ||
		bHasInvalidParamSandboxId ||
		bHasInvalidParamDeploymentId ||
		bHasInvalidParamClientCreds)
	{
		return false;
	}

	EOS_Platform_RTCOptions RtcOptions = { 0 };
	RtcOptions.ApiVersion = EOS_PLATFORM_RTCOPTIONS_API_LATEST;

#ifdef _WIN32
	// Get absolute path for xaudio2_9redist.dll file
#ifdef DXTK
	wchar_t CurDir[MAX_PATH + 1] = {};
	::GetCurrentDirectoryW(MAX_PATH + 1u, CurDir);
	std::wstring BasePath = std::wstring(CurDir);
	std::string XAudio29DllPath = FStringUtils::Narrow(BasePath);
#endif
#ifdef EOS_DEMO_SDL
	std::string XAudio29DllPath = SDL_GetBasePath();
#endif // EOS_DEMO_SDL
	//	XAudio29DllPath.append("/xaudio2_9redist.dll");

	EOS_Windows_RTCOptions WindowsRtcOptions = { 0 };
	WindowsRtcOptions.ApiVersion = EOS_WINDOWS_RTCOPTIONS_API_LATEST;
	//	WindowsRtcOptions.XAudio29DllPath = XAudio29DllPath.c_str();
	RtcOptions.PlatformSpecificOptions = &WindowsRtcOptions;
#else
	RtcOptions.PlatformSpecificOptions = NULL;
#endif // _WIN32

	//	PlatformOptions.RTCOptions = &RtcOptions;

#if ALLOW_RESERVED_PLATFORM_OPTIONS
//	SetReservedPlatformOptions(PlatformOptions);
#else
	PlatformOptions.Reserved = NULL;
#endif // ALLOW_RESERVED_PLATFORM_OPTIONS

	PlatformHandle = EOS_Platform_Create(&PlatformOptions);

	if (PlatformHandle == nullptr)
	{
		return false;
	}

	bIsInit = true;

	return true;
}

void FPlatform::Release()
{
	bIsInit = false;
	PlatformHandle = nullptr;
	bIsShuttingDown = true;
}

void FPlatform::Update()
{
	if (PlatformHandle)
	{
		EOS_Platform_Tick(PlatformHandle);
	}

	if (!bIsInit && !bIsShuttingDown)
	{
		if (!bHasShownCreateFailedError)
		{
			bHasShownCreateFailedError = true;
			FDebugLog::LogError(L"[EOS SDK] Platform Create Failed!");
		}
	}

	if (bHasInvalidParamProductId ||
		bHasInvalidParamSandboxId ||
		bHasInvalidParamDeploymentId ||
		bHasInvalidParamClientCreds)
	{
		if (!bHasShownInvalidParamsErrors)
		{
			bHasShownInvalidParamsErrors = true;
			if (bHasInvalidParamProductId)
			{
				FDebugLog::LogError(L"[EOS SDK] Product Id is empty, add your product id from Epic Games DevPortal to EosConstants");
			}
			if (bHasInvalidParamSandboxId)
			{
				FDebugLog::LogError(L"[EOS SDK] Sandbox Id is empty, add your sandbox id from Epic Games DevPortal to EosConstants");
			}
			if (bHasInvalidParamDeploymentId)
			{
				FDebugLog::LogError(L"[EOS SDK] Deployment Id is empty, add your deployment id from Epic Games DevPortal to EosConstants");
			}
			if (bHasInvalidParamClientCreds)
			{
				FDebugLog::LogError(L"[EOS SDK] Client credentials are invalid, check clientid and clientsecret in EosConstants");
			}

			//			FGameEvent PopupEvent(EGameEventType::ShowPopupDialog, L"One or more parameters required for EOS_Platform_Create are invalid. Check EosConstants have been set up correctly.");
			//			FGame::Get().(PopupEvent);
		}
	}
}
