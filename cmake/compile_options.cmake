
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(compile_options
        /bigobj
        /permissive-
        /WX # treat warnings as errors
        /W4 # Baseline reasonable warnings
        /we4242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        /we4244 # conversion from 'type1' to 'type_2', possible loss of data
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4263 # 'function': member function does not override any base class virtual member function
        /we4265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
        /we4287 # 'operator': unsigned/negative constant mismatch
        /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
        /we4296 # 'operator': expression is always 'boolean_value'
        /we4311 # 'variable': pointer truncation from 'type1' to 'type2'
        /we4545 # expression before comma evaluates to a function which is missing an argument list
        /we4546 # function call before comma missing argument list
        /we4547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /we4549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /we4555 # expression has no effect; expected expression with side- effect
        /we4619 # pragma warning: there is no warning number 'number'
        /we4640 # Enable warning on thread un-safe static member initialization
        /we4826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        /we4905 # wide string literal cast to 'LPSTR'
        /we4906 # string literal cast to 'LPWSTR'
        /we4928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /we5038 # data member 'member1' will be initialized after data member 'member2'
        /Zc:__cplusplus
        PARENT_SCOPE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(compile_options
        -Wall # reasonable and standard
        -Wcast-align # warn for potential performance problem casts
        -Wconversion # warn on type conversions that may lose data
        -Wdouble-promotion # warn if float is implicitly promoted to double
        -Werror # treat warnings as errors
        -Wextra
        -Wformat=2 # warn on security issues around functions that format output (i.e., printf)
        -Wimplicit-fallthrough # Warns when case statements fall-through. (Included with -Wextra in GCC, not in clang)
        -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
        -Wnull-dereference # warn if a null dereference is detected
        -Wold-style-cast # warn for c-style casts
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wsign-conversion # warn on sign conversions
        -Wunused # warn on anything being unused
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat> # do not warn on use of non-C++98 standard
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat-pedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-extra-semi-stmt>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c++20-compat>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-pre-c++20-compat-pedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-reserved-identifier>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-undef>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-switch-default>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-switch-enum>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-prototypes>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-template>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-unsafe-buffer-usage>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation-unknown-command>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-float-equal>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-exit-time-destructors>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-global-constructors>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-newline-eof>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-ctad-maybe-unsupported>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-array-bounds>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-overread>
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-branches> # warn if if / else branches have duplicated code
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond> # warn if if / else chain has duplicated conditions
        $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op> # warn about logical operations being used where bitwise were probably wanted
        $<$<CXX_COMPILER_ID:GNU>:-Wno-subobject-linkage> # suppress warnings about subobject linkage
    )
    if (NOT "${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        set(compile_options ${compile_options} -ftemplate-backtrace-limit=0 -pedantic)
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.3)
        set(compile_options ${compile_optoins} PRIVATE "-Wno-error=shift-negative-value")
    endif()
endif()
