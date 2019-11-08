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
	bMustRegenerateGradSRV = false;
	bisUnloading = false;

	CurrentTexture = NULL;
	CurrentRenderTarget = NULL;
	TextureParameterSRV = NULL;
	GradParameter = NULL;
	CurGradTexture = NULL;
	GradParameterSRV = NULL;
	CurNormMapRT = NULL;
	CurNormalTexture = NULL;

	m_pQuadVB[0].Position = FVector4(-1.0f, 1.0f, 0.0f, 1.0f);
	m_pQuadVB[1].Position = FVector4(1.0f, 1.0f, 0.0f, 1.0f);
	m_pQuadVB[2].Position = FVector4(-1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[3].Position = FVector4(1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[0].UV = FVector2D(0, 0);
	m_pQuadVB[1].UV = FVector2D(1, 0);
	m_pQuadVB[2].UV = FVector2D(0, 1);
	m_pQuadVB[3].UV = FVector2D(1, 1);

}

FDisplayShaderExecute::~FDisplayShaderExecute()
{
	bisUnloading = true;
}

DECLARE_GPU_STAT_NAMED(SIM_Display, TEXT("SIM_RTDisplay"));

void FDisplayShaderExecute::ExecuteDisplayShader(
	UTextureRenderTarget2D* RenderTarget, UTextureRenderTarget2D* NormMapRT, UTextureRenderTarget2D* GradMapRT,
	FTexture2DRHIRef InputTexture, FTexture2DRHIRef InGradTexture, const FEWaveData &eWaveData)
	
{
	check(IsInGameThread());

	if (bisUnloading || bIsPixelShaderExecuting)
		return;
	if (!RenderTarget || !NormMapRT )
		return;

	bIsPixelShaderExecuting = true;
	
	if (TextureParameter != InputTexture)
	{
		TextureParameter.SafeRelease();
		TextureParameter = NULL;
		bMustRegenerateSRV = true;
	}
	
	if (GradParameter != InGradTexture)
	{
		GradParameter.SafeRelease();
		GradParameter = NULL;
		bMustRegenerateGradSRV = true;
	}
	
	CurrentRenderTarget = RenderTarget;
	CurNormMapRT = NormMapRT;
	CurGradMapRT = GradMapRT;

	TextureParameter = InputTexture;
	GradParameter = InGradTexture;

	m_PSVariableParm.choppyness = eWaveData.choppyScale;
	m_PSVariableParm.dx = eWaveData.SimGridSize.X / (1.0f*eWaveData.TexMapSize.X);
	m_PSVariableParm.dy = eWaveData.SimGridSize.Y / (1.0f*eWaveData.TexMapSize.Y);
	
	m_Lx = eWaveData.SimGridSize.X;
	m_Ly = eWaveData.SimGridSize.Y;

	//m_PSVariableParm.choppyness = 1.3f;
	//m_PSVariableParm.dx = 10 / 512.0f;
	//m_PSVariableParm.dy = 10.0 / 512.0f;
	FDisplayShaderExecute* MyShader = this;
	ENQUEUE_RENDER_COMMAND(DisplayShaderRunner)(
		[MyShader](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_GPU_STAT(RHICmdList, SIM_Display);
			MyShader->ExecuteDisplayShader_RenderThread(RHICmdList);
		}
	);
	
	ENQUEUE_RENDER_COMMAND(NormalMapGen)(
		[MyShader](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_GPU_STAT(RHICmdList, SIM_Display);
			MyShader->ExecuteNormalRT_RenderThread(RHICmdList);
		}
	);

	if (GradMapRT)
	{
		ENQUEUE_RENDER_COMMAND(GradDisplay)(
			[MyShader](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_GPU_STAT(RHICmdList, SIM_Display);
			MyShader->ExecuteGrad_RenderThread(RHICmdList);
		}
		);
	}
	bIsPixelShaderExecuting = false;
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

	PixelShader->SetParameters(RHICmdList, TextureParameterSRV, TextureParameter, m_PSVariableParm.choppyness, m_Lx, m_Ly);
	PixelShader->SetUniformBuffers(RHICmdList, m_PSVariableParm);

	/*m_pQuadVB[0].Position = FVector4(-1.0f, 1.0f, 0.0f, 1.0f);
	m_pQuadVB[1].Position = FVector4(1.0f, 1.0f, 0.0f, 1.0f);
	m_pQuadVB[2].Position = FVector4(-1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[3].Position = FVector4(1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[0].UV = FVector2D(0, 0);
	m_pQuadVB[1].UV = FVector2D(1, 0);
	m_pQuadVB[2].UV = FVector2D(0, 1);
	m_pQuadVB[3].UV = FVector2D(1, 1);
	*/
	DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, m_pQuadVB, sizeof(m_pQuadVB[0]));

	PixelShader->UnbindBuffers(RHICmdList);
	RHICmdList.CopyToResolveTarget(
		CurrentRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(),
		CurrentRenderTarget->GetRenderTargetResource()->TextureRHI,	FResolveParams()
	);

}

