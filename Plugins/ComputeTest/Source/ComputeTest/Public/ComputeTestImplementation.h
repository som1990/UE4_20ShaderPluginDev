#pragma once
#include "Engine.h"
#include "CSShaders.h"

class COMPUTETEST_API FComputeTestExecute
{
public:
	FComputeTestExecute(int32 sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FComputeTestExecute();

	void ExecuteComputeShader(FTexture2DRHIRef _InTexture, FColor DisplayColor);

	void ExecuteComputeShaderInternal(FRHICommandListImmediate& RHICmdList);

	FTexture2DRHIRef GetTexture() { return Texture; }

private:
	bool bIsComputeShaderExecuting;
	bool bIsUnloading;
	bool bMustRegenerateUAV;
	bool bMustRegenerateSRV;

	ERHIFeatureLevel::Type FeatureLevel;

	FTexture2DRHIRef Texture;
	FTexture2DRHIRef InputTexture;
	FLinearColor inColor;

	FUnorderedAccessViewRHIRef TextureUAV;
	FShaderResourceViewRHIRef InTextureSRV;
};