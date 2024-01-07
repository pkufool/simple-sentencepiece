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

#include "ssentencepiece/python/csrc/ssentencepiece.h"
#include "ssentencepiece/csrc/ssentencepiece.h"
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace ssentencepiece {

void PybindSsentencepiece(py::module &m) {
  using PyClass = Ssentencepiece;
  py::class_<PyClass>(m, "Sstencepiece")
      .def(py::init([](const std::string &vocab_path,
                       int32_t num_threads = 10) -> std::unique_ptr<PyClass> {
             return std::make_unique<PyClass>(vocab_path, num_threads);
           }),
           py::arg("vocab_path"), py::arg("num_threads") = 10)
      .def(
          "encode",
          [](PyClass &self, const std::string &str, bool output_id)
              -> std::variant<std::vector<std::string>, std::vector<int32_t>> {
            if (output_id) {
              std::vector<int32_t> oids;
              self.Encode(str, &oids);
              return oids;
            } else {
              std::vector<std::string> ostrs;
              self.Encode(str, &ostrs);
              return ostrs;
            }
          },
          py::call_guard<py::gil_scoped_release>())
}

PYBIND11_MODULE(_ssentencepiece, m) {
  m.doc() = "Python wrapper for simple sentencepiece.";

  PybindSsentencepiece(m);
}

} // namespace ssentencepiece
