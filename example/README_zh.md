# 将 simple-sentencepiece 集成到 C++ 项目

英文版本见 [README.md](README.md)。

本目录提供一个**最小可运行**的示例，演示如何把 `simple-sentencepiece` 作为
纯 C++ 库集成到你自己的项目里。

相比官方 `google/sentencepiece`，本库最大的差异是：

> **C++ 端不依赖 protobuf，也不依赖任何第三方库。** 只需要 C++14 编译器
> 和 pthread，就可以把分词能力嵌入到已有系统，而不会与系统中的其它
> protobuf 版本（或符号）发生冲突。

因此，**本示例面向的主要读者是 C++ 侧的使用者**：你想把这个库链接到
自己的推理服务 / ASR 引擎 / 离线处理管线里，而不是只用 Python 接口。

---

## 目录结构

```
example/
├── CMakeLists.txt   # 三种集成方式：subdir / fetch / installed
├── demo.cc          # 一个端到端的 demo（单句+批量+解码+查表）
└── README.md        # 本文件
```

## 公共 C++ API（节选）

头文件 `ssentencepiece/csrc/ssentencepiece.h` 提供的唯一类
`ssentencepiece::Ssentencepiece`，核心接口：

```cpp
// 从 .vocab 文件加载
Ssentencepiece(const std::string &vocab_path,
               int32_t num_threads = std::thread::hardware_concurrency());

// 从任意 std::istream 加载（方便内嵌到 protobuf 之外的自家序列化格式里）
Ssentencepiece(std::istream &is, int32_t num_threads = ...);

// 编码：单句 / 批量，输出 pieces 或 ids
void Encode(const std::string &str, std::vector<std::string> *out) const;
void Encode(const std::string &str, std::vector<int32_t>    *out) const;
void Encode(const std::vector<std::string> &in,
            std::vector<std::vector<int32_t>> *out) const;   // 并行

// 解码
std::string              Decode(const std::vector<int32_t> &ids) const;
std::vector<std::string> Decode(const std::vector<std::vector<int32_t>> &ids) const;

// 查表
int32_t                  VocabSize() const;
int32_t                  PieceToId(const std::string &piece) const;
std::string              IdToPiece(int32_t id) const;
```

批量版本内部用一个线程池并行处理，`num_threads` 构造参数控制线程数。

---

## 三种集成方式

`CMakeLists.txt` 通过 `-DSSPIECE_INTEGRATION=<mode>` 在以下三种方式里切换：

### A. add_subdirectory（推荐，submodule / vendored 代码）

最常见的用法：把 simple-sentencepiece 作为 git submodule 或第三方源码
放进你的项目，然后直接 `add_subdirectory`。这也是本示例的默认模式。

在你的 `CMakeLists.txt` 里：

```cmake
# 关掉 Python 绑定（不需要 pybind11），关掉测试
set(SBPE_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
set(SBPE_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)   # 想用静态库的话

add_subdirectory(third_party/simple-sentencepiece)

# 然后直接链接这个 target 即可
target_link_libraries(your_target PRIVATE ssentencepiece_core)
```

构建并运行本示例：

```bash
cd example
cmake -S . -B build -DSSPIECE_INTEGRATION=subdir
cmake --build build -j
./build/ssentencepiece_demo \
    ../ssentencepiece/python/tests/testdata/bpe.vocab \
    "HELLO WORLD" "I LOVE BEIJING"
```

### B. FetchContent（不想维护 submodule）

让 CMake 在配置阶段自动拉取源码：

```cmake
include(FetchContent)
set(SBPE_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  ssentencepiece
  GIT_REPOSITORY https://github.com/pkufool/simple-sentencepiece.git
  GIT_TAG        master)
FetchContent_MakeAvailable(ssentencepiece)

target_link_libraries(your_target PRIVATE ssentencepiece_core)
```

运行：

```bash
cmake -S . -B build -DSSPIECE_INTEGRATION=fetch
cmake --build build -j
```

### C. 使用预编译好的安装产物

先安装一份到某个 prefix：

```bash
# 在 simple-sentencepiece 仓库根目录
cmake -S . -B build \
    -DSBPE_BUILD_PYTHON=OFF \
    -DCMAKE_INSTALL_PREFIX=$HOME/local/ssentencepiece
cmake --build build -j
cmake --install build
```

然后在自己的项目里这样消费：

```bash
cmake -S . -B build \
    -DSSPIECE_INTEGRATION=installed \
    -DSSPIECE_ROOT=$HOME/local/ssentencepiece
cmake --build build -j
```

本示例 `CMakeLists.txt` 里的 `installed` 分支演示了如何用
`add_library(... UNKNOWN IMPORTED)` + `find_library` 来引用这份安装产物，
可以直接复制到你自己的构建脚本里。

---

## 运行 demo 会看到什么

```
Loaded vocab from: ../ssentencepiece/python/tests/testdata/bpe.vocab
Vocab size       : 500

=== Single-sentence Encode ===
input : "HELLO WORLD"
  pieces: ["▁HE", "LL", "O", "▁WORLD"]
  ids   : [22, 58, 24, 425]
  decode: "HELLO WORLD"
...
=== Batched Encode ===
[0] "HELLO WORLD"
  ids   : [22, 58, 24, 425]
...
```

`demo.cc` 从 Step 1 到 Step 6 依次覆盖了：**加载 vocab → 单句 encode →
批量并行 encode → 批量 decode → 特殊 token 查表**，是把这个库接入你
自己系统的最短路径模板。

---

## 集成到现有系统的一些建议

- **线程数**：如果你的上层框架自己有线程池，把 `num_threads` 设成 `1`
  避免嵌套并行；如果是离线批处理，把它设成 CPU 核数即可。
- **vocab 来源**：除了 `.vocab` 文件路径，构造函数也接受 `std::istream&`，
  便于从内存 buffer / zip 资源 / 自定义打包格式里加载。
- **线程安全**：`Encode` / `Decode` / `PieceToId` / `IdToPiece` 都是
  `const` 方法且不修改内部状态，多线程并发读取是安全的。
- **字节 BPE**：如果 vocab 里含有 `<0x..>` 形式的 fallback bytes 或
  byte-BPE 词表，`Encode` 会自动识别并做 byte 转换，调用方无感知。
  参考单元测试 [ssentencepiece_test.cc](../ssentencepiece/csrc/ssentencepiece_test.cc)
  中的 `TestEncodeBbpe` / `TestEncodeBbpe2`。
- **无 protobuf**：这正是你选这个库的原因——生成的
  `libssentencepiece_core.{a,so}` 只依赖 libstdc++ 和 pthread，
  可以用 `ldd` 自行验证。
