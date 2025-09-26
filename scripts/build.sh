#!/bin/bash

# Expressive Accumulator 构建脚本
# 将编译文件输出到 bin/ 目录

echo "=== Expressive Accumulator Build Script ==="

# 检查依赖
echo "检查依赖..."
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ not found, please install Xcode Command Line Tools"
    exit 1
fi

# 创建输出目录
echo "创建输出目录..."
mkdir -p bin

# 编译标志
CXX_FLAGS="-std=c++17 -O3 -Wall -Wextra"
INCLUDE_FLAGS="-I/opt/homebrew/include -I./include -I./third_party/mcl/include"
LIB_FLAGS="-L/opt/homebrew/lib -L./third_party/mcl/lib -lgmp -lflint -lssl -lcrypto ./third_party/mcl/lib/libmcl.a"

# 编译综合测试
echo "编译综合功能测试..."
g++ $CXX_FLAGS $INCLUDE_FLAGS $LIB_FLAGS -o bin/comprehensive_test examples/comprehensive_test.cpp src/expressive_accumulator.cpp

if [ $? -eq 0 ]; then
    echo "✅ 综合功能测试编译成功"
else
    echo "❌ 综合功能测试编译失败"
    exit 1
fi

# 编译性能测试
echo "编译性能基准测试..."
g++ $CXX_FLAGS $INCLUDE_FLAGS $LIB_FLAGS -o bin/performance_test examples/performance_test.cpp src/expressive_accumulator.cpp

if [ $? -eq 0 ]; then
    echo "✅ 性能基准测试编译成功"
else
    echo "❌ 性能基准测试编译失败"
    exit 1
fi

echo ""
echo "🎉 构建完成！"
echo ""
echo "可执行文件位置："
echo "  bin/comprehensive_test    # 综合功能测试"
echo "  bin/performance_test      # 性能基准测试"
echo ""
echo "运行测试："
echo "  ./bin/comprehensive_test"
echo "  ./bin/performance_test"
echo ""
echo "或使用测试脚本："
echo "  ./scripts/test.sh"
