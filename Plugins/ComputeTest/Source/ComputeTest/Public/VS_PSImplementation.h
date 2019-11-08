#pragma once

#include "Engine.h"
#include "EWaveParm.h"
#include "CSShaders.h"


class COMPUTETEST_API FDisplayShaderExecute
{
public:
	FDisplayShaderExecute(int sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FDisplayShaderExecute();

	void ExecuteDisplayShader(
		UTextureRenderTarget2D* RenderTarget, UTextureRenderTarget2D* NormMapRT, UTextureRenderTarget2D* GradMapRT,
		FTexture2DRHIRef InputTexture, FTexture2DRHIRef InGradTexture, const FEWaveData &eWaveData);

	void ExecuteDisplayShader_RenderThread(FRHICommandListImmediate &RHICmdList);

	void ExecuteNormalRT_RenderThread(FRHICommandListImmediate &RHICmdList);

	void ExecuteGrad_RenderThread(FRHICommandListImmediate &RHICmdList);
protected:
	bool bIsPixelShaderExecuting;
	bool bMustRegenerateSRV;
	bool bisUnloading;
	bool bMustRegenerateGradSRV;

	float m_Lx, m_Ly;

	ERHIFeatureLevel::Type FeatureLevel;

	FTexture2DRHIRef CurrentTexture;
	FTexture2DRHIRef TextureParameter;
	FTexture2DRHIRef GradParameter;
	FTexture2DRHIRef CurGradTexture;
	FTexture2DRHIRef CurNormalTexture;
	UTextureRenderTarget2D* CurrentRenderTarget;
	UTextureRenderTarget2D* CurNormMapRT;
	UTextureRenderTarget2D* CurGradMapRT;

	FShaderResourceViewRHIRef TextureParameterSRV;
	FShaderResourceViewRHIRef GradParameterSRV;

	FQuadVertex m_pQuadVB[4];
	FEWavePSVariableParameters m_PSVariableParm;

};
