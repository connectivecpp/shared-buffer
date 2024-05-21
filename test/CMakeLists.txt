# Copyright (c) 2024 by Cliff Green
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required ( VERSION 3.14 FATAL_ERROR )

# create project
project ( shared_buffer_test LANGUAGES CXX )

# add executable
add_executable ( shared_buffer_test shared_buffer_test.cpp )
target_compile_features ( shared_buffer_test PRIVATE cxx_std_20 )

# add dependencies
include ( ../cmake/download_cpm.cmake )

CPMAddPackage ( "gh:catchorg/Catch2@3.5.4" )
CPMAddPackage ( "gh:connectivecpp/utility-rack@1.0.0" )

# link dependencies
target_link_libraries ( shared_buffer_test PRIVATE utility_rack Catch2::Catch2WithMain )

enable_testing()

add_test ( NAME run_shared_buffer_test COMMAND shared_buffer_test )
set_tests_properties ( run_shared_buffer_test 
  PROPERTIES PASS_REGULAR_EXPRESSION "All tests passed"
  )

