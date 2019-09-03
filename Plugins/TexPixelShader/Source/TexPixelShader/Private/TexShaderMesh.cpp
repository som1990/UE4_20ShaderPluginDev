// Fill out your copyright notice in the Description page of Project Settings.

#include "TexShaderMesh.h"
#include "Classes/Components/StaticMeshComponent.h"

// Sets default values
ATexShaderMesh::ATexShaderMesh()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	InputTexture = NULL;
	ColorToBlend = FColor::Green;
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyShaderMesh"));
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	//MeshComp->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MeshComp->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ATexShaderMesh::BeginPlay()
{
	Super::BeginPlay();
	TexPixelShading = new FTexPSImplementation(ColorToBlend, GetWorld()->Scene->GetFeatureLevel());
	//This adds the render target Texture to the target material. 
	if (MeshComp != nullptr) {
		MeshComp->SetMaterial(0, MaterialToApply);

		UMaterialInstanceDynamic* MID =
			MeshComp->CreateAndSetMaterialInstanceDynamic(0);

		//This Cast conversion requires "Engine.h"
		UTexture* CastedRenderTarget = Cast<UTexture>(RenderTarget);
		MID->SetTextureParameterValue("InputTexture", CastedRenderTarget);
	}

	//Enabling input within the Actor
	EnableInput(GetWorld()->GetFirstPlayerController());
	UInputComponent* MyInputComp = this->InputComponent;

	if (MyInputComp)
	{
		SetupMyPlayerInputComponent(MyInputComp);
	}

	InputTexture = static_cast<FTexture2DResource*>(TextureToDisplay->Resource)->GetTexture2DRHI();
	
		 
}

void ATexShaderMesh::BeginDestroy()
{
	Super::BeginDestroy();
	if (TexPixelShading) {
		delete TexPixelShading;
	}
	if (InputTexture)
	{
		InputTexture.SafeRelease();
		InputTexture = NULL;
	}
}


// Called every frame
void ATexShaderMesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (InputTexture && RenderTarget)
		TexPixelShading->ExecutePixelShader(RenderTarget, InputTexture, ColorToBlend, BlendAmount);
}

void ATexShaderMesh::SetupMyPlayerInputComponent(UInputComponent* InputComponent)
{
	check(InputComponent);
	InputComponent->BindAction("SavePSOutput", IE_Pressed, this,
		&ATexShaderMesh::SavePixelShaderOutput);
}

void ATexShaderMesh::SavePixelShaderOutput()
{
	TexPixelShading->Save();
}

