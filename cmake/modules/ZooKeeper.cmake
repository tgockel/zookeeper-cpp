# Utilities for configuring and running a ZooKeeper Server.
cmake_minimum_required(VERSION 3.5)

include(CMakeParseArguments)

find_package(Java COMPONENTS Runtime)
if(NOT Java_Runtime_FOUND)
  message(FATAL_ERROR "Could not find Java Runtime")
endif()

# execute_jar
# Similar to execute_process, but drops "java -jar" in front for you so you can execute a JAR file in the same way you
# would a process.
macro(execute_jar)
  execute_process(COMMAND "${Java_JAVA_EXECUTABLE}" "-jar" ${ARGN})
endmacro()

# execute_java_cp
# Similar to execute_process, but drops "java -cp" in front for you so you can execute a collection of Java locations in
# the same way you would a process (albeit more annoyingly).
macro(execute_java_cp)
  execute_process(COMMAND "${Java_JAVA_EXECUTABLE}" "-cp" ${ARGN})
endmacro()

find_program(IVY_JAR
             NAMES ivy.jar
             PATHS "/usr/share/java"
            )
if(NOT IVY_JAR)
  message(FATAL_ERROR "Could not find Apache Ivy")
endif()

# find_zookeeper_server
# Get the ZooKeeper server JARs from Ivy.
#
#  VERSION: ZooKeeper version to fetch. This can be any Ivy pattern (for example, "3.5+").
#  OUTPUT_CLASSPATH: A variable to output a classpath that can be used to run the server.
#
# Example:
#
#   find_zookeeper_server(VERSION "3.5+" OUTPUT_CLASSPATH ZOOKEEPER_SERVER_CLASSPATH)
#   execute_java_cp("${ZOOKEEPER_SERVER_CLASSPATH}" "org.apache.zookeeper.server.quorum.QuorumPeerMain" 2181 zk-data)
function(find_zookeeper_server)
  set(options)
  set(oneValueArgs VERSION OUTPUT_CLASSPATH)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT ARG_VERSION)
    message(FATAL_ERROR "You must specify a VERSION to fetch")
  endif()
  if(NOT ARG_OUTPUT_CLASSPATH)
    message(FATAL_ERROR "You must specify an OUTPUT_CLASSPATH")
  endif()

  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/ivy-cache")
  set(TEMP_CLASSPATH_FILE "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/ivy-cache/zookeeper-${ARG_VERSION}.txt")

  # There does not appear to be a good way to tell Ivy to be reasonable about progress reporting, so we'll silence it
  # completely and hope the ellipsis in the status message will prevent people from thinking the system hung.
  message(STATUS "Fetching ZooKeeper Server ${ARG_VERSION}...")
  execute_jar(${IVY_JAR} "-dependency" "org.apache.zookeeper" "zookeeper" "${ARG_VERSION}"
                         "-cachepath" "${TEMP_CLASSPATH_FILE}"
              OUTPUT_VARIABLE IVY_FETCH_OUTPUT
              ERROR_VARIABLE  IVY_FETCH_ERROR
             )
  if(EXISTS "${TEMP_CLASSPATH_FILE}")
    file(READ "${TEMP_CLASSPATH_FILE}" CLASSPATH)
    string(STRIP "${CLASSPATH}" CLASSPATH)
    message(STATUS " > SUCCESS!")
    set(${ARG_OUTPUT_CLASSPATH} "${CLASSPATH}" PARENT_SCOPE)
  else()
    message(SEND_ERROR "Could not fetch ZooKeeper Server ${ARG_VERSION}\n"
                       "Ivy fetch output:\n${IVY_FETCH_OUTPUT}\n"
                       "Ivy fetch errors:\n${IVY_FETCH_ERROR}"
           )
  endif()
endfunction()
