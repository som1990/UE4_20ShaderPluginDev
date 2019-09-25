// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "GameFramework/Actor.h"
#include "ComputeTest.h"
#include "ComputeTestImplementation.h"
#include "CSTestSim.generated.h"


UCLASS()
class ACSTestSim : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACSTestSim();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "SGPlugin|LoadSource")
	void LoadHeightMapSource(float _magnitude, UTexture2D* SourceMap, FColor DisplayColor);
	
	UFUNCTION(BlueprintCallable, Category = "SGPlugin|PreviewTexture")
	void GeneratePreviewTexture(UTexture2D* &OutTexture);
	UFUNCTION(BlueprintCallable, Category = "SGPlugin|TexInitialization")
	void setOutputDimensions(int xSize, int ySize);

	void RHIRef2Texture2D(FTexture2DRHIRef rhiTexture, UTexture2D* OutTexture);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	float prevU, prevV;
	int texSizeX, texSizeY;
	FComputeTestExecute* testComputeShader;
	FTexture2DRHIRef InputTexture;
	FTexture2DRHIRef OutputTexture;
	UTexture2D* Texture2Display;
	
	bool bIsTextureGeneratingExecuting;
	bool bIsTextureDimensionsSet;
};
