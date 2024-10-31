// This file is part of https://github.com/btzy/duality
#pragma once

// This file contains compatibility stuff for older compilers that do not support all the features
// we want to use.

#if defined(__cpp_static_call_operator) && __cpp_static_call_operator >= 202207L
#define DUALITY_STATIC_CALL static
#define DUALITY_CONST_CALL
#else
#define DUALITY_STATIC_CALL
#define DUALITY_CONST_CALL const
#endif
