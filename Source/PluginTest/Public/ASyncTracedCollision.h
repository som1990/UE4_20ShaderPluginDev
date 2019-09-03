// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Engine.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASyncTracedCollision.generated.h"

UCLASS()
class PLUGINTEST_API AASyncTracedCollision : public AActor
{
	GENERATED_BODY()
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class UStaticMeshComponent* MeshComp;
	
public:	
	// Sets default values for this actor's properties
	AASyncTracedCollision();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UFUNCTION(BlueprintCallable)
	void SetWantsTrace();

	UFUNCTION(BlueprintImplementableEvent)
	void RecieveOnTraceCompleted(const TArray<FHitResult> & Results);

	FTraceHandle RequestTrace();
	void OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);
	void ImplementTraceResults(const FTraceDatum& TraceData);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ETraceTypeQuery> MyTraceType;

	FTraceHandle LastTraceHandle;
	FTraceDelegate TraceDelegate;
	uint32 bWantsTrace : 1; 

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};
