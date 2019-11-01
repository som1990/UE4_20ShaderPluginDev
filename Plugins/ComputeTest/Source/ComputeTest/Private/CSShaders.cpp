
#include "CSShaders.h"
#include "ComputeTestPrivatePCH.h"


IMPLEMENT_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters, TEXT("CSVariables"))

IMPLEMENT_SHADER_TYPE(, FAddSourceHeightCS, TEXT("/Plugin/ComputeTest/Private/MyCS.usf"), TEXT("AddSourceHeightCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FCalcEWaveCS, TEXT("/Plugin/ComputeTest/Private/MyCS.usf"), TEXT("CalcEWaveCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FAdvectFieldsCS, TEXT("/Plugin/ComputeTest/Private/MyCS.usf"), TEXT("AdvectFieldsCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FGenGradCS, TEXT("/Plugin/ComputeTest/Private/MyCS.usf"), TEXT("GenGradCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FApplyFieldsCS, TEXT("/Plugin/ComputeTest/Private/MyCS.usf"), TEXT("ApplyFieldsCS"), SF_Compute);


IMPLEMENT_SHADER_TYPE(, FMyQuadVS, TEXT("/Plugin/ComputeTest/Private/MyVS_PS.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FMyQuadPS, TEXT("/Plugin/ComputeTest/Private/MyVS_PS.usf"), TEXT("MainPS"), SF_Pixel);

