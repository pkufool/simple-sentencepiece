/**
 * Copyright      2024  Wei Kang (wkang@pku.edu.cn)
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

#include "gtest/gtest.h"

#include <iostream>
#include <string>
#include <vector>

#include "ssentencepiece/csrc/ssentencepiece.h"

namespace ssentencepiece {

TEST(Sstencepiece, TestBasic) {
  std::string vocab_path = "ssentencepiece/python/tests/testdata/bpe.vocab";
  Sstencepiece processor(vocab_path);
  std::vector<std::string> pieces;
  processor.Encode("HELLO WORLD  CHINA", &pieces);
  for (auto piece : pieces) {
    std::cout << piece << " ";
  }
  std::cout << std::endl;

  std::vector<int32_t> ids;
  processor.Encode("HELLO WORLD  CHINA", &ids);
  for (auto id : ids) {
    std::cout << id << " ";
  }
  std::cout << std::endl;
}

} // namespace ssentencepiece
