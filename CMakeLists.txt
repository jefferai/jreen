CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
PROJECT(Jreen)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_JREEN_VERSION_MAJOR 1 CACHE INT "Major Jreen version number" FORCE)
set(CMAKE_JREEN_VERSION_MINOR 0 CACHE INT "Minor Jreen version number" FORCE)
set(CMAKE_JREEN_VERSION_PATCH 1 CACHE INT "Release Jreen version number" FORCE)
set(CMAKE_JREEN_VERSION_STRING "${CMAKE_JREEN_VERSION_MAJOR}.${CMAKE_JREEN_VERSION_MINOR}.${CMAKE_JREEN_VERSION_PATCH}" CACHE STRING "Jreen version string" FORCE)

# Search for source and headers in source directory (non-recursive)
FILE(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
FILE(GLOB HDR "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

# Require QT 4.6
SET(QT_MIN_VERSION "4.6.0")

# Search for QT4
FIND_PACKAGE(Qt4 COMPONENTS QtCore QtNetwork REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE(jreenMacros)
FIND_PACKAGE(QCA2 REQUIRED)
#FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(LibIDN)


# FindZLIB is broken on Ubuntu, so find the library using pkg-config
if(LINUX)
	find_package(PkgConfig)
	pkg_search_module(ZLIB_PKGCONF REQUIRED zlib)
	find_path(ZLIB_INCLUDE_DIR
	  NAMES zlib.h
	  PATHS ${ZLIB_PKGCONF_INCLUDE_DIRS}
	)
	find_library(ZLIB_LIBRARIES
	  NAMES z
	  PATHS ${ZLIB_PKGCONF_LIBRARY_DIRS}
	)
	if(NOT ZLIB_INCLUDE_DIR)
	  message(SEND_ERROR "Could not find zlib includes.")
	endif(NOT ZLIB_INCLUDE_DIR)
	if(NOT ZLIB_LIBRARIES)
	  message(SEND_ERROR "Could not find zlib library.")
	endif(NOT ZLIB_LIBRARIES)
else()
	find_package(ZLIB REQUIRED)
endif()

#add_subdirectory(3rdparty)

#spike

# Add include directories
INCLUDE_DIRECTORIES(.
	${CMAKE_CURRENT_BINARY_DIR}
	3rdparty
	${QCA2_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)
IF(LIBIDN_FOUND)
	INCLUDE_DIRECTORIES(${LIBIDN_INCLUDE_DIR})
	ADD_DEFINITIONS("-DHAVE_IDN=1")
ENDIF(LIBIDN_FOUND)

LIST(APPEND SRC ${3RD_PARTY_SRC_C})
LIST(APPEND SRC ${3RD_PARTY_SRC})
LIST(APPEND HDR ${3RD_PARTY_HDR})

option(JREEN_USE_SIMPLESASL "Use SimpleSASL" ON)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/simplesasl")
	set(JREEN_USE_SIMPLESASL OFF)
endif()
if(JREEN_USE_SIMPLESASL)
	add_definitions("-DHAVE_SIMPLESASL")
	file(GLOB_RECURSE SIMPLESASL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/simplesasl/*.cpp")
	file(GLOB_RECURSE SIMPLESASL_SRC_C "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/simplesasl/*.c")
	file(GLOB_RECURSE SIMPLESASL_HDR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/simplesasl/*.h")
	list(APPEND SRC ${SIMPLESASL_SRC})
	list(APPEND SRC ${SIMPLESASL_SRC_C})
	list(APPEND HDR ${SIMPLESASL_HDR})
endif()

option(JREEN_USE_SYSTEM_JDNS "Use system JDNS" OFF)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jdns")
	set(JREEN_USE_SYSTEM_JDNS ON)
endif()
if(JREEN_USE_SYSTEM_JDNS)
	# For Linux it was found already before at line 29
	if(NOT LINUX)
		find_package(PkgConfig)
	endif()
	pkg_search_module(JDNS REQUIRED qjdns)
	include_directories(${JDNS_INCLUDE_DIR})
	list(APPEND EXTRA_LIBS ${JDNS_LIBRARIES})
else()
	file(GLOB_RECURSE JDNS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jdns/*.cpp")
	file(GLOB_RECURSE JDNS_SRC_C "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jdns/*.c")
	file(GLOB_RECURSE JDNS_HDR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jdns/*.h")
	list(APPEND SRC ${JDNS_SRC})
	list(APPEND SRC ${JDNS_SRC_C})
	list(APPEND HDR ${JDNS_HDR})
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")
endif()

# Generate moc files
JREEN_WRAP_CPP(MOC_SRC ${HDR})

# This project will generate library

if(SYMBIAN)
	set(JREEN_BUILD_TYPE STATIC)
else()
set(JREEN_BUILD_TYPE SHARED)
endif()

ADD_LIBRARY(jreen ${JREEN_BUILD_TYPE} ${SRC} ${MOC_SRC} ${HDR} )

#compiler flags
#fixme add macro to clang
if(CMAKE_COMPILER_IS_GNUCXX)
	IF(SYMBIAN)
		#disable stupid warnings
		ADD_DEFINITIONS("-w")
	ELSE()
		ADD_DEFINITIONS("-Wall -Wextra" ) #add -Werror
	ENDIF()
	if(NOT WIN32)
		ADD_DEFINITIONS("-fvisibility=hidden" )
	endif(NOT WIN32)
elseif(MSVC)
	ADD_DEFINITIONS("/W3" )
endif()

set_target_properties(jreen PROPERTIES
	VERSION ${CMAKE_JREEN_VERSION_STRING}
	SOVERSION ${CMAKE_JREEN_VERSION_MAJOR}
	DEFINE_SYMBOL J_BUILD_LIBRARY
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/jreen" )
file(GLOB JREEN_INCLUDE_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
foreach(header ${JREEN_INCLUDE_HEADERS})
	if(NOT ${header} MATCHES ".*_p\\.h")
		if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/include/jreen/${header}")
			file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/jreen/${header}"
		"#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/${header}\"
	"
			)
		endif(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/include/jreen/${header}")
	LIST(APPEND JREEN_INSTALL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/${header}")
    endif(NOT ${header} MATCHES ".*_p\\.h")
endforeach(header)

set(JREEN_LIBRARIES jreen CACHE INTERNAL "" )
set(JREEN_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include" CACHE INTERNAL "")

IF(WIN32)
	LIST(APPEND EXTRA_LIBS ws2_32)
ENDIF()
IF(LIBIDN_FOUND)
	LIST(APPEND EXTRA_LIBS ${LIBIDN_LIBRARIES})
ENDIF(LIBIDN_FOUND)

# Link with Qt
#IF(SYMBIAN)
#ENDIF()

JREEN_TARGET_LINK_LIBRARIES(jreen
	${QT_LIBRARIES}
	${QCA2_LIBRARIES}
	${ZLIB_LIBRARIES}
	${EXTRA_LIBS}
)

# Install package config file
if(NOT WIN32)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/libjreen.pc.cmake
		${CMAKE_CURRENT_BINARY_DIR}/libjreen.pc
	)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjreen.pc
	DESTINATION lib${LIB_SUFFIX}/pkgconfig
)
endif(NOT WIN32)

install( TARGETS jreen
	RUNTIME DESTINATION bin
		COMPONENT jreen
	LIBRARY DESTINATION lib${LIB_SUFFIX}
		COMPONENT jreen
	ARCHIVE DESTINATION lib${LIB_SUFFIX}
		COMPONENT jreenDevel
	BUNDLE DESTINATION library
)

INSTALL(FILES ${JREEN_INSTALL_HEADERS}
	DESTINATION include/jreen
		COMPONENT jreenDevel
)
