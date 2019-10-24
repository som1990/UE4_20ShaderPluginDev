#include "ComputeTestImplementation.h"
#include "ComputeTestPrivatePCH.h"
#include "GPUFFTCS.h"
#include "Public/RHIStaticStates.h"
#include "Public/PipelineStateCache.h"

#define NUM_THREADS_PER_GROUP 32
DECLARE_LOG_CATEGORY_EXTERN(InternalShaderLog, Log, All);
DEFINE_LOG_CATEGORY(InternalShaderLog);
FComputeTestExecute::FComputeTestExecute(int32 sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	bSimulatorInitialized = false;
	FeatureLevel = ShaderFeatureLevel;
	bIsUnloading = false;
	bIsComputeShaderExecuting = false;
	bMustRegenerateSRV = false;
	bMustRegenerateObsSRV = false;
	bMustRegenerateFlowSRV = false;
	bUseFlowMap = true;
	bUseObsMap = true;

	InputTexture = NULL;
	InTextureSRV = NULL;

	ObsTexture = NULL;
	ObsTextureSRV = NULL;

	FlowTexture = NULL;
	FlowTextureSRV = NULL;

	FRHIResourceCreateInfo createInfo;
	OutTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createInfo);
	OutTextureUAV = RHICreateUnorderedAccessView(OutTexture);
	OutTextureSRV = RHICreateShaderResourceView(OutTexture, 0);

	FRHIResourceCreateInfo createTmpInfo;
	TmpTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createTmpInfo);
	TmpTextureUAV = RHICreateUnorderedAccessView(TmpTexture);
	TmpTextureSRV = RHICreateShaderResourceView(TmpTexture, 0);

	FRHIResourceCreateInfo createEWaveInfo;
	WaveTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createEWaveInfo);
	WaveTextureUAV = RHICreateUnorderedAccessView(WaveTexture);
	WaveTextureSRV = RHICreateShaderResourceView(WaveTexture, 0);

	FrequencySize = FIntPoint(FMath::RoundUpToPowerOfTwo(sizeX), FMath::RoundUpToPowerOfTwo(sizeY));

	int output_size = sizeX * sizeY;
	TResourceArray<FVector4> h0_phi0_data;
	h0_phi0_data.Init(FVector4(0.f, 0.f, 0.f, 0.f),output_size);
	uint32 float4_stride = 4 * sizeof(float);
	CreateBufferAndUAV(&h0_phi0_data, output_size*float4_stride, float4_stride, &h0_phi0_SB_RW, &h0_phi0_UAV, &h0_phi0_SRV);
	bSimulatorInitialized = true;
}

FComputeTestExecute::~FComputeTestExecute()
{
	bIsUnloading = true;
}

void FComputeTestExecute::CreateBufferAndUAV(FResourceArrayInterface* Data, uint32 byte_width, uint32 byte_stride, FStructuredBufferRHIRef* ppBuffer, FUnorderedAccessViewRHIRef* ppUAV, FShaderResourceViewRHIRef* ppSRV)
{
	FRHIResourceCreateInfo ResourceCreateInfo;
	ResourceCreateInfo.ResourceArray = Data;
	*ppBuffer = RHICreateStructuredBuffer(byte_stride, Data->GetResourceDataSize(), (BUF_UnorderedAccess | BUF_ShaderResource), ResourceCreateInfo);

	*ppUAV = RHICreateUnorderedAccessView(*ppBuffer, false, false);
	*ppSRV = RHICreateShaderResourceView(*ppBuffer);
}

void FComputeTestExecute::ClearInternalData()
{
	if (OutTextureUAV != NULL)
	{
		OutTextureUAV.SafeRelease();
		OutTextureUAV = NULL;
	}

	if (InTextureSRV != NULL)
	{
		InTextureSRV.SafeRelease();
		InTextureSRV = NULL;
	}

	if (InputTexture != NULL)
	{
		InputTexture.SafeRelease();
		InputTexture = NULL;
	}
	
	if (OutTexture != NULL)
	{
		OutTexture.SafeRelease();
		OutTexture = NULL;
	}
	
	if (h0_phi0_SB_RW != NULL)
	{
		h0_phi0_SB_RW.SafeRelease();
		h0_phi0_SB_RW = NULL;
	}

	if (h0_phi0_UAV != NULL)
	{
		h0_phi0_UAV.SafeRelease();
		h0_phi0_UAV = NULL;
	}

	if (h0_phi0_SRV != NULL)
	{
		h0_phi0_SRV.SafeRelease();
		h0_phi0_SRV = NULL;
	}

	if (TmpTextureUAV != NULL)
	{
		TmpTextureUAV.SafeRelease();
		TmpTextureUAV = NULL;
	}

	if (TmpTextureSRV != NULL)
	{
		TmpTextureSRV.SafeRelease();
		TmpTextureSRV = NULL;
	}

	if (TmpTexture != NULL)
	{
		TmpTexture.SafeRelease();
		TmpTexture = NULL;
	}

	if (WaveTextureUAV != NULL)
	{
		WaveTextureUAV.SafeRelease();
		WaveTextureUAV = NULL;
	}

	if (WaveTextureSRV != NULL)
	{
		WaveTextureSRV.SafeRelease();
		WaveTextureSRV = NULL;
	}

	if (WaveTexture != NULL)
	{
		WaveTexture.SafeRelease();
		WaveTexture = NULL;
	}

	if (ObsTextureSRV != NULL)
	{
		ObsTextureSRV.SafeRelease();
		ObsTextureSRV = NULL;
	}

	if (ObsTexture != NULL)
	{
		ObsTexture.SafeRelease();
		ObsTexture = NULL;
	}

	if (FlowTextureSRV != NULL)
	{
		FlowTextureSRV.SafeRelease();
		FlowTextureSRV = NULL;
	}

	if (FlowTexture != NULL)
	{
		FlowTexture.SafeRelease();
		FlowTexture = NULL;
	}
}

