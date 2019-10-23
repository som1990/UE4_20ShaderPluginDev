#pragma once
#include "Engine.h"
#include "CSShaders.h"

class COMPUTETEST_API FComputeTestExecute
{
public:
	FComputeTestExecute(int32 sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FComputeTestExecute();

	void ExecuteComputeShader(UTextureRenderTarget2D* InRenderTarget, FTexture2DRHIRef _inputTexture, const FColor &DisplayColor, float _mag, float _delTime, bool bUseRenderTarget);

	bool ExecuteComputeShaderInternal(FRHICommandListImmediate& RHICmdList);
	bool ExecuteEWave(FRHICommandListImmediate& RHICmdList);
	bool ExecuteFFT(FRHICommandListImmediate& RHICmdList, bool bIsForward);
	bool ExecuteApplyFields(FRHICommandListImmediate& RHICmdList);

	FTexture2DRHIRef GetTexture() { return OutTexture; }

protected:
	void CreateBufferAndUAV(FResourceArrayInterface* Data, uint32 byte_width, uint32 byte_stride, FStructuredBufferRHIRef* ppBuffer, FUnorderedAccessViewRHIRef* ppUAV, FShaderResourceViewRHIRef* ppSRV);
	void ClearInternalData();

private:
	bool bIsComputeShaderExecuting;
	bool bIsUnloading;
	bool bSimulatorInitialized;
	bool bMustRegenerateSRV;
	FIntPoint FrequencySize;

	ERHIFeatureLevel::Type FeatureLevel;
	
	FLinearColor inColor;

	FTexture2DRHIRef OutTexture;
	FUnorderedAccessViewRHIRef OutTextureUAV;
	FShaderResourceViewRHIRef OutTextureSRV;

	FTexture2DRHIRef InputTexture;
	FShaderResourceViewRHIRef InTextureSRV;

	FTexture2DRHIRef TmpTexture;
	FUnorderedAccessViewRHIRef TmpTextureUAV;
	FShaderResourceViewRHIRef TmpTextureSRV;

	FTexture2DRHIRef WaveTexture;
	FUnorderedAccessViewRHIRef WaveTextureUAV;
	FShaderResourceViewRHIRef WaveTextureSRV;
	
	FStructuredBufferRHIRef h0_phi0_SB_RW;
	FUnorderedAccessViewRHIRef h0_phi0_UAV;
	FShaderResourceViewRHIRef h0_phi0_SRV;
	
	FComputeShaderVariableParameters m_VariableParameters;
	
};