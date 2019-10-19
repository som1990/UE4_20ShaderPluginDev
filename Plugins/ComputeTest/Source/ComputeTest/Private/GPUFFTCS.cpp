#include "GPUFFTCS.h"
#include "SceneUtils.h"
#include "GlobalShader.h"

FString MyGPUFFT::FFT_TypeName(const FFT_XFORM_TYPE& XFormType)
{
	switch (static_cast<int>(XFormType))
	{
	case 0 /*FORWARD_HORIZONTAL*/:	return FString(TEXT("Forward Horizontal"));	break;
	case 1 /*FORWARD_VERTICAL*/:	return FString(TEXT("Forward Vertical"));	break;
	case 2 /*INVERSE_HORIZONTAL*/:	return FString(TEXT("Inverse Horizontal"));	break;
	case 3 /*INVERSE_VERTICAL*/:	return FString(TEXT("Inverse Vertical"));
	default:	return FString(TEXT("Error"));
	}
}

bool MyGPUFFT::IsHorizontal(const FFT_XFORM_TYPE& XFormType)
{
	return (XFormType == FFT_XFORM_TYPE::FORWARD_HORIZONTAL || XFormType == FFT_XFORM_TYPE::INVERSE_HORIZONTAL);
}

bool MyGPUFFT::IsForward(const FFT_XFORM_TYPE& XFormType)
{
	return (XFormType == FFT_XFORM_TYPE::FORWARD_HORIZONTAL || XFormType == FFT_XFORM_TYPE::FORWARD_VERTICAL);
}

uint32 MyGPUFFT::MaxScanLineLength() { return 4096; }

namespace MyGPUFFT
{
	
	class FGSComplexTransformBaseCS : public FGlobalShader 
	{
	public:

		FGSComplexTransformBaseCS() {};

		FGSComplexTransformBaseCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
			FGlobalShader(Initializer)
		{
			SrcROTexture.Bind(Initializer.ParameterMap, TEXT("SrcTexture"));
			DstRWTexture.Bind(Initializer.ParameterMap, TEXT("DstTexture"));
			TransformType.Bind(Initializer.ParameterMap, TEXT("TransformType"));
			SrcRectMin.Bind(Initializer.ParameterMap, TEXT("SrcRectMin"));
			SrcRectMax.Bind(Initializer.ParameterMap, TEXT("SrcRectMax"));
			DstExtent.Bind(Initializer.ParameterMap, TEXT("DstExtent"));
			DstRect.Bind(Initializer.ParameterMap, TEXT("DstRect"));

		}

		void SetCSParameters(FRHICommandList& RHICmdList,
			const FFT_XFORM_TYPE& XFormType,
			const FShaderResourceViewRHIParamRef &SrcTexture,
			const FIntRect& SrcRect, const FIntRect& DstRectValue)
		{
			const FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

			if (SrcROTexture.IsBound())
			{
				RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, SrcROTexture.GetBaseIndex(), SrcTexture);
			}

			uint32 TransformTypeValue = MyGPUFFT::BitEncode(XFormType);

			SetShaderValue(RHICmdList, ComputeShaderRHI, TransformType, TransformTypeValue);
			SetShaderValue(RHICmdList, ComputeShaderRHI, SrcRectMin, SrcRect.Min);
			SetShaderValue(RHICmdList, ComputeShaderRHI, SrcRectMax, SrcRect.Max);
			SetShaderValue(RHICmdList, ComputeShaderRHI, DstRect, DstRectValue);
			SetShaderValue(RHICmdList, ComputeShaderRHI, DstExtent, DstRectValue.Size());

		}

		void SetOutput(FRHICommandList& RHICmdList, const FUnorderedAccessViewRHIRef& OutputBufferUAV)
		{
			FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

			if (DstRWTexture.IsBound())
				RHICmdList.SetUAVParameter(ComputeShaderRHI, DstRWTexture.GetBaseIndex(), OutputBufferUAV);
		}

		void UnbindBuffers(FRHICommandList& RHICmdList)
		{
			FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

			if (DstRWTexture.IsBound())
			{
				RHICmdList.SetUAVParameter(ComputeShaderRHI,
					DstRWTexture.GetBaseIndex(), FUnorderedAccessViewRHIRef());
			}

			if (SrcROTexture.IsBound())
			{
				RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI,
					SrcROTexture.GetBaseIndex(), FShaderResourceViewRHIRef());
			}
		}

		FShaderResourceParameter& DestinationResourceParameter() { return DstRWTexture; }

		virtual bool Serialize(FArchive& Ar) override 
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
			Ar << SrcROTexture << DstRWTexture << TransformType
				<< SrcRectMin << SrcRectMax << DstExtent << DstRect;
			return bShaderHasOutdatedParameters;
		}

	public:
		FShaderResourceParameter SrcROTexture;
		FShaderResourceParameter DstRWTexture;
		FShaderParameter TransformType;
		FShaderParameter SrcRectMin;
		FShaderParameter SrcRectMax;
		FShaderParameter DstExtent;
		FShaderParameter DstRect;

	};
	
	template <int PowRadixSignalLength>
	class TGSComplexTransformCS : public FGSComplexTransformBaseCS
	{
		DECLARE_SHADER_TYPE(TGSComplexTransformCS, Global);

	
		static const TCHAR* GetSourceFilename() { return TEXT("/Plugin/ComputeTest/Private/MyFFT.usf"); }
		static const TCHAR* GetFunctionName() { return TEXT("GroupSharedComplexFFTCS"); }

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) && (!IsMetalPlatform(Parameters.Platform) || RHIGetShaderLanguageVersion(Parameters.Platform) >= 2) && (Parameters.Platform != SP_METAL_MRT);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
			OutEnvironment.SetDefine(TEXT("INCLUDE_GROUP_SHARED_COMPLEX_FFT"), 1);
			OutEnvironment.SetDefine(TEXT("SCAN_LINE_LENGTH"), PowRadixSignalLength);
		}

		TGSComplexTransformCS() {};

		TGSComplexTransformCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGSComplexTransformBaseCS(Initializer)
		{}
	};

}

