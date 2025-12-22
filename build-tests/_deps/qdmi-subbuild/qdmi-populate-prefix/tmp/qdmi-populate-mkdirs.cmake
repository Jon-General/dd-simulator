# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspaces/dd-simulator/build-tests/_deps/qdmi-src"
  "/workspaces/dd-simulator/build-tests/_deps/qdmi-build"
  "/workspaces/dd-simulator/build-tests/_deps/qdmi-subbuild/qdmi-populate-prefix"
  "/workspaces/dd-simulator/build-tests/_deps/qdmi-subbuild/qdmi-populate-prefix/tmp"
  "/workspaces/dd-simulator/build-tests/_deps/qdmi-subbuild/qdmi-populate-prefix/src/qdmi-populate-stamp"
  "/workspaces/dd-simulator/build-tests/_deps/qdmi-subbuild/qdmi-populate-prefix/src"
  "/workspaces/dd-simulator/build-tests/_deps/qdmi-subbuild/qdmi-populate-prefix/src/qdmi-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/dd-simulator/build-tests/_deps/qdmi-subbuild/qdmi-populate-prefix/src/qdmi-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspaces/dd-simulator/build-tests/_deps/qdmi-subbuild/qdmi-populate-prefix/src/qdmi-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
