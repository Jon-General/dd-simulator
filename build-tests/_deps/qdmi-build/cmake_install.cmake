# Install script for directory: /workspaces/dd-simulator/build-tests/_deps/qdmi-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/qdmi" TYPE FILE FILES
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-build/qdmi-config.cmake"
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-build/qdmi-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "qdmi_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/qdmi/qdmi" TYPE FILE FILES
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-src/include/qdmi/client.h"
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-src/include/qdmi/constants.h"
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-src/include/qdmi/device.h"
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-src/include/qdmi/types.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "qdmi_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/qdmi/qdmi-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/qdmi/qdmi-targets.cmake"
         "/workspaces/dd-simulator/build-tests/_deps/qdmi-build/CMakeFiles/Export/fd6ce45569c2deba045534c91734c3a3/qdmi-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/qdmi/qdmi-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/qdmi/qdmi-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/qdmi" TYPE FILE FILES "/workspaces/dd-simulator/build-tests/_deps/qdmi-build/CMakeFiles/Export/fd6ce45569c2deba045534c91734c3a3/qdmi-targets.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "qdmi_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/qdmi" TYPE FILE FILES
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-src/cmake/Cache.cmake"
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-src/cmake/prefix_defs.txt"
    "/workspaces/dd-simulator/build-tests/_deps/qdmi-src/cmake/PrefixHandling.cmake"
    )
endif()

