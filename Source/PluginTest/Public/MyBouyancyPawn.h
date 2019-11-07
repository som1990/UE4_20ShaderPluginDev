// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "RenderCore.h"
#include "RHICommandList.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "GameFramework/Pawn.h"
#include "MyBouyancyPawn.generated.h"

UCLASS()
class PLUGINTEST_API AMyBouyancyPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMyBouyancyPawn();

	UPROPERTY(Category = HeightMap, EditAnywhere)
	UTextureRenderTarget2D* RenderTarget;

	UFUNCTION(BlueprintCallable, Category = "HeightMap | Update")
	void UpdateBuffer();

	UFUNCTION(BlueprintCallable, Category = "HeightMap | Texture Helper")
	FColor GetRenderTargetValue(float x, float y, FVector2D gridSize);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	TArray<FColor> ColorBuffer;
	
};
