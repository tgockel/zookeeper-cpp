cmake_minimum_required(VERSION 3.5)

file(READ src/zk/config.hpp CONFIG_HPP_STR)
string(REGEX REPLACE ".*# *define +ZKPP_VERSION_MAJOR +([0-9]+).*" "\\1" ZKPP_VERSION_MAJOR "${CONFIG_HPP_STR}")
string(REGEX REPLACE ".*# *define +ZKPP_VERSION_MINOR +([0-9]+).*" "\\1" ZKPP_VERSION_MINOR "${CONFIG_HPP_STR}")
string(REGEX REPLACE ".*# *define +ZKPP_VERSION_PATCH +([0-9]+).*" "\\1" ZKPP_VERSION_PATCH "${CONFIG_HPP_STR}")

set(ZKPP_VERSION "${ZKPP_VERSION_MAJOR}.${ZKPP_VERSION_MINOR}.${ZKPP_VERSION_PATCH}")
project(zookeeper-cpp
        LANGUAGES CXX
        VERSION "${ZKPP_VERSION}"
       )
set(PROJECT_SO_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
message(STATUS "Software Version: ${ZKPP_VERSION}")

################################################################################
# CMake                                                                        #
################################################################################

cmake_policy(VERSION 3.5)
cmake_policy(SET CMP0037 OLD) # allow generation of "test" target
set(CMAKE_REQUIRED_QUIET YES) # tell check_include_file_cxx to keep quiet

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(BuildFunctions)
include(CheckIncludeFileCXX)
include(ConfigurationSetting)
include(ListSplit)
include(ZooKeeper)

################################################################################
# Build Configuration                                                          #
################################################################################

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message(STATUS "No build type selected, default to ${CMAKE_BUILD_TYPE}")
endif()

set(VALID_BUILD_TYPES Debug Release)
if(NOT ${CMAKE_BUILD_TYPE} IN_LIST VALID_BUILD_TYPES)
  message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}\nValid build types are: ${VALID_BUILD_TYPES}")
endif()
message(STATUS "Configuration: ${CMAKE_BUILD_TYPE}")

set(ZKPP_SERVER_VERSIONS "3.5.5;3.4.14"
    CACHE STRING "The ZooKeeper server versions to run tests against. The first in the list is the default."
   )

message(STATUS "Features:")
build_option(NAME       CODE_COVERAGE
             DOC        "Enable code coverage (turns on the test-coverage target)"
             DEFAULT    OFF
             CONFIGS_ON Debug
            )

configuration_setting(NAME    BUFFER
                      DOC     "Type to use for zk::buffer"
                      DEFAULT STD_VECTOR
                      OPTIONS
                        STD_VECTOR
                        STD_STRING
                        CUSTOM
                     )

configuration_setting(NAME    FUTURE
                      DOC     "Type to use for zk::future<T> and zk::promise<T>"
                      DEFAULT STD
                      OPTIONS
                        STD
                        STD_EXPERIMENTAL
                        BOOST
                        CUSTOM
                     )

set(CXX_STANDARD c++17
    CACHE STRING "The language standard to target for C++."
   )

