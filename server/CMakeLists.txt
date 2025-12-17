cmake_minimum_required(VERSION 3.16)

project(WinSockServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================== SOURCE FILES ==================
set(SOURCES
    src/server.cpp
    src/config.cpp
    src/apps.cpp
    src/http_utils.cpp
    src/keylogger.cpp
    src/processes.cpp
    src/router.cpp
)

# ================== EXECUTABLE ==================
add_executable(server ${SOURCES})

# ================== INCLUDE PATH ==================
target_include_directories(server PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# ================== LINK LIBS ==================
target_link_libraries(server
    ws2_32   # WinSock
    shell32  # ShellExecute
)

# ================== WARNINGS ==================
if(MSVC)
    target_compile_options(server PRIVATE /W4)
else()
    target_compile_options(server PRIVATE -Wall -Wextra)
endif()

# ================== OUTPUT NAME ==================
set_target_properties(server PROPERTIES OUTPUT_NAME "server")