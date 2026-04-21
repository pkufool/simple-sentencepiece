# Integrating simple-sentencepiece into a C++ Project

This directory provides a **minimal, runnable** example showing how to embed
`simple-sentencepiece` as a pure-C++ library in your own project.

The key difference from the official `google/sentencepiece` library is:

> **The C++ core has no protobuf dependency and no other third-party runtime
> requirements.** A C++14 compiler and pthread are all you need, so the
> library links cleanly into any existing system without conflicting with
> whatever version of protobuf (or protobuf symbols) is already present.

This example is therefore primarily aimed at **C++ consumers**: people who
want to link simple-sentencepiece into an inference server, ASR engine, or
offline processing pipeline — not just call it from Python.

For the Chinese version of this guide, see [README_zh.md](README_zh.md).

---

## Directory layout

```
example/
├── CMakeLists.txt   # Three integration strategies: subdir / fetch / installed
├── demo.cc          # End-to-end demo (single + batch encode/decode + lookup)
├── README.md        # This file (English)
└── README_zh.md     # Chinese version
```

## Public C++ API (summary)

The single class `ssentencepiece::Ssentencepiece` is declared in
`ssentencepiece/csrc/ssentencepiece.h`:

```cpp
// Load from a .vocab file path
Ssentencepiece(const std::string &vocab_path,
               int32_t num_threads = std::thread::hardware_concurrency());

// Load from any std::istream (handy when vocab is embedded in your own format)
Ssentencepiece(std::istream &is, int32_t num_threads = ...);

// Encode: single or batch, output pieces or integer ids
void Encode(const std::string &str, std::vector<std::string> *out) const;
void Encode(const std::string &str, std::vector<int32_t>    *out) const;
void Encode(const std::vector<std::string> &in,
            std::vector<std::vector<int32_t>> *out) const;   // parallel

// Decode
std::string              Decode(const std::vector<int32_t> &ids) const;
std::vector<std::string> Decode(const std::vector<std::vector<int32_t>> &ids) const;

// Lookup
int32_t     VocabSize() const;
int32_t     PieceToId(const std::string &piece) const;
std::string IdToPiece(int32_t id) const;
```

Batch variants use an internal thread pool; the `num_threads` constructor
argument controls its size.

---

## Three integration strategies

`CMakeLists.txt` selects the integration mode via
`-DSSPIECE_INTEGRATION=<mode>` (default: `subdir`).

### A. add_subdirectory (recommended — submodule / vendored source)

The most common approach: place simple-sentencepiece inside your project as a
git submodule or copied source tree and call `add_subdirectory`.

In your own `CMakeLists.txt`:

```cmake
# Disable Python bindings (no pybind11 needed) and tests
set(SBPE_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
set(SBPE_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)   # optional: static lib

add_subdirectory(third_party/simple-sentencepiece)

target_link_libraries(your_target PRIVATE ssentencepiece_core)
```

Build and run this example:

```bash
cd example
cmake -S . -B build -DSSPIECE_INTEGRATION=subdir
cmake --build build -j
./build/ssentencepiece_demo \
    ../ssentencepiece/python/tests/testdata/bpe.vocab \
    "HELLO WORLD" "I LOVE BEIJING"
```

### B. FetchContent (no submodule required)

Let CMake pull the source automatically at configure time:

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

```bash
cmake -S . -B build -DSSPIECE_INTEGRATION=fetch
cmake --build build -j
```

### C. Pre-installed copy

First install a copy to a prefix:

```bash
# Run from the simple-sentencepiece repo root
cmake -S . -B build \
    -DSBPE_BUILD_PYTHON=OFF \
    -DCMAKE_INSTALL_PREFIX=$HOME/local/ssentencepiece
cmake --build build -j
cmake --install build
```

Then consume it from your project:

```bash
cmake -S . -B build \
    -DSSPIECE_INTEGRATION=installed \
    -DSSPIECE_ROOT=$HOME/local/ssentencepiece
cmake --build build -j
```

The `installed` branch in `CMakeLists.txt` shows the
`add_library(... UNKNOWN IMPORTED)` + `find_library` pattern you can copy
straight into your own build scripts.

---

## Expected output

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

`demo.cc` walks through Steps 1–6: **load vocab → single encode → batched
parallel encode → batched decode → special-token lookup**. It is intended as
the shortest path template for wiring the library into your own system.

---

## Integration tips

- **Thread count**: if your framework has its own thread pool, set
  `num_threads=1` to avoid nested parallelism; for offline batch jobs, use
  the hardware concurrency.
- **Vocab source**: the constructor also accepts `std::istream&`, so you can
  load from an in-memory buffer, a zip entry, or any custom serialization
  format.
- **Thread safety**: `Encode`, `Decode`, `PieceToId`, and `IdToPiece` are all
  `const` and do not modify shared state — concurrent reads from multiple
  threads are safe.
- **Byte BPE**: if the vocab contains `<0x..>` fallback bytes or a byte-BPE
  vocabulary, `Encode` detects and handles the byte conversion automatically —
  no extra code required. See
  [ssentencepiece_test.cc](../ssentencepiece/csrc/ssentencepiece_test.cc)
  for `TestEncodeBbpe` / `TestEncodeBbpe2`.
- **Zero runtime deps**: `libssentencepiece_core.{a,so}` depends only on
  libstdc++ and pthread. Verify with `ldd` after building.
