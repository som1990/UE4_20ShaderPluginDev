// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ComputeTestPrivatePCH.h"

class FComputeTestModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;



	static inline FComputeTestModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FComputeTestModule >("ComputeTest");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ComputeTest");
	}
};

