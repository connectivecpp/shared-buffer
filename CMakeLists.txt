# Copyright (c) 2024-2025 by Cliff Green
#
# https://github.com/connectivecpp/shared-buffer
#
# I'm still learning CMake, so improvement suggestions are always welcome.
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required ( VERSION 3.14 FATAL_ERROR )

project ( shared_buffer 
	  LANGUAGES CXX 
	  DESCRIPTION "Reference counted character buffer classes"
	  HOMEPAGE_URL "https://github.com/connectivecpp/shared-buffer/" )

option ( SHARED_BUFFER_BUILD_TESTS "Build unit tests" OFF )
option ( SHARED_BUFFER_BUILD_EXAMPLES "Build examples" OFF )
option ( SHARED_BUFFER_INSTALL "Install header only library" OFF )

# add library targets

add_library ( shared_buffer INTERFACE )
add_library ( chops::shared_buffer ALIAS shared_buffer )

# configure library target

target_include_directories ( shared_buffer INTERFACE
                             $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
			     $<INSTALL_INTERFACE:include/> )
target_compile_features ( shared_buffer INTERFACE cxx_std_20 )

# check to build unit tests
if ( ${SHARED_BUFFER_BUILD_TESTS} )
  enable_testing()
  add_subdirectory ( test )
endif ()

# check to build example code
if ( ${SHARED_BUFFER_BUILD_EXAMPLES} )
  add_subdirectory ( example )
endif ()

# check to install
if ( ${SHARED_BUFFER_INSTALL} )
  set ( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt )
  include ( CPack )
endif ()

# end of file

