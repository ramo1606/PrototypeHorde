cmake_minimum_required(VERSION 3.15)
project(Prototype-Horde C)

# Set C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Define source and header files
file(GLOB SOURCES
    src/*.c
    #src/core/*.c
    #src/game/*.c
    #src/renderer/*.c
    #src/systems/*.c
    #src/tools/*.c
)

file(GLOB HEADERS
    include/*.h
    include/core/*.h
    include/core/rini/*.h
    include/core/rmem/*.h
    include/core/rres/*.h
    include/core/rres/externals/*.h
    #include/game/*.h
    include/raygui/*.h
    #include/renderer/*.h
    #include/systems/*.h
    #include/tools/*.h
)

# Options for build configuration
option(BUILD_FOR_RG35XX "Build for RG35XX Plus" OFF)
option(USE_EXTERNAL_GLFW "Use external GLFW" OFF)

# Download raylib using FetchContent
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)

# Configure raylib build options based on target
if(BUILD_FOR_RG35XX)
    message(STATUS "Configuring for RG35XX Plus")
    
    # Set up custom SDL2 library paths before raylib configuration
    if(NOT DEFINED RG35XX_LIBS_PATH)
        set(RG35XX_LIBS_PATH "${CMAKE_SOURCE_DIR}/rg35xx-sysroot/usr/lib" CACHE PATH "Path to RG35XX libraries")
    endif()
    
    # Create SDL2 imported target to satisfy raylib's dependency
    add_library(SDL2::SDL2 SHARED IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${RG35XX_LIBS_PATH}/libSDL2-2.0.so.0"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/rg35xx-sysroot/usr/include"
    )
    
    # Disable components not needed for RG35XX
    set(PLATFORM "SDL" CACHE STRING "Force SDL platform" FORCE)
    set(OPENGL_VERSION "ES 3.0" CACHE STRING "Force OpenGL ES 3.0" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "Don't use GLFW on RG35XX" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

    # Graphics options for OpenGL ES 3.0
    set(GRAPHICS "GRAPHICS_API_OPENGL_ES3" CACHE STRING "Use OpenGL ES 3.0" FORCE)

else()
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        message(STATUS "Configuring for Windows (Desktop)")
    else()
        message(STATUS "Configuring for Linux (Desktop)")
    endif()
    
    # Desktop build uses GLFW and OpenGL
    set(PLATFORM "Desktop" CACHE STRING "Desktop platform" FORCE)
    set(OPENGL_VERSION "3.3" CACHE STRING "OpenGL version" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
endif()

# Make raylib available
FetchContent_MakeAvailable(raylib)

# Add include directories
include_directories(include)
include_directories(include/core)
include_directories(include/core/rini)
include_directories(include/core/rmem)
include_directories(include/core/rres)
include_directories(include/core/rres/externals)

# Create the executable with all source files
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

if(BUILD_FOR_RG35XX)
    # RG35XX Plus specific configuration
    
    # Set up EGL and GLESv2 libraries
    set(EGL_LIBRARY "${RG35XX_LIBS_PATH}/libEGL.so" CACHE FILEPATH "EGL library")
    set(GLES3_LIBRARY "${RG35XX_LIBS_PATH}/libGLESv2.so" CACHE FILEPATH "GLESv2 library")
    
    # Check if the library files actually exist
    if(NOT EXISTS "${RG35XX_LIBS_PATH}/libSDL2-2.0.so.0")
        message(FATAL_ERROR "SDL2 library not found at: ${RG35XX_LIBS_PATH}/libSDL2-2.0.so.0")
    endif()
    if(NOT EXISTS "${EGL_LIBRARY}")
        message(FATAL_ERROR "EGL library not found at: ${EGL_LIBRARY}")
    endif()
    if(NOT EXISTS "${GLES3_LIBRARY}")
        message(FATAL_ERROR "GLESv2 library not found at: ${GLES2_LIBRARY}")
    endif()
    
    # Link libraries for RG35XX
    target_link_libraries(${PROJECT_NAME} 
        raylib
        ${EGL_LIBRARY}
        ${GLES3_LIBRARY}
        m
        pthread
        dl
    )
    
    # Add compile definitions for OpenGL ES 3.0
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        GRAPHICS_API_OPENGL_ES3
        PLATFORM_SDL
    )
    
    # Set output directory for RG35XX binary
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/rg35xx
    )
    
else()
    # Desktop configuration (Windows via MSVC/MinGW, or native Linux)
    target_link_libraries(${PROJECT_NAME} raylib)
    
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Windows target (MSVC or MinGW cross-compile)
        if(MINGW)
            # Static link the C runtime so the .exe is self-contained
            target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc)
            target_link_libraries(${PROJECT_NAME} winmm gdi32 opengl32)
        endif()
        
        if(MSVC)
        # Windows-specific flags for Visual Studio
        target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
        set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        
        # Set as startup project in Visual Studio
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
        
        # Create source groups (filters) for Visual Studio
        source_group("Header Files" FILES ${HEADERS})
        source_group("Source Files" FILES ${SOURCES})
        
        # Organize by directories
        foreach(source IN LISTS HEADERS)
            get_filename_component(source_path "${source}" PATH)
            string(REPLACE "/" "\\" source_path_msvc "${source_path}")
            source_group("Header Files\\${source_path_msvc}" FILES "${source}")
        endforeach()
        
        foreach(source IN LISTS SOURCES)
            get_filename_component(source_path "${source}" PATH)
            string(REPLACE "/" "\\" source_path_msvc "${source_path}")
            source_group("Source Files\\${source_path_msvc}" FILES "${source}")
        endforeach()
        endif()
    else()
        # Native Linux build
        target_link_libraries(${PROJECT_NAME} m pthread dl)
    endif()
endif()

# Copy assets if they exist
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    )
endif()