
#include "CSShaders.h"
#include "ComputeTestPrivatePCH.h"


IMPLEMENT_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters, TEXT("CSVariables"))

IMPLEMENT_SHADER_TYPE(, FAddSourceHeightCS, TEXT("/Plugin/ComputeTest/Private/MyCS.usf"), TEXT("AddSourceHeightCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FMyQuadVS, TEXT("/Plugin/ComputeTest/Private/MyVS_PS.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FMyQuadPS, TEXT("/Plugin/ComputeTest/Private/MyVS_PS.usf"), TEXT("MainPS"), SF_Pixel);

