// Fill out your copyright notice in the Description page of Project Settings.

#include "ASyncTracedCollision.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"


// Sets default values
AASyncTracedCollision::AASyncTracedCollision()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyGridMesh"));
	RootComponent = CreateAbstractDefaultSubobject<USceneComponent>(TEXT("Root"));
	MeshComp->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AASyncTracedCollision::BeginPlay()
{
	Super::BeginPlay();
	TraceDelegate.BindUObject(this, &AASyncTracedCollision::OnTraceCompleted);
}

void AASyncTracedCollision::SetWantsTrace()
{
	if (!GetWorld()->IsTraceHandleValid(LastTraceHandle, false))
	{
		bWantsTrace = true;
	}
}

FTraceHandle AASyncTracedCollision::RequestTrace()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return FTraceHandle();

	bool bTraceComplex = true;
	bool bIgnoreSelf = true;
	auto objectParams = FCollisionObjectQueryParams(ECC_Pawn);
	auto Params = FCollisionQueryParams(FName(TEXT("AsyncRequestTrace")), bTraceComplex,this);

	FVector Start = GetActorLocation();
	FVector End = Start + FVector(0.f, 500.f, 0.f);
	return World->AsyncLineTraceByObjectType(EAsyncTraceType::Multi, Start, End, objectParams, Params, &TraceDelegate);
}

void AASyncTracedCollision::OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	ensure(Handle == LastTraceHandle);
	ImplementTraceResults(Data);
	LastTraceHandle._Data.FrameNumber = 0;
}

void AASyncTracedCollision::ImplementTraceResults(const FTraceDatum& TraceData)
{
	int numHits = TraceData.OutHits.Num();
	GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, TEXT("Num Hits: ") + FString::FromInt(numHits));

	RecieveOnTraceCompleted(TraceData.OutHits);
}

// Called every frame
void AASyncTracedCollision::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (LastTraceHandle._Data.FrameNumber != 0)
	{
		FTraceDatum OutData;
		if (GetWorld()->QueryTraceData(LastTraceHandle, OutData))
		{
			LastTraceHandle._Data.FrameNumber = 0;
			ImplementTraceResults(OutData);
		}

		if (bWantsTrace)
		{
			LastTraceHandle = RequestTrace();
			bWantsTrace = false;
		}
	}

}

