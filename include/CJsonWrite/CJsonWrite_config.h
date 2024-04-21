#pragma once

// header file with your assert macro definition here. assert.h's assert macro is fine for most cases, but if you're using an internal assert system this is the place to set it.
#include <assert.h>

// Define JsonAssert here
#define JsonAssert(condition) assert((condition))
#define JsonAssertMsg(condition, message) assert((condition) && (message))


// Define your int and float types here. stdint.h's int32_t and float are fine for most cases, but for smaller architectures, you may want int16_t or int8_t. You might even have your own typedefs and prefer using that. This is the place to set them.
#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

typedef int32_t int_type;
typedef float float_type;
