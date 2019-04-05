// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyMovementPawn.generated.h"

UCLASS()
class PLUGINTEST_API AMyMovementPawn : public APawn
{
	GENERATED_BODY()
		UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		class UStaticMeshComponent* SphereVisual;
public:
	// Sets default values for this pawn's properties
	AMyMovementPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	FVector CurrentVelocity;

	FVector RayDirection;
	float TraceDistance;

	UPROPERTY(EditAnywhere, Category = Shader)
	class AActor* TextureMesh;
};