set(CXX_WARNINGS "-Wall -Wextra -Wconversion -Werror")
set(CXX_EXTRA_FLAGS "-Wl,--no-as-needed")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=${CXX_STANDARD} ${CXX_WARNINGS} -ggdb3 ${CXX_EXTRA_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -DZKPP_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

################################################################################
# External Libraries                                                           #
################################################################################

find_library(zookeeper_LIBRARIES zookeeper_mt)
set(ZKPP_LIB_DEPENDENCIES ${ZKPP_LIB_DEPENDENCIES} ${zookeeper_LIBRARIES})

include_directories("${PROJECT_SOURCE_DIR}/src")

if (ZKPP_BUILD_SETTING_FUTURE STREQUAL "BOOST")
    find_package(Boost
                 1.52.0
                 REQUIRED
                 thread)
    set(ZKPP_LIB_DEPENDENCIES ${ZKPP_LIB_DEPENDENCIES} ${Boost_LIBRARIES})
endif()



################################################################################
# GTest                                                                        #
################################################################################

find_package(GTest)

if(GTest_FOUND)
  # Make it look like find_library
  set(gtest_LIBRARIES GTest::GTest)
elseif(EXISTS "/usr/src/googletest/googletest/src" OR EXISTS "/usr/src/gtest/src/")
  # GTest's packaging on Ubuntu (googletest or libgtest-dev, depending on which version) contains all the source files
  # instead of a library and headers. CMake has a package discovery for GTest, but it does not pick up on this for you,
  # so we'll build it manually here.
  message(STATUS "GTest is not installed, but the sources were found...adding them to the build")
  if(EXISTS "/usr/src/googletest/googletest/src")
    # Prefer the "googletest" version, as sometimes the "gtest" one is an empty directory (this seems to be the result
    # of a packaging issue)
    set(GTEST_SRC_ROOT "/usr/src/googletest/googletest/src")
  else()
    set(GTEST_SRC_ROOT "/usr/src/gtest/src")
  endif()

  if(EXISTS "${GTEST_SRC_ROOT}/gtest-all.cc")
    set(gtest_lib_cpps "${GTEST_SRC_ROOT}/gtest-all.cc")
    message(STATUS "Building with ${GTEST_SRC_ROOT}/gtest-all.cc")
  else()
    file(GLOB gtest_lib_cpps "${GTEST_SRC_ROOT}/gtest-*.cc")
    message(STATUS "Building with gtest_lib_cpps=${gtest_lib_cpps}")
  endif()
  add_library(gtest SHARED ${gtest_lib_cpps})

  # GTest uses relative imports incorrectly, so make sure to add it to the include path.
  target_include_directories(gtest PRIVATE "${GTEST_SRC_ROOT}/..")
  # Also disable -Werror
  target_compile_options(gtest PRIVATE "-Wno-error")

  set(gtest_LIBRARIES gtest)
else()
  message(SEND_ERROR "GTest was not found")
endif()

################################################################################
# Building                                                                     #
################################################################################

build_module(NAME zkpp-tests
             PATH src/zk/tests
             LINK_LIBRARIES
               ${gtest_LIBRARIES}
            )

build_module(NAME zkpp
             PATH src/zk
             NO_RECURSE
             LINK_LIBRARIES
             ${ZKPP_LIB_DEPENDENCIES}
             )

build_module(NAME zkpp-server
             PATH src/zk/server
             LINK_LIBRARIES
               zkpp
            )

target_link_libraries(zkpp_tests zkpp-server zkpp-server_tests)

################################################################################
# ZooKeeper Server Testing                                                     #
################################################################################

foreach(server_version IN LISTS ZKPP_SERVER_VERSIONS)
  find_zookeeper_server(VERSION "${server_version}"
                        OUTPUT_CLASSPATH server_classpath
                       )
  set(generated_cpp "${CMAKE_CURRENT_BINARY_DIR}/generated/src/zk/server/classpath_registration_${server_version}.cpp")
  configure_file(src/zk/server/classpath_registration_template.cpp.in "${generated_cpp}" @ONLY)
  target_sources(zkpp-server_tests PRIVATE "${generated_cpp}")
endforeach()

################################################################################
# Targets                                                                      #
################################################################################

add_custom_target(test
                  COMMAND $<TARGET_FILE:zkpp-tests_prog>
                          "--gtest_output=xml:test-results.xml"
                          "--gtest_death_test_style=threadsafe"
                  DEPENDS zkpp-tests_prog
                  BYPRODUCTS test-results.xml
                  USES_TERMINAL
                 )

# Similar to test, but run it inside of GDB with GTest options one would want when running in GDB.
add_custom_target(gdbtest
                  COMMAND "gdb"
                          "-args"
                          $<TARGET_FILE:zkpp-tests_prog>
                          "--gtest_output=xml:test-results-gdb.xml"
                          "--gtest_death_test_style=threadsafe"
                          "--gtest_break_on_failure=1"
                          "--gtest_catch_exceptions=0"
                  DEPENDS zkpp-tests_prog
                  BYPRODUCTS test-results-gdb.xml
                  USES_TERMINAL
                 )

if(ZKPP_BUILD_OPTION_CODE_COVERAGE)
  include(CodeCoverage)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  setup_target_for_coverage(test-coverage zkpp-tests_prog coverage "--gtest_death_test_style=threadsafe")
endif()

################################################################################
# Packaging                                                                    #
################################################################################

set(ZKPP_PACKAGE_SYSTEM ""
    CACHE STRING "The packaging system to generate a package build file for (leave blank to not build a package)"
   )
if(ZKPP_PACKAGE_SYSTEM)
  message(STATUS "Packaging system ${ZKPP_PACKAGE_SYSTEM}:")
  set(PROJECT_PACKAGE_VERSION "${PROJECT_VERSION}-1") # TODO: Allow arbitrary tags here.

  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(PROJECT_BUILD_ARCHITECTURE "amd64")
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    set(PROJECT_BUILD_ARCHITECTURE "i386")
  else()
    set(PROJECT_BUILD_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
  endif()

  set(all_packages)
  foreach(module zkpp zkpp-server)
    message(STATUS "  ${module}:")
    set(package_name ${module})
    string(REGEX REPLACE "-"  "/" header_path "${module}")
    string(REGEX REPLACE "pp" ""  header_path "${header_path}")

    if(ZKPP_PACKAGE_SYSTEM STREQUAL "DEB")
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/install/deb/lib${package_name}")
        message(STATUS "    lib${package_name}")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/install/deb/lib${package_name}/control.in"
                       "${CMAKE_CURRENT_BINARY_DIR}/install/lib${package_name}${PROJECT_SO_VERSION}/DEBIAN/control"
                       @ONLY IMMEDIATE
                      )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/install/deb/lib${package_name}/postinst.in")
          configure_file("${CMAKE_CURRENT_SOURCE_DIR}/install/deb/lib${package_name}/postinst.in"
                         "${CMAKE_CURRENT_BINARY_DIR}/intermediate/lib${package_name}${PROJECT_SO_VERSION}/DEBIAN/postinst"
                         @ONLY IMMEDIATE
                        )
          file(COPY "${CMAKE_CURRENT_BINARY_DIR}/intermediate/lib${package_name}${PROJECT_SO_VERSION}/DEBIAN/postinst"
               DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/install/lib${package_name}${PROJECT_SO_VERSION}/DEBIAN"
               FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
              )
        endif()

        add_custom_target("lib${package_name}.deb"
                          BYPRODUCTS "install/lib${package_name}${PROJECT_SO_VERSION}.deb"
                          COMMAND rm -rf "install/lib${package_name}${PROJECT_SO_VERSION}${CMAKE_INSTALL_PREFIX}/lib"
                          COMMAND mkdir -p "install/lib${package_name}${PROJECT_SO_VERSION}${CMAKE_INSTALL_PREFIX}/lib"
                          COMMAND cp "$<TARGET_FILE:${module}>" "install/lib${package_name}${PROJECT_SO_VERSION}${CMAKE_INSTALL_PREFIX}/lib"
                          COMMAND dpkg --build "install/lib${package_name}${PROJECT_SO_VERSION}"
                          DEPENDS
                            "${CMAKE_CURRENT_BINARY_DIR}/install/lib${package_name}${PROJECT_SO_VERSION}/DEBIAN/control"
                            ${module}
                          )
        list(APPEND all_packages "lib${package_name}.deb")
      endif()

      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/install/deb/lib${package_name}-dev")
        message(STATUS "    lib${package_name}-dev")
        set(staging_dir "install/lib${package_name}${PROJECT_SO_VERSION}-dev${CMAKE_INSTALL_PREFIX}/include/${header_path}")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/install/deb/lib${package_name}-dev/control.in"
                       "${CMAKE_CURRENT_BINARY_DIR}/install/lib${package_name}${PROJECT_SO_VERSION}-dev/DEBIAN/control"
                       @ONLY IMMEDIATE
                      )
        add_custom_target("lib${package_name}-dev.deb"
                          BYPRODUCTS "install/lib${package_name}${PROJECT_SO_VERSION}-dev.deb"
                          COMMAND rm -rf "${staging_dir}"
                          COMMAND mkdir -p "${staging_dir}"
                          COMMAND cp -t "${staging_dir}" ${${module}_LIBRARY_HEADERS}
                          COMMAND dpkg --build "install/lib${package_name}${PROJECT_SO_VERSION}-dev"
                          DEPENDS
                            "${CMAKE_CURRENT_BINARY_DIR}/install/lib${package_name}${PROJECT_SO_VERSION}-dev/DEBIAN/control"
                            ${${module}_LIBRARY_HEADERS}
                         )
        list(APPEND all_packages "lib${package_name}-dev.deb")
      endif()
    else()
      message(FATAL_ERROR "Unknown ZKPP_PACKAGE_SYSTEM=${ZKPP_PACKAGE_SYSTEM}")
    endif()
  endforeach()
  add_custom_target(package
                    DEPENDS ${all_packages}
                    COMMENT "Built all packages"
                   )
endif()
