cmake_minimum_required(VERSION 3.10)
project(MemoryPacker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

# --- Source files ---
set(CORE_SOURCES
    Core/GameConfig.h
    Core/GridState.h
    Core/ScannerDB.h
    Core/PlacementState.h
)

set(ALGORITHM_SOURCES
    Algorithms/AlgorithmBase.h
    Algorithms/Greedy.h
    Algorithms/Greedy.cpp
    Algorithms/Backtracking.h
    Algorithms/Backtracking.cpp
    Algorithms/BranchAndBound.h
    Algorithms/BranchAndBound.cpp
    Algorithms/GeneticAlgorithm.h
    Algorithms/GeneticAlgorithm.cpp
)

set(GAME_SOURCES
    Game/Game.h
    Game/Game.cpp
)

set(RENDERER_SOURCES
    Renderer/GridRenderer.h
    Renderer/GridRenderer.cpp
    Renderer/ChartWidget.h
    Renderer/ChartWidget.cpp
    Renderer/UIPanel.h
    Renderer/UIPanel.cpp
    Renderer/Renderer.h
    Renderer/Renderer.cpp
)

add_executable(${PROJECT_NAME}
    main.cpp
    Application.h
    Application.cpp
    ${CORE_SOURCES}
    ${ALGORITHM_SOURCES}
    ${GAME_SOURCES}
    ${RENDERER_SOURCES}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} raylib)

target_precompile_headers(${PROJECT_NAME} PUBLIC gpch.h)

# Source groups for Visual Studio
source_group("Core"       FILES ${CORE_SOURCES})
source_group("Algorithms" FILES ${ALGORITHM_SOURCES})
source_group("Game"       FILES ${GAME_SOURCES})
source_group("Renderer"  FILES ${RENDERER_SOURCES})