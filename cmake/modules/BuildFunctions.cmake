include(CMakeParseArguments)
include(ListSplit)

# build_option
# Creates a build option, which is configurable via a CMake option. If the option is set to anything non-default, a
# macro with the name `ZKPP_ENABLE_${NAME}` is exported with the value of `0` or `1`.
#
#  - NAME: The name of the build option.
#  - DOC: Documentation to place in the configuration GUI.
#  - DEFAULT: The default value of the configuration (ON or OFF).
#  - CONFIGS_ON[]: List of build configurations this option should be ON for.
#  - CONFIGS_OFF[]: List of build configurations this option should be OFF for.
function(build_option)
  set(options)
  set(oneValueArgs   NAME DOC DEFAULT)
  set(multiValueArgs CONFIGS_ON CONFIGS_OFF)
  cmake_parse_arguments(OPT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED VALID_BUILD_TYPES)
    message(SEND_ERROR "VALID_BUILD_TYPES not set -- future build_option errors will be inaccurrate")
  endif()

  foreach(bt IN LISTS OPT_CONFIGS_ON OPT_CONFIGS_OFF)
    if(NOT ${bt} IN_LIST VALID_BUILD_TYPES)
      message(WARNING "Specified configuration for invalid CMAKE_BUILD_TYPE=${bt}")
    endif()
  endforeach()

  if(${CMAKE_BUILD_TYPE} IN_LIST OPT_CONFIGS_ON)
    set(ENABLED ON)
  elseif(${CMAKE_BUILD_TYPE} IN_LIST OPT_CONFIGS_OFF)
    set(ENABLED OFF)
  else()
    set(ENABLED ${OPT_DEFAULT})
  endif()

  set(ZKPP_BUILD_OPTION_${OPT_NAME} ${ENABLED}
      CACHE BOOL "${OPT_DOC}"
     )

  if(ZKPP_BUILD_OPTION_${OPT_NAME})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DZKPP_ENABLE_${OPT_NAME}=1" PARENT_SCOPE)
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DZKPP_ENABLE_${OPT_NAME}=0" PARENT_SCOPE)
  endif()

  message(STATUS "  ${OPT_NAME}: ${ENABLED}")
endfunction()

# build_module
# Adds a module to build.
#
#  NAME: The name of this module.
#  PATH: The path to find the source files for this module. It is legal to specify more than one PATH in this list.
#  LINK_LIBRARIES: A list of libraries to link to
#  PROTOTYPE: If set, the module should be considered a "prototype." It will not be built by default and does not
#             consider warnings as errors.
#  NO_RECURSE: Do not search recursively.
function(build_module)
  set(options        PROTOTYPE NO_RECURSE)
  set(oneValueArgs   NAME)
  set(multiValueArgs LINK_LIBRARIES PATH)
  cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(STATUS "${MODULE_NAME} : ${MODULE_PATH}")

  if(MODULE_PROTOTYPE)
    set(BUILD_PROTOTYPE_${MODULE_NAME} OFF
        CACHE BOOL "Build the '${MODULE_NAME}' program?"
       )
    message(STATUS "  ! prototype ${BUILD_PROTOTYPE_${MODULE_NAME}}")
    if(NOT BUILD_PROTOTYPE_${MODULE_NAME})
      return()
    endif()
  endif()

  if(MODULE_NO_RECURSE)
    set(CPP_SEARCH GLOB)
  else()
    set(CPP_SEARCH GLOB_RECURSE)
  endif()

  set(all_library_cpps)
  set(all_library_hpps)
  set(main_name)
  foreach(subpath ${MODULE_PATH})
    file(${CPP_SEARCH} local_library_cpps RELATIVE_PATH "." "${subpath}/*.cpp")
    file(${CPP_SEARCH} local_library_hpps RELATIVE_PATH "." "${subpath}/*.hpp")
    file(GLOB          local_main_name    RELATIVE_PATH "." "${subpath}/main.cpp")

    if(local_main_name)
      if(main_name)
        message(SEND_ERROR "Found main.cpp in different paths for ${MODULE_NAME} (${main_name} and ${local_main_name})")
      endif()

      set(main_name ${local_main_name})
      list(REMOVE_ITEM local_library_cpps ${local_main_name})
    endif()
    list(APPEND all_library_cpps ${local_library_cpps})
    list(APPEND all_library_hpps ${local_library_hpps})
  endforeach()

  list_split(library_test_cpps library_cpps "${all_library_cpps}" "_tests.cpp")
  list_split(library_test_hpps library_notest_hpps "${all_library_hpps}" "_tests.hpp")
  list_split(library_detail_hpps library_hpps "${library_notest_hpps}" "detail")
  list(APPEND library_cpps ${library_detail_hpps})
  set(MODULE_TARGETS)

  if(main_name)
    message(STATUS "  + executable")
    list(APPEND MODULE_TARGETS ${MODULE_NAME}_prog)
    add_executable(${MODULE_NAME}_prog ${main_name})
    target_link_libraries(${MODULE_NAME}_prog ${MODULE_LINK_LIBRARIES})
    set_target_properties(${MODULE_NAME}_prog
                          PROPERTIES
                            OUTPUT_NAME ${MODULE_NAME}
                         )
    set(${MODULE_NAME}_MAIN_SOURCES main_name PARENT_SCOPE)
  endif()

  if(library_cpps)
    list(LENGTH library_cpps library_cpps_length)
    message(STATUS "  + library (${library_cpps_length})")
    list(APPEND MODULE_TARGETS ${MODULE_NAME})
    add_library(${MODULE_NAME} SHARED ${library_cpps})
    set_target_properties(${MODULE_NAME}
                          PROPERTIES
                              SOVERSION ${PROJECT_SO_VERSION}
                              VERSION   ${PROJECT_SO_VERSION}
                         )
    target_link_libraries(${MODULE_NAME} ${MODULE_LINK_LIBRARIES})
    if(main_name)
      target_link_libraries(${MODULE_NAME}_prog ${MODULE_NAME})
    endif()
    set(${MODULE_NAME}_LIBRARY_SOURCES ${library_cpps} PARENT_SCOPE)
    set(${MODULE_NAME}_LIBRARY_HEADERS ${library_hpps} PARENT_SCOPE)
  endif()

  if(library_test_cpps)
    list(LENGTH library_test_cpps library_test_cpps_length)
    message(STATUS "  + test library (${library_test_cpps_length})")
    list(APPEND MODULE_TARGETS ${MODULE_NAME}_tests)
    add_library(${MODULE_NAME}_tests SHARED ${library_test_cpps})
    set_target_properties(${MODULE_NAME}_tests
                          PROPERTIES
                              SOVERSION ${PROJECT_SO_VERSION}
                              VERSION   ${PROJECT_SO_VERSION}
                         )
    target_link_libraries(${MODULE_NAME}_tests zkpp-tests)
    if(library_cpps)
      target_link_libraries(${MODULE_NAME}_tests ${MODULE_NAME})
    endif()
    target_link_libraries(zkpp-tests_prog ${MODULE_NAME}_tests)
  endif()

  if(MODULE_PROTOTYPE)
    foreach(target ${MODULE_TARGETS})
      set_target_properties(${target}
                            PROPERTIES
                              COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error"
                           )
    endforeach()
  endif()
endfunction()
