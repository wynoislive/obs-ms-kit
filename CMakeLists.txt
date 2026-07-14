# --- Detect if the plugin is build out of tree or not ---
if(CMAKE_PROJECT_NAME STREQUAL "obs-studio")
  set(BUILD_OUT_OF_TREE OFF)
else()
  set(BUILD_OUT_OF_TREE ON)
  cmake_minimum_required(VERSION 3.28...3.30)
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${PROJECT_NAME} MODULE)

target_link_libraries(${PROJECT_NAME} PRIVATE OBS::libobs)

if(BUILD_OUT_OF_TREE)
  find_package(libobs REQUIRED)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE OBS::frontend-api)
endif()

find_package(Qt6 COMPONENTS Widgets Core)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
if((OS_LINUX OR OS_FREEBSD OR OS_OPENBSD) AND Qt6_VERSION VERSION_LESS "6.9.0")
	find_package(Qt6 COMPONENTS GuiPrivate)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt::GuiPrivate)
endif()

target_compile_options(
${PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header
                                -Wno-comma>)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(
${PROJECT_NAME}
PROPERTIES AUTOMOC ON
            AUTOUIC ON
            AUTORCC ON)

if(BUILD_OUT_OF_TREE)
  find_package(CURL REQUIRED)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)

if(OS_WINDOWS)
  configure_file(cmake/windows/resources/installer-Windows.iss.in "${CMAKE_CURRENT_BINARY_DIR}/installer-Windows.generated.iss")
endif()

set(KERNEL_HEADERS
  runtime/kernel/IService.hpp
  runtime/kernel/ILogger.hpp
  runtime/kernel/ServiceRegistry.hpp
  runtime/kernel/IKernelContext.hpp
  runtime/kernel/IScheduler.hpp
  runtime/kernel/IEventDispatcher.hpp
  runtime/kernel/IConfigManager.hpp
  runtime/kernel/Scheduler.hpp
  runtime/kernel/KernelContext.hpp
  runtime/kernel/EventDispatcher.hpp
)

set(ENGINE_HEADERS
  runtime/engine/config/OutputProfile.hpp
  runtime/engine/output/OutputRuntimeState.hpp
  runtime/engine/output/IOutputSession.hpp
  runtime/engine/output/OutputController.hpp
  runtime/engine/output/OutputSession.hpp
  runtime/engine/output/ReconnectPolicy.hpp
  runtime/engine/output/HealthMonitor.hpp
  runtime/engine/network/NetworkBuffer.hpp
  runtime/engine/video/FrameDistributor.hpp
  runtime/engine/video/IScaler.hpp
  runtime/engine/video/Scaler.hpp
  runtime/engine/encoder/IEncoderInstance.hpp
  runtime/engine/encoder/EncoderInstance.hpp
  runtime/engine/protocol/IProtocolClient.hpp
  runtime/engine/protocol/RtmpClient.hpp
)

target_sources(${PROJECT_NAME} PRIVATE
  config-dialog.cpp
  config-utils.cpp
  output-dialog.cpp
  stream-key-input.cpp
  multistream.cpp
  file-updater.c
  runtime/engine/output/OutputSession.cpp
  runtime/engine/output/OutputController.cpp
  runtime/engine/output/ReconnectPolicy.cpp
  runtime/engine/output/HealthMonitor.cpp
  runtime/engine/network/NetworkBuffer.cpp
  runtime/engine/video/FrameDistributor.cpp
  runtime/engine/video/Scaler.cpp
  runtime/engine/encoder/EncoderInstance.cpp
  runtime/engine/protocol/RtmpClient.cpp
  runtime/kernel/Scheduler.cpp
  runtime/kernel/KernelContext.cpp
  runtime/kernel/EventDispatcher.cpp
	resources.qrc
	config-dialog.hpp
	config-utils.hpp
	output-dialog.hpp
	stream-key-input.hpp
    multistream.hpp
	file-updater.h
  ${KERNEL_HEADERS}
  ${ENGINE_HEADERS})

if(BUILD_OUT_OF_TREE)
	set_target_properties_plugin(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
else()
	set_target_properties_obs(${PROJECT_NAME} PROPERTIES FOLDER "plugins/aitum" PREFIX "")
endif()
