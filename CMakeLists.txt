
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4)

ADD_DEFINITIONS()

LINK_DIRECTORIES()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

PROJECT(example C)
SET(SOURCE_EXTENSION c)

SET(RTIDDSGEN $ENV{RTIMEHOME}/rtiddsgen/scripts/rtiddsgen)
SET_PROPERTY(GLOBAL PROPERTY RULE_MESSAGES OFF)

SET(IDL_SOURCE_C example.idl)

SET(IDL_GEN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/gen)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})

IF (NOT EXISTS gen)
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory gen)
ENDIF()

SET(RTI_LIB_PREFIX rti_me)
SET(RTI_LIB_SUFFIX)

IF (NOT (DEFINED RTI_SHARED_LIB))
    IF (${CMAKE_BUILD_TYPE} MATCHES "[Rr]elease")
        SET(RTI_LIB_SUFFIX "z")
        SET(RTI_DEBUG_BUILD FALSE)
        ADD_DEFINITIONS(-DNDEBUG)
    ELSE()
        SET(RTI_LIB_SUFFIX "zd")
        SET(RTI_DEBUG_BUILD TRUE)
    ENDIF()
ELSE()
    IF (${CMAKE_BUILD_TYPE} MATCHES "[Rr]elease")
        SET(RTI_LIB_SUFFIX "")
        SET(RTI_DEBUG_BUILD FALSE)
        ADD_DEFINITIONS(-DNDEBUG)
    ELSE()
        SET(RTI_LIB_SUFFIX "d")
        SET(RTI_DEBUG_BUILD TRUE)
    ENDIF()
ENDIF()

SET(MICRO_C_LIBS rti_me_rhsm${RTI_LIB_SUFFIX} 
                 rti_me_whsm${RTI_LIB_SUFFIX} 
                 rti_me_discdpse${RTI_LIB_SUFFIX} 
                 rti_me_discdpde${RTI_LIB_SUFFIX} 
                 rti_me${RTI_LIB_SUFFIX}
                                                   )

MESSAGE("RTIME_TARGET_NAME = ${RTIME_TARGET_NAME}")
INCLUDE_DIRECTORIES(gen/ $ENV{RTIMEHOME}/include $ENV{RTIMEHOME}/include/rti_me)
LINK_DIRECTORIES($ENV{RTIMEHOME}/lib/${RTIME_TARGET_NAME})

FOREACH(idl ${IDL_SOURCE_C})
    GET_FILENAME_COMPONENT(filename ${idl} NAME)
    STRING(REGEX REPLACE "\\.idl" "" basename ${filename})
    LIST(APPEND IDL_GEN_C ${IDL_GEN_ROOT}/${basename}.${SOURCE_EXTENSION}
                          ${IDL_GEN_ROOT}/${basename}Plugin.${SOURCE_EXTENSION}
                          ${IDL_GEN_ROOT}/${basename}Support.${SOURCE_EXTENSION})
                          
    LIST(APPEND IDL_GEN_H ${IDL_GEN_ROOT}/${basename}.h
                          ${IDL_GEN_ROOT}/${basename}Plugin.h  
                          ${IDL_GEN_ROOT}/${basename}Support.h)
                          
    ADD_CUSTOM_COMMAND(OUTPUT ${IDL_GEN_C} ${IDL_GEN_H} 
                       COMMAND ${RTIDDSGEN} -verbosity 1 -language C -micro 
                               -replace 
                               -d ${IDL_GEN_ROOT}/. ${CMAKE_CURRENT_SOURCE_DIR}/${idl}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${idl}
                       COMMENT "Regeneration type support plugin from ${idl}"
    )    
    SET_SOURCE_FILES_PROPERTIES(${IDL_GEN_C} ${IDL_GEN_H} PROPERTIES GENERATED 1)
ENDFOREACH()

ADD_EXECUTABLE(example_publisher
               ${CMAKE_CURRENT_SOURCE_DIR}/example_publisher.${SOURCE_EXTENSION}
               ${CMAKE_CURRENT_SOURCE_DIR}/exampleApplication.${SOURCE_EXTENSION} 
               ${CMAKE_CURRENT_SOURCE_DIR}/exampleApplication.h 
                                             ${IDL_GEN_C} ${IDL_GEN_H})

ADD_EXECUTABLE(example_subscriber 
               ${CMAKE_CURRENT_SOURCE_DIR}/example_subscriber.${SOURCE_EXTENSION}
               ${CMAKE_CURRENT_SOURCE_DIR}/exampleApplication.${SOURCE_EXTENSION}
               ${CMAKE_CURRENT_SOURCE_DIR}/exampleApplication.h 
                                             ${IDL_GEN_C} ${IDL_GEN_H})

TARGET_LINK_LIBRARIES(example_publisher  ${MICRO_C_LIBS} ${PLATFORM_LIBS})
TARGET_LINK_LIBRARIES(example_subscriber ${MICRO_C_LIBS} ${PLATFORM_LIBS})
