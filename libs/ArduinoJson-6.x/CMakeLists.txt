# ArduinoJson - arduinojson.org
# Copyright Benoit Blanchon 2014-2020
# MIT License

cmake_minimum_required(VERSION 3.0)

project(ArduinoJson VERSION 6.15.1)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)

	if(CMAKE_CXX_COMPILER_ID STREQUAL  "GNU")
		if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8)
			add_compile_options(-g -Og)
		else()
			add_compile_options(-g -O0)
		endif()
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0)
			add_compile_options(-g -Og)
		else()
			add_compile_options(-g -O0)
		endif()
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
		if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.0)
			add_compile_options(-g -Og)
		else()
			add_compile_options(-g -O0)
		endif()
	endif()

	if(${COVERAGE})
		set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
	endif()

	add_subdirectory(extras/tests)
	add_subdirectory(extras/fuzzing)
endif()
