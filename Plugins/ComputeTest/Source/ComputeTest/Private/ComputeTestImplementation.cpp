#include "ComputeTestImplementation.h"
#include "ComputeTestPrivatePCH.h"
#include "GPUFFTCS.h"
#include "Public/RHIStaticStates.h"
#include "Public/PipelineStateCache.h"
#include "Public/Stats/Stats2.h"
#include "Public/SceneUtils.h"


#define NUM_THREADS_PER_GROUP 32
DECLARE_LOG_CATEGORY_EXTERN(InternalShaderLog, Log, All);
DEFINE_LOG_CATEGORY(InternalShaderLog);

//DECLARE_STATS_GROUP(TEXT("SIM"), STATGROUP_SIM, STATCAT_Advanced);
//DECLARE_CYCLE_STAT(TEXT("Total SimTime"), STAT_TotSimTime, STATGROUP_SIM);



FComputeTestExecute::FComputeTestExecute(int32 sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	
	bSimulatorInitialized = false;
	FeatureLevel = ShaderFeatureLevel;
	bIsUnloading = false;
	bIsComputeShaderExecuting = false;
	bMustRegenerateSRV = false;
	bMustRegenerateObsSRV = false;
	bMustRegenerateFlowSRV = false;
	bGenGrad = false;
	bcalcNonLinear = false;
	
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
	TmpFFTTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createTmpInfo);
	TmpFFTTextureUAV = RHICreateUnorderedAccessView(TmpFFTTexture);
	TmpFFTTextureSRV = RHICreateShaderResourceView(TmpFFTTexture, 0);

	FRHIResourceCreateInfo createEWaveInfo;
	WaveTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createEWaveInfo);
	WaveTextureUAV = RHICreateUnorderedAccessView(WaveTexture);
	WaveTextureSRV = RHICreateShaderResourceView(WaveTexture, 0);
	
	FRHIResourceCreateInfo createDxDyInfo;
	DxDyTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createDxDyInfo);
	DxDyTextureUAV = RHICreateUnorderedAccessView(DxDyTexture);
	DxDyTextureSRV = RHICreateShaderResourceView(DxDyTexture, 0);
	
	FRHIResourceCreateInfo createDvxDvyInfo;
	DvxDvyTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createDvxDvyInfo);
	DvxDvyTextureUAV = RHICreateUnorderedAccessView(DvxDvyTexture);
	DvxDvyTextureSRV = RHICreateShaderResourceView(DvxDvyTexture, 0);

	FRHIResourceCreateInfo createTransInfo;
	TransitionTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createTransInfo);
	TransitionTextureUAV = RHICreateUnorderedAccessView(TransitionTexture);
	TransitionTextureSRV = RHICreateShaderResourceView(TransitionTexture, 0);

	FRHIResourceCreateInfo createFFTTransInfo;
	FFTTransitionTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createFFTTransInfo);
	FFTTransitionTextureUAV = RHICreateUnorderedAccessView(FFTTransitionTexture);
	FFTTransitionTextureSRV = RHICreateShaderResourceView(FFTTransitionTexture, 0);


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

	if (TmpFFTTextureUAV != NULL)
	{
		TmpFFTTextureUAV.SafeRelease();
		TmpFFTTextureUAV = NULL;
	}

	if (TmpFFTTextureSRV != NULL)
	{
		TmpFFTTextureSRV.SafeRelease();
		TmpFFTTextureSRV = NULL;
	}

	if (TmpFFTTexture != NULL)
	{
		TmpFFTTexture.SafeRelease();
		TmpFFTTexture = NULL;
	}

	if (TransitionTextureUAV != NULL)
	{
		TransitionTextureUAV.SafeRelease();
		TransitionTextureUAV = NULL;
	}

	if (TransitionTextureSRV != NULL)
	{
		TransitionTextureSRV.SafeRelease();
		TransitionTextureSRV = NULL;

	}

	if (TransitionTexture != NULL)
	{
		TransitionTexture.SafeRelease();
		TransitionTexture = NULL;
	}

	if (FFTTransitionTextureUAV != NULL)
	{
		FFTTransitionTextureUAV.SafeRelease();
		FFTTransitionTextureUAV = NULL;
	}

	if (FFTTransitionTextureSRV != NULL)
	{
		FFTTransitionTextureSRV.SafeRelease();
		FFTTransitionTextureSRV = NULL;

	}

	if (TransitionTexture != NULL)
	{
		FFTTransitionTexture.SafeRelease();
		FFTTransitionTexture = NULL;
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

	if (DxDyTextureSRV != NULL)
	{
		DxDyTextureSRV.SafeRelease();
		DxDyTextureSRV = NULL;
	}
	if (DxDyTextureUAV != NULL)
	{
		DxDyTextureUAV.SafeRelease();
		DxDyTextureUAV = NULL;
	}
	if (DxDyTexture != NULL)
	{
		DxDyTexture.SafeRelease();
		DxDyTexture = NULL;
	}
	if (DvxDvyTextureSRV != NULL)
	{
		DvxDvyTextureSRV.SafeRelease();
		DvxDvyTextureSRV = NULL;
	}
	if (DvxDvyTextureUAV != NULL)
	{
		DvxDvyTextureUAV.SafeRelease();
		DvxDvyTextureUAV = NULL;
	}
	if (DvxDvyTexture != NULL)
	{
		DvxDvyTexture.SafeRelease();
		DvxDvyTexture = NULL;
	}
}



