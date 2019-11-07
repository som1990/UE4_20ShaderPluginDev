// Fill out your copyright notice in the Description page of Project Settings.

#include "MyBouyancyPawn.h"


// Sets default values
AMyBouyancyPawn::AMyBouyancyPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AMyBouyancyPawn::UpdateBuffer()
{
	FTextureRenderTarget2DResource* RenderResource =
		(FTextureRenderTarget2DResource*)RenderTarget->GameThread_GetRenderTargetResource();

	struct FReadSurfaceContext
	{
		FRenderTarget* SrcRenderTarget;
		TArray<FColor>* OutData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};

	ColorBuffer.Reset();
	FReadSurfaceContext* ReadSurfaceContext = new FReadSurfaceContext();

	ReadSurfaceContext->SrcRenderTarget = RenderResource;
	ReadSurfaceContext->OutData = &ColorBuffer;
	ReadSurfaceContext->Rect = FIntRect(0, 0, RenderResource->GetSizeX(), RenderResource->GetSizeY());
	ReadSurfaceContext->Flags = FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX);
	
/*
	ENQUEUE_RENDER_COMMAND(ReadHeight)(
		[ReadSurfaceContext](FRHICommandListImmediate& RHICmdList)
	{
		RHICmdList.ReadSurfaceData(
			ReadSurfaceContext->SrcRenderTarget->GetRenderTargetTexture(),
			ReadSurfaceContext->Rect,
			*ReadSurfaceContext->OutData,
			ReadSurfaceContext->Flags
		);
	}
	);
	*/
}

FColor AMyBouyancyPawn::GetRenderTargetValue(float x, float y, FVector2D gridSize)
{
	if (RenderTarget == NULL || ColorBuffer.Num() == 0)
		return FColor(0);
	
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

	return ColorBuffer[index];
}

// Called when the game starts or when spawned
void AMyBouyancyPawn::BeginPlay()
{
	Super::BeginPlay();
	
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

