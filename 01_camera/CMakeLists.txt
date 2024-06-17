cmake_minimum_required(VERSION 3.1.0)

project(camera_test VERSION 1.0.0 LANGUAGES C)


add_executable(capture_sample src/capture_sample.c)
set_target_properties(capture_sample PROPERTIES
    OUTPUT_NAME capture_sample)

add_executable(capture_sample_performance src/capture_sample_performance.c)
set_target_properties(capture_sample_performance PROPERTIES
    OUTPUT_NAME capture_sample_performance)
    
include(GNUInstallDirs)
install(TARGETS capture_sample DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS capture_sample_performance DESTINATION ${CMAKE_INSTALL_BINDIR})
message(STATUS "summary of build options:
    Install prefix:  ${CMAKE_INSTALL_PREFIX}
    Target system:   ${CMAKE_SYSTEM_NAME}
    Compiler:
      C compiler:    ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_ID})
      CFLAGS:        ${CMAKE_C_FLAGS_${_build_type}} ${CMAKE_C_FLAGS}
")

