using System;
using System.Globalization;
using UnrealBuildTool;
using System.IO;

public class EOSPlugin : ModuleRules
{
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../ThirdParty/")); }
    }

    private string GetEosSDKIncludePath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "EOS/")); }
    }

    private string GetEosSDKLibPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../EOSSDK/Lib/")); }
    }

    public bool LoadEosSDKLib(ReadOnlyTargetRules Target)
    {
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string LibName = "EOSSDK-Win64-Shipping.lib";
            System.Console.WriteLine("----- lib true path:" + Path.Combine(GetEosSDKLibPath, LibName));
            PublicAdditionalLibraries.Add(Path.Combine(GetEosSDKLibPath, LibName));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string LibName = "libEOSSDK-Linux-Shipping.so";
            System.Console.WriteLine("----- lib true path:" + Path.Combine(GetEosSDKLibPath, LibName));
            PublicAdditionalLibraries.Add(Path.Combine(GetEosSDKLibPath, LibName));
        }

        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "EOS")));
        System.Console.WriteLine("----- include true path:1 " + Path.GetFullPath(Path.Combine(ModuleDirectory, "EOS")));

        PublicIncludePaths.Add(Path.Combine(GetEosSDKIncludePath, "Include"));
        System.Console.WriteLine("----- include true path:3 " + Path.Combine(GetEosSDKIncludePath, "Include"));

        PublicIncludePaths.Add(Path.Combine(GetEosSDKIncludePath, "Shared\\Source"));
        System.Console.WriteLine("----- include true path:2 " + Path.Combine(GetEosSDKIncludePath, "Shared\\Source"));

        PublicIncludePaths.Add(Path.Combine(GetEosSDKIncludePath, "Shared\\Source\\Core"));
        System.Console.WriteLine("----- include true path:4 " + Path.Combine(GetEosSDKIncludePath, "Shared\\Source\\Core"));

        PublicIncludePaths.Add(Path.Combine(GetEosSDKIncludePath, "Shared\\Source\\Steam"));
        System.Console.WriteLine("----- include true path:5 " + Path.Combine(GetEosSDKIncludePath, "Shared\\Source\\Steam"));

        PublicIncludePaths.Add(Path.Combine(GetEosSDKIncludePath, "Shared\\Source\\Utils"));
        System.Console.WriteLine("----- include true path:6 " + Path.Combine(GetEosSDKIncludePath, "Shared\\Source\\Utils"));

        return true;
    }

    public EOSPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableExceptions = true;

        LoadEosSDKLib(Target);

        PublicIncludePaths.AddRange(
            new string[] {	
                // ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
            }
			);

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);


        bool EnableEos = false;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            EnableEos = true;
        }

        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            EnableEos = true;
        }

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            EnableEos = false;
        }

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            EnableEos = false;
        }

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            EnableEos = false;
        }

        if (EnableEos)
        {
            //EosAntiCheat反外挂 1 开启，0关闭
            PublicDefinitions.Add("USE_EOSAC_SYSTEM=1");
            PublicDefinitions.Add("ALLOW_RESERVED_PLATFORM_OPTIONS=1");
            PublicDefinitions.Add("EOS_STEAM_ENABLED=1");
        }
        else
        {
            //EosAntiCheat反外挂 1 开启，0关闭
            PublicDefinitions.Add("USE_EOSAC_SYSTEM=0");
            PublicDefinitions.Add("ALLOW_RESERVED_PLATFORM_OPTIONS=0");
            PublicDefinitions.Add("EOS_STEAM_ENABLED=0");
        }
    }
}