DECLARE_GPU_STAT_NAMED(SIM_Total, TEXT("SIM_Total"));
void FComputeTestExecute::ExecuteComputeShader(
	UTextureRenderTarget2D* inputRenderTarget, FTexture2DRHIRef inputTexture, 
	FTexture2DRHIRef _obsTexture, FTexture2DRHIRef _flowMap, 
	FColor &DisplayColor, const FEWaveData &eWaveData)
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

	m_VariableParameters.mag = eWaveData.magnitude;
	m_VariableParameters.deltaTime = eWaveData.delTime;
	//m_VariableParameters.choppyScale = _choppyScale;
	
	m_Lx = eWaveData.SimGridSize.X;
	m_Ly = eWaveData.SimGridSize.Y;

	bGenGrad = eWaveData.bGenGrad;
	bcalcNonLinear = eWaveData.bCalcNonLinear;

	struct RenderTargetInput {
		UTextureRenderTarget2D* inputRT;
		bool bUseRenderTarget;
		FTexture2DRHIRef inTexture;
		bool bUseObsMap;
		FTexture2DRHIRef obsTexture;
		float _velScale;
	};
	//TSharedPtr<RenderTargetInput, ESPMode::ThreadSafe> rtInput = MakeShared<RenderTargetInput, ESPMode::ThreadSafe>();
	RenderTargetInput* rtInput = new RenderTargetInput();
	rtInput->inputRT = inputRenderTarget;
	rtInput->bUseRenderTarget = eWaveData.bUseRenderTarget;
	//rtInput->bUseRenderTarget = true;
	rtInput->inTexture = inputTexture;
	rtInput->obsTexture = _obsTexture;
	rtInput->bUseObsMap = bUseObsMap;
	rtInput->_velScale = eWaveData.flowScale;
	//TSharedPtr<FComputeTestExecute, ESPMode::ThreadSafe> MyShader = this;
	FComputeTestExecute* MyShader = this;
	
	bool SuccessInput = true;

	ENQUEUE_RENDER_COMMAND(ComputeShaderRun)(
		[MyShader, rtInput, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			//SCOPE_CYCLE_COUNTER(SIM_AddingSources);
			//SCOPED_GPU_STAT(RHICmdList, SIM_AddingSources);
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);
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

			if (MyShader->bMustRegenerateSRV)
			{
				MyShader->bMustRegenerateSRV = false;
				if (NULL != MyShader->InTextureSRV)
				{
					MyShader->InTextureSRV.SafeRelease();
					MyShader->InTextureSRV = NULL;
				}

				MyShader->InTextureSRV = RHICreateShaderResourceView(MyShader->InputTexture, 0);
			}

			if (rtInput->bUseObsMap)
			{
				if (MyShader->ObsTexture != rtInput->obsTexture)
				{
					MyShader->bMustRegenerateObsSRV = true;
				}
				MyShader->ObsTexture = rtInput->obsTexture;
			}

			if (MyShader->bMustRegenerateObsSRV)
			{
				MyShader->bMustRegenerateObsSRV = false;
				if (NULL != MyShader->ObsTextureSRV)
				{
					MyShader->ObsTextureSRV.SafeRelease();
					MyShader->ObsTextureSRV = NULL;
				}

				MyShader->ObsTextureSRV = RHICreateShaderResourceView(MyShader->ObsTexture, 0);
			}

			SuccessInput = MyShader->ExecuteComputeShaderInternal(
				RHICmdList, MyShader->InTextureSRV, MyShader->ObsTextureSRV, 
				MyShader->FFTTransitionTextureUAV, MyShader->h0_phi0_UAV);
		}
	);

	ENQUEUE_RENDER_COMMAND(ForwardFFT)(
		[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			//SCOPE_CYCLE_COUNTER(SIM_ForwardFFT);

			//SCOPED_GPU_STAT(RHICmdList, SIM_ForwardFFT);
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);

			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(
					RHICmdList, MyShader->FFTTransitionTextureSRV, MyShader->FFTTransitionTextureUAV, true);
			}
		}
	);

	ENQUEUE_RENDER_COMMAND(CalcEWave)(
		[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			//SCOPE_CYCLE_COUNTER(SIM_CalcEWave);

			//SCOPED_GPU_STAT(RHICmdList, SIM_CalcEWave);
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);

			
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteEWave(
					RHICmdList, MyShader->FFTTransitionTextureSRV, MyShader->WaveTextureUAV);
			}
		}
	);

	ENQUEUE_RENDER_COMMAND(InverseFFT)(
		[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			//SCOPE_CYCLE_COUNTER(SIM_InverseFFT);

			//SCOPED_GPU_STAT(RHICmdList, SIM_InverseFFT);
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);

			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(
					RHICmdList, MyShader->WaveTextureSRV, MyShader->WaveTextureUAV,false);
			}
		}
	);

	struct AdvectFields {
		FTexture2DRHIRef _flowMap;
		bool bUseFlowMap;
		float velScale;
	};
	//TSharedPtr<AdvectFields, ESPMode::ThreadSafe> advectVars = MakeShared<AdvectFields, ESPMode::ThreadSafe>();
	AdvectFields* advectVars = new AdvectFields();
	advectVars->_flowMap = _flowMap;
	advectVars->bUseFlowMap = bUseFlowMap;
	advectVars->velScale = eWaveData.flowScale;
	//advectVars->velScale = 0.0f;
	ENQUEUE_RENDER_COMMAND(AdvectField)(
		[MyShader, advectVars, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			//SCOPE_CYCLE_COUNTER(SIM_SLAdvectWave);

			//SCOPED_GPU_STAT(RHICmdList, SIM_SLAdvectWave);
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);
			
			if (advectVars->bUseFlowMap)
			{
				if (MyShader->FlowTexture != advectVars->_flowMap)
				{
					MyShader->bMustRegenerateFlowSRV = true;
				}
				MyShader->FlowTexture = advectVars->_flowMap;
			}

			if (MyShader->bMustRegenerateFlowSRV)
			{
				MyShader->bMustRegenerateFlowSRV = false;
				if (NULL != MyShader->FlowTextureSRV)
				{
					MyShader->FlowTextureSRV.SafeRelease();
					MyShader->FlowTextureSRV = NULL;
				}

				MyShader->FlowTextureSRV = RHICreateShaderResourceView(MyShader->FlowTexture, 0);
			}

			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteAdvectFields(
					RHICmdList, advectVars->velScale, 
					MyShader->WaveTextureSRV, MyShader->FlowTextureSRV, 
					MyShader->TransitionTextureUAV);
			}
		}
	);
	
	if (eWaveData.bCalcNonLinear || eWaveData.bGenGrad)
	{
		ENQUEUE_RENDER_COMMAND(ForwardFFTHeight)(
			[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);

			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(
					RHICmdList, MyShader->TransitionTextureSRV, MyShader->FFTTransitionTextureUAV, true);
			}
		}
		);

		//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, MyShader->WaveTextureUAV);
		ENQUEUE_RENDER_COMMAND(CalcNonLinearAndGrad)(
			[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);

			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteNonLinearAndGrad(
					RHICmdList, MyShader->FFTTransitionTextureSRV, MyShader->DxDyTextureUAV, MyShader->DvxDvyTextureUAV);
			}

		}
		);


		ENQUEUE_RENDER_COMMAND(InverseFFTPhi)(
			[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(
					RHICmdList, MyShader->DvxDvyTextureSRV, MyShader->DvxDvyTextureUAV, false);
			}

			//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, MyShader->WaveTextureUAV);


			//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, MyShader->DxDyTextureUAV);

		}
		);

		ENQUEUE_RENDER_COMMAND(InverseFFTDxDy)(
			[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_GPU_STAT(RHICmdList, SIM_Total);
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(RHICmdList,
					MyShader->DxDyTextureSRV, MyShader->DxDyTextureUAV, false);
			}
		}
		);
	}

	ENQUEUE_RENDER_COMMAND(ApplyFields)(
		[MyShader, rtInput, &SuccessInput](FRHICommandListImmediate& RHICmdList)
	{
		SCOPED_GPU_STAT(RHICmdList, SIM_Total);
		if (rtInput->bUseObsMap)
		{
			if (MyShader->ObsTexture != rtInput->obsTexture)
			{
				MyShader->bMustRegenerateObsSRV = true;
			}
			MyShader->ObsTexture = rtInput->obsTexture;
		}

		if (MyShader->bMustRegenerateObsSRV)
		{
			MyShader->bMustRegenerateObsSRV = false;
			if (NULL != MyShader->ObsTextureSRV)
			{
				MyShader->ObsTextureSRV.SafeRelease();
				MyShader->ObsTextureSRV = NULL;
			}

			MyShader->ObsTextureSRV = RHICreateShaderResourceView(MyShader->ObsTexture, 0);
		}
		if (SuccessInput)
		{
			SuccessInput = MyShader->ExecuteApplyFields(
				RHICmdList, 1.0, MyShader->TransitionTextureSRV, MyShader->DxDyTextureSRV,
				MyShader->ObsTextureSRV,
				MyShader->h0_phi0_UAV, MyShader->OutTextureUAV);
			RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, MyShader->OutTextureUAV);
		}
	}
	);
	//	UE_LOG(InternalShaderLog, Warning, TEXT("Shader Computed"));
	
	bIsComputeShaderExecuting = false;
}