void FComputeTestExecute::ExecuteComputeShader(
	UTextureRenderTarget2D* inputRenderTarget, FTexture2DRHIRef inputTexture, 
	FTexture2DRHIRef _obsTexture, FTexture2DRHIRef _flowMap, 
	FColor &DisplayColor, float _mag, float _delTime, bool bUseRenderTarget)
{
	check(IsInGameThread());

	if (!bSimulatorInitialized)
	{
		return;
	}

	if (bIsUnloading || bIsComputeShaderExecuting)
		return;
	bIsComputeShaderExecuting = true;

	bUseObsMap = (_obsTexture != NULL) ? true : false;
	bUseFlowMap = (_flowMap != NULL) ? true : false;
	
	inColor = FLinearColor(DisplayColor.R / 255.0, DisplayColor.G / 255.0, DisplayColor.B / 255.0, DisplayColor.A / 255.0);
	//UE_LOG(InternalShaderLog, Warning, TEXT("bUseFlowMap: %d"), bUseFlowMap);

	m_VariableParameters.mag = _mag;
	m_VariableParameters.deltaTime = _delTime;
	
	struct RenderTargetInput {
		UTextureRenderTarget2D* inputRT;
		bool bUseRenderTarget;
		FTexture2DRHIRef inTexture;
		bool bUseObsMap;
		FTexture2DRHIRef obsTexture;
	};
	//TSharedPtr<RenderTargetInput, ESPMode::ThreadSafe> rtInput = MakeShared<RenderTargetInput, ESPMode::ThreadSafe>();
	RenderTargetInput* rtInput = new RenderTargetInput();
	rtInput->inputRT = inputRenderTarget;
	rtInput->bUseRenderTarget = bUseRenderTarget;
	rtInput->inTexture = inputTexture;
	rtInput->obsTexture = _obsTexture;
	rtInput->bUseObsMap = bUseObsMap;
	//TSharedPtr<FComputeTestExecute, ESPMode::ThreadSafe> MyShader = this;
	FComputeTestExecute* MyShader = this;
	
	bool SuccessInput = true;

	ENQUEUE_RENDER_COMMAND(ComputeShaderRun)(
		[MyShader, rtInput, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			
			FTexture2DRHIRef rtTexture = nullptr;
			if (rtInput->bUseRenderTarget)
			{
				rtTexture = rtInput->inputRT->GetRenderTargetResource()->GetRenderTargetTexture();
				if (MyShader->InputTexture != rtTexture)
				{
					MyShader->bMustRegenerateSRV = true;
				}
				MyShader->InputTexture = rtTexture;
			}
			else
			{
				if (MyShader->InputTexture != rtInput->inTexture)
				{
					MyShader->bMustRegenerateSRV = true;
				}
				MyShader->InputTexture = rtInput->inTexture;
			}
			if (rtInput->bUseObsMap)
			{
				if (MyShader->ObsTexture != rtInput->obsTexture)
				{
					MyShader->bMustRegenerateObsSRV = true;
				}
				MyShader->ObsTexture = rtInput->obsTexture;
			}
			SuccessInput = MyShader->ExecuteComputeShaderInternal(RHICmdList);
		}
	);

	ENQUEUE_RENDER_COMMAND(ForwardFFT)(
		[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(RHICmdList, true);
			}
		}
	);

	ENQUEUE_RENDER_COMMAND(CalcEWave)(
		[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteEWave(RHICmdList);
			}
		}
	);

	ENQUEUE_RENDER_COMMAND(InverseFFT)(
		[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(RHICmdList, false);
			}
		}
	);

	struct AdvectFields {
		FTexture2DRHIRef _flowMap;
		bool bUseFlowMap;
	};
	//TSharedPtr<AdvectFields, ESPMode::ThreadSafe> advectVars = MakeShared<AdvectFields, ESPMode::ThreadSafe>();
	AdvectFields* advectVars = new AdvectFields();
	advectVars->_flowMap = _flowMap;
	advectVars->bUseFlowMap = bUseFlowMap;

	ENQUEUE_RENDER_COMMAND(ApplyField)(
		[MyShader, advectVars, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			if (advectVars->bUseFlowMap)
			{
				if (MyShader->FlowTexture != advectVars->_flowMap)
				{
					MyShader->bMustRegenerateFlowSRV = true;
				}
				MyShader->FlowTexture = advectVars->_flowMap;
			}
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteApplyFields(RHICmdList);
			}
		}
	);
	
