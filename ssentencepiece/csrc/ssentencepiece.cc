/**
 * Copyright      2024    Wei Kang (wkang@pku.edu.cn)
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

#include "ssentencepiece/csrc/ssentencepiece.h"
#include "ssentencepiece/csrc/byte_utils.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <tuple>
#include <utility>

namespace ssentencepiece {

using byte_utils::ByteDecodeSlice;
using byte_utils::CodepointToUtf8;
using byte_utils::GetBcharToByte;
using byte_utils::GetPrintableBchars;
using byte_utils::kPrintableBaseChars;
using byte_utils::Utf8ToCodepoints;

void Ssentencepiece::Build(const std::string &vocab_path) {
  std::ifstream is(vocab_path);
  if (!is) {
    std::cerr << "Open vocab file failed : " << vocab_path.c_str();
    exit(-1);
  }
  Build(is);
}

void Ssentencepiece::Build(std::istream &is) {
  LoadVocab(is);

  std::vector<const char *> keys(tokens_.size());
  std::vector<size_t> length(tokens_.size());
  std::vector<int32_t> values(tokens_.size());

  std::iota(values.begin(), values.end(), 0);

  std::stable_sort(values.begin(), values.end(),
                   [&tokens = tokens_](size_t i1, size_t i2) {
                     return tokens[i1] < tokens[i2];
                   });

  for (int32_t i = 0; i < values.size(); ++i) {
    keys[i] = tokens_[values[i]].c_str();
    length[i] = tokens_[values[i]].size();
  }

  da_.build(keys.size(), keys.data(), length.data(), values.data());
}

void Ssentencepiece::GetDag(const std::string &str, DagType *dag) const {
  dag->resize(str.size());
  for (int32_t i = 0; i < str.size(); ++i) {
    int32_t MAX_HIT = str.size() - i;
    const char *query = str.data() + i;
    std::vector<int32_t> results(MAX_HIT);
    std::size_t num_matches =
        da_.commonPrefixSearch(query, results.data(), MAX_HIT);
    std::vector<DagItem> items;
    for (int32_t j = 0; j < num_matches; ++j) {
      int32_t idx = results[j];
      std::string tmp = tokens_[idx];
      items.push_back(std::make_tuple(scores_[idx], i + tmp.size(), idx));
    }
    (*dag)[i] = items;
  }
}

void Ssentencepiece::CalcDp(const std::string &str, const DagType &dag,
                            std::vector<DagItem> *route) const {
  route->resize(str.size() + 1);
  (*route)[str.size()] = std::make_tuple(0.0, 0, 0);
  for (int32_t i = str.size() - 1; i >= 0; i--) {
    float max_score = -std::numeric_limits<float>::infinity();
    int32_t max_idx = -1;
    int32_t index = 0;
    for (const auto &item : dag[i]) {
      float score =
          std::get<0>(item) + std::get<0>((*route)[std::get<1>(item)]);
      if (score > max_score) {
        max_score = score;
        max_idx = std::get<1>(item);
        index = std::get<2>(item);
      } else if (score == max_score) {
        max_idx = std::get<1>(item);
        index = std::get<2>(item);
      } else {
        continue;
      }
    }
    (*route)[i] = std::make_tuple(
        max_score == -std::numeric_limits<float>::infinity() ? 0 : max_score,
        max_idx, index);
  }
}

void Ssentencepiece::Cut(const std::string &str,
                         const std::vector<DagItem> &route,
                         std::vector<std::string> *ostrs) const {
  ostrs->clear();
  int32_t i = 0;
  int32_t unk_start = -1;
  while (i < str.size()) {
    int32_t next_index = std::get<1>(route[i]);
    if (next_index == -1) {
      if (fallback_bytes_) {
        std::string tmp = tokens_[(unsigned char)(str[i]) + bytes_offset_];
        ostrs->push_back(tmp);
      } else {
        if (unk_start == -1) {
          unk_start = i;
        }
      }
      i += 1;
    } else {
      // keep the original string if it is not in the token table
      // compatible with sentencepiece
      if (unk_start != -1) {
        if (unk_start != i) {
          ostrs->push_back(str.substr(unk_start, i - unk_start));
        }
        unk_start = -1;
      }
      ostrs->push_back(str.substr(i, std::get<1>(route[i]) - i));
      i = next_index;
    }
  }
  // if the last token is unknown, we need to add it to the result
  if (unk_start != -1) {
    if (unk_start != i) {
      ostrs->push_back(str.substr(unk_start, i - unk_start));
    }
  }
}

void Ssentencepiece::Cut(const std::string &str,
                         const std::vector<DagItem> &route,
                         std::vector<int32_t> *oids) const {
  oids->clear();
  int32_t i = 0;
  while (i < str.size()) {
    int32_t next_index = std::get<1>(route[i]);
    if (next_index == -1) {
      if (fallback_bytes_) {
        int32_t tmp = (unsigned char)(str[i]) + bytes_offset_;
        oids->push_back(tmp);
      } else {
        if (oids->empty() || oids->back() != unk_id_) {
          oids->push_back(unk_id_);
        }
      }
      i += 1;
    } else {
      oids->push_back(std::get<2>(route[i]));
      i = next_index;
    }
  }
}

std::string Ssentencepiece::Encode(const std::string &str,
                                   std::vector<DagItem> *route) const {
  std::istringstream iss(is_byte_bpe_ ? ByteEncode(str) : str);
  std::ostringstream oss;
  std::string word;
  while (iss >> word) {
    oss << "▁" << word;
  }
  std::string norm_str = oss.str();
  DagType dag;
  GetDag(norm_str, &dag);
  CalcDp(norm_str, dag, route);
  return norm_str;
}

void Ssentencepiece::Encode(const std::string &str,
                            std::vector<std::string> *ostrs) const {
  std::vector<DagItem> route;
  std::string norm_str = Encode(str, &route);
  Cut(norm_str, route, ostrs);
}

void Ssentencepiece::Encode(const std::string &str,
                            std::vector<int32_t> *oids) const {
  std::vector<DagItem> route;
  std::string norm_str = Encode(str, &route);
  Cut(norm_str, route, oids);
}

void Ssentencepiece::Encode(
    const std::vector<std::string> &strs,
    std::vector<std::vector<std::string>> *ostrs) const {
  ostrs->resize(strs.size());
  std::vector<std::future<void>> results;
  for (int32_t i = 0; i < strs.size(); ++i) {
    results.emplace_back(pool_->enqueue([this, i, &strs, ostrs] {
      return this->Encode(strs[i], &((*ostrs)[i]));
    }));
  }

  for (auto &&result : results) {
    result.get();
  }
}

void Ssentencepiece::Encode(const std::vector<std::string> &strs,
                            std::vector<std::vector<int32_t>> *oids) const {
  oids->resize(strs.size());
  std::vector<std::future<void>> results;
  for (int32_t i = 0; i < strs.size(); ++i) {
    results.emplace_back(pool_->enqueue([this, i, &strs, oids] {
      return this->Encode(strs[i], &((*oids)[i]));
    }));
  }

  for (auto &&result : results) {
    result.get();
  }
}

std::string Ssentencepiece::Decode(const std::vector<int32_t> &ids) const {
  std::ostringstream oss;
  for (auto id : ids) {
    std::string token = tokens_[id];
    // Replace ▁ with a space
    // Unicode 9601, hex 0x2581, utf8 0xe29681
    const uint8_t *p = reinterpret_cast<const uint8_t *>(token.c_str());
    if (p[0] == 0xe2 && p[1] == 0x96 && p[2] == 0x81) {
      token = token.replace(0, 3, " ");
    }
    if (token.substr(0, 3) == "<0x") {
      assert(fallback_bytes_);
      oss << (char)(id - bytes_offset_);
    } else {
      oss << token;
    }
  }
  std::string res = oss.str();
  if (res.size() > 0 && res[0] == ' ') {
    res = res.substr(1); // trim first space
  }
  if (is_byte_bpe_)
    return SmartByteDecode(res);
  return res;
}

std::vector<std::string>
Ssentencepiece::Decode(const std::vector<std::vector<int32_t>> &ids) const {
  std::vector<std::string> res;
  std::vector<std::future<std::string>> results;
  for (const auto &id : ids) {
    results.emplace_back(
        pool_->enqueue([this, &id] { return this->Decode(id); }));
  }
  for (auto &&result : results) {
    res.push_back(result.get());
  }
  return res;
}

void Ssentencepiece::LoadVocab(std::istream &is) {
  tokens_.clear();
  std::string line;
  std::string token;
  float score;
  while (std::getline(is, line)) {
    std::istringstream iss(line);
    if (!(iss >> token >> score)) {
      std::cerr
          << "Each line in vocab should contain two items (seperate by space), "
             "the first one is bpe token, the second one is score, given : "
          << line.c_str();
      exit(-1);
    }
    if (token == "<0x00>") {
      fallback_bytes_ = true;
      bytes_offset_ = tokens_.size();
    }
    if (token == "<unk>") {
      unk_id_ = tokens_.size();
    }
    tokens_.push_back(token);
    scores_.push_back(score);
  }
  is_byte_bpe_ = DetectByteBpe();
}

bool Ssentencepiece::DetectByteBpe() const {
  if (tokens_.empty())
    return false;
  const auto &bchars = GetPrintableBchars();
  for (const auto &token : tokens_) {
    // Skip special tokens enclosed in angle brackets (<unk>, <blk>, <0xXX>…)
    if (!token.empty() && token.front() == '<' && token.back() == '>')
      continue;
    auto cps = Utf8ToCodepoints(token);
    // Strip leading ▁ (U+2581)
    size_t start = 0;
    while (start < cps.size() && cps[start] == 0x2581)
      ++start;
    if (start == cps.size())
      continue; // token is only ▁
    for (size_t k = start; k < cps.size(); ++k) {
      if (bchars.find(cps[k]) == bchars.end())
        return false;
    }
  }
  return true;
}

std::string Ssentencepiece::ByteEncode(const std::string &input) const {
  // Normalize whitespace at codepoint level, then map each UTF-8 byte to its
  // printable base char (identical to byte_encode() in byte_utils.py).
  auto cps = Utf8ToCodepoints(input);
  std::string normalized;
  bool prev_ws = false;
  for (uint32_t cp : cps) {
    bool is_ws = (cp == 0x20 || cp == 0x09 || cp == 0x0A || cp == 0x0B ||
                  cp == 0x0C || cp == 0x0D);
    if (is_ws) {
      if (!prev_ws)
        normalized += ' ';
      prev_ws = true;
    } else {
      normalized += CodepointToUtf8(cp);
      prev_ws = false;
    }
  }
  std::string result;
  result.reserve(normalized.size() * 2);
  for (unsigned char b : normalized)
    result += CodepointToUtf8(kPrintableBaseChars[b]);
  return result;
}

std::string Ssentencepiece::SmartByteDecode(const std::string &input) const {
  // Convert each printable-base-char back to its raw byte, then decode UTF-8.
  // On failure, use DP to recover the maximum number of valid Unicode chars
  // (same semantics as smart_byte_decode() in byte_utils.py).
  auto cps = Utf8ToCodepoints(input);
  int n = (int)cps.size();

  std::string simple = ByteDecodeSlice(cps, 0, n);
  if (!simple.empty() || n == 0)
    return simple;

  // DP: f[i] = max valid chars decoded from cps[0..i), pt[i] = predecessor
  std::vector<int> f(n + 1, 0), pt(n + 1, 0);
  for (int i = 1; i <= n; ++i) {
    f[i] = f[i - 1];
    pt[i] = i - 1;
    for (int j = 1; j <= std::min(4, i); ++j) {
      if (!ByteDecodeSlice(cps, i - j, i).empty() && f[i - j] + 1 > f[i]) {
        f[i] = f[i - j] + 1;
        pt[i] = i - j;
      }
    }
  }
  std::string result;
  int cur = n;
  while (cur > 0) {
    if (f[cur] == f[pt[cur]] + 1)
      result = ByteDecodeSlice(cps, pt[cur], cur) + result;
    cur = pt[cur];
  }
  return result;
}

int32_t Ssentencepiece::PieceToId(const std::string &piece) const {
  auto it = std::find(tokens_.begin(), tokens_.end(), piece);
  if (it == tokens_.end()) {
    return unk_id_;
  }
  return std::distance(tokens_.begin(), it);
}

std::vector<int32_t>
Ssentencepiece::PieceToId(const std::vector<std::string> &pieces) const {
  std::vector<int32_t> res;
  for (const auto &piece : pieces) {
    res.push_back(PieceToId(piece));
  }
  return res;
}

std::string Ssentencepiece::IdToPiece(int32_t id) const {
  if (id == unk_id_) {
    return "<unk>";
  }
  return tokens_[id];
}

std::vector<std::string>
Ssentencepiece::IdToPiece(const std::vector<int32_t> &ids) const {
  std::vector<std::string> res;
  for (const auto &id : ids) {
    res.push_back(IdToPiece(id));
  }
  return res;
}

} // namespace ssentencepiece
