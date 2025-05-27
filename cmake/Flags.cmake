function(apply_compile_flags target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Target '${target}' does not exist. Cannot apply compile flags.")
  endif()

  # The scope is set to PUBLIC to propagate the flags to all targets that link to this target
  if(MSVC)
    # MSVC
    target_compile_options(${target} PUBLIC
      /W4               # Enable high warning level (almost all warnings)
      /WX               # Treat warnings as errors
      /utf-8            # Use UTF-8 encoding for source and execution
      /permissive-      # Enforce strict C++ standard conformance
      /Zc:__cplusplus   # Set __cplusplus macro to correct value
      /Zc:preprocessor  # Use conforming preprocessor
    )
  else()
    # Clang, GCC
    target_compile_options(${target} PUBLIC
      -Wall                  # Enable most warning messages
      -Wextra                # Enable extra warning messages
      -Wpedantic             # Enforce ISO C++ standards compliance
      -Werror                # Treat all warnings as errors
      -Wconversion           # Warn on implicit type conversions that may alter values
      -Wsign-conversion      # Warn on sign conversions
      -Wshadow               # Warn when variables shadow others in scope
      -Wnon-virtual-dtor     # Warn on classes with virtual functions but non-virtual destructors
      -Wold-style-cast       # Warn on C-style casts in C++ code
      -Woverloaded-virtual   # Warn when derived class function hides virtual function
      -Wnull-dereference     # Warn if null pointer dereference is detected
      -Wdouble-promotion     # Warn when float is implicitly promoted to double
      -Wcast-align           # Warn on casts that increase required alignment
      -Wformat=2             # Enable format string security warnings
      -Wunused               # Warn on unused variables, functions, etc.
      -finput-charset=UTF-8  # Set input file character encoding to UTF-8
      -fexec-charset=UTF-8   # Set execution character encoding to UTF-8
    )

    # Additional GCC-specific flags
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(${target} PUBLIC
        -Wlogical-op           # Warn about suspicious logical operations
        -Wduplicated-cond      # Warn about duplicated conditions in if-else chains
        -Wduplicated-branches  # Warn about duplicated branches in if-else chains
      )
    endif()
  endif()

  message(STATUS "Applied compile flags to target '${target}'.")
endfunction()
