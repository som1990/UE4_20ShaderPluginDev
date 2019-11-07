// Fill out your copyright notice in the Description page of Project Settings.

#include "CSTestSim.h"
#include "Public/Stats/Stats2.h"
#include "Public/SceneUtils.h"
DECLARE_LOG_CATEGORY_EXTERN(ComputeLog, Log, All);
DEFINE_LOG_CATEGORY(ComputeLog);
// Sets default values
ACSTestSim::ACSTestSim()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	prevU = prevV = 0.f;
	texSizeX = 0;
	texSizeY = 0;
	bIsTextureGeneratingExecuting = false;
	bIsTextureDimensionsSet = false;
	InputTexture = NULL;
	Texture2Display = NULL;
	ObsTexture = NULL;
	FlowTexture = NULL;
}

// Called when the game starts or when spawned
void ACSTestSim::BeginPlay()
{
	Super::BeginPlay();
	texSizeX = FMath::RoundUpToPowerOfTwo(EWaveConfig.TexMapSize.X);
	texSizeY = FMath::RoundUpToPowerOfTwo(EWaveConfig.TexMapSize.Y);
	//texSizeX = FMath::RoundUpToPowerOfTwo(512);
	//texSizeY = FMath::RoundUpToPowerOfTwo(512);

	testComputeShader = new FComputeTestExecute(texSizeX, texSizeY, GetWorld()->Scene->GetFeatureLevel());
	testDisplayShader = new FDisplayShaderExecute(texSizeX, texSizeY, GetWorld()->Scene->GetFeatureLevel());
}


void ACSTestSim::BeginDestroy()
{
	Super::BeginDestroy();

	if (InputTexture)
	{
		InputTexture.SafeRelease();
		InputTexture = NULL;
	}
	if (Texture2Display)
	{
		Texture2Display->ConditionalBeginDestroy();
		Texture2Display = NULL;
	}
	if (ObsTexture)
	{
		ObsTexture.SafeRelease();
		ObsTexture = NULL;
	}
	if (FlowTexture)
	{
		FlowTexture.SafeRelease();
		FlowTexture = NULL;
	}
	
	if (testComputeShader) {
		delete testComputeShader;
	}
	if (testDisplayShader)
	{
		delete testDisplayShader;
	}
}


//DECLARE_STATS_GROUP(TEXT("SIM"), STATGROUP_SIM, STATCAT_Advanced);
//DECLARE_CYCLE_STAT(TEXT("Total CalcTime"), STAT_TotSimTime, STATGROUP_SIM);