//Implement all the global shaders

IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<2>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<4>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<8>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<16>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<32>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<64>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<128>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<256>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<512>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<1024>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<2048>, SF_Compute);
IMPLEMENT_SHADER_TYPE2(MyGPUFFT::TGSComplexTransformCS<4096>, SF_Compute);


/*
#define GROUPSHARED_COMPLEX_TRANSFORM(_Length) \
typedef MyGPUFFT::TGSComplexTransformCS< _Length> FGSComplexTransformCSLength##_Length; \
IMPLEMENT_SHADER_TYPE2(FGSComplexTransformCSLength##_Length, SF_Compute);

GROUPSHARED_COMPLEX_TRANSFORM(2)  GROUPSHARED_COMPLEX_TRANSFORM(16)  GROUPSHARED_COMPLEX_TRANSFORM(128)   GROUPSHARED_COMPLEX_TRANSFORM(1024)

GROUPSHARED_COMPLEX_TRANSFORM(4)  GROUPSHARED_COMPLEX_TRANSFORM(32)  GROUPSHARED_COMPLEX_TRANSFORM(256)   GROUPSHARED_COMPLEX_TRANSFORM(2048)

GROUPSHARED_COMPLEX_TRANSFORM(8)  GROUPSHARED_COMPLEX_TRANSFORM(64)  GROUPSHARED_COMPLEX_TRANSFORM(512)   GROUPSHARED_COMPLEX_TRANSFORM(4096)
// NB: FFTBLOCK(8192, false/true) won't work because the max number of threads in a group 1024 is less than the requested 8192 / 2 

#undef GROUPSHARED_COMPLEX_TRANSFORM
*/

namespace MyGPUFFT
{
	
	FGSComplexTransformBaseCS* GetComplexFFTCS(const FGPUFFTShaderContext::ShaderMapType& ShaderMap, const uint32 TransformLength)
	{
		FGSComplexTransformBaseCS* Result = NULL;
		
		#define GET_COMPLEX_SHADER(_LENGTH) ShaderMap.GetShader<TGSComplexTransformCS<_LENGTH>>();
	
		switch (TransformLength)
		{
		case 2:		Result = GET_COMPLEX_SHADER(2);		break;
		case 4:		Result = GET_COMPLEX_SHADER(4);		break;
		case 8:		Result = GET_COMPLEX_SHADER(8);		break;
		case 16:	Result = GET_COMPLEX_SHADER(16);	break;
		case 32:	Result = GET_COMPLEX_SHADER(32);	break;
		case 64:	Result = GET_COMPLEX_SHADER(64);	break;
		case 128:	Result = GET_COMPLEX_SHADER(128);	break;
		case 256:	Result = GET_COMPLEX_SHADER(256);	break;
		case 512:	Result = GET_COMPLEX_SHADER(512);	break;
		case 1024:	Result = GET_COMPLEX_SHADER(1024);	break;
		case 2048:	Result = GET_COMPLEX_SHADER(2048);	break;
		case 4096:	Result = GET_COMPLEX_SHADER(4096);	break;

		default:
			ensureMsgf(0, TEXT("The FFT block height is not supported"));
			break;
		}

		#undef GET_COMPLEX_SHADER
		
		return Result;
		
	}
	

