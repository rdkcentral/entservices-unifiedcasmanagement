# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(PLUGIN_NAME UnifiedCASManagement)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
if (NOT RDK_SERVICES_L1_TEST AND NOT RDK_SERVICE_L2_TEST)
find_package(${NAMESPACE}Protocols REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
endif()

list(APPEND CMAKE_MODULE_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

find_package(IARMBus)

if (NOT RDK_SERVICES_L1_TEST AND NOT RDK_SERVICE_L2_TEST)
find_package(LMPLAYER REQUIRED)
else()
include_directories(
        ${PROJECT_SOURCE_DIR}/../entservices-testframework/Tests/mocks/
        ${PROJECT_SOURCE_DIR}/helpers
)
endif()

if (LMPLAYER_FOUND)
	add_library(${MODULE_NAME} SHARED
	        UnifiedCASManagement.cpp
	        Module.cpp
	        LibMediaPlayerImpl.cpp
	        )
else(LMPLAYER_FOUND)
    message ("MISSING A PLAYER IMPLEMENTATION.")
	add_library(${MODULE_NAME} SHARED
	        UnifiedCASManagement.cpp
	        Module.cpp
	        )
endif(LMPLAYER_FOUND)

if (IARMBus_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE ${IARMBUS_INCLUDE_DIRS})
    target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${IARMBUS_LIBRARIES})
else (IARMBus_FOUND)
    message ("Module IARMBus required.")
    target_include_directories(${MODULE_NAME} PRIVATE ${IARMBUS_INCLUDE_DIRS})
    target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${IARMBUS_LIBRARIES})
endif(IARMBus_FOUND)

add_definitions( -DRT_PLATFORM_LINUX=1 )

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES)

if (RDK_SERVICE_L2_TEST)
    message ("L2 test Enabled")
    find_library(TESTMOCKLIB_LIBRARIES NAMES TestMocklib)
    if (TESTMOCKLIB_LIBRARIES)
        message ("linking mock libraries ${TESTMOCKLIB_LIBRARIES} library")
        target_link_libraries(${MODULE_NAME} PRIVATE ${TESTMOCKLIB_LIBRARIES})
    else (TESTMOCKLIB_LIBRARIES)
        message ("Require ${TESTMOCKLIB_LIBRARIES} library")
    endif (TESTMOCKLIB_LIBRARIES)
endif()

target_include_directories(${MODULE_NAME} PRIVATE ../helpers)

if (LMPLAYER_FOUND)
    add_definitions(-DLMPLAYER_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE ${LMPLAYER_INCLUDE_DIRS})
    target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${LMPLAYER_LIBRARIES})

else(LMPLAYER_FOUND)
    message ("MISSING A PLAYER IMPLEMENTATION.")
endif(LMPLAYER_FOUND)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
