CMAKE_MINIMUM_REQUIRED(VERSION 3.16)

PROJECT(botsort VERSION 1.0 LANGUAGES CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all source files
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Create library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Set include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Find and link OpenCV
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})


set(EIGEN3_INCLUDE_DIR "${3rdlib_DIR}/Eigen3/inlcude")
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR}})
target_link_libraries(${PROJECT_NAME} Eigen3::Eigen  )

if(NOT ${CUDA_FOUND})
    message(WARNING "CUDA not found, ReID won't be built")
else()
    message(STATUS "CUDA version ${CUDA_VERSION_STRING} found")
    target_include_directories(${PROJECT_NAME} PUBLIC ${CUDA_INCLUDE_DIRS}  ${TensorRT_INCLUDE_DIRS})

    target_sources(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src/TRT_InferenceEngine/TensorRT_InferenceEngine.cpp
        ${PROJECT_SOURCE_DIR}/src/ReID.cpp)
    target_link_libraries(${PROJECT_NAME} ${CUDA_LIBRARIES} ${TensorRT_LIBRARIES})
endif()

TARGET_COMPILE_FEATURES(${PROJECT_NAME} PRIVATE cxx_std_17)
TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE /W0 /WX-)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
				  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
				  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
				  DEBUG_POSTFIX "d"
				  )

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
else()
    # add_definitions(-DPROFILE=1)
    add_compile_options(-O3)
endif()

IF(MSVC)		
    INCLUDE(GenerateExportHeader)
    GENERATE_EXPORT_HEADER(${PROJECT_NAME}
        BASE_NAME ${PROJECT_NAME}      
        EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}_export.h
    )
ENDIF(MSVC)