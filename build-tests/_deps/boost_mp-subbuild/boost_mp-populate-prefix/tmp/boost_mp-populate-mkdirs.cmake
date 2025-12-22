# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspaces/dd-simulator/build-tests/_deps/boost_mp-src"
  "/workspaces/dd-simulator/build-tests/_deps/boost_mp-build"
  "/workspaces/dd-simulator/build-tests/_deps/boost_mp-subbuild/boost_mp-populate-prefix"
  "/workspaces/dd-simulator/build-tests/_deps/boost_mp-subbuild/boost_mp-populate-prefix/tmp"
  "/workspaces/dd-simulator/build-tests/_deps/boost_mp-subbuild/boost_mp-populate-prefix/src/boost_mp-populate-stamp"
  "/workspaces/dd-simulator/build-tests/_deps/boost_mp-subbuild/boost_mp-populate-prefix/src"
  "/workspaces/dd-simulator/build-tests/_deps/boost_mp-subbuild/boost_mp-populate-prefix/src/boost_mp-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/dd-simulator/build-tests/_deps/boost_mp-subbuild/boost_mp-populate-prefix/src/boost_mp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspaces/dd-simulator/build-tests/_deps/boost_mp-subbuild/boost_mp-populate-prefix/src/boost_mp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