//	UE_LOG(InternalShaderLog, Warning, TEXT("Shader Computed"));
	
	bIsComputeShaderExecuting = false;
}

bool FComputeTestExecute::ExecuteComputeShaderInternal(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());
	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}

	if (bMustRegenerateSRV)
	{
		bMustRegenerateSRV = false;
		if (NULL != InTextureSRV)
		{
			InTextureSRV.SafeRelease();
			InTextureSRV = NULL;
		}

		InTextureSRV = RHICreateShaderResourceView(InputTexture, 0);
	}

	if (bMustRegenerateObsSRV)
	{
		if (NULL != ObsTextureSRV)
		{
			ObsTextureSRV.SafeRelease();
			ObsTextureSRV = NULL;
		}

		ObsTextureSRV = RHICreateShaderResourceView(ObsTexture, 0);
	}
	
	TShaderMapRef<FAddSourceHeightCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, inColor, bUseObsMap, InTextureSRV, ObsTextureSRV);
	ComputeShader->SetOutput(RHICmdList, OutTextureUAV, h0_phi0_UAV);
	ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);
	//RHICmdList.DispatchComputeShader(InputTexture->GetSizeX()/8, InputTexture->GetSizeY()/8,1);
	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(InputTexture->GetSizeX() / NUM_THREADS_PER_GROUP), FMath::CeilToInt(InputTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, OutTextureUAV);

	
	return true;
	
}

bool FComputeTestExecute::ExecuteEWave(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());
	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}
	//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, OutTextureUAV);

	TShaderMapRef<FCalcEWaveCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, 1.0f, 1.0f, OutTextureSRV);
	ComputeShader->SetOutput(RHICmdList, WaveTextureUAV);
	ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);
	//UE_LOG(LogTemp, Warning, TEXT("OutTextureDims: %d, %d"), FMath::CeilToInt(512 / NUM_THREADS_PER_GROUP), FMath::CeilToInt(512 / NUM_THREADS_PER_GROUP));
	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(OutTexture->GetSizeX() / NUM_THREADS_PER_GROUP), FMath::CeilToInt(OutTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, WaveTextureUAV);

	return true;
}

bool FComputeTestExecute::ExecuteFFT(FRHICommandListImmediate& RHICmdList, bool bIsForward)
{
	check(IsInRenderingThread());

	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}

	MyGPUFFT::FGPUFFTShaderContext FFTContext(RHICmdList, *GetGlobalShaderMap(FeatureLevel));
	FIntRect SrcRect(FIntPoint(0, 0), FrequencySize);
	bool SuccessValue = true;
	bool bIsHorizontal = true;
	if (bIsForward)
	{
		SuccessValue = MyGPUFFT::FFTImage2D(FFTContext, FrequencySize, bIsHorizontal, bIsForward, SrcRect, OutTextureSRV, OutTextureUAV, TmpTextureUAV, TmpTextureSRV);
	}
	else
	{
		SuccessValue = MyGPUFFT::FFTImage2D(FFTContext, FrequencySize, bIsHorizontal, bIsForward, SrcRect, WaveTextureSRV, OutTextureUAV, TmpTextureUAV, TmpTextureSRV);

	}
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, OutTextureUAV);
	return SuccessValue;
}

bool FComputeTestExecute::ExecuteApplyFields(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());

	if (bIsUnloading)
	{
		return false;
	}

	if (bMustRegenerateFlowSRV)
	{
		if (NULL != FlowTextureSRV)
		{
			FlowTextureSRV.SafeRelease();
			FlowTextureSRV = NULL;
		}

		FlowTextureSRV = RHICreateShaderResourceView(FlowTexture, 0);
	}


	TShaderMapRef<FApplyFieldsCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, OutTextureSRV, FlowTextureSRV, bUseFlowMap);
	ComputeShader->SetOutput(RHICmdList, h0_phi0_UAV);
	ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);

	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(OutTexture->GetSizeX() / NUM_THREADS_PER_GROUP), FMath::CeilToInt(OutTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, h0_phi0_UAV);

	return true;
}

