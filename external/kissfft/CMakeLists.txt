IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_LIST_DIR})
  IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    MESSAGE(STATUS "Setting CMAKE_BUILD_TYPE type to 'Debug' as none was specified.")
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
  ENDIF()

  PROJECT(kissfft LANGUAGES C)

  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

  IF(WIN32 AND MSVC)
    OPTION(PA_DLL_LINK_WITH_STATIC_RUNTIME "Link with static runtime libraries (minimizes runtime dependencies)" ON)
    IF(PA_DLL_LINK_WITH_STATIC_RUNTIME)
      FOREACH(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        IF(${flag_var} MATCHES "/MD")
          STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF()
      ENDFOREACH()
    ENDIF()
  ENDIF()
ENDIF()

set(FFT_HEADERS include/kiss_fft.h)
set(FFT_SOURCES src/kiss_fft.c include/_kiss_fft_guts.h)

add_library(kissfft SHARED
    ${FFT_SOURCES}
    ${FFT_HEADERS})

#target_compile_definitions(kissfft PRIVATE USE_SIMD=1)

add_library(kissfft_static STATIC
    ${FFT_SOURCES}
    ${FFT_HEADERS})

#target_compile_definitions(kissfft_static PRIVATE USE_SIMD=1)

# export lib
target_include_directories(kissfft
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(kissfft_static
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
