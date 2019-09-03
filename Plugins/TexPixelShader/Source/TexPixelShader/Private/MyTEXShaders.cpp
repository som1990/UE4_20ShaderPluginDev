#include "MyTEXShaders.h"
#include "TexPixelShaderPrivatePCH.h"


IMPLEMENT_UNIFORM_BUFFER_STRUCT(FPixelShaderVariableParameters, TEXT("PSVariable"))

IMPLEMENT_SHADER_TYPE(, FMyTestVS, TEXT("/Plugin/TexPixelShader/Private/TexVS_PS.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FMyTestPS, TEXT("/Plugin/TexPixelShader/Private/TexVS_PS.usf"), TEXT("MainPS"), SF_Pixel);

IMPLEMENT_MODULE(FDefaultModuleImpl, TexPixelShader);