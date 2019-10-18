#include "ComputeTestImplementation.h"
#include "ComputeTestPrivatePCH.h"
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
	
	InputTexture = NULL;
	InTextureSRV = NULL;
	FRHIResourceCreateInfo createInfo;
	OutTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createInfo);
	OutTextureUAV = RHICreateUnorderedAccessView(OutTexture);
	OutTextureSRV = RHICreateShaderResourceView(OutTexture, 0);

	FRHIResourceCreateInfo createTmpInfo;
	TmpTexture = RHICreateTexture2D(sizeX, sizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, createTmpInfo);
	TmpTextureUAV = RHICreateUnorderedAccessView(TmpTexture);
	TmpTextureSRV = RHICreateShaderResourceView(TmpTexture, 0);

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
}

void FComputeTestExecute::ExecuteComputeShader(UTextureRenderTarget2D* inputRenderTarget, FTexture2DRHIRef inputTexture, const FColor &DisplayColor, float _mag, float _delTime, bool bUseRenderTarget)
{
	check(IsInGameThread());

	if (!bSimulatorInitialized)
	{
		return;
	}

	if (bIsUnloading || bIsComputeShaderExecuting)
		return;
	bIsComputeShaderExecuting = true;

	inColor = FLinearColor(DisplayColor.R / 255.0, DisplayColor.G / 255.0, DisplayColor.B / 255.0, DisplayColor.A / 255.0);
	UE_LOG(InternalShaderLog, Warning, TEXT("Shader Variables Initialized"));

	m_VariableParameters.mag = _mag;
	m_VariableParameters.deltaTime = _delTime;
	
	struct RenderTargetInput {
		UTextureRenderTarget2D* inputRT;
		bool bUseRenderTarget;
		FTexture2DRHIRef inTexture;
	};
	RenderTargetInput* rtInput = new RenderTargetInput();
	rtInput->inputRT = inputRenderTarget;
	rtInput->bUseRenderTarget = bUseRenderTarget;
	rtInput->inTexture = inputTexture;
	FComputeTestExecute* MyShader = this;

	bool SuccessInput = true;

	ENQUEUE_RENDER_COMMAND(ComputeShaderRun)(
		[MyShader, rtInput, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			check(IsInRenderingThread());
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
			
			  
			SuccessInput = MyShader->ExecuteComputeShaderInternal(RHICmdList);
		}
	);

	ENQUEUE_RENDER_COMMAND(ForwardFFT)(
		[MyShader, &SuccessInput](FRHICommandListImmediate& RHICmdList)
		{
			check(IsInRenderingThread());
			if (SuccessInput)
			{
				SuccessInput = MyShader->ExecuteFFT(RHICmdList);
			}
		}
	);
	bIsComputeShaderExecuting = false;

}

bool FComputeTestExecute::ExecuteComputeShaderInternal(FRHICommandListImmediate& RHICmdList)
{
	
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

	
	TShaderMapRef<FAddSourceHeightCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, inColor, InTextureSRV);
	ComputeShader->SetOutput(RHICmdList, OutTextureUAV, h0_phi0_UAV);
	ComputeShader->SetUniformBuffers(RHICmdList, m_VariableParameters);
	//RHICmdList.DispatchComputeShader(InputTexture->GetSizeX()/8, InputTexture->GetSizeY()/8,1);
	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(InputTexture->GetSizeX() / NUM_THREADS_PER_GROUP), FMath::CeilToInt(InputTexture->GetSizeY() / NUM_THREADS_PER_GROUP), 1);
	ComputeShader->UnbindBuffers(RHICmdList);

	
	return true;
	
}

bool FComputeTestExecute::ExecuteFFT(FRHICommandListImmediate& RHICmdList)
{
	if (bIsUnloading)
	{
		ClearInternalData();
		return false;
	}

	bool SuccessValue = false;
	bool bIsForward = true;
	bool bIsHorizontal = true;

	return SuccessValue;
}

