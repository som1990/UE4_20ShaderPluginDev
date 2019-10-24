#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"


BEGIN_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters, )
UNIFORM_MEMBER(float, mag)
UNIFORM_MEMBER(float, deltaTime)
END_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters)

typedef TUniformBufferRef<FComputeShaderVariableParameters> FComputeShaderVariableParameterRef;


class FAddSourceHeightCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAddSourceHeightCS, Global);

public:
	FAddSourceHeightCS() {}
	FAddSourceHeightCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : 
		FGlobalShader(Initializer)
	{
		MyColorParameter.Bind(Initializer.ParameterMap, TEXT("MyColor"));
		bUseObsMap.Bind(Initializer.ParameterMap, TEXT("bUseObsMap"));
		OutputSurface.Bind(Initializer.ParameterMap, TEXT("OutputSurface"));
		ObstructionParm.Bind(Initializer.ParameterMap, TEXT("SrcObstruction"));
		TextureParameter.Bind(Initializer.ParameterMap, TEXT("SrcTexture"));
		TexMapSampler.Bind(Initializer.ParameterMap, TEXT("TexMapSampler"));
		H0_phi0_RW.Bind(Initializer.ParameterMap, TEXT("h0_phi_RW"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("ADD_SOURCE_HEIGHTFIELD"), 1);
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), FAddSourceHeightCS::NumThreadsPerGroup());
	}

	static int32 NumThreadsPerGroup() {return 32;}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FLinearColor& Color,
		uint32 useObsMap,
		const FShaderResourceViewRHIParamRef& TextureParameterSRV,
		const FShaderResourceViewRHIParamRef& ObstructionParmSRV
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(RHICmdList, ComputeShaderRHI, MyColorParameter, Color);
		SetShaderValue(RHICmdList, ComputeShaderRHI, bUseObsMap, useObsMap);
		if (TextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, TextureParameter.GetBaseIndex(), TextureParameterSRV);
			FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			SetSamplerParameter(RHICmdList, ComputeShaderRHI, TexMapSampler, SamplerStateLinear);
			
		}
		if (ObstructionParm.IsBound() )
		{
			if (useObsMap){
				RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, ObstructionParm.GetBaseIndex(), ObstructionParmSRV);
			}
			else {
				RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, ObstructionParm.GetBaseIndex(), FShaderResourceViewRHIRef());
			}
		}
	}

	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputSurfaceUAV, FUnorderedAccessViewRHIParamRef Ht0_phi_UAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (OutputSurface.IsBound())
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputSurface.GetBaseIndex(), OutputSurfaceUAV);
		if (H0_phi0_RW.IsBound())
			RHICmdList.SetUAVParameter(ComputeShaderRHI, H0_phi0_RW.GetBaseIndex(), Ht0_phi_UAV);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);
		Ar << MyColorParameter << bUseObsMap << OutputSurface
			<< ObstructionParm << TextureParameter << TexMapSampler << H0_phi0_RW;
		return bShaderHasOutdatedParams;
	}

	void SetUniformBuffers(FRHICommandList& RHICmdList, FComputeShaderVariableParameters& VariableParameters)
	{
		FComputeShaderVariableParameterRef VariableParametersBuffer;

		VariableParametersBuffer =
			FComputeShaderVariableParameterRef::CreateUniformBufferImmediate(
				VariableParameters, UniformBuffer_SingleDraw);

		SetUniformBufferParameter(RHICmdList, GetComputeShader(),
			GetUniformBufferParameter<FComputeShaderVariableParameters>(),
			VariableParametersBuffer);
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutputSurface.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI,
				OutputSurface.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}

		if (H0_phi0_RW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI,
				H0_phi0_RW.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}

		if (TextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				TextureParameter.GetBaseIndex(), FShaderResourceViewRHIRef());
		}

		if (ObstructionParm.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				ObstructionParm.GetBaseIndex(), FShaderResourceViewRHIRef());
		}
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

private:
	FShaderParameter MyColorParameter;
	FShaderParameter bUseObsMap;
	FShaderResourceParameter OutputSurface;

	FShaderResourceParameter ObstructionParm;
	FShaderResourceParameter TextureParameter;
	FShaderResourceParameter TexMapSampler;
	FShaderResourceParameter H0_phi0_RW;


};

class FCalcEWaveCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCalcEWaveCS, Global);

public:
	FCalcEWaveCS() {}
	FCalcEWaveCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		worldGridWidth.Bind(Initializer.ParameterMap, TEXT("worldGridWidth"));
		worldGridHeight.Bind(Initializer.ParameterMap, TEXT("worldGridHeight"));
		OutHt_Pt_RW.Bind(Initializer.ParameterMap, TEXT("Out_hT_pT"));
		InHt_Pt_RO.Bind(Initializer.ParameterMap, TEXT("In_hT_pT"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("CALC_EWAVE"), 1);
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), FCalcEWaveCS::NumThreadsPerGroup());
	}

	static int32 NumThreadsPerGroup() { return 32; }

	void SetParameters(
		FRHICommandList& RHICmdList,
		const float &worldWidth,
		const float &worldHeight,
		FShaderResourceViewRHIParamRef InputTextureSRV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(RHICmdList, ComputeShaderRHI, worldGridWidth, worldWidth);
		SetShaderValue(RHICmdList, ComputeShaderRHI, worldGridHeight, worldHeight);

		if (InHt_Pt_RO.IsBound())
		{
			//UE_LOG(LogTemp, Warning, TEXT("SRV being set"));
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InHt_Pt_RO.GetBaseIndex(), InputTextureSRV);
		}
	}

	void SetOutput(
		FRHICommandList& RHICmdList,
		const FUnorderedAccessViewRHIParamRef &OutTextureUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutHt_Pt_RW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutHt_Pt_RW.GetBaseIndex(), OutTextureUAV);
		}
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutDatedParams = FGlobalShader::Serialize(Ar);
		Ar << worldGridWidth << worldGridHeight << OutHt_Pt_RW << InHt_Pt_RO;
		return bShaderHasOutDatedParams;
	}

	void SetUniformBuffers(FRHICommandList& RHICmdList, FComputeShaderVariableParameters& VariableParameters)
	{
		FComputeShaderVariableParameterRef VariableParametersBuffer;

		VariableParametersBuffer = FComputeShaderVariableParameterRef::CreateUniformBufferImmediate(
			VariableParameters, UniformBuffer_SingleDraw);

		SetUniformBufferParameter(RHICmdList, GetComputeShader(),
			GetUniformBufferParameter<FComputeShaderVariableParameters>(),
			VariableParametersBuffer);
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		
		if (OutHt_Pt_RW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, 
				OutHt_Pt_RW.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}

		if (InHt_Pt_RO.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				InHt_Pt_RO.GetBaseIndex(), FShaderResourceViewRHIRef());
		}
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

