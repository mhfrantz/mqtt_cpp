CMAKE_MINIMUM_REQUIRED (VERSION 2.8.6)
PROJECT (mqtt_cpp)

IF ((CMAKE_VERSION VERSION_GREATER 3.1) OR
    (CMAKE_VERSION VERSION_EQUAL 3.1))
    CMAKE_POLICY(SET CMP0054 NEW)
ENDIF ()

IF (MQTT_NO_TLS)
    MESSAGE (STATUS "TLS disabled")
    SET (CMAKE_CXX_FLAGS "-DMQTT_NO_TLS ${CMAKE_CXX_FLAGS}")
ELSE ()
    MESSAGE (STATUS "TLS enabled")
    FIND_PACKAGE (OpenSSL)
ENDIF ()

IF (MQTT_USE_WS)
    MESSAGE (STATUS "WebSocket enabled")
    SET (CMAKE_CXX_FLAGS "-DMQTT_USE_WS ${CMAKE_CXX_FLAGS}")
ELSE ()
    MESSAGE (STATUS "WebSocket disabled")
ENDIF ()

IF (MQTT_USE_STR_CHECK)
    MESSAGE (STATUS "UTF8String check enabled")
    SET (CMAKE_CXX_FLAGS "-DMQTT_USE_STR_CHECK ${CMAKE_CXX_FLAGS}")
ELSE ()
    MESSAGE (STATUS "UTF8String check disabled")
ENDIF ()

IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    SET (CMAKE_CXX_FLAGS "/bigobj ${CMAKE_CXX_FLAGS}")
ENDIF ()

SET (Boost_USE_STATIC_LIBS        ON) # only find static libs
SET (Boost_USE_MULTITHREADED      ON)
# SET (Boost_USE_STATIC_RUNTIME    OFF)
FIND_PACKAGE (Boost 1.59.0 REQUIRED COMPONENTS chrono timer test_exec_monitor system)
FIND_PACKAGE (Threads)

IF (NOT MQTT_BEAST_INCLUDE_DIR)
    IF (${Boost_VERSION} LESS 106600)
        SET (MQTT_BEAST_INCLUDE_DIR "external/beast/include")
    ENDIF ()
ENDIF ()

INCLUDE_DIRECTORIES (
    ${Boost_INCLUDE_DIR}
    ${MQTT_BEAST_INCLUDE_DIR}
    include
)

IF (NOT MQTT_NO_TLS)
    INCLUDE_DIRECTORIES (
        ${OpenSSL_INCLUDE_DIR}
        ${MQTT_OPENSSL_INCLUDE_DIR}
    )
ENDIF ()

ENABLE_TESTING ()
ADD_SUBDIRECTORY (test)
ADD_SUBDIRECTORY (example)

# Doxygen
FIND_PACKAGE (Doxygen)
IF (DOXYGEN_FOUND)
    LIST (APPEND Doxyfile_CONTENT
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "FILE_PATTERNS      = *.hpp" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "OUTPUT_DIRECTORY   = doc" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "PROJECT_NAME       = \"mqtt_cpp\"" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "EXTRACT_ALL        = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "RECURSIVE          = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "INPUT              = ${CMAKE_CURRENT_SOURCE_DIR}/include" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    )
    IF (DOXYGEN_DOT_FOUND)
        LIST (APPEND Doxyfile_CONTENT
            COMMAND ${CMAKE_COMMAND} -E echo "HAVE_DOT       = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        )
    ENDIF ()
    ADD_CUSTOM_TARGET (
        doxygen
        ${Doxyfile_CONTENT}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        VERBATIM
    )
ENDIF ()
