// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMovementPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
AMyMovementPawn::AMyMovementPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	USphereComponent* SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	RootComponent = SphereComponent;
	SphereComponent->InitSphereRadius(40.0f);
	SphereComponent->SetCollisionProfileName(TEXT("Pawn"));

	SphereVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualSphere"));
	SphereVisual->SetupAttachment(RootComponent);
	RayDirection = FVector(0.0f, 0.0f, -1.0f);
	TraceDistance = 500.0f;
	prevU = prevV = u = v = 0.0f;
	mag = 0.f;
	speed = 100.0f;
}

// Called when the game starts or when spawned
void AMyMovementPawn::BeginPlay()
{
	Super::BeginPlay();
	FirstHit = true;
	
}

void AMyMovementPawn::BeginDestroy()
{
	Super::BeginDestroy();
}

// Called every frame
void AMyMovementPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	bool test = IsValid(this);
	//Block taking care of Movement
	if (!CurrentVelocity.IsZero())
	{
		FVector NewLocation = GetActorLocation() + (CurrentVelocity*DeltaTime);
		
		if (test)
		{
			SetActorLocation(NewLocation);
		}
	}
	
	//Calc Magnitude
	mag = CurrentVelocity.Size();
	/*
	//Block for transferring Position to Texture
	{
		FHitResult OutHit;
		FVector Start = GetActorLocation();
		FVector End = Start + RayDirection * TraceDistance;
		//FCollisionQueryParams CollisionParams;
		//CollisionParams.bTraceComplex = true;
		//CollisionParams.AddIgnoredActor(this);
		TArray<AActor*>IgnoreActor;
	
		IgnoreActor.Add(this);
		//bool hit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, CollisionParams);
		bool hit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, ETraceTypeQuery::TraceTypeQuery1, true, IgnoreActor, EDrawDebugTrace::None, OutHit, true);

		if (hit && TextureMesh)
		{
			if (OutHit.GetActor() == TextureMesh)
			{
				FVector2D outUV;  
				UGameplayStatics::FindCollisionUV(OutHit, 0, outUV);
				UMaterialInterface* MaterialAttached = OutHit.GetComponent()->GetMaterial(0);
				UMaterialInstanceDynamic* MID = OutHit.GetComponent()->CreateDynamicMaterialInstance(0, OutHit.GetComponent()->GetMaterial(0));
				MID->SetVectorParameterValue("UV", FLinearColor(outUV.X, outUV.Y, 0.0f, 0.0f));
				if (FirstHit)
				{
					prevU = outUV.X;
					prevV = outUV.Y;
					FirstHit = false;
				}
				u = outUV.X;
				v = outUV.Y;
				
				//GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Blue, TEXT("Mag: ") + FString::SanitizeFloat(mag));
			}
			else
			{
				FirstHit = true;
				mag = 0;
			}
		}
		else
		{
			FirstHit = true;
			mag = 0;
		}
	}
	*/
}

// Called to bind functionality to input
void AMyMovementPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis("MoveForward", this, &AMyMovementPawn::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AMyMovementPawn::MoveRight);

}

void AMyMovementPawn::MoveForward(float AxisValue)
{
	CurrentVelocity.X = FMath::Clamp(AxisValue, -1.0f, 1.0f) * speed;
}

void AMyMovementPawn::MoveRight(float AxisValue)
{
	CurrentVelocity.Y = FMath::Clamp(AxisValue, -1.0f, 1.0f) * speed;
}

