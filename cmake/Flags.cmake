function(apply_compile_flags target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Target '${target}' does not exist. Cannot apply compile flags.")
  endif()

  # The scope is set to PUBLIC to propagate the flags to all targets that link to this target
  if(NOT MSVC)
    # Clang, GCC
    target_compile_options(${target} PUBLIC
      -Wall                  # Enable all common warnings
      -Wextra                # Enable extra warnings
      -Wpedantic             # Enforce ISO C++ standards strictly
      -Werror                # Treat all warnings as errors
      -Wconversion           # Warn on implicit type conversions that may change value
      -Wsign-conversion      # Warn on sign conversions
      -Wshadow               # Warn when variables shadow others
      -Wnon-virtual-dtor     # Warn on classes with virtual functions but non-virtual destructors
      -Wold-style-cast       # Warn on C-style casts
      -Woverloaded-virtual   # Warn when a derived class function hides a virtual function
      -Wnull-dereference     # Warn if null dereference is detected
      -Wdouble-promotion     # Warn when a float is implicitly promoted to double
      -Wcast-align           # Warn on cast that increases required alignment
      -Wformat=2             # Enable format warnings (printf, etc.)
      -Wunused               # Warn on anything unused
      -finput-charset=UTF-8  # Set input file charset to UTF-8
      -fexec-charset=UTF-8   # Set execution charset to UTF-8
    )
  else()
    # MSVC
    target_compile_options(${target} PUBLIC
      /W4              # Enable high warning level
      /WX              # Treat warnings as errors
      /utf-8           # Use UTF-8 encoding for source and execution
      /permissive-     # Enforce strict C++ standard conformance
      /Zc:__cplusplus  # Set __cplusplus macro to correct value
    )
  endif()
  message(STATUS "Compile flags applied to target '${target}'.")
endfunction()
