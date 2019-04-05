#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

BEGIN_UNIFORM_BUFFER_STRUCT(FPixelShaderVariableParameters, )
UNIFORM_MEMBER(float, BlendFactor)
END_UNIFORM_BUFFER_STRUCT(FPixelShaderVariableParameters)

typedef TUniformBufferRef<FPixelShaderVariableParameters> FPixelShaderVariableParameterRef;

struct FQuadVertex
{
	FVector4 Position;
	FVector2D UV;
};

class FQuadVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual ~FQuadVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FQuadVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}

};

class FMyTestVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyTestVS, Global);
public:
	FMyTestVS() {}
	FMyTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
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

class FMyTestPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyTestPS, Global);

public:
	FMyTestPS() {}
	FMyTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:FGlobalShader(Initializer)
	{
		//This is used to let the shader system have parameters be available in the shader. 
		//The second Parameter has the name of the Parameter in the shader
		//The third Parameter determines if it's optional or mandatory.
		MyColorParameter.Bind(Initializer.ParameterMap, TEXT("MyColor"));
		TextureParameter.Bind(Initializer.ParameterMap, TEXT("TextureParameter"));
		TexMapSampler.Bind(Initializer.ParameterMap, TEXT("TexMapSampler"));
	}

	//This method is used to set non buffer(Texture) parameters. 
	void SetParameters(
		FRHICommandList& RHICmdList,
		const FLinearColor& Color,
		FShaderResourceViewRHIRef TextureParameterSRV,
		FTextureRHIParamRef InputTextureRef
	)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		// Stores incoming color parameter to MyColorParameter
		SetShaderValue(RHICmdList, PixelShaderRHI, MyColorParameter, Color);
		//Store the shader resource view to the texture parameter
		/*if (TextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI,
				TextureParameter.GetBaseIndex(), TextureParameterSRV);
		}
		*/
		FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		
		SetSamplerParameter(RHICmdList, PixelShaderRHI, TexMapSampler, SamplerStateLinear);
		SetTextureParameter(RHICmdList, PixelShaderRHI, TextureParameter, InputTextureRef);
	}

	void SetUniformBuffers(FRHICommandList& RHICmdList, FPixelShaderVariableParameters& VariableParameters)
	{
		FPixelShaderVariableParameterRef VariableParametersBuffer;

		VariableParametersBuffer =
			FPixelShaderVariableParameterRef::CreateUniformBufferImmediate(
				VariableParameters, UniformBuffer_SingleDraw);

		SetUniformBufferParameter(RHICmdList, GetPixelShader(),
			GetUniformBufferParameter<FPixelShaderVariableParameters>(),
			VariableParametersBuffer);
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		/*FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		if (TextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI,
				TextureParameter.GetBaseIndex(), FShaderResourceViewRHIRef());
		}*/
	
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << MyColorParameter << TextureParameter << TexMapSampler;
		return bShaderHasOutdatedParameters;
	}

	//Required Function
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

private:
	//If using Textures use FShaderResourceParameter
	FShaderParameter MyColorParameter;

	FShaderResourceParameter TextureParameter;
	FShaderResourceParameter TexMapSampler;

};