	void DispatchGSComplexFFTCS(FGPUFFTShaderContext& Context, const FFTDescription& FFTDesc,
		const FShaderResourceViewRHIParamRef& SrcTexture, const FIntRect& SrcRect,
		FUnorderedAccessViewRHIRef& DstUAV)
	{
		const FGPUFFTShaderContext::ShaderMapType& ShaderMap = Context.GetShaderMap();
		FRHICommandListImmediate& RHICmdList = Context.GetRHICmdList();

		const uint32 TransformLength = FFTDesc.SignalLength;
		const FString TransformName = FFTDesc.FFT_TypeName();
		const FIntPoint DstExtent = FFTDesc.TransformExtent();

		//ensureMsgf(0, TEXT("FFT: Complex % s of size %d"), *TransformName, TransformLength);
		UE_LOG(LogTemp, Warning, TEXT("FFT: Complex % s of size %d"), *TransformName, TransformLength);
		//SCOPED_DRAW_EVENT(RHICmdList, ComplexFFTImage, TEXT("FFT: Complex % s of size %d"), *TransformName, TransformLength);
		FGSComplexTransformBaseCS* ComputeShader = GetComplexFFTCS(ShaderMap, TransformLength);
		
		RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
		ComputeShader->SetOutput(RHICmdList, DstUAV);
		ComputeShader->SetCSParameters(RHICmdList, FFTDesc.XFormType, SrcTexture, SrcRect, FIntRect(FIntPoint(0, 0), DstExtent));

		const uint32 NumSignals = FFTDesc.IsHorizontal() ? SrcRect.Size().Y : SrcRect.Size().X;
		RHICmdList.DispatchComputeShader(1, 1, NumSignals);
		ComputeShader->UnbindBuffers(RHICmdList);
	}

	bool MyComplexFFTImage1D::MyGroupShared(FGPUFFTShaderContext& Context, const FFTDescription& FFTDesc, const FIntRect& SrcWindow, const FShaderResourceViewRHIParamRef& SrcTexture, FUnorderedAccessViewRHIRef& DstUAV)
	{
		bool SuccessValue = true;

		check(FMath::IsPowerOfTwo(FFTDesc.SignalLength));

		if (FitsInGroupSharedMemory(FFTDesc))
		{
			DispatchGSComplexFFTCS(Context, FFTDesc, SrcTexture, SrcWindow, DstUAV);
		}
		else
		{
			SuccessValue = false;

			ensureMsgf(0, TEXT("The FFT size is too large for group shared memory"));
		}

		return SuccessValue;
	}

	bool MyGPUFFT::FFTImage2D(FGPUFFTShaderContext& Context, const FIntPoint& FrequencySize, bool bHorizontalFirst, bool bForward, const FIntRect& ROIRect, const FShaderResourceViewRHIParamRef &SrcTexture, FUnorderedAccessViewRHIRef& DstBuffer_UAV,  FUnorderedAccessViewRHIRef& TmpBuffer_UAV, const FShaderResourceViewRHIParamRef &TmpBuffer_SRV)
	{
		using MyGPUFFT::FFTDescription;

		FRHICommandListImmediate& RHICmdList = Context.GetRHICmdList();

		const FIntRect FFTResultRect = ROIRect;
		const FIntPoint ImageSpaceExtent = ROIRect.Size();

		/*FFTDescription Generation*/
		FFT_XFORM_TYPE dir_horizontal, dir_vertical;

		if (bForward)
		{
			dir_horizontal = FFT_XFORM_TYPE::FORWARD_HORIZONTAL;
			dir_vertical = FFT_XFORM_TYPE::FORWARD_VERTICAL;
		}

		else
		{
			dir_horizontal = FFT_XFORM_TYPE::INVERSE_HORIZONTAL;
			dir_vertical = FFT_XFORM_TYPE::INVERSE_VERTICAL;
		}

		FFTDescription ComplexFFTDescOne = (bHorizontalFirst) ? FFTDescription(dir_horizontal, FrequencySize) : FFTDescription(dir_vertical, FrequencySize);
		FFTDescription ComplexFFTDescTwo = (bHorizontalFirst) ? FFTDescription(dir_vertical, FrequencySize) : FFTDescription(dir_horizontal, FrequencySize);


		//Temp Double Buffers
		const FIntPoint TmpExtent = (bHorizontalFirst) ? FIntPoint(FrequencySize.X, ImageSpaceExtent.Y) : FIntPoint(ImageSpaceExtent.X, FrequencySize.Y);

		const FIntRect TmpRect = FIntRect(FIntPoint(0, 0), TmpExtent);

		bool SuccessValue = true;
		//TODO: FFT Transform Shader Definitions
		if (FitsInGroupSharedMemory(ComplexFFTDescOne))
		{
			SuccessValue = SuccessValue &&
				MyComplexFFTImage1D::MyGroupShared(Context, ComplexFFTDescOne, TmpRect, SrcTexture, TmpBuffer_UAV);
		}
		else
		{
			SuccessValue = false;
		}

		RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, TmpBuffer_UAV);

		const FIntPoint TmpExtent2 = (bHorizontalFirst) ? FIntPoint(ImageSpaceExtent.X, FrequencySize.Y) : FIntPoint(FrequencySize.X, ImageSpaceExtent.Y);
		const FIntRect TmpRect2 = FIntRect(FIntPoint(0, 0), TmpExtent);

		// Complex transform in the other direction: TmpBuffer fills DstBuffer
		if (FitsInGroupSharedMemory(ComplexFFTDescTwo))
		{
			SuccessValue = SuccessValue &&
				MyComplexFFTImage1D::MyGroupShared(Context, ComplexFFTDescTwo, TmpRect2, TmpBuffer_SRV, DstBuffer_UAV);
		}
		else
		{
			SuccessValue = false;
		}
		return SuccessValue;
	}



}