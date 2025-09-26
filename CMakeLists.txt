cmake_minimum_required(VERSION 3.16)
project(ExpressiveAccumulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 在 Homebrew 的默认路径中查找库和头文件
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")

# --- 1. 查找依赖 ---
find_package(OpenSSL REQUIRED)
# GMP 和 FLINT 都没有提供标准的 CMake 配置文件，我们手动创建接口
add_library(gmp_interface INTERFACE)
target_include_directories(gmp_interface INTERFACE /opt/homebrew/include)
target_link_libraries(gmp_interface INTERFACE /opt/homebrew/lib/libgmp.a)

add_library(flint_interface INTERFACE)
target_include_directories(flint_interface INTERFACE /opt/homebrew/include)
target_link_libraries(flint_interface INTERFACE /opt/homebrew/lib/libflint.dylib)


# --- 2. 构建 MCL ---
add_subdirectory(third_party/mcl)


# --- 3. 定义可执行文件 ---
# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(comprehensive_test examples/comprehensive_test.cpp src/expressive_accumulator.cpp)
add_executable(performance_test examples/performance_test.cpp src/expressive_accumulator.cpp)


# --- 4. 设置链接 ---
# 添加头文件目录
target_include_directories(comprehensive_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(performance_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 链接库文件
target_link_libraries(comprehensive_test PRIVATE
    mcl
    flint_interface
    gmp_interface
    ${OPENSSL_LIBRARIES}
)
target_link_libraries(performance_test PRIVATE
    mcl
    flint_interface
    gmp_interface
    ${OPENSSL_LIBRARIES}
)