#include "../../Include/Core/Math/VectorTypes.h"

// Vector benchmarks
void BM_v2_Startup();
void BM_v2_Shutdown();
void BM_v2Add_Scalar();
void BM_v2Add_Vectorized();
void BM_m2MulV2_fScalar();
void BM_m2MulV2_fVectorized();
void BM_m2MulV2_iScalar();
void BM_m2MulV2_iVectorized();

void BM_m2MulV2_fScalar_MT_Startup();
void BM_m2MulV2_fScalar_MT();
void BM_m2MulV2_fScalar_MT_Shutdown();

void BM_v4_Startup();
void BM_v4_Shutdown();
void BM_v4_MT_Startup();
void BM_v4_MT_Shutdown();
void BM_m4MulV4_fScalar();
void BM_m4MulV4_fVectorized();
void BM_m4MulV4_fScalar_MT();
void BM_m4MulV4_fVectorized_MT();
