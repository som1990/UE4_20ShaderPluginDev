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


BEGIN_UNIFORM_BUFFER_STRUCT(FEWavePSVariableParameters, )
UNIFORM_MEMBER(float, choppyness)
UNIFORM_MEMBER(float, dx)
UNIFORM_MEMBER(float, dy)
END_UNIFORM_BUFFER_STRUCT(FEWavePSVariableParameters)


typedef TUniformBufferRef<FEWavePSVariableParameters> FEWavePSVariableParameterRef;

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
		m_Lx.Bind(Initializer.ParameterMap, TEXT("m_Lx"));
		m_Ly.Bind(Initializer.ParameterMap, TEXT("m_Ly"));
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
		float _Lx,
		float _Ly,
		const FShaderResourceViewRHIParamRef& TextureParameterSRV,
		const FShaderResourceViewRHIParamRef& ObstructionParmSRV
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(RHICmdList, ComputeShaderRHI, MyColorParameter, Color);
		SetShaderValue(RHICmdList, ComputeShaderRHI, bUseObsMap, useObsMap);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Lx, _Lx);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Ly, _Ly);

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
			<< ObstructionParm << TextureParameter << TexMapSampler << H0_phi0_RW << m_Lx << m_Ly;
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

	FShaderParameter m_Lx;
	FShaderParameter m_Ly;

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

class FAdvectFieldsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAdvectFieldsCS, Global);
public:
	FAdvectFieldsCS() {}
	FAdvectFieldsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		inTextureParameter.Bind(Initializer.ParameterMap, TEXT("SrcTexture"));
		flowMapParameter.Bind(Initializer.ParameterMap, TEXT("FlowMap"));
		inTexSampler.Bind(Initializer.ParameterMap, TEXT("SrcSampler"));
		h0_phi0_RW.Bind(Initializer.ParameterMap, TEXT("h0_phi0_RW"));
		bUseFlowMap.Bind(Initializer.ParameterMap, TEXT("bUseFlowMap"));
		velScale.Bind(Initializer.ParameterMap, TEXT("velScale"));
		m_Lx.Bind(Initializer.ParameterMap, TEXT("m_Lx"));
		m_Ly.Bind(Initializer.ParameterMap, TEXT("m_Ly"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("APPLY_ADVECTION"), 1);
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), FAdvectFieldsCS::NumThreadsPerGroup());
	}

	static int32 NumThreadsPerGroup() { return 32; }

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FShaderResourceViewRHIParamRef& InTextureSRV,
		const FShaderResourceViewRHIParamRef& _flowMapParameter,
		uint32 _bUseFlowMap,
		float _velScale,
		float _Lx,
		float _Ly
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		SetShaderValue(RHICmdList, ComputeShaderRHI, bUseFlowMap, _bUseFlowMap);
		SetShaderValue(RHICmdList, ComputeShaderRHI, velScale, _velScale);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Lx, _Lx);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Ly, _Ly);

		if (inTextureParameter.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, inTextureParameter.GetBaseIndex(), InTextureSRV);
			FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			SetSamplerParameter(RHICmdList, ComputeShaderRHI, inTexSampler, SamplerStateLinear);
		}

		if (flowMapParameter.IsBound())
		{
			
			RHICmdList.SetShaderResourceViewParameter(
				ComputeShaderRHI, flowMapParameter.GetBaseIndex(), _flowMapParameter);
			
			
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
		   << h0_phi0_RW << bUseFlowMap<< velScale << m_Lx << m_Ly;

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
	FShaderParameter velScale;
	FShaderParameter m_Lx;
	FShaderParameter m_Ly;
	
};


class FGenGradCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FGenGradCS, Global);
public:
	FGenGradCS() {}
	FGenGradCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		RO_Ht_PHIt.Bind(Initializer.ParameterMap, TEXT("RO_Ht_PHIt"));
		dx_dy.Bind(Initializer.ParameterMap, TEXT("dx_dy"));
		dPhiX_dPhiY.Bind(Initializer.ParameterMap, TEXT("dPhix_dPhiy"));
		genGrad.Bind(Initializer.ParameterMap, TEXT("genGrad"));
		calcNonLinear.Bind(Initializer.ParameterMap, TEXT("calcNonLinear"));
		m_Lx.Bind(Initializer.ParameterMap, TEXT("m_Lx"));
		m_Ly.Bind(Initializer.ParameterMap, TEXT("m_Ly"));

	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);

	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("GEN_GRAD"), 1);
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), FGenGradCS::NumThreadsPerGroup());
	}

	static int32 NumThreadsPerGroup() { return 32; }
	
	void SetParameters(
		FRHICommandList& RHICmdList,
		const FShaderResourceViewRHIParamRef& RO_Ht_PHItSRV,
		uint32 _bGenGrad,
		uint32 _bcalcNonLinear,
		float _Lx,
		float _Ly
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		SetShaderValue(RHICmdList, ComputeShaderRHI, genGrad, _bGenGrad);
		SetShaderValue(RHICmdList, ComputeShaderRHI, calcNonLinear, _bcalcNonLinear);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Lx, _Lx);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Ly, _Ly);

		if (RO_Ht_PHIt.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(
				ComputeShaderRHI, RO_Ht_PHIt.GetBaseIndex(), RO_Ht_PHItSRV);
		}
	}

	void SetOutput(
		FRHICommandList& RHICmdList,
		FUnorderedAccessViewRHIRef& dx_dyUAV,
		FUnorderedAccessViewRHIRef& RW_Ht_PHItUAV
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (dx_dy.IsBound())
		{
			
			RHICmdList.SetUAVParameter(ComputeShaderRHI, dx_dy.GetBaseIndex(), dx_dyUAV);
			
		}
		if (dPhiX_dPhiY.IsBound())
		{
			
			RHICmdList.SetUAVParameter(ComputeShaderRHI, dPhiX_dPhiY.GetBaseIndex(), RW_Ht_PHItUAV);
			
		}
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);
		Ar << RO_Ht_PHIt << dx_dy << dPhiX_dPhiY << genGrad << calcNonLinear
		   << m_Lx << m_Ly;

		return bShaderHasOutdatedParams;
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (RO_Ht_PHIt.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				RO_Ht_PHIt.GetBaseIndex(), FShaderResourceViewRHIRef());
		}

		if (dx_dy.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI,
				dx_dy.GetBaseIndex(), FUnorderedAccessViewRHIRef());
		}

		if (dPhiX_dPhiY.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI,
				dPhiX_dPhiY.GetBaseIndex(), FUnorderedAccessViewRHIRef());
		}
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

private:
	FShaderResourceParameter RO_Ht_PHIt;
	FShaderResourceParameter dx_dy;
	FShaderResourceParameter dPhiX_dPhiY;

	FShaderParameter genGrad;
	FShaderParameter calcNonLinear;
	FShaderParameter m_Lx;
	FShaderParameter m_Ly;

};

class FApplyFieldsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FApplyFieldsCS, Global);
public:
	FApplyFieldsCS() {}
	FApplyFieldsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SrcTexture.Bind(Initializer.ParameterMap, TEXT("SrcTexture"));
		ObsTexture.Bind(Initializer.ParameterMap, TEXT("ObsTexture"));
		dPhix_dPhiy.Bind(Initializer.ParameterMap, TEXT("dPhix_dPhiy"));
		dx_dy.Bind(Initializer.ParameterMap, TEXT("dx_dy"));
		dPhiSampler.Bind(Initializer.ParameterMap, TEXT("dPhiSampler"));

		DstTexture.Bind(Initializer.ParameterMap, TEXT("DstTexture"));
		h0_phi0_RW.Bind(Initializer.ParameterMap, TEXT("h0_phi0_RW"));
		
		bUseNonLinear.Bind(Initializer.ParameterMap, TEXT("bUseNonLinear"));
		bUseObsTexture.Bind(Initializer.ParameterMap, TEXT("bUseObsTexture"));
		advScale.Bind(Initializer.ParameterMap, TEXT("advScale"));
		m_Lx.Bind(Initializer.ParameterMap, TEXT("m_Lx"));
		m_Ly.Bind(Initializer.ParameterMap, TEXT("m_Ly"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("APPLY_FIELDS"),1);
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), FApplyFieldsCS::NumThreadsPerGroup());
	}

	static int32 NumThreadsPerGroup() { return 32; }

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FShaderResourceViewRHIParamRef& InTextureSRV,
		const FShaderResourceViewRHIParamRef& dx_dySRV,
		const FShaderResourceViewRHIParamRef& dPhix_dPhiySRV,
		const FShaderResourceViewRHIParamRef& obsTextureSRV
		
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (SrcTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, SrcTexture.GetBaseIndex(), InTextureSRV);
		}

		if (ObsTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, ObsTexture.GetBaseIndex(), obsTextureSRV);	
		}

		if (dx_dy.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, dx_dy.GetBaseIndex(), dx_dySRV);
		}

		if (dPhix_dPhiy.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, dPhix_dPhiy.GetBaseIndex(), dPhix_dPhiySRV);
		}
		
		if (dPhiSampler.IsBound())
		{
			FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			SetSamplerParameter(RHICmdList, ComputeShaderRHI, dPhiSampler, SamplerStateLinear);
		}
		
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		uint32 _useNonLinear,
		uint32 _useObsTex,
		float _advScale,
		float _Lx,
		float _Ly
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(RHICmdList, ComputeShaderRHI, bUseObsTexture, _useObsTex);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Lx, _Lx);
		SetShaderValue(RHICmdList, ComputeShaderRHI, m_Ly, _Ly);
		SetShaderValue(RHICmdList, ComputeShaderRHI, bUseNonLinear, _useNonLinear);
		SetShaderValue(RHICmdList, ComputeShaderRHI, advScale, _advScale);

	}

	void SetOutput(FRHICommandList& RHICmdList, const FUnorderedAccessViewRHIRef& outStructBufferUAV, const FUnorderedAccessViewRHIRef& dstTextureUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (h0_phi0_RW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, h0_phi0_RW.GetBaseIndex(), outStructBufferUAV);
		}

		if (DstTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, DstTexture.GetBaseIndex(), dstTextureUAV);
		}
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);
		Ar << SrcTexture << ObsTexture << dPhix_dPhiy << dx_dy << dPhiSampler 
			<< DstTexture << h0_phi0_RW << bUseNonLinear << bUseObsTexture 
			<< advScale << m_Lx << m_Ly;

		return bShaderHasOutdatedParams;
	}
	/*
	void SetUniformBuffers(FRHICommandList& RHICmdList, FComputeShaderVariableParameters& VariableParameters)
	{
		FComputeShaderVariableParameterRef VariableParametersBuffer;

		VariableParametersBuffer = FComputeShaderVariableParameterRef::CreateUniformBufferImmediate(
			VariableParameters, UniformBuffer_SingleDraw);

		SetUniformBufferParameter(RHICmdList, GetComputeShader(),
			GetUniformBufferParameter<FComputeShaderVariableParameters>(),
			VariableParametersBuffer);
	}
	*/
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		if (h0_phi0_RW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI,
				h0_phi0_RW.GetBaseIndex(), FUnorderedAccessViewRHIRef());
		}

		if (DstTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI,
				DstTexture.GetBaseIndex(), FUnorderedAccessViewRHIRef());
		}

		if (SrcTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				SrcTexture.GetBaseIndex(), FShaderResourceViewRHIRef());
		}

		if (ObsTexture.IsBound())
		{	
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, 
				ObsTexture.GetBaseIndex(), FShaderResourceViewRHIRef());	
		}

		if (dx_dy.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, 
				dx_dy.GetBaseIndex(), FShaderResourceViewRHIRef());
		}

		if (dPhix_dPhiy.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
				dPhix_dPhiy.GetBaseIndex(), FShaderResourceViewRHIRef());
		}
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

