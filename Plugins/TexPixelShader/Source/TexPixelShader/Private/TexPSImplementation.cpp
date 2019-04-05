// Fill out your copyright notice in the Description page of Project Settings.

#include "TexPixelShaderPrivatePCH.h"
#include "Public/RHIStaticStates.h"
#include "Public/PipelineStateCache.h"
#include "GameFramework/InputSettings.h"

TGlobalResource<FQuadVertexDeclaration> GQuadVertexDeclaration;

FTexPSImplementation::FTexPSImplementation(FColor StartColor, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;
	
	bIsPixelShaderExecuting = false;
	bMustRegenerateSRV = false;
	bisUnloading = false;
	bSave = false;

	CurrentTexture = NULL;
	CurrentRenderTarget = NULL;
	TextureParameterSRV = NULL;

	VariableParameters = FPixelShaderVariableParameters();

	m_startCol = FLinearColor(StartColor.R / 255.0, StartColor.G / 255.0, StartColor.B / 255.0, StartColor.A / 255.0);

	
}

FTexPSImplementation::~FTexPSImplementation()
{
	bisUnloading = true;
}

void FTexPSImplementation::ExecutePixelShader(UTextureRenderTarget2D* RenderTarget, FTexture2DRHIRef InputTexture, FColor InputColor, float BlendAmount)
{
	check(IsInGameThread());

	if (bisUnloading || bIsPixelShaderExecuting)
		return;
	if (!RenderTarget)
		return;

	bIsPixelShaderExecuting = true;

	if (TextureParameter != InputTexture)
		bMustRegenerateSRV = true;

	VariableParameters.BlendFactor = BlendAmount;

	CurrentRenderTarget = RenderTarget;
	TextureParameter = InputTexture;
	m_startCol = FLinearColor(InputColor.R / 255.0, InputColor.G / 255.0, InputColor.B / 255.0, InputColor.A / 255.0);
	
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FPixelShaderRunner,
		FTexPSImplementation*, MyShader, this,
		{
			MyShader->ExecutePixelShaderInternal();
		}
	);

}

void FTexPSImplementation::ExecutePixelShaderInternal()
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

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	//If our input texture reference has changed, we need to recreate our SRV
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

	TShaderMapRef<FMyTestVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FMyTestPS> PixelShader(GetGlobalShaderMap(FeatureLevel));

	//Way to get renderTarget Texture
	CurrentTexture = CurrentRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
	SetRenderTarget(RHICmdList, CurrentTexture, FTextureRHIRef());

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
	//TextureParameter is a FTexture2DRHIRef
	PixelShader->SetParameters(RHICmdList, m_startCol, TextureParameterSRV,TextureParameter);
	PixelShader->SetUniformBuffers(RHICmdList, VariableParameters);

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
	    FResolveParams());

	if (bSave)
	{
		bSave = false;
		SaveScreenshot(RHICmdList);
	}

	bIsPixelShaderExecuting = false;

}

void FTexPSImplementation::SaveScreenshot(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());

	TArray<FColor> Bitmap;

	FReadSurfaceDataFlags ReadDataFlags;
	ReadDataFlags.SetLinearToGamma(false);
	ReadDataFlags.SetOutputStencil(false);
	ReadDataFlags.SetMip(0);

	RHICmdList.ReadSurfaceData(CurrentTexture, FIntRect(0, 0, CurrentTexture->GetSizeX(), CurrentTexture->GetSizeY()), Bitmap, ReadDataFlags);
	if (Bitmap.Num())
	{
		IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);
		const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("VisualizeTexture"));
		uint32 ExtendXWithMSAA = Bitmap.Num() / CurrentTexture->GetSizeY();

		FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, CurrentTexture->GetSizeY(), Bitmap.GetData());
		UE_LOG(LogConsoleResponse, Display, TEXT("Context was saved to \"%s\""), *FPaths::ScreenShotDir());
	}
	else
	{
		UE_LOG(LogConsoleResponse, Error, TEXT("Failed to save BMP, format or texture type is not supported"));
	}
}
