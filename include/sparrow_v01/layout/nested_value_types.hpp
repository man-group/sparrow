#pragma once

// we bundle all nested value types in this file
// otherwise one has to include **all** nested value types
// whereever the nested value type variant is used 
// (otherwise there will be incomplete type errors)
#include "sparrow_v01/layout/list_layout/list_value.hpp"
#include "sparrow_v01/layout/struct_layout/struct_value.hpp"