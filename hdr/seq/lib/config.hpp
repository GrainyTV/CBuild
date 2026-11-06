#pragma once

#ifndef __cplusplus
    #error "This is supposed to be a C++ library"
#else
    #if !(__cplusplus >= 202002L)
        #error "You need at least C++20 standard to use this library"
    #endif
#endif
