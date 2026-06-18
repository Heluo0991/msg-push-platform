# Makefile —— 从零开始版本
# 使用: make          → 编译 mpserver
#       make clean    → 删除编译产物
#       make test     → 编译并运行测试
#       make debug    → 带调试符号编译 (gdb 用)
#
# 语法规则:
#   target: dependencies
#   	recipe (前面必须是 TAB，不是空格!)
#
#   make 找到第一个 target，检查它的 dependencies 是否比 target 新
#   如果 dependency 更新 → 执行 recipe 重建 target
#   如果 target 不存在 → 执行 recipe

# ============================================================
# 变量 —— 大写 = 常量约定，后面 $(VAR) 引用
# ============================================================
CXX      = g++                       # 编译器
CXXFLAGS = -std=c++17 -Wall -Wextra  # 编译选项: C++17 + 全部警告
OPTFLAGS = -O2 -march=native         # 优化级别 (release)
DBGFLAGS = -g -O0                    # 调试选项 (debug)
LDFLAGS  = -lsqlite3 -lpthread       # 链接选项: SQLite + pthread

TARGET   = mpserver                  # 最终可执行文件名

# 源文件列表 —— 只列 .cpp (头文件被 include，不单独编译)
SRCS     = src/main.cpp
# 最终项目: SRCS = $(wildcard src/*.cpp)   ← 自动收集所有 .cpp

# 目标文件列表 —— 把 .cpp 换成 .o
OBJS     = $(SRCS:.cpp=.o)

# ============================================================
# 默认目标: make 不带参数时执行第一个 target
# ============================================================
all: $(TARGET)

# 链接: 所有 .o → 可执行文件
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@ $(LDFLAGS)
#         ^^^^^^^^  ^^^^^^^^  ^^    ^^   ^^^^^^^^
#         编译选项   优化     全部依赖 输出 链接库

# 编译: .cpp → .o (隐式规则)
# $< = 第一个依赖项 (源文件)
# $@ = 当前 target 名 (目标文件)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@
#                               ^^  ^^    ^^
#                               只编译 源文件 输出

# ============================================================
# 调试版本: make debug
# ============================================================
debug: CXXFLAGS += $(DBGFLAGS)
debug: OPTFLAGS =      # 覆盖优化选项
debug: all

# ============================================================
# 测试: make test
# ============================================================
# Day 1: 底层组件
test_queue:
	$(CXX) $(CXXFLAGS) test/test_queue.cpp -o test/test_queue $(LDFLAGS) && ./test/test_queue

test_pool:
	$(CXX) $(CXXFLAGS) test/test_pool.cpp -o test/test_pool $(LDFLAGS) && ./test/test_pool

# Day 2: 协议
test_protocol:
	$(CXX) $(CXXFLAGS) -I src test/test_protocol.cpp -o test/test_protocol && ./test/test_protocol

# Day 4: 数据库
test_db:
	$(CXX) $(CXXFLAGS) -I src test/test_db.cpp -o test/test_db $(LDFLAGS) && ./test/test_db
	@rm -f test.db

# 一键全跑（仅自动化测试，不含 nc 手动集成测试和 valgrind）
test: test_queue test_pool test_protocol test_db
	@echo "=== 全部单元测试通过 ==="

# ============================================================
# 清理: make clean
# ============================================================
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f test/test_queue test/test_pool test/test_protocol test/test_db

# ============================================================
# 声明哪些 target 不是真实文件 (防止和同名文件冲突)
# ============================================================
.PHONY: all debug test clean
