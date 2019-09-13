// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "GameFramework/Actor.h"
#include "IntersectionToRT.generated.h"

UCLASS()
class PLUGINTEST_API AIntersectionToRT : public AActor
{
	GENERATED_BODY()
	
	
public:	
	UPROPERTY(BlueprintReadWrite, Category = Mesh)
	class UStaticMeshComponent* MeshComp;
	// Sets default values for this actor's properties
	AIntersectionToRT();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TexSize")
	int TexSize;

protected:
	UFUNCTION(BlueprintCallable)
	void GenerateTrace(TArray<float> &u_outs, TArray<float> &v_outs);

	UFUNCTION(BlueprintCallable)
		void GenerateRTFromTrace(UTexture2D* &GeneratedTexture);
	
	void GenerateRTFromTrace_Internal(const int32 srcWidth, const int32 srcHeight, const TArray<FColor> &SrcData, const bool UseAlpha);

	FTraceHandle RequestTrace(FVector Start, FVector End, UWorld* world);

	TArray<FTraceHandle> TraceHandles;
	TArray<int> bWantsTrace;

	TArray<float> uArray;
	TArray<float> vArray;

	TArray<FColor> PixData;
	float xSize;
	float ySize;
	//FTraceDelegate TraceDelegate;
	UTexture2D* Texture;

};