void FDisplayShaderExecute::ExecuteNormalRT_RenderThread(FRHICommandListImmediate &RHICmdList)
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
	TShaderMapRef<FMyGenGradFoldingPS> PixelShader(GetGlobalShaderMap(FeatureLevel));

	//FEWavePSVariableParameterRef dispParameter = FEWavePSVariableParameterRef::CreateUniformBufferImmediate(m_PSVariableParm, UniformBuffer_SingleFrame);

	CurNormalTexture = CurNormMapRT->GetRenderTargetResource()->GetRenderTargetTexture();
	SetRenderTarget(RHICmdList, CurNormalTexture, FTexture2DRHIRef());
	
	FGraphicsPipelineStateInitializer PSOInitializer;
	RHICmdList.ApplyCachedRenderTargets(PSOInitializer);

	PSOInitializer.PrimitiveType = PT_TriangleStrip;
	PSOInitializer.BoundShaderState.VertexDeclarationRHI = GQuadVertexDeclaration.VertexDeclarationRHI;
	PSOInitializer.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	PSOInitializer.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	PSOInitializer.BlendState = TStaticBlendState<>::GetRHI();
	PSOInitializer.RasterizerState = TStaticRasterizerState<>::GetRHI();
	PSOInitializer.DepthStencilState = TStaticDepthStencilState<false,CF_Always>::GetRHI();
	

	SetGraphicsPipelineState(RHICmdList, PSOInitializer);

	PixelShader->SetParameters(RHICmdList, CurrentRenderTarget->GetRenderTargetResource()->TextureRHI, m_PSVariableParm.choppyness, m_Lx, m_Ly);
	//PixelShader->SetUniformBuffers(RHICmdList, dispParameter);

	

	DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, m_pQuadVB, sizeof(m_pQuadVB[0]));

	PixelShader->UnbindBuffers(RHICmdList);
	
	RHICmdList.GenerateMips(CurNormMapRT->GetRenderTargetResource()->TextureRHI);
}

void FDisplayShaderExecute::ExecuteGrad_RenderThread(FRHICommandListImmediate &RHICmdList)
{
	check(IsInRenderingThread());
	if (bisUnloading)
	{
		if (NULL != GradParameterSRV)
		{
			GradParameterSRV.SafeRelease();
			GradParameterSRV = NULL;
		}
		return;
	}

	if (bMustRegenerateGradSRV)
	{
		bMustRegenerateGradSRV = false;

		if (NULL != GradParameterSRV)
		{
			GradParameterSRV.SafeRelease();
			GradParameterSRV = NULL;
		}

		GradParameterSRV = RHICreateShaderResourceView(GradParameter, 0);
	}

	TShaderMapRef<FMyQuadVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FMyDisplayGradPS> PixelShader(GetGlobalShaderMap(FeatureLevel));

	//FEWavePSVariableParameterRef dispParameter = FEWavePSVariableParameterRef::CreateUniformBufferImmediate(m_PSVariableParm, UniformBuffer_SingleFrame);

	CurGradTexture = CurGradMapRT->GetRenderTargetResource()->GetRenderTargetTexture();
	SetRenderTarget(RHICmdList, CurGradTexture, FTexture2DRHIRef());

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

	PixelShader->SetParameters(RHICmdList,GradParameter);
	//PixelShader->SetUniformBuffers(RHICmdList, dispParameter);

	DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, m_pQuadVB, sizeof(m_pQuadVB[0]));

	PixelShader->UnbindBuffers(RHICmdList);

	RHICmdList.GenerateMips(CurNormMapRT->GetRenderTargetResource()->TextureRHI);
}
