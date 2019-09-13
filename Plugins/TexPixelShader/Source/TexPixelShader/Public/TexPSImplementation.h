// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "../Private/MyTEXShaders.h"

/**
 * 
 */
class TEXPIXELSHADER_API FTexPSImplementation
{
public:
	FTexPSImplementation(FColor StartColor, ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FTexPSImplementation();

	void ExecutePixelShader(UTextureRenderTarget2D* RenderTarget, FTexture2DRHIRef InputTexture, FColor BlendColor, float BlendAmount);

	/************************************************************************/
	/* Only execute this from the render thread!!!                          */
	/************************************************************************/
	void ExecutePixelShaderInternal();

	/************************************************************************/
	/* Save a screenshot of the target to the project saved folder          */
	/************************************************************************/
	void Save()
	{
		bSave = true;
	}

protected:
	bool bIsPixelShaderExecuting;
	bool bMustRegenerateSRV;
	bool bisUnloading;
	bool bSave;

	FPixelShaderVariableParameters VariableParameters;
	ERHIFeatureLevel::Type FeatureLevel;

	FTexture2DRHIRef CurrentTexture;
	FTexture2DRHIRef TextureParameter;
	UTextureRenderTarget2D* CurrentRenderTarget;
	FLinearColor m_startCol;
	
	//Shader Resource View (SRV)- is used to read texture files but cannot write
	//Unordered Access View(UAV) - is used to read and write texture files. 
	//		They usually write texture files at the same time as reading so can be used immediately by the pixel shader
	//Since we are only reading data we do not need UAVs and SRV will be enough.
	FShaderResourceViewRHIRef TextureParameterSRV;
	
	FQuadVertex m_pQuadVB[4];

	void SaveScreenshot(FRHICommandListImmediate& RHICmdList);
};
