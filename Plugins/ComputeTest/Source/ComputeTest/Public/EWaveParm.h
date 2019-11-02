#pragma once
#include "Engine.h"
#include "CoreMinimal.h"
#include "EWaveParm.generated.h"

USTRUCT(BlueprintType)
struct FEWaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D TexMapSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Grid Size (in m(s))"))
	FVector2D SimGridSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float magnitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float delTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float choppyScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float flowScale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	bool bUseRenderTarget;

	/*Defaults*/
	FEWaveData()
	{
		TexMapSize = FVector2D(512, 512);
		SimGridSize = FVector2D(10, 10);
		magnitude = 1.0f;
		delTime = 0.0f;
		choppyScale = 1.3f;
		flowScale = 0.0f;
		bUseRenderTarget = true;
	}
};