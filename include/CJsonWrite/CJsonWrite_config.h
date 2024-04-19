#pragma once

// header file with your assert macro
#include <assert.h>

// header file with common types (default: "CJsonWrite/CJsonWrite_types.h")
#include "CJsonWrite/CJsonWrite_types.h"

// Define JsonAssert here
#define JsonAssert(condition) assert(condition)
