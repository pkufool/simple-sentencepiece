/**
 * Copyright      2026    Wei Kang (wkang@pku.edu.cn)
 *
 * See LICENSE for clarification regarding multiple authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SSENTENCEPIECE_CSRC_BYTE_UTILS_H_
#define SSENTENCEPIECE_CSRC_BYTE_UTILS_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ssentencepiece {
namespace byte_utils {

// Identical to PRINTABLE_BASE_CHARS in byte_utils.py.
// Maps each byte value (0-255) to a printable Unicode codepoint.
// clang-format off
static const uint32_t kPrintableBaseChars[256] = {
    256, 257, 258, 259, 260, 261, 262, 263,
    264, 265, 266, 267, 268, 269, 270, 271,
    272, 273, 274, 275, 276, 277, 278, 279,
    280, 281, 282, 283, 284, 285, 286, 287, // bytes 0-31
    32,  33,  34,  35,  36,  37,  38,  39,
    40,  41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  55,
    56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  67,  68,  69,  70,  71,
    72,  73,  74,  75,  76,  77,  78,  79,
    80,  81,  82,  83,  84,  85,  86,  87,
    88,  89,  90,  91,  92,  93,  94,  95,
    96,  97,  98,  99,  100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, // bytes 32-126
    288, 289, 290, 291, 292, 293, 294, 295,
    296, 297, 298, 299, 300, 301, 302, 303,
    304, 305, 308, 309, 310, 311, 312, 313,
    314, 315, 316, 317, 318, 321, 322, 323,
    324, 325, 326, 327, 328, 330, 331, 332,
    333, 334, 335, 336, 337, 338, 339, 340,
    341, 342, 343, 344, 345, 346, 347, 348,
    349, 350, 351, 352, 353, 354, 355, 356,
    357, 358, 359, 360, 361, 362, 363, 364,
    365, 366, 367, 368, 369, 370, 371, 372,
    373, 374, 375, 376, 377, 378, 379, 380,
    381, 382, 384, 385, 386, 387, 388, 389,
    390, 391, 392, 393, 394, 395, 396, 397,
    398, 399, 400, 401, 402, 403, 404, 405,
    406, 407, 408, 409, 410, 411, 412, 413,
    414, 415, 416, 417, 418, 419, 420, 421,
    422, // bytes 127-255
};
// clang-format on

inline std::string CodepointToUtf8(uint32_t cp) {
  std::string s;
  if (cp < 0x80) {
    s += (char)cp;
  } else if (cp < 0x800) {
    s += (char)(0xC0 | (cp >> 6));
    s += (char)(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    s += (char)(0xE0 | (cp >> 12));
    s += (char)(0x80 | ((cp >> 6) & 0x3F));
    s += (char)(0x80 | (cp & 0x3F));
  } else {
    s += (char)(0xF0 | (cp >> 18));
    s += (char)(0x80 | ((cp >> 12) & 0x3F));
    s += (char)(0x80 | ((cp >> 6) & 0x3F));
    s += (char)(0x80 | (cp & 0x3F));
  }
  return s;
}

inline std::vector<uint32_t> Utf8ToCodepoints(const std::string &s) {
  std::vector<uint32_t> result;
  size_t i = 0;
  while (i < s.size()) {
    uint8_t c = (uint8_t)s[i];
    uint32_t cp;
    if (c < 0x80) {
      cp = c;
      i += 1;
    } else if (c < 0xE0) {
      cp = (uint32_t)(c & 0x1F) << 6 | ((uint8_t)s[i + 1] & 0x3F);
      i += 2;
    } else if (c < 0xF0) {
      cp = (uint32_t)(c & 0x0F) << 12 |
           (uint32_t)((uint8_t)s[i + 1] & 0x3F) << 6 |
           ((uint8_t)s[i + 2] & 0x3F);
      i += 3;
    } else {
      cp = (uint32_t)(c & 0x07) << 18 |
           (uint32_t)((uint8_t)s[i + 1] & 0x3F) << 12 |
           (uint32_t)((uint8_t)s[i + 2] & 0x3F) << 6 |
           ((uint8_t)s[i + 3] & 0x3F);
      i += 4;
    }
    result.push_back(cp);
  }
  return result;
}

// Identical to Python's BCHAR_TO_BYTE (including BPE_UNK -> space).
inline const std::unordered_map<uint32_t, uint8_t> &GetBcharToByte() {
  static const std::unordered_map<uint32_t, uint8_t> m = [] {
    std::unordered_map<uint32_t, uint8_t> tmp;
    for (int b = 0; b < 256; ++b)
      tmp[kPrintableBaseChars[b]] = (uint8_t)b;
    tmp[0x2047] = 32; // BPE_UNK (U+2047) -> space
    return tmp;
  }();
  return m;
}

inline const std::unordered_set<uint32_t> &GetPrintableBchars() {
  static const std::unordered_set<uint32_t> s = [] {
    std::unordered_set<uint32_t> tmp;
    for (int b = 0; b < 256; ++b)
      tmp.insert(kPrintableBaseChars[b]);
    return tmp;
  }();
  return s;
}

// Try to interpret codepoints[start, end) as raw bytes and decode as UTF-8.
// Returns the decoded string, or "" if not valid UTF-8.
inline std::string ByteDecodeSlice(const std::vector<uint32_t> &cps, int start,
                                   int end) {
  const auto &m = GetBcharToByte();
  std::vector<uint8_t> bytes;
  bytes.reserve(end - start);
  for (int i = start; i < end; ++i) {
    auto it = m.find(cps[i]);
    if (it == m.end())
      return "";
    bytes.push_back(it->second);
  }
  size_t j = 0;
  while (j < bytes.size()) {
    uint8_t b = bytes[j];
    size_t len;
    if (b < 0x80)
      len = 1;
    else if ((b & 0xE0) == 0xC0)
      len = 2;
    else if ((b & 0xF0) == 0xE0)
      len = 3;
    else if ((b & 0xF8) == 0xF0)
      len = 4;
    else
      return "";
    if (j + len > bytes.size())
      return "";
    for (size_t k = 1; k < len; ++k)
      if ((bytes[j + k] & 0xC0) != 0x80)
        return "";
    j += len;
  }
  return std::string(bytes.begin(), bytes.end());
}

} // namespace byte_utils
} // namespace ssentencepiece

#endif // SSENTENCEPIECE_CSRC_BYTE_UTILS_H_
