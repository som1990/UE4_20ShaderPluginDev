#pragma once
#include "CoreMinimal.h"
#include "Math/NumericLimits.h"
#include "RendererInterface.h"
#include "RHI.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"

namespace MyGPUFFT
{
	enum class FFT_XFORM_TYPE : int
	{
		FORWARD_HORIZONTAL = 0,
		FORWARD_VERTICAL = 1,

		INVERSE_HORIZONTAL = 2,
		INVERSE_VERTICAL = 3
	};

	static FFT_XFORM_TYPE GetInverseOfXForm(const FFT_XFORM_TYPE& XForm)
	{
		return static_cast<FFT_XFORM_TYPE> ((static_cast<int> (XForm) + 2) % 4);
	}

	FString FFT_TypeName(const FFT_XFORM_TYPE& XFormType);


	bool IsHorizontal(const FFT_XFORM_TYPE& XFormType);


	bool IsForward(const FFT_XFORM_TYPE& XFormType);

	static uint32 BitEncode(const FFT_XFORM_TYPE& XFormType)
	{
		//Encoding the lowest bit as 0x00 or 0x01
		uint32 BitEncodedValue = MyGPUFFT::IsHorizontal(XFormType) ? 1 : 0;

		if (MyGPUFFT::IsForward(XFormType))
		{
			BitEncodedValue |= 2; // 2 is 0x10 so we encoding the second lowest bit for forward.
		}
		return BitEncodedValue;
	}

	class FFTDescription
	{
	public:
		FFTDescription(const FFT_XFORM_TYPE& XForm, const FIntPoint& XFormExtent) :
			XFormType(XForm)
		{
			if (MyGPUFFT::IsHorizontal(XFormType))
			{
				SignalLength = XFormExtent.X;
				NumScanLines = XFormExtent.Y;
			}
			else
			{
				SignalLength = XFormExtent.Y;
				NumScanLines = XFormExtent.X;
			}
		}
		FFTDescription() {}

		// The Transform extent used to construct the description
		FIntPoint TransformExtent() const
		{
			const bool bIsHorizontal = MyGPUFFT::IsHorizontal(XFormType);

			return ((bIsHorizontal) ? FIntPoint(SignalLength, NumScanLines) : FIntPoint(NumScanLines, SignalLength));
		}

		bool IsHorizontal() const
		{
			return MyGPUFFT::IsHorizontal(XFormType);
		}

		bool IsForward() const
		{
			return MyGPUFFT::IsForward(XFormType);
		}
		FString FFT_TypeName() const
		{
			return MyGPUFFT::FFT_TypeName(XFormType);
		}

		//member data public
		FFT_XFORM_TYPE XFormType = FFT_XFORM_TYPE::FORWARD_HORIZONTAL;
		uint32 SignalLength = 0;
		uint32 NumScanLines = 0;
	};

	class FGPUFFTShaderContext
	{
	public:
		typedef TShaderMap<FGlobalShaderType> ShaderMapType;

		FGPUFFTShaderContext(FRHICommandListImmediate& CmdList, const FGPUFFTShaderContext::ShaderMapType& Map)
			: RHICmdList(&CmdList), ShaderMap(&Map)
		{}

		FRHICommandListImmediate& GetRHICmdList() { return *RHICmdList; }
		const ShaderMapType& GetShaderMap() const { return *ShaderMap; }

		~FGPUFFTShaderContext()
		{
		}

	private:
		FGPUFFTShaderContext();
		FRHICommandListImmediate* RHICmdList;
		const ShaderMapType* ShaderMap;

	};

	uint32 MaxScanLineLength(); //Returns 4096


	static bool FitsInGroupSharedMemory(const uint32& Length)
	{
		return !(MaxScanLineLength() < Length);
	}

	static bool FitsInGroupSharedMemory(const FFTDescription& FFTDesc)
	{
		return FitsInGroupSharedMemory(FFTDesc.SignalLength);
	}

	static EPixelFormat PixelFormat() { return PF_A32B32G32R32F; }

	typedef FVector FPreFilter;
	static bool IsActive(const FPreFilter& Filter) { return (Filter.X < Filter.Y); }

