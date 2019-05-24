// Fill out your copyright notice in the Description page of Project Settings.

#include "CSTestSim.h"


// Sets default values
ACSTestSim::ACSTestSim()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACSTestSim::BeginPlay()
{
	Super::BeginPlay();
	
}

FString ACSTestSim::PrintUV(float u, float v) const
{
	return FString::Printf(TEXT("Hello World: %f, %f"), u, v);
}

// Called every frame
void ACSTestSim::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

