#include "ComputeTestImplementation.h"
#include "ComputeTestPrivatePCH.h"
#include "Public/RHIStaticStates.h"
#include "Public/PipelineStateCache.h"

#define NUM_THREADS_PER_GROUP 32
DECLARE_LOG_CATEGORY_EXTERN(InternalShaderLog, Log, All);
DEFINE_LOG_CATEGORY(InternalShaderLog);
FComputeTestExecute::FComputeTestExecute(ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;
	bIsUnloading = false;
	bIsComputeShaderExecuting = false;
	bMustRegenerateUAV = false;
	bMustRegenerateSRV = false;

	FRHIResourceCreateInfo CreateInfo;
	Texture = NULL;
	TextureUAV = NULL;
	InputTexture = NULL;
	InTextureSRV = NULL;

}

FComputeTestExecute::~FComputeTestExecute()
{
	bIsUnloading = true;
}

void FComputeTestExecute::ExecuteComputeShader(FTexture2DRHIRef _InTexture, FTexture2DRHIRef _OutputTexture, FColor DisplayColor)
{
	check(IsInGameThread());

	if (bIsUnloading || bIsComputeShaderExecuting)
		return;
	bIsComputeShaderExecuting = true;
	if (Texture != _OutputTexture)
	{
		bMustRegenerateUAV = true;
	}
	if (InputTexture != _InTexture)
	{
		bMustRegenerateSRV = true;
	}

	Texture = _OutputTexture;
	InputTexture = _InTexture;
	inColor = FLinearColor(DisplayColor.R / 255.0, DisplayColor.G / 255.0, DisplayColor.B / 255.0, DisplayColor.A / 255.0);
	UE_LOG(InternalShaderLog, Warning, TEXT("Shader Variables Initialized"));

	
	FComputeTestExecute* MyShader = this;
	ENQUEUE_RENDER_COMMAND(ComputeShaderRun)(
		[MyShader](FRHICommandListImmediate& RHICmdList)
		{
			MyShader->ExecuteComputeShaderInternal(RHICmdList);
		}
	);
}

void FComputeTestExecute::ExecuteComputeShaderInternal(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());

	if (bIsUnloading)
	{
		if (NULL != TextureUAV)
		{
			TextureUAV.SafeRelease();
			TextureUAV = NULL;
		}
		if (NULL != InTextureSRV)
		{
			InTextureSRV.SafeRelease();
			InTextureSRV = NULL;
		}
		return;
	}

	if (bMustRegenerateUAV)
	{
		bMustRegenerateUAV = false;

		if (NULL != TextureUAV)
		{
			TextureUAV.SafeRelease();
			TextureUAV = NULL;
		}
		
		TextureUAV = RHICreateUnorderedAccessView(Texture);
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

	TShaderMapRef<FRenderUVCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, inColor, InTextureSRV, InputTexture);
	ComputeShader->SetOutput(RHICmdList, TextureUAV);
	//RHICmdList.DispatchComputeShader(InputTexture->GetSizeX()/8, InputTexture->GetSizeY()/8,1);
	DispatchComputeShader(RHICmdList, *ComputeShader, FMath::CeilToInt(InputTexture->GetSizeX() / 32), FMath::CeilToInt(InputTexture->GetSizeY() / 32), 1);
	ComputeShader->UnbindBuffers(RHICmdList);

	bIsComputeShaderExecuting = false;
}
