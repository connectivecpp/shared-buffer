# Copyright (c) 2024 by Cliff Green
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required ( VERSION 3.14 FATAL_ERROR )

# create project
project ( shared_buffer_example LANGUAGES CXX )

# add executable
add_executable ( shared_buffer_example shared_buffer_example.cpp )
target_compile_features ( shared_buffer_example PRIVATE cxx_std_20 )

# link dependencies
target_link_libraries ( shared_buffer_example PRIVATE shared_buffer )

