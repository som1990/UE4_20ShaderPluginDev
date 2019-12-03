// Fill out your copyright notice in the Description page of Project Settings.

#include "MyBouyancyPawn.h"
#include "Public/RHIStaticStates.h"
#include "RHICommandList.h"
#include "ObjectMacros.h"
DECLARE_LOG_CATEGORY_EXTERN(BouyancyLog, Log, All);
DEFINE_LOG_CATEGORY(BouyancyLog);

// Sets default values
AMyBouyancyPawn::AMyBouyancyPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SeekY = 0;
	SizeY = 512;
	ReadPixelsFence = new FRenderCommandFence();
	OldColor = FColor(0);
	
}

void AMyBouyancyPawn::UpdateBuffer()
{
	if (!GetWorld() || PollingHandle.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Unable to start read for render target, world is nullptr or polling handle is in use "));
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(PollingHandle, this, &AMyBouyancyPawn::CheckFence, 0.01f, true);
	ReadPixels();
}

void AMyBouyancyPawn::ReadPixels()
{
	ColorBuffer.Reset();

	int32 y = SeekY;
	int32 bottom = SeekY = SeekY + SizeY;

	if(RenderResource->GetSizeXY().Y <= bottom)
	{
		bottom = RenderResource->GetSizeXY().Y;
	}

	struct FReadSurfaceContext
	{
		FRenderTarget* SrcRenderTarget;
		TArray<FColor>* OutData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};

	
	FReadSurfaceContext* ReadSurfaceContext = new FReadSurfaceContext();

	ReadSurfaceContext->SrcRenderTarget = RenderResource;
	ReadSurfaceContext->OutData = &ColorBuffer;
	ReadSurfaceContext->Rect = FIntRect(0, y, RenderResource->GetSizeXY().X, bottom);
	ReadSurfaceContext->Flags = FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX);

	//UE_LOG(BouyancyLog, Warning, TEXT("Bouyancy Height Resource, Dimensions: %d, %d"), RenderResource->GetSizeX(), RenderResource->GetSizeY());
	if (!RenderResource)
		return;

	//FlushRenderingCommands();
	AMyBouyancyPawn* myShader = this;
	ENQUEUE_RENDER_COMMAND(ReadHeightRT)(
		[ReadSurfaceContext, myShader](FRHICommandListImmediate& RHICmdList)
	{
		FTexture2DRHIRef CurRT = myShader->RenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
		RHICmdList.ReadSurfaceData(
			CurRT,
			ReadSurfaceContext->Rect,
			*ReadSurfaceContext->OutData,
			ReadSurfaceContext->Flags
		);
	}
	);

	ReadPixelsFence->BeginFence();
}

void AMyBouyancyPawn::CheckFence()
{
	if (!RenderResource)
	{
		StopReading();
		return;
	}
	if (ReadPixelsFence->IsFenceComplete())
	{
		if (IsFinishedReading())
		{
			StopReading();
		}
		else
		{
			ReadPixels();
		}
	}
}

void AMyBouyancyPawn::StopReading()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PollingHandle);
	}
}

bool AMyBouyancyPawn::IsFinishedReading()
{
	return RenderTarget->SizeY < SeekY + SizeY;
}

FColor AMyBouyancyPawn::GetRenderTargetValue(float x, float y, FVector2D gridSize)
{
	if (RenderTarget == NULL || ColorBuffer.Num() == 0)
		return FColor(0);
	
	if (ReadPixelsFence->IsFenceComplete())
	{
		float width = RenderTarget->GetSurfaceWidth();
		float height = RenderTarget->GetSurfaceHeight();

		float normalizedX = (x / gridSize.X) + 0.5f;
		float normalizedY = (y / gridSize.Y) + 0.5f;

		int i = (int)(normalizedX * width);
		int j = (int)(normalizedY * height);

		if (i < 0) i = 0;
		if (i >= width) i = width - 1;
		if (j < 0) j = 0;
		if (j >= height) j = height - 1;

		int index = i + j * width;
		if (index < 0) index = 0;
		if (index >= ColorBuffer.Num()) index = ColorBuffer.Num();
		OldColor = ColorBuffer[index];

	}
	return OldColor;
}

// Called when the game starts or when spawned
void AMyBouyancyPawn::BeginPlay()
{
	Super::BeginPlay();
	RenderResource =
		RenderTarget->GameThread_GetRenderTargetResource();
	
}

// Called every frame
void AMyBouyancyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyBouyancyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

