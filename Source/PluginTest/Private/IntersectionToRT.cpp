// Fill out your copyright notice in the Description page of Project Settings.

#include "IntersectionToRT.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

// Sets default values
AIntersectionToRT::AIntersectionToRT()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GridMesh"));
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	MeshComp->SetupAttachment(RootComponent);

	TexSize = 512;

}

// Called when the game starts or when spawned
void AIntersectionToRT::BeginPlay()
{
	Super::BeginPlay();
	bWantsTrace.Init(0, TexSize*2);
	TraceHandles.SetNum(TexSize*2);
	uArray.Init(0,TexSize);
	vArray.Init(0,TexSize);
	PixData.Init(FColor(0, 0, 0, 255), TexSize*TexSize);
	//Setting Bounds
	FVector bound, center;
	GetActorBounds(false, center, bound);
	xSize = bound.X * 2;
	ySize = bound.Y * 2;
	Texture = UTexture2D::CreateTransient(
		TexSize,
		TexSize,
		PF_B8G8R8A8
	);
	
}

// Called every frame
void AIntersectionToRT::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void AIntersectionToRT::GenerateTrace(TArray<float> &u_outs, TArray<float> &v_outs)
{
	AIntersectionToRT* MyObject = this;
	UWorld* world = GetWorld();
	FVector actorLocation = GetActorLocation();
		
	int numCollisions = 0;
	for (int i = 0; i < MyObject->TexSize * 2; i++)
	{
		int row = floor(i / 2);
		if (!(i % 2))
			numCollisions = 0;
		if (MyObject->TraceHandles[i]._Data.FrameNumber != 0)
		{
			FTraceDatum OutData;
			if (world->QueryTraceData(MyObject->TraceHandles[i], OutData))
			{
				MyObject->TraceHandles[i]._Data.FrameNumber = 0;

				numCollisions += OutData.OutHits.Num();
				float u = 0;
				if (OutData.OutHits.IsValidIndex(0)) {
					float loc = OutData.OutHits[0].Distance;
					u = loc / (MyObject->ySize);
				}
				//TArray<float> hitU;


				if (!(i % 2))
					MyObject->uArray[row] = u;
				else
					MyObject->vArray[row] = 1 - u;

				//ImplementTraceResults(OutData);

			}
		}

		if (!world->IsTraceHandleValid(MyObject->TraceHandles[i], false))
		{
			FVector Start = actorLocation - FVector(MyObject->xSize*0.5, MyObject->ySize*0.5, 0.f) + FVector((i*0.5 / MyObject->TexSize)*MyObject->xSize, 0, 0);
			FVector End = Start + FVector(0.f, MyObject->ySize, 0.f);
			if (!(i % 2))
				MyObject->TraceHandles[i] = MyObject->RequestTrace(Start, End, world);
			else
				MyObject->TraceHandles[i] = MyObject->RequestTrace(End, Start, world);
				
		}
		if (i % 2)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::FromInt(row) + TEXT(" : NumHits = ") + FString::FromInt(numCollisions));
			/*if (numCollisions)
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Blue, TEXT("Collision U,V: ") + FString::SanitizeFloat(MyObject->uArray[row]) + TEXT(", ") + FString::SanitizeFloat(MyObject->vArray[row]));
			*/
		}
	}
	u_outs = uArray;
	v_outs = vArray;
}

void AIntersectionToRT::GenerateRTFromTrace( UTexture2D* &ResultantTexture)
{
	PixData.Init(FColor(0, 0, 0, 255), TexSize*TexSize);

	for (int32 y = 0; y < TexSize; y++)
	{
		int u = FMath::FloorToInt((uArray[TexSize - 1 - y] * (TexSize - 1)) + 0.5);
		int v = FMath::FloorToInt((vArray[TexSize - 1 - y] * (TexSize - 1)) + 0.5);
		//Assuming object is smaller and within the simulation grid
		if ((uArray[TexSize - 1 - y] != 0) || vArray[TexSize - 1 - y] != 1)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow,FString::FromInt(y) + TEXT(" : ") + FString::FromInt(u) + TEXT(", ") + FString::FromInt(v));
			for (int32 x = u; x <= v; x++) 
			{
				int ind = y + (TexSize - 1 - x)*TexSize;
				PixData[ind] = FColor(255, 255, 255, 255);
			}
		}
	}
	
	
	//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, TEXT("Before: ") + FString::FromInt(sizeof(Texture)));
	GenerateRTFromTrace_Internal(TexSize, TexSize, PixData, false);
	//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, TEXT("After: ") + FString::FromInt(sizeof(Texture)));


	ResultantTexture = Texture;
}



void AIntersectionToRT::GenerateRTFromTrace_Internal(const int32 srcWidth, const int32 srcHeight, const TArray<FColor> &SrcData, const bool UseAlpha)
{
	
	
	//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, TEXT("After: ") + FString::FromInt(sizeof(Texture)));

	//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, TEXT("Size: ") + FString::FromInt(srcWidth) + TEXT(", ") + FString::FromInt(srcHeight));
	uint8* MipData = static_cast<uint8*>(Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	uint8* DestPtr = NULL;
	const FColor* SrcPtr = NULL;
	for (int32 y = 0; y < srcHeight; y++)
	{
		DestPtr = &MipData[(srcHeight - 1 - y) * srcWidth * sizeof(FColor)];
		SrcPtr = const_cast<FColor*>(&SrcData[(srcHeight - 1 - y)* srcWidth]);
		for (int32 x = 0; x < srcWidth; x++)
		{
			*DestPtr++ = SrcPtr->B;
			*DestPtr++ = SrcPtr->G;
			*DestPtr++ = SrcPtr->R;
			if (UseAlpha)
			{
				*DestPtr++ = SrcPtr->A;
			}
			else
			{
				*DestPtr++ = 0xFF;
			}
			SrcPtr++;
		}
	}

	Texture->PlatformData->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
}

FTraceHandle AIntersectionToRT::RequestTrace(FVector Start, FVector End, UWorld* World)
{
	if (World == nullptr)
		return FTraceHandle();

	bool bTraceComplex = true;
	auto objectParams = FCollisionObjectQueryParams(ECC_Pawn);
	auto Params = FCollisionQueryParams(FName(TEXT("AsyncRequestTrace")), bTraceComplex, this);

	return World->AsyncLineTraceByObjectType(EAsyncTraceType::Multi, Start, End, objectParams, Params);
}