	struct MyComplexFFTImage1D
	{
		/**
		* The requirements of the complex 1d FFT.
		*
		* @param FFTDesc         - Indicates the number of frequencies and scan lines as well as direction.
		* @param MinBufferSize   - On return: The size of Dst buffer required. If multipass is required
		*                          this size also applies to the tmpBuffer
		* @param bUseGroupShared - On return: indicates if the transform requires the multipass
		*                          or if we can use the much faster group shared variant
		*/
		static void Requirements(const FFTDescription& FFTDesc, FIntPoint& MinBufferSize, bool& bUseMultipass)
		{
			MinBufferSize = FFTDesc.TransformExtent();
			bUseMultipass = !FitsInGroupSharedMemory(FFTDesc);
		}

		/**
		* One Dimensional Fast Fourier Transform of two signals in a two dimensional buffer.
		* The each scanline of the float4 buffer is interpreted as two complex signals (x+iy and z + iw).
		*
		* On input a scanline (lenght N) contains 2 interlaced signals float4(r,g,b,a) = {z_n, q_n}.
		* where z_n = r_n + i g_n  and q_n = b_n + i a_n
		*
		* On output a scanline will be N  elements long with the following data layout
		* Entries k = [0,     N-1] hold  float4(Z_k, Q_k)
		* where the array {Z_k} is the transform of the signal {z_n}
		*
		* @param Context    - RHI and ShadersMap
		* @param FFTDesc    - Defines the scanline direction and count and transform type
		* @param SrcWindow  - Location of the data in the SrcTexture to be transformed. Zero values pad
		*                     the data to fit the  FFTDesc TrasformExtent.
		* @param SrcTexture - The two dimensional float4 input buffer to transform
		* @param DstUAV     - The buffer to be populated by the result of the transform.
		*
		* @return           - True on success.  Will only fail if the buffer is too large for supported transform method
		*
		* NB: This function does not transition resources.  That must be done by the calling scope.
		*/
		static bool MyGroupShared(FGPUFFTShaderContext& Context, const FFTDescription& FFTDesc,
			const FIntRect& SrcWindow, const FShaderResourceViewRHIParamRef& SrcTexture,
			FUnorderedAccessViewRHIRef& DstUAV);


		/**
		* One Dimensional Fast Fourier Transform of two signals in a two dimensional buffer.
		* The each scanline of the float4 buffer is interpreted as two complex signals (x+iy and z + iw).
		*
		* On input a scanline (lenght N) contains 2 interlaced signals float4(r,g,b,a) = {z_n, q_n}.
		* where z_n = r_n + i g_n  and q_n = b_n + i a_n
		*
		* On output a scanline will be N  elements long with the following data layout
		* Entries k = [0,     N-1] hold  float4(Z_k, Q_k)
		* where the array {Z_k} is the transform of the signal {z_n}
		*
		* @param Context     - RHI and ShadersMap
		* @param FFTDesc     - Defines the scanline direction and count and transform type
		* @param SrcWindow   - Location of the data in the SrcTexture to be transformed. Zero values pad
		*                      the data to fit the  FFTDesc TrasformExtent.
		* @param SrcTexture  - The two dimensional float4 input buffer to transform
		* @param DstBuffer   - The buffer to be populated by the result of the transform.
		* @param TmpBuffer   - A temporary buffer used when ping-ponging between iterations.
		* @param bScrubNaNs  - A flag to replace NaNs and negative values with zero.
		*                      This is for use when reading in real data as the first step in two-for-one multipass
		* @return            - True on success. Will only fail if the scanline length is not a power of two.
		*                     Not required to fit in group shared memory.
		*
		* NB: This function does not transition resources.  That must be done by the calling scope.
		*/
		//TODO : IMPLEMENT MUltipass for data larger than 4096 (We don't really need that large a buffer for our project)
		static bool MultiPass(FGPUFFTShaderContext& Context, const FFTDescription& FFTDesc,
			const FIntRect& SrcWindow, const FTextureRHIRef& SrcTexture, FSceneRenderTargetItem& DstBuffer,
			FSceneRenderTargetItem& TmpBuffer, const bool bScrubNaNs = false)
		{

		}

	};

	bool FFTImage2D(FGPUFFTShaderContext& Context, const FIntPoint& FrequencySize, bool bHorizontalFirst, bool bForward,
		const FIntRect& ROIRect, const FShaderResourceViewRHIParamRef& SrcTexture,
		FUnorderedAccessViewRHIRef& DstBuffer_UAV,
		FUnorderedAccessViewRHIRef& TmpBuffer_UAV, const FShaderResourceViewRHIParamRef &TmpBuffer_SRV);
}