void ACSTestSim::LoadHeightMapSource(
	float _mag, float _delTime,
	UTexture2D* SourceMap, UTexture2D* ObsMap, 
	UTexture2D* FlowMap, FColor DisplayColor, UTextureRenderTarget2D* InRenderTarget)
{
	//SCOPE_CYCLE_COUNTER(STAT_TotSimTime);
	check(IsInGameThread());
	if (!RenderTarget2Display || !NormalMapRenderTarget)
	{
		UE_LOG(ComputeLog, Error, TEXT("RenderTarget2Display and/or NormalMapRenderTarget not provided. Returning"));

		return;
	}

	EWaveConfig.magnitude = _mag;
	EWaveConfig.delTime = _delTime;

	bUseObsMap = true;
	bUseFlowMap = true;
	int sizeX, sizeY;
	if (EWaveConfig.bUseRenderTarget)
	//if(true)
	{
		sizeX = InRenderTarget->SizeX;
		sizeY = InRenderTarget->SizeY;
	}
	else
	{
		sizeX = SourceMap->GetSizeX();
		sizeY = SourceMap->GetSizeY();
	}
	if (sizeX != texSizeX || sizeY != texSizeY )
	{
		UE_LOG(ComputeLog, Error, TEXT("Set Dimensions don't match input texture map. Returning."));
		bIsTextureDimensionsSet = false;
		return;
	}
	
	if (ObsMap)
	{
		if (ObsMap->GetSizeX() != texSizeX || ObsMap->GetSizeY() != texSizeY)
		{
			//UE_LOG(ComputeLog, Warning, TEXT("Correct Obstruction Map Dimensions not provided"));
			bUseObsMap = false;
		}
	}
	else {
		//UE_LOG(ComputeLog, Warning, TEXT("Obstruction Map not provided"));

		bUseObsMap = false;
	}

	if (FlowMap)
	{
		if (FlowMap->GetSizeX() != texSizeX || FlowMap->GetSizeY() != texSizeY)
		{
			//UE_LOG(ComputeLog, Warning, TEXT("Correct Flow Map Dimensions not provided"));
			bUseFlowMap = false;
		}

	}
	else {
		//UE_LOG(ComputeLog, Warning, TEXT("Flow Map not provided"));
		bUseFlowMap = false;
	}
	if (!bIsTextureDimensionsSet)
	{
		//UE_LOG(ComputeLog, Warning, TEXT("TextureDimensions are not Set. Returning"));
		return;
	}
	//GEngine->AddOnScreenDebugMessage(-1, 0.2, FColor::Yellow, TEXT("Magnitude: ") + FString::SanitizeFloat(_magnitude));
	
	if (InputTexture != NULL)
	{
		InputTexture.SafeRelease();
		InputTexture = NULL;
		//UE_LOG(ComputeLog, Warning, TEXT("InputTexture Reset."));
	}

	if (ObsTexture != NULL)
	{
		ObsTexture.SafeRelease();
		ObsTexture = NULL;
		//UE_LOG(ComputeLog, Warning, TEXT("Obstruction Texture Reset."));
	}
	
	if (FlowTexture != NULL)
	{
		FlowTexture.SafeRelease();
		FlowTexture = NULL;
		//UE_LOG(ComputeLog, Warning, TEXT("Flow Texture Reset."));

	}

	if (bUseObsMap)
	{
		ObsTexture = static_cast<FTexture2DResource*>(ObsMap->Resource)->GetTexture2DRHI();
	}

	if (bUseFlowMap)
	{
		FlowTexture = static_cast<FTexture2DResource*>(FlowMap->Resource)->GetTexture2DRHI();
	}

	if (SourceMap)
	{
		InputTexture = static_cast<FTexture2DResource*>(SourceMap->Resource)->GetTexture2DRHI();
	}
	//UE_LOG(ComputeLog, Warning, TEXT("RHITexture2D Extracted, Dimensions: %d, %d"), SourceMap->PlatformData->Mips[0].SizeX, SourceMap->PlatformData->Mips[0].SizeY);
	if (InputTexture || InRenderTarget)
	{
		//FTexture2DResource* uTex2DRes = static_cast<FTexture2DResource*>(SourceMap->Resource);
		
		testComputeShader->ExecuteComputeShader(
			InRenderTarget,InputTexture, ObsTexture, FlowTexture, DisplayColor, EWaveConfig);
		
		//FlushRenderingCommands();
		//UE_LOG(ComputeLog, Warning, TEXT("Compute Shader Executed."));
		
		testDisplayShader->ExecuteDisplayShader(RenderTarget2Display, NormalMapRenderTarget, testComputeShader->GetTexture(), EWaveConfig);
		//UE_LOG(ComputeLog, Warning, TEXT("RenderTarget Generated."));
			
	}
}

void ACSTestSim::GeneratePreviewTexture(UTexture2D* &OutTexture)
{
	if (!bIsTextureDimensionsSet)
	{
		UE_LOG(ComputeLog, Warning, TEXT("TextureDimensions are not Set. Returning"));
		return;
	}
	if (bIsTextureGeneratingExecuting)
		return;
	bIsTextureGeneratingExecuting = true;
 	auto temp = testComputeShader->GetTexture();
 	if (temp)
 	{
		UE_LOG(ComputeLog, Warning, TEXT("ComputeShaderActive "));
		Texture2Display = UTexture2D::CreateTransient(temp->GetSizeX(), temp->GetSizeY(), PF_B8G8R8A8);
 		RHIRef2Texture2D(temp, Texture2Display);
 	}
 	bIsTextureGeneratingExecuting = false;
 	OutTexture = Texture2Display;
}

