cmake_minimum_required(VERSION 3.11.4)
project(my_cuda)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_STANDARD 17)

set(MY_SOURCE_FILES
    include/my_cuda.h
    include/CUDA_info.h
    include/CUDA_memory_managed_array.h
    include/CUDA_memory_managed_item.h
    include/CUDA_memory_managed_pointer.h
    include/CUDA_print.h
    include/CUDA_strong_primitive.h
    include/CUDA_utils.h
   )


# List header directories in project
set(MY_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

#Create dependancy list
set(DEPENDANCY_LIST "")
LIST(APPEND DEPENDANCY_LIST "quicky_exception")

macro(CUDA_CONVERT_FLAGS EXISTING_TARGET)
    get_property(old_flags TARGET ${EXISTING_TARGET} PROPERTY COMPILE_OPTIONS)
    if(NOT "${old_flags}" STREQUAL "")
        get_property(old_cxx_standard TARGET ${EXISTING_TARGET} PROPERTY CXX_STANDARD)
        string(REPLACE ";" "," CUDA_flags "${old_flags}")
        # C++ standard set to 11 and not propagated due to issues when compiling with -std=c++17
        set_property(TARGET ${EXISTING_TARGET} PROPERTY COMPILE_OPTIONS
                     "$<$<BUILD_INTERFACE:$<COMPILE_LANGUAGE:CXX>>:${old_flags}>$<$<BUILD_INTERFACE:$<COMPILE_LANGUAGE:CUDA>>:-Xcompiler=${CUDA_flags}$<$<BOOL:${old_cxx_standard}>:,-std=c++11>$<$<CONFIG:Debug>:,-g>>"
                     )
    endif()
endmacro()

macro(HANDLE_CUDA_FILES A_PROJECT_NAME LIST_OF_FILES)
    # Search for CUDA files
    foreach(SOURCE_FILE IN ITEMS ${LIST_OF_FILES})
        if(${SOURCE_FILE} MATCHES "(.*\).cu$")
            message("CUDA SOURCE DETECTED")
            set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
            set_target_properties(${A_PROJECT_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
            CUDA_CONVERT_FLAGS(${A_PROJECT_NAME})
            if(NOT DEFINED CMAKE_CUDA_STANDARD)
                set(CMAKE_CUDA_STANDARD 11)
                set(CMAKE_CUDA_STANDARD_REQUIRED ON)
                set(CMAKE_CUDA_FLAGS ${CMAKE_CUDA_FLAGS} " --keep --keep-dir=${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY} --source-in-ptx --ptxas-options=-v -arch=sm_75 -gencode=arch=compute_75,code=sm_75")
                if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
                    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -G")
                else()
                    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -lineinfo")
                endif()
            endif()
            break()
        endif()
    endforeach(SOURCE_FILE)
endmacro()

#------------------------------
#- Generic part
#------------------------------
set(DEPENDANCY_OBJECTS "")
set(LINKED_LIBRARIES "")
#iterate on dependancies
foreach(DEPENDANCY_ITEM IN ITEMS ${DEPENDANCY_LIST})
    message("${PROJECT_NAME} need dependancy ${DEPENDANCY_ITEM}")
    set(IS_IMPORTED FALSE)
    #Get information from dependancies if not already done
    if(NOT TARGET ${DEPENDANCY_ITEM})
        set(SUB_DEPENDANCY_OBJECTS "")
        set(SUB_LINKED_LIBRARIES "")
        add_subdirectory($ENV{QUICKY_REPOSITORY}/${DEPENDANCY_ITEM} ${CMAKE_CURRENT_BINARY_DIR}/${DEPENDANCY_ITEM})
        get_target_property(IS_IMPORTED ${DEPENDANCY_ITEM} IMPORTED)
        if(NOT ${IS_IMPORTED})
            list(APPEND DEPENDANCY_OBJECTS "$<TARGET_OBJECTS:${DEPENDANCY_ITEM}>" ${SUB_DEPENDANCY_OBJECTS})
        else()
            list(APPEND LINKED_LIBRARIES ${DEPENDANCY_ITEM})
        endif()
        list(APPEND LINKED_LIBRARIES ${SUB_LINKED_LIBRARIES})
    endif()

    # Complete include directories with directories coming from dependancy because of transitivity issues with Object libraries
    if(NOT ${IS_IMPORTED})
        set(MY_INCLUDE_DIRECTORIES ${MY_INCLUDE_DIRECTORIES} $<TARGET_PROPERTY:${DEPENDANCY_ITEM},INCLUDE_DIRECTORIES>)
    endif()

endforeach(DEPENDANCY_ITEM)

#Prepare targets
get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(IS_DIRECTORY ${HAS_PARENT})
    message("Declare library ${PROJECT_NAME}")
    add_library(${PROJECT_NAME} OBJECT ${MY_SOURCE_FILES})
    set(SUB_DEPENDANCY_OBJECTS ${DEPENDANCY_OBJECTS} PARENT_SCOPE)
    set(SUB_LINKED_LIBRARIES ${LINKED_LIBRARIES} PARENT_SCOPE)
else()
    #set(CMAKE_VERBOSE_MAKEFILE ON)
    # string(REPLACE " " ";" DEPENDANCY_OBJECTS ${DEPENDANCY_OBJECTS})
    if (NOT DEFINED ENABLE_CUDA_CODE)
        set(ENABLE_CUDA_CODE ON)
    endif ()
    if(${ENABLE_CUDA_CODE})
        set (MY_CPP_FLAGS -DENABLE_CUDA_CODE)
        enable_language(CUDA)
        set(MY_SOURCE_FILES
            ${MY_SOURCE_FILES}
            test/src/test.cu
           )
    else(${ENABLE_CUDA_CODE})
        set (MY_CPP_FLAGS -pedantic)
    endif(${ENABLE_CUDA_CODE})

    set(MY_SOURCE_FILES
        ${MY_SOURCE_FILES}
        test/include/test_if.h
        test/include/test.h
        test/include/example_object.h
        test/src/test.cpp
       )
    set(MY_INCLUDE_DIRECTORIES
        ${MY_INCLUDE_DIRECTORIES}
        test/include
       )
    add_executable(${PROJECT_NAME} ${MY_SOURCE_FILES} ${DEPENDANCY_OBJECTS} test/src/main.cpp)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -D$<UPPER_CASE:${PROJECT_NAME}>_SELF_TEST)
    message(Linked librarries ${LINKED_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} ${LINKED_LIBRARIES})
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -g -O0 -DNDEBUG ${MY_CPP_FLAGS})
    HANDLE_CUDA_FILES(${PROJECT_NAME} "${MY_SOURCE_FILES}")
endif()


target_include_directories(${PROJECT_NAME} PUBLIC ${MY_INCLUDE_DIRECTORIES})

foreach(DEPENDANCY_ITEM IN ITEMS ${DEPENDANCY_LIST})
    add_dependencies(${PROJECT_NAME} ${DEPENDANCY_ITEM})
endforeach(DEPENDANCY_ITEM)

#Force use of -std=c++11 instead of -std=gnu++11
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

#EOF
