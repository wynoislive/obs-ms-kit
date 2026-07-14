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

# ----------------------------------------------------------------------------
# Cross-Platform Qt6 and libobs Discovery Hooks (Local Developer Mode Only)
# ----------------------------------------------------------------------------
if(NOT DEFINED ENV{GITHUB_ACTIONS})
    if(APPLE)
        # Target Homebrew default installation prefixes for Apple Silicon & Intel macOS
        list(APPEND CMAKE_PREFIX_PATH 
            "/opt/homebrew/opt/qt"
            "/usr/local/opt/qt"
            "/opt/homebrew"
        )
        set(CMAKE_MACOSX_RPATH ON)
        
    elseif(UNIX AND NOT APPLE)
        # Force system paths for standard Linux distros
        list(APPEND CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake")
    endif()
endif()


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
  runtime/kernel/ConfigManager.hpp
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
  runtime/engine/protocol/SrtClient.hpp
  runtime/engine/protocol/WhipClient.hpp
  runtime/engine/analytics/PerformanceRuleEngine.hpp
  runtime/ui/CreatorHubWidget.hpp
  runtime/ui/AddDestinationDialog.hpp
  runtime/test/MockOutputSession.hpp
  runtime/test/PerformanceRuleTestHarness.hpp
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
  runtime/engine/protocol/SrtClient.cpp
  runtime/engine/protocol/WhipClient.cpp
  runtime/kernel/Scheduler.cpp
  runtime/kernel/KernelContext.cpp
  runtime/kernel/EventDispatcher.cpp
  runtime/kernel/ConfigManager.cpp
  runtime/engine/analytics/PerformanceRuleEngine.cpp
  runtime/ui/CreatorHubWidget.cpp
  runtime/ui/AddDestinationDialog.cpp
  runtime/test/PerformanceRuleTestHarness.cpp
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

# ============================================================================
# Milestone 10: Installation & Distribution Packaging
# ============================================================================

# 1. Establish strict, canonical install paths
set(MSK_PLUGIN_NAME "obs-ms-kit")

if(WIN32)
    set(MSK_INSTALL_BIN "obs-plugins/64bit")
    set(MSK_INSTALL_DATA "data/obs-plugins/${MSK_PLUGIN_NAME}")
else()
    # Support standard POSIX directory layouts for future cross-platform builds
    set(MSK_INSTALL_BIN "obs-plugins")
    set(MSK_INSTALL_DATA "data/obs-plugins/${MSK_PLUGIN_NAME}")
endif()

message(STATUS "[MS-KIT] Configuring install target distributions...")
message(STATUS "        Binary Destination: ${MSK_INSTALL_BIN}")
message(STATUS "        Resource Destination: ${MSK_INSTALL_DATA}")

# 2. Package the primary binary target
install(TARGETS aitum-multistream
    LIBRARY DESTINATION ${MSK_INSTALL_BIN}
    RUNTIME DESTINATION ${MSK_INSTALL_BIN}
    ARCHIVE DESTINATION ${MSK_INSTALL_BIN}
)

# 3. Package UI Assets (Icons, SVGs, QSS Stylesheets)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    install(DIRECTORY assets/
        DESTINATION ${MSK_INSTALL_DATA}/assets
        FILES_MATCHING 
        PATTERN "*.png" 
        PATTERN "*.svg" 
        PATTERN "*.qss"
    )
    message(STATUS "        Registered UI Asset directory for distribution.")
endif()

# 4. Package UI Locale Translations
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/locale")
    install(DIRECTORY locale/
        DESTINATION ${MSK_INSTALL_DATA}/locale
        FILES_MATCHING 
        PATTERN "*.ini" 
        PATTERN "*.json"
    )
    message(STATUS "        Registered Localization dictionary for distribution.")
endif()

# 5. Package Immutable Configuration Schemas
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/profile-schema.json")
    install(FILES config/profile-schema.json
        DESTINATION ${MSK_INSTALL_DATA}/config
    )
    message(STATUS "        Registered Configuration Schema for distribution.")
endif()

# ============================================================================
# Rapid Development Deployment Target
# ============================================================================

# Expose a cached variable to easily point to your local test OBS Studio folder
set(OBS_DIR "" CACHE PATH "Path to your local testing OBS Studio installation folder")

if(OBS_DIR)
    add_custom_target(dev-install
        # 1. Hot-deploy the updated compiled binary
        COMMAND ${CMAKE_COMMAND} -E copy 
            $<TARGET_FILE:aitum-multistream> 
            "${OBS_DIR}/${MSK_INSTALL_BIN}/$<TARGET_FILE_NAME:aitum-multistream>"
            
        # 2. Sync active resource changes
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "${CMAKE_CURRENT_SOURCE_DIR}/assets" 
            "${OBS_DIR}/${MSK_INSTALL_DATA}/assets"
            
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "${CMAKE_CURRENT_SOURCE_DIR}/locale" 
            "${OBS_DIR}/${MSK_INSTALL_DATA}/locale"
            
        COMMENT "Hot-deploying MS-Kit binaries and UI assets to test environment: ${OBS_DIR}"
        DEPENDS aitum-multistream
    )
endif()

# ============================================================================
# Milestone 13: CPack Deployment Packaging (macOS DMG & Linux DEB)
# ============================================================================

# 1. Base CPack Metadata (shared across all distribution targets)
set(CPACK_PACKAGE_NAME "obs-ms-kit")
set(CPACK_PACKAGE_VENDOR "Wyno")
set(CPACK_PACKAGE_CONTACT "https://github.com/wynoislive/obs-ms-kit")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Automated multi-destination streaming plugin for OBS Studio")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# 2. Platform-Specific Installer Definitions
if(APPLE)
    # DragNDrop generator compiles a clean, mountable DMG disk image
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "obs-ms-kit Installer")
    set(CPACK_DMG_FORMAT "UDZO") # Compressed read-only format
    
    # Standard macOS target directory path for OBS Studio global plugins
    set(CPACK_PACKAGING_INSTALL_PREFIX "/Library/Application Support/obs-studio/plugins")

elseif(UNIX AND NOT APPLE)
    # DEB generator packages binaries, assets, and dependencies into a Debian package
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Wyno <wynoislive@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "video")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    
    # Ensure the package manager checks for an existing OBS installation on the client system
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "obs-studio (>= 30.0)")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()

# 3. Include the CPack engine module to hook existing install() commands
include(CPack)


