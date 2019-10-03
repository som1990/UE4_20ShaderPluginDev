#include "VS_PSImplementation.h"
#include "ComputeTestPrivatePCH.h"
#include "Public/RHIStaticStates.h"
#include "Public/PipelineStateCache.h"

TGlobalResource<FQuadVertexDeclaration> GQuadVertexDeclaration;

FDisplayShaderExecute::FDisplayShaderExecute(int sizeX, int32 sizeY, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;

	bIsPixelShaderExecuting = false;
	bMustRegenerateSRV = false;
	bisUnloading = false;

	CurrentTexture = NULL;
	CurrentRenderTarget = NULL;
	TextureParameterSRV = NULL;

}

FDisplayShaderExecute::~FDisplayShaderExecute()
{
	bisUnloading = true;
}

void FDisplayShaderExecute::ExecuteDisplayShader(UTextureRenderTarget2D* RenderTarget, FTexture2DRHIRef InputTexture)
{
	check(IsInGameThread());

	if (bisUnloading || bIsPixelShaderExecuting)
		return;
	if (!RenderTarget)
		return;

	bIsPixelShaderExecuting = true;
	
	if (TextureParameter != InputTexture)
	{
		TextureParameter.SafeRelease();
		TextureParameter = NULL;
		bMustRegenerateSRV = true;
	}
	CurrentRenderTarget = RenderTarget;
	TextureParameter = InputTexture;

	FDisplayShaderExecute* MyShader = this;
	ENQUEUE_RENDER_COMMAND(DisplayShaderRunner)(
		[MyShader](FRHICommandListImmediate& RHICmdList)
		{
			MyShader->ExecuteDisplayShader_RenderThread(RHICmdList);
		}
	);
}

void FDisplayShaderExecute::ExecuteDisplayShader_RenderThread(FRHICommandListImmediate &RHICmdList)
{
	check(IsInRenderingThread());
	if (bisUnloading)
	{
		if (NULL != TextureParameterSRV)
		{
			TextureParameterSRV.SafeRelease();
			TextureParameterSRV = NULL;
		}
		return;
	}

	if (bMustRegenerateSRV)
	{
		bMustRegenerateSRV = false;

		if (NULL != TextureParameterSRV)
		{
			TextureParameterSRV.SafeRelease();
			TextureParameterSRV = NULL;
		}

		TextureParameterSRV = RHICreateShaderResourceView(TextureParameter, 0);
	}

	TShaderMapRef<FMyQuadVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FMyQuadPS> PixelShader(GetGlobalShaderMap(FeatureLevel));

	CurrentTexture = CurrentRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
	SetRenderTarget(RHICmdList, CurrentTexture, FTexture2DRHIRef());

	FGraphicsPipelineStateInitializer PSOInitializer;
	RHICmdList.ApplyCachedRenderTargets(PSOInitializer);

	PSOInitializer.PrimitiveType = PT_TriangleStrip;
	PSOInitializer.BoundShaderState.VertexDeclarationRHI = GQuadVertexDeclaration.VertexDeclarationRHI;
	PSOInitializer.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	PSOInitializer.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	PSOInitializer.BlendState = TStaticBlendState<>::GetRHI();
	PSOInitializer.RasterizerState = TStaticRasterizerState<>::GetRHI();
	PSOInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	SetGraphicsPipelineState(RHICmdList, PSOInitializer);

	PixelShader->SetParameters(RHICmdList, TextureParameterSRV, TextureParameter);
	
	m_pQuadVB[0].Position = FVector4(-1.0f, 1.0f, 0.0f, 1.0f);
	m_pQuadVB[1].Position = FVector4(1.0f, 1.0f, 0.0f, 1.0f);
	m_pQuadVB[2].Position = FVector4(-1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[3].Position = FVector4(1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[0].UV = FVector2D(0, 0);
	m_pQuadVB[1].UV = FVector2D(1, 0);
	m_pQuadVB[2].UV = FVector2D(0, 1);
	m_pQuadVB[3].UV = FVector2D(1, 1);

	DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, m_pQuadVB, sizeof(m_pQuadVB[0]));

	PixelShader->UnbindBuffers(RHICmdList);
	RHICmdList.CopyToResolveTarget(
		CurrentRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(),
		CurrentRenderTarget->GetRenderTargetResource()->TextureRHI,
		FResolveParams()
	);
	
	bIsPixelShaderExecuting = false;

}