private: 
	FShaderParameter worldGridWidth;
	FShaderParameter worldGridHeight;

	FShaderResourceParameter OutHt_Pt_RW;
	FShaderResourceParameter InHt_Pt_RO;
	
};

class FApplyFieldsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FApplyFieldsCS, Global);
public:
	FApplyFieldsCS() {}
	FApplyFieldsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		inTextureParameter.Bind(Initializer.ParameterMap, TEXT("SrcTexture"));
		flowMapParameter.Bind(Initializer.ParameterMap, TEXT("FlowMap"));
		inTexSampler.Bind(Initializer.ParameterMap, TEXT("SrcSampler"));
		h0_phi0_RW.Bind(Initializer.ParameterMap, TEXT("h0_phi0_RW"));
		bUseFlowMap.Bind(Initializer.ParameterMap, TEXT("bUseFlowMap"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("APPLY_HEIGHT"), 1);
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), FApplyFieldsCS::NumThreadsPerGroup());
	}

	static int32 NumThreadsPerGroup() { return 32; }

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FShaderResourceViewRHIParamRef& InTextureSRV,
		const FShaderResourceViewRHIParamRef& _flowMapParameter,
		uint32 _bUseFlowMap
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		SetShaderValue(RHICmdList, ComputeShaderRHI, bUseFlowMap, _bUseFlowMap);
		if (inTextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, inTextureParameter.GetBaseIndex(), InTextureSRV);
			FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			SetSamplerParameter(RHICmdList, ComputeShaderRHI, inTexSampler, SamplerStateLinear);
		}

		if (flowMapParameter.IsBound())
		{
			if (_flowMapParameter != NULL)
			{
				RHICmdList.SetShaderResourceViewParameter(
					ComputeShaderRHI, flowMapParameter.GetBaseIndex(), _flowMapParameter);
			}
			else
			{
				RHICmdList.SetShaderResourceViewParameter(
					ComputeShaderRHI, flowMapParameter.GetBaseIndex(),FShaderResourceViewRHIRef());
			}
		}
		
		
	}

	void SetOutput(FRHICommandList& RHICmdList, const FUnorderedAccessViewRHIRef& outStructBufferUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (h0_phi0_RW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, h0_phi0_RW.GetBaseIndex(), outStructBufferUAV);
		}
	}
	
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);
		Ar << inTextureParameter << flowMapParameter << inTexSampler 
		   << h0_phi0_RW << bUseFlowMap;

		return bShaderHasOutdatedParams;
	}

	void SetUniformBuffers(FRHICommandList& RHICmdList, FComputeShaderVariableParameters& VariableParameters)
	{
		FComputeShaderVariableParameterRef VariableParametersBuffer;

		VariableParametersBuffer = FComputeShaderVariableParameterRef::CreateUniformBufferImmediate(
			VariableParameters, UniformBuffer_SingleDraw);

		SetUniformBufferParameter(RHICmdList, GetComputeShader(),
			GetUniformBufferParameter<FComputeShaderVariableParameters>(),
			VariableParametersBuffer);
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (h0_phi0_RW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI,
				h0_phi0_RW.GetBaseIndex(), FUnorderedAccessViewRHIRef());
		}

		if (inTextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				inTextureParameter.GetBaseIndex(), FShaderResourceViewRHIRef());
		}

		if (flowMapParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				flowMapParameter.GetBaseIndex(), FShaderResourceViewRHIRef());
		}
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

private:
	FShaderResourceParameter inTextureParameter;
	FShaderResourceParameter flowMapParameter;
	FShaderResourceParameter inTexSampler;
	FShaderResourceParameter h0_phi0_RW;

	FShaderParameter bUseFlowMap;
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
		TexMapSampler.Bind(Initializer.ParameterMap, TEXT("TexMapSampler"));
	}
	
	void SetParameters(
		FRHICommandList& RHICmdList,
		FShaderResourceViewRHIParamRef TextureParameterSRV,
		FTextureRHIParamRef InputTextureRef
	)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		if (TextureParameter.IsBound()) {
			
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, TextureParameter.GetBaseIndex(), TextureParameterSRV);
			FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			SetSamplerParameter(RHICmdList, PixelShaderRHI, TexMapSampler, SamplerStateLinear);
			//SetTextureParameter(RHICmdList, PixelShaderRHI, TextureParameter, InputTextureRef);
		}

		
		
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		if (TextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI,
				TextureParameter.GetBaseIndex(), FShaderResourceViewRHIRef());
		}
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	virtual bool Serialize(FArchive &Ar) override 
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << TextureParameter << TexMapSampler;
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