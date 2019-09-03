// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ComputeTest.h"
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
	UFUNCTION(BlueprintCallable, Category = "SGPlugin|UVProject")
		FString PrintUV(float u, float v) const;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