void ACSTestSim::setOutputDimensions(int32 xSize, int32 ySize)
{
	bIsTextureDimensionsSet = false;
	
	//UE_LOG(ComputeLog, Warning, TEXT("SourceTexture, Dimensions: %d, %d"), xSize, ySize);
	if ((xSize < 1) || (ySize < 1))
	{
		UE_LOG(ComputeLog, Warning, TEXT("One or Both Input Dimensions are 0.0. Returning"));
		return;
	}
	if (testComputeShader)
	{
		delete testComputeShader;
	}
	if (testDisplayShader)
	{
		delete testDisplayShader;
	}
	testComputeShader = new FComputeTestExecute(xSize, ySize, GetWorld()->Scene->GetFeatureLevel());
	testDisplayShader = new FDisplayShaderExecute(xSize, ySize, GetWorld()->Scene->GetFeatureLevel());
	texSizeX = xSize;
	texSizeY = ySize;
	EWaveConfig.TexMapSize = FVector2D(texSizeX, texSizeY);
	UE_LOG(ComputeLog, Warning, TEXT("OutTexture Extracted, Dimensions: %d, %d"), xSize, ySize);
	bIsTextureDimensionsSet = true;
	
}

void ACSTestSim::RHIRef2Texture2D(FTexture2DRHIRef rhiTexture, UTexture2D* OutTexture)
{
	
	struct FReadSurfaceContext {
		FTexture2DRHIRef Texture;
		TArray<FColor>* OutData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};
	TArray<FColor> OutPixels;
	OutPixels.Reset();
	FReadSurfaceContext ReadSurfaceContext = { 
		rhiTexture, &OutPixels, 
		FIntRect(0,0,rhiTexture->GetSizeX(), rhiTexture->GetSizeY()), 
		FReadSurfaceDataFlags() 
	};
	UE_LOG(ComputeLog, Warning, TEXT("ReadSurfaceContext created."));
	ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand) (
	[ReadSurfaceContext](FRHICommandListImmediate& RHICmdList)
		{
		check(IsInRenderingThread());
		RHICmdList.ReadSurfaceData(
			ReadSurfaceContext.Texture,
			ReadSurfaceContext.Rect,
			*ReadSurfaceContext.OutData,
			ReadSurfaceContext.Flags
		);
		}
	);
	FlushRenderingCommands();
	UE_LOG(ComputeLog, Warning, TEXT("PixelArrayCreated"));
	uint8* MipData = static_cast<uint8*>(OutTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	int32 Rows = OutTexture->GetSizeY();
	int32 Cols = OutTexture->GetSizeX();

	uint8* DestPtr = NULL;
	const FColor* SrcPtr = NULL;

	for (int32 rowNum = 0; rowNum < Rows; ++rowNum)
	{
		int idx = (rowNum)*Cols ;
		DestPtr = &MipData[idx * sizeof(FColor)];
		SrcPtr = const_cast<FColor*>(&OutPixels[idx]);
		for (int32 colNum = 0; colNum < Cols; ++colNum)
		{
			
			*DestPtr++ = SrcPtr->B;
			*DestPtr++ = SrcPtr->G;
			*DestPtr++ = SrcPtr->R;
			*DestPtr++ = SrcPtr->A;

			SrcPtr++;
		}
	}
	OutTexture->PlatformData->Mips[0].BulkData.Unlock();
	OutTexture->UpdateResource();
	UE_LOG(ComputeLog, Warning, TEXT("Texture2DCreated"));
}

// Called every frame
void ACSTestSim::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