private:
	FShaderResourceParameter SrcTexture;
	FShaderResourceParameter ObsTexture;
	FShaderResourceParameter dPhix_dPhiy;
	FShaderResourceParameter dx_dy;
	FShaderResourceParameter dPhiSampler;

	FShaderResourceParameter DstTexture;
	FShaderResourceParameter h0_phi0_RW;
	
	FShaderParameter bUseNonLinear;
	FShaderParameter bUseObsTexture;
	FShaderParameter advScale;
	FShaderParameter m_Lx;
	FShaderParameter m_Ly;

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
		choppyness.Bind(Initializer.ParameterMap, TEXT("choppyness"));
		L_x.Bind(Initializer.ParameterMap, TEXT("L_x"));
		L_y.Bind(Initializer.ParameterMap, TEXT("L_y"));
	}
	
	void SetParameters(
		FRHICommandList& RHICmdList,
		FShaderResourceViewRHIParamRef TextureParameterSRV,
		FTextureRHIParamRef InputTextureRef,
		float _choppy, float _Lx, float _Ly
	)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetShaderValue(RHICmdList, PixelShaderRHI, choppyness, _choppy);
		SetShaderValue(RHICmdList, PixelShaderRHI, L_x, _Lx);
		SetShaderValue(RHICmdList, PixelShaderRHI, L_y, _Ly);

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

	void SetUniformBuffers(FRHICommandList& RHICmdList, const FEWavePSVariableParameters& VariableParameters)
	{
		FEWavePSVariableParameterRef VariableParametersBuffer;

		VariableParametersBuffer = FEWavePSVariableParameterRef::CreateUniformBufferImmediate(
			VariableParameters, UniformBuffer_SingleFrame);

		SetUniformBufferParameter(RHICmdList, GetPixelShader(),
			GetUniformBufferParameter<FEWavePSVariableParameters>(),
			VariableParametersBuffer);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	virtual bool Serialize(FArchive &Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << TextureParameter << TexMapSampler << choppyness << L_x << L_y;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("DISP_MAP"), 1);

	}

private:
	FShaderResourceParameter TextureParameter;
	FShaderResourceParameter TexMapSampler;

	FShaderParameter choppyness;
	FShaderParameter L_x;
	FShaderParameter L_y;
};


class FMyGenGradFoldingPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyGenGradFoldingPS, Global);

public:
	FMyGenGradFoldingPS() {}
	FMyGenGradFoldingPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:FGlobalShader(Initializer)
	{
		SrcTexParm.Bind(Initializer.ParameterMap, TEXT("SrcTexParm"));
		SrcTexSampler.Bind(Initializer.ParameterMap, TEXT("SrcTexSampler"));
		choppyness.Bind(Initializer.ParameterMap, TEXT("choppyness"));
		L_x.Bind(Initializer.ParameterMap, TEXT("L_x"));
		L_y.Bind(Initializer.ParameterMap, TEXT("L_y"));

	}

	void SetParameters(FRHICommandList& RHICmdList,
		const FTextureRHIParamRef& TextureRHI, 
		float _choppy, float _Lx, float _Ly)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetShaderValue(RHICmdList, PixelShaderRHI, choppyness, _choppy);
		SetShaderValue(RHICmdList, PixelShaderRHI, L_x, _Lx);
		SetShaderValue(RHICmdList, PixelShaderRHI, L_y, _Ly);

		FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		SetTextureParameter(RHICmdList, PixelShaderRHI, SrcTexParm, SrcTexSampler, SamplerStateLinear, TextureRHI);

	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		
		if (SrcTexParm.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(
				PixelShaderRHI, SrcTexParm.IsBound(), FShaderResourceViewRHIRef());
		}
		

	}

	void SetUniformBuffers(FRHICommandList& RHICmdList, const FEWavePSVariableParameterRef& VariableParametersBuffer)
	{
		SetUniformBufferParameter(RHICmdList, GetPixelShader(),
			GetUniformBufferParameter<FEWavePSVariableParameters>(),
			VariableParametersBuffer);
	}

	virtual bool Serialize(FArchive &Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SrcTexParm << SrcTexSampler << choppyness << L_x << L_y;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		OutEnvironment.SetDefine(TEXT("NORMAL_MAP"), 1);

	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return true;
	}

	

private:
	FShaderResourceParameter SrcTexParm;
	FShaderResourceParameter SrcTexSampler;

	FShaderParameter choppyness;
	FShaderParameter L_x;
	FShaderParameter L_y;
};