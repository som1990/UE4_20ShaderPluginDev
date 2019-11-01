// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "GameFramework/Actor.h"
#include "ComputeTest.h"
#include "ComputeTestImplementation.h"
#include "VS_PSImplementation.h"
#include "CSTestSim.generated.h"


UCLASS()
class ACSTestSim : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACSTestSim();
	UFUNCTION(BlueprintCallable, Category = "SGPlugin|LoadSource")
		void LoadHeightMapSource(
			float _magnitude, float _delTime, float _choppyScale, float flowScale, UTexture2D* SourceMap, 
			UTexture2D* ObsMap, UTexture2D* FlowMap, FColor DisplayColor, 
			UTextureRenderTarget2D* InRenderTarget, bool buseRenderTarget);

	UFUNCTION(BlueprintCallable, Category = "SGPlugin|PreviewTexture")
		void GeneratePreviewTexture(UTexture2D* &OutTexture);
	UFUNCTION(BlueprintCallable, Category = "SGPlugin|TexInitialization")
		void setOutputDimensions(int32 xSize, int32 ySize);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* RenderTarget2Display;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;
	
	void RHIRef2Texture2D(FTexture2DRHIRef rhiTexture, UTexture2D* OutTexture);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	float prevU, prevV;
	int texSizeX, texSizeY;
	FComputeTestExecute* testComputeShader;
	FDisplayShaderExecute* testDisplayShader;

	FTexture2DRHIRef InputTexture;
	UTexture2D* Texture2Display;

	FTexture2DRHIRef ObsTexture;
	FTexture2DRHIRef FlowTexture;

	bool bIsTextureGeneratingExecuting;
	bool bIsTextureDimensionsSet;
	bool bUseObsMap;
	bool bUseFlowMap;

};
