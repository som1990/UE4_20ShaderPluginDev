// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "GameFramework/Actor.h"
#include "TexPSImplementation.h"
#include "TexShaderMesh.generated.h"

UCLASS()
class PLUGINTEST_API ATexShaderMesh : public AActor
{
	GENERATED_BODY()
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class UStaticMeshComponent* MeshComp;
	
public:	
	// Sets default values for this actor's properties
	ATexShaderMesh();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void SetupMyPlayerInputComponent(UInputComponent* InputComponent);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
		FColor ColorToBlend;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
		UTexture2D* TextureToDisplay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo, meta = (ClampMin = "0.0", ClampMax = "1.0", 
		UIMin = "0.0", UIMax = "1.0"))
		float BlendAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
		UMaterialInterface* MaterialToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
		UTextureRenderTarget2D* RenderTarget;

private:
	FTexPSImplementation* TexPixelShading;
	
	void SavePixelShaderOutput();
	FTexture2DRHIRef InputTexture;
};
