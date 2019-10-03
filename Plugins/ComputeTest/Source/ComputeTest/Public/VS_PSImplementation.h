#pragma once

#include "Engine.h"
#include "CSShaders.h"


class COMPUTETEST_API FDisplayShaderExecute
{
public:
	FDisplayShaderExecute(int sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FDisplayShaderExecute();

	void ExecuteDisplayShader(UTextureRenderTarget2D* RenderTarget, FTexture2DRHIRef InputTexture);

	void ExecuteDisplayShader_RenderThread(FRHICommandListImmediate &RHICmdList);


protected:
	bool bIsPixelShaderExecuting;
	bool bMustRegenerateSRV;
	bool bisUnloading;

	ERHIFeatureLevel::Type FeatureLevel;

	FTexture2DRHIRef CurrentTexture;
	FTexture2DRHIRef TextureParameter;
	UTextureRenderTarget2D* CurrentRenderTarget;

	FShaderResourceViewRHIRef TextureParameterSRV;

	FQuadVertex m_pQuadVB[4];


};