bool FComputeTestExecute::ExecuteComputeShaderInternal(
	FRHICommandListImmediate& RHICmdList, 
	const FShaderResourceViewRHIRef& _SrcTexSRV, const FShaderResourceViewRHIRef& _ObsTexSRV,
	FUnorderedAccessViewRHIRef& DstTexUAV, FUnorderedAccessViewRHIRef& StructBufferUAV)
{
	check(IsInRenderingThread());
	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}

	TShaderMapRef<FAddSourceHeightCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, inColor, bUseObsMap, m_Lx, m_Ly, _SrcTexSRV, _ObsTexSRV);
	ComputeShader->SetOutput(RHICmdList, DstTexUAV, StructBufferUAV);
	ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);
	//RHICmdList.DispatchComputeShader(InputTexture->GetSizeX()/8, InputTexture->GetSizeY()/8,1);
	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(InputTexture->GetSizeX() / NUM_THREADS_PER_GROUP), FMath::CeilToInt(InputTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, OutTextureUAV);

	
	return true;
	
}

bool FComputeTestExecute::ExecuteEWave(
	FRHICommandListImmediate& RHICmdList,
	const FShaderResourceViewRHIParamRef& SrcSRV,
	FUnorderedAccessViewRHIRef& DstUAV)
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

	ComputeShader->SetParameters(RHICmdList, m_Lx, m_Ly, SrcSRV);
	ComputeShader->SetOutput(RHICmdList, DstUAV);
	ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);
	//UE_LOG(LogTemp, Warning, TEXT("OutTextureDims: %d, %d"), FMath::CeilToInt(512 / NUM_THREADS_PER_GROUP), FMath::CeilToInt(512 / NUM_THREADS_PER_GROUP));
	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(OutTexture->GetSizeX() / NUM_THREADS_PER_GROUP), FMath::CeilToInt(OutTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, WaveTextureUAV);

	return true;
}

