
cmake_minimum_required(VERSION 3.13)
project(Mineshaft)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -Wall")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

# include project directories
include_directories("include")

# find tblgen
find_package(TblGen REQUIRED CONFIG)
include_directories("${TBLGEN_INCLUDE_DIR}")
message(STATUS "Found TblGen ${TBLGEN_MAIN_INCLUDE_DIR}")

# find LLVM
find_package(LLVM 10 REQUIRED CONFIG)

# llvm libraries
llvm_map_components_to_libnames(llvm_libs support)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found LLVM libs ${llvm_libs}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
set(GLEW_LIBRARIES "/usr/local/lib/libGLEW.dylib")

find_package(assimp REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})

find_package(SFML REQUIRED graphics network audio window system)
include_directories(${SFML_INCLUDE_DIRS})

include_directories(/usr/local/include/nlohmann)
include_directories(/usr/local/include/stb/include)

set(SOURCE_FILES main.cpp
        src/Texture/BasicTexture.cpp include/mineshaft/Texture/BasicTexture.h
        src/Texture/TextureAtlas.cpp include/mineshaft/Texture/TextureAtlas.h
        src/Model/Model.cpp include/mineshaft/Model/Model.h
        src/Camera.cpp include/mineshaft/Camera.h
        src/Shader/Shader.cpp include/mineshaft/Shader/Shader.h
        src/Application.cpp include/mineshaft/Application.h
        src/utils.cpp include/mineshaft/utils.h
        src/Light/Light.cpp include/mineshaft/Light/Light.h
        src/World/Block.cpp include/mineshaft/World/Block.h
        include/mineshaft/Config.h
        include/mineshaft/World/Chunk.h src/World/Chunk.cpp
        include/mineshaft/World/World.h src/World/World.cpp include/mineshaft/Texture/TextureArray.h src/Texture/TextureArray.cpp include/mineshaft/Event/Event.h src/Event/Event.cpp include/mineshaft/Event/EventDispatcher.h src/Event/EventDispatcher.cpp include/mineshaft/Entity/Entity.h include/mineshaft/Entity/Player.h src/Entity/Entity.cpp src/Entity/Player.cpp include/mineshaft/Support/TextRenderer.h src/Support/TextRenderer.cpp include/mineshaft/Support/Noise/SimplexNoise.h src/Support/Noise/SimplexNoise.cpp include/mineshaft/World/WorldGenerator.h src/World/WorldGenerator.cpp include/mineshaft/GameSave.h src/GameSave.cpp include/mineshaft/Support/Worker.h)

add_executable(mineshaft ${SOURCE_FILES})
add_executable(mineshaft-asan ${SOURCE_FILES})

target_compile_options(mineshaft-asan
        PUBLIC "-fsanitize=address"
        PUBLIC "-fno-omit-frame-pointer"
        PUBLIC "-fvisibility=hidden")

add_library(mineshaft-tblgens SHARED
        src/Support/TblGenBackends/BlockBackend.cpp src/Support/TblGenBackends/BiomeBackend.cpp)

set(ASSIMP_LIBRARY "assimp")
add_library(${ASSIMP_LIBRARY} SHARED IMPORTED)
set_target_properties(${ASSIMP_LIBRARY} PROPERTIES IMPORTED_LOCATION
        "${ASSIMP_LIBRARY_DIRS}/libassimp.dylib")

target_link_libraries(mineshaft ${OPENGL_LIBRARIES} ${GLM_LIBRARIES}
        glfw ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${SFML_LIBRARIES}
        ${ASSIMP_LIBRARIES} ${llvm_libs})

target_link_libraries(mineshaft-asan ${OPENGL_LIBRARIES} ${GLM_LIBRARIES}
        glfw ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${SFML_LIBRARIES}
        ${ASSIMP_LIBRARIES} ${llvm_libs} -fsanitize=address
        -fno-omit-frame-pointer)

target_link_libraries(mineshaft-tblgens ${llvm_libs} ${SFML_LIBRARIES})