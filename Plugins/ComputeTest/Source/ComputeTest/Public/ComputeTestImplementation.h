#pragma once
#include "Engine.h"
#include "EWaveParm.h"
#include "CSShaders.h"

class COMPUTETEST_API FComputeTestExecute
{
public:
	FComputeTestExecute(int32 sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FComputeTestExecute();

	void ExecuteComputeShader(
		UTextureRenderTarget2D* InRenderTarget, FTexture2DRHIRef _inputTexture, 
		FTexture2DRHIRef _obsTexture, FTexture2DRHIRef _flowMap,
		FColor &DisplayColor, const FEWaveData& eWaveData);

	bool ExecuteComputeShaderInternal(
		FRHICommandListImmediate& RHICmdList, 
		const FShaderResourceViewRHIRef& _SrcTexSRV, const FShaderResourceViewRHIRef& _ObsTexSRV, 
		FUnorderedAccessViewRHIRef& DstTexUAV, FUnorderedAccessViewRHIRef& StructBufferUAV);
	
	bool ExecuteEWave(
		FRHICommandListImmediate& RHICmdList,
		const FShaderResourceViewRHIParamRef& SrcSRV,
		FUnorderedAccessViewRHIRef& DstUAV);
	bool ExecuteFFT(
		FRHICommandListImmediate& RHICmdList, 
		const FShaderResourceViewRHIParamRef& SrcSRV,
		FUnorderedAccessViewRHIRef& DstUAV, bool bIsForward);

	bool ExecuteAdvectFields(
		FRHICommandListImmediate& RHICmdList, float velScale,
		const FShaderResourceViewRHIRef& _SrcTexSRV, const FShaderResourceViewRHIRef& _velTexSRV,
		FUnorderedAccessViewRHIRef& DstTexUAV
		);
	bool ExecuteApplyFields(
		FRHICommandListImmediate& RHICmdList, float _velScale,
		const FShaderResourceViewRHIRef& _SrcTexSRV, const FShaderResourceViewRHIRef& gradTexSRV,
		const FShaderResourceViewRHIRef& obsTexSRV,
		FUnorderedAccessViewRHIRef& StructBufferUAV, FUnorderedAccessViewRHIRef& _DstTexUAV
	);
	bool ExecuteNonLinearAndGrad(
		FRHICommandListImmediate& RHICmdList, 
		const FShaderResourceViewRHIRef& SrcSRV, 
		FUnorderedAccessViewRHIRef& gradUAV, FUnorderedAccessViewRHIRef& dstUAV);
	FTexture2DRHIRef GetTexture() { return OutTexture; }

protected:
	void CreateBufferAndUAV(FResourceArrayInterface* Data, uint32 byte_width, uint32 byte_stride, FStructuredBufferRHIRef* ppBuffer, FUnorderedAccessViewRHIRef* ppUAV, FShaderResourceViewRHIRef* ppSRV);
	void ClearInternalData();

private:
	bool bIsComputeShaderExecuting;
	bool bIsUnloading;
	bool bSimulatorInitialized;
	bool bMustRegenerateSRV;
	bool bMustRegenerateObsSRV;
	bool bMustRegenerateFlowSRV;
	bool bUseObsMap;
	bool bUseFlowMap;
	
	
	float m_Lx, m_Ly;
	bool bGenGrad;
	bool bcalcNonLinear; 

	FIntPoint FrequencySize;

	ERHIFeatureLevel::Type FeatureLevel;
	
	FLinearColor inColor;

	FTexture2DRHIRef OutTexture;
	FUnorderedAccessViewRHIRef OutTextureUAV;
	FShaderResourceViewRHIRef OutTextureSRV;

	FTexture2DRHIRef InputTexture;
	FShaderResourceViewRHIRef InTextureSRV;

	FTexture2DRHIRef ObsTexture;
	FShaderResourceViewRHIRef ObsTextureSRV;

	FTexture2DRHIRef FlowTexture;
	FShaderResourceViewRHIRef FlowTextureSRV;

	FTexture2DRHIRef TmpFFTTexture;
	FUnorderedAccessViewRHIRef TmpFFTTextureUAV;
	FShaderResourceViewRHIRef TmpFFTTextureSRV;

	FTexture2DRHIRef TransitionTexture;
	FUnorderedAccessViewRHIRef TransitionTextureUAV;
	FShaderResourceViewRHIRef TransitionTextureSRV;

	FTexture2DRHIRef FFTTransitionTexture;
	FUnorderedAccessViewRHIRef FFTTransitionTextureUAV;
	FShaderResourceViewRHIRef FFTTransitionTextureSRV;

	FTexture2DRHIRef WaveTexture;
	FUnorderedAccessViewRHIRef WaveTextureUAV;
	FShaderResourceViewRHIRef WaveTextureSRV;

	FTexture2DRHIRef DxDyTexture;
	FUnorderedAccessViewRHIRef DxDyTextureUAV;
	FShaderResourceViewRHIRef DxDyTextureSRV;
	
	FTexture2DRHIRef DvxDvyTexture;
	FUnorderedAccessViewRHIRef DvxDvyTextureUAV;
	FShaderResourceViewRHIRef DvxDvyTextureSRV;

	FStructuredBufferRHIRef h0_phi0_SB_RW;
	FUnorderedAccessViewRHIRef h0_phi0_UAV;
	FShaderResourceViewRHIRef h0_phi0_SRV;

	FComputeShaderVariableParameters m_VariableParameters;
	
};