bool FComputeTestExecute::ExecuteFFT(
	FRHICommandListImmediate& RHICmdList, 
	const FShaderResourceViewRHIParamRef& SrcSRV, 
	FUnorderedAccessViewRHIRef& DstUAV, bool bIsForward)
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
	SuccessValue = MyGPUFFT::FFTImage2D(
		FFTContext, FrequencySize, bIsHorizontal, bIsForward, SrcRect, 
		SrcSRV, DstUAV, TmpFFTTextureUAV, TmpFFTTextureSRV);
	/*
	if (bIsForward)
	{
		SuccessValue = MyGPUFFT::FFTImage2D(FFTContext, FrequencySize, bIsHorizontal, bIsForward, SrcRect, OutTextureSRV, OutTextureUAV, TmpTextureUAV, TmpTextureSRV);
	}
	else
	{
		SuccessValue = MyGPUFFT::FFTImage2D(FFTContext, FrequencySize, bIsHorizontal, bIsForward, SrcRect, WaveTextureSRV, OutTextureUAV, TmpTextureUAV, TmpTextureSRV);

	}
	*/
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, OutTextureUAV);
	return SuccessValue;
}

bool FComputeTestExecute::ExecuteAdvectFields(
	FRHICommandListImmediate& RHICmdList, float _velScale,
	const FShaderResourceViewRHIRef& _SrcTexSRV, const FShaderResourceViewRHIRef& _velTexSRV,
	FUnorderedAccessViewRHIRef& DstTexUAV
	)
{
	check(IsInRenderingThread());

	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}

	TShaderMapRef<FAdvectFieldsCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, _SrcTexSRV, _velTexSRV, bUseFlowMap, _velScale, m_Lx, m_Ly);
	ComputeShader->SetOutput(RHICmdList, DstTexUAV);
	ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);

	DispatchComputeShader(
		RHICmdList, *ComputeShader, 
		FMath::CeilToInt(OutTexture->GetSizeX() / NUM_THREADS_PER_GROUP), 
		FMath::CeilToInt(OutTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	
	ComputeShader->UnbindBuffers(RHICmdList);
	//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, h0_phi0_UAV);

	return true;
}

bool FComputeTestExecute::ExecuteApplyFields(
	FRHICommandListImmediate& RHICmdList, float _velScale,
	const FShaderResourceViewRHIRef& _SrcTexSRV, const FShaderResourceViewRHIRef& gradTexSRV,
	const FShaderResourceViewRHIRef& obsTexSRV,
	FUnorderedAccessViewRHIRef& StructBufferUAV, FUnorderedAccessViewRHIRef& _DstTexUAV
	)
{
	check(IsInRenderingThread());
	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}

	TShaderMapRef<FApplyFieldsCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
	ComputeShader->SetParameters(RHICmdList, _SrcTexSRV, gradTexSRV, DvxDvyTextureSRV, obsTexSRV);
	ComputeShader->SetParameters(RHICmdList, bcalcNonLinear, bUseObsMap, _velScale, m_Lx, m_Ly);
	ComputeShader->SetOutput(RHICmdList, StructBufferUAV, _DstTexUAV);
	//ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);

	DispatchComputeShader(
		RHICmdList, *ComputeShader, 
		FMath::CeilToInt(OutTexture->GetSizeX() / NUM_THREADS_PER_GROUP), 
		FMath::CeilToInt(OutTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);
	
	return true;
}

bool FComputeTestExecute::ExecuteNonLinearAndGrad(
	FRHICommandListImmediate& RHICmdList, const FShaderResourceViewRHIRef& SrcSRV, 
	FUnorderedAccessViewRHIRef& gradUAV, FUnorderedAccessViewRHIRef& dstUAV)
{
	check(IsInRenderingThread());
	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}

	TShaderMapRef<FGenGradCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
	ComputeShader->SetParameters(RHICmdList, SrcSRV, bGenGrad, bcalcNonLinear, m_Lx, m_Ly);
	ComputeShader->SetOutput(RHICmdList, gradUAV, dstUAV);
	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(OutTexture->GetSizeX() / NUM_THREADS_PER_GROUP), FMath::CeilToInt(OutTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);
	return true;

}

