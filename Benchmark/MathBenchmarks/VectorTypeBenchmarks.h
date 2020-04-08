#include "../../Include/Math/VectorTypes.h"

// Vector benchmarks
void BM_v2Add_Scalar();
void BM_v2Add_Vectorized();
void BM_m2MulV2_fScalar();
void BM_m2MulV2_fVectorized();
void BM_m2MulV2_iScalar();

void BM_m2MulV2_iScalar_MT_Startup();
void BM_m2MulV2_iScalar_MT();
void BM_m2MulV2_iScalar_MT_Shutdown();
