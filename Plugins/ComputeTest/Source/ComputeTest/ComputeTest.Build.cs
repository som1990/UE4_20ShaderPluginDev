// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using System.IO;
using UnrealBuildTool;

public class ComputeTest : ModuleRules
{
    public ComputeTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "Private/ComputeTestPrivatePCH.h";

        PublicIncludePaths.AddRange(
            new string[] {
			// ... add public include paths required here ...
            Path.Combine(ModuleDirectory, "Public")
            //"ComputeTest/Public",
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
			// ... add other private include paths required here ...
            //Path.Combine(ModuleDirectory,"Private")
            "ComputeTest/Private",
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "ShaderCore",
            "RHI"
                // ... add other public dependencies that you statically link with here ...
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
            "CoreUObject",
            "Engine",


                // ... add private dependencies that you statically link with here ...	
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}
