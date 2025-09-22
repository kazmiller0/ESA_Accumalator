cmake_minimum_required(VERSION 3.10)
project(esa_accumulator VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenSSL
find_package(OpenSSL REQUIRED)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DDEBUG")
endif()

# 包含目录
include_directories(include)
include_directories(${OPENSSL_INCLUDE_DIR})

# 源文件列表
set(ESA_SOURCES
    src/bigint_impl.cpp
    src/group_element_impl.cpp
    src/zk_proof_impl.cpp
    src/polynomial_commitment.cpp
    src/simple_polynomial_accumulator.cpp
)

# 头文件列表
set(ESA_HEADERS
    include/basic_types.h
    include/polynomial_commitment.h
    include/simple_polynomial_accumulator.h
)

# 创建静态库
add_library(esa_lib STATIC ${ESA_SOURCES} ${ESA_HEADERS})

# 设置目标属性
target_include_directories(esa_lib PUBLIC include)
target_link_libraries(esa_lib ${OPENSSL_LIBRARIES})

# 创建示例程序
# add_executable(esa_examples examples/usage_examples.cpp)
# target_link_libraries(esa_examples esa_lib)

# 创建简化多项式示例程序
add_executable(simple_polynomial_examples examples/simple_polynomial_examples.cpp)
target_link_libraries(simple_polynomial_examples esa_lib)

# 安装规则
install(TARGETS esa_lib simple_polynomial_examples
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${ESA_HEADERS} DESTINATION include)