#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"


BEGIN_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters, )
UNIFORM_MEMBER(float, inputU)
UNIFORM_MEMBER(float, inputV)
END_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters)

typedef TUniformBufferRef<FComputeShaderVariableParameters> FComputeShaderVariableParameterRef;


class FRenderUVCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FRenderUVCS, Global);

public:
	FRenderUVCS() {}
	FRenderUVCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		MyColorParameter.Bind(Initializer.ParameterMap, TEXT("MyColor"));
		OutputSurface.Bind(Initializer.ParameterMap, TEXT("OutputSurface"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FLinearColor& Color
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(RHICmdList, ComputeShaderRHI, MyColorParameter, Color);
	}

	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputSurfaceUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (OutputSurface.IsBound())
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputSurface.GetBaseIndex(), OutputSurfaceUAV);
	}
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);
		Ar << MyColorParameter << OutputSurface;
		return bShaderHasOutdatedParams;
	}
	void SetUniformBuffers(FRHICommandList& RHICmdList, FComputeShaderVariableParameters& VariableParameters)
	{
		FComputeShaderVariableParameterRef VariableParametersBuffer;

		VariableParametersBuffer = FComputeShaderVariableParameterRef::CreateUniformBufferImmediate(VariableParameters, UniformBuffer_SingleDraw);

		SetUniformBufferParameter(RHICmdList, GetComputeShader(), GetUniformBufferParameter<FComputeShaderVariableParameters>(), VariableParametersBuffer);
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutputSurface.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputSurface.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
	
private:
	FShaderParameter MyColorParameter;
	FShaderResourceParameter OutputSurface;
	
};

struct FQuadVertex
{
	FVector4 Position;
	FVector2D UV;
};

class FQuadVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;
	virtual ~FQuadVertexDeclaration(){}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FQuadVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, UV), VET_Float2,1,Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override 
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

class FMyQuadVS : public FGlobalShader 
{
	DECLARE_SHADER_TYPE(FMyQuadVS, Global);
public:
	FMyQuadVS() {}
	FMyQuadVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{}
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

class FMyQuadPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyQuadPS, Global);

public:
	FMyQuadPS(){}
	FMyQuadPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:FGlobalShader(Initializer)
	{
		TextureParameter.Bind(Initializer.ParameterMap, TEXT("TextureParameter"));
	}
	
	void SetParameters(
		FRHICommandList& RHICmdList,
		FShaderResourceViewRHIParamRef TextureParameterSRV
	)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		if (TextureParameter.IsBound()) {
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, TextureParameter.GetBaseIndex(), TextureParameterSRV);
		}

		FSamplerStateRHIParamRef SamperStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		//SetSamplerParameter(RHICmdList, PixelShaderRHI, TexMapSampler, SamperStateLinear);
		
	}
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	virtual bool Serialize(FArchive &Ar) override 
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return true;
	}

private:
	FShaderResourceParameter TextureParameter;
	FShaderResourceParameter TexMapSampler;
};