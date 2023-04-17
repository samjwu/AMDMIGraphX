/*
    Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
*/

#ifndef ROCM_WRAPPER_HALF_HPP
#define ROCM_WRAPPER_HALF_HPP

#if defined(ROCM_NO_WRAPPER_HEADER_WARNING) || defined(ROCM_WRAPPER_GAVE_WARNING)
/* include file */
#include "half/half.hpp"
#else
/* give warning */
#if defined(_MSC_VER)
#pragma message(": warning:This file is deprecated. Use the header file from /home/samwu103/Documents/GitHub/AMDMIGraphX/depend/cget/pkg/ROCmSoftwarePlatform__half/install/include/half/half.hpp by using #include <half/half.hpp>")
#elif defined(__GNUC__)
#warning "This file is deprecated. Use the header file from /home/samwu103/Documents/GitHub/AMDMIGraphX/depend/cget/pkg/ROCmSoftwarePlatform__half/install/include/half/half.hpp by using #include <half/half.hpp>"
#endif
/* include file */
#define ROCM_WRAPPER_GAVE_WARNING
#include "half/half.hpp"
#undef ROCM_WRAPPER_GAVE_WARNING
#endif /* defined(ROCM_NO_WRAPPER_HEADER_WARNING) || defined(ROCM_WRAPPER_GAVE_WARNING) */

#endif /* ROCM_WRAPPER_HALF_HPP */


