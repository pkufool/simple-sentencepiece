# Copyright      2024 Wei Kang (wkang@pku.edu.cn)
#
# See ../../../LICENSE for clarification regarding multiple authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import _ssentencepiece
import os
import unicodedata
from typing import List, Union

import importlib_resources
from ssentencepiece.byte_utils import byte_encode, smart_byte_decode, PRINTABLE_BASE_CHARS

PRINTABLE_BCHARS = set(chr(c) for c in PRINTABLE_BASE_CHARS)


class Ssentencepiece:
    def __init__(
        self, model: str = "librispeech-500", num_threads: int = os.cpu_count()
    ):
        """
        Construct a Ssentencepiece object.

        Args:
          model:
            The path of vocab file or sentencepiece model file.
          num_threads:
            The number of worker threads when encode/decode multiple sequences.
            Default `os.cpu_count()`.
        """
        self.model = model
        self.num_threads = num_threads
        self.use_sentencepiece = False

        resolved_model = model
        if not os.path.exists(resolved_model):
            ref = (
                importlib_resources.files("ssentencepiece")
                / f"resources/{resolved_model}.vocab"
            )
            with importlib_resources.as_file(ref) as path:
                resolved_model = str(path)
            if not os.path.exists(resolved_model):
                resolved_model = model

        try:
            self.processor = _ssentencepiece.ssentencepiece(
                resolved_model, num_threads
            )
        except Exception:
            import sentencepiece as spm

            self.use_sentencepiece = True
            self.sp = spm.SentencePieceProcessor()
            self.sp.Load(model)

        self.is_byte_bpe = self._detect_byte_bpe()

    def _detect_byte_bpe(self) -> bool:
        try:
            size = self.vocab_size()
            for i in range(size):
                piece = self.id_to_piece(i)
                if piece.startswith("<") and piece.endswith(">"):
                    continue
                # sentencepiece uses ▁ as space marker
                stripped = piece.lstrip("▁")
                if not stripped:
                    continue
                if not all(c in PRINTABLE_BCHARS for c in stripped):
                    return False
            return size > 0
        except Exception:
            return False

    def __getstate__(self):
        return {
            "model": self.model,
            "num_threads": self.num_threads,
            "use_sentencepiece": self.use_sentencepiece,
            "is_byte_bpe": self.is_byte_bpe,
        }

    def __setstate__(self, state):
        self.model = state["model"]
        self.num_threads = state["num_threads"]
        self.use_sentencepiece = state["use_sentencepiece"]
        self.is_byte_bpe = state.get("is_byte_bpe", False)
        if self.use_sentencepiece:
            import sentencepiece as spm

            self.sp = spm.SentencePieceProcessor()
            self.sp.Load(self.model)
        else:
            self.processor = _ssentencepiece.ssentencepiece(
                self.model, self.num_threads
            )

    def encode(
        self, text: Union[str, List[str]], out_type=int
    ) -> Union[List[Union[int, str]], List[List[Union[int, str]]]]:
        """
        Encode the text into tokens or token ids, almost the same as the encode
        interface in sentencepiece.

        Args:
          text:
            The input text, could be a single string or a list of strings.
          out_type:
            Whether to output tokens (out_type=str) or token ids(out_type=int).

        Return:
          If text is a single string and out_type is str, outputs a list of strings.
          If text is a single string and out_type is int, outputs a list of ints.
          If text is a list of strings and out_type is str, outputs a list of list of strings.
          If text is a list of strings and out_type is int, outputs a list of list of ints.
        """
        if self.is_byte_bpe:
            if isinstance(text, str):
                text = byte_encode(text)
            else:
                text = [byte_encode(t) for t in text]

        if self.use_sentencepiece:
            if isinstance(text, str):
                return self.sp.encode(text, out_type=out_type)
            else:
                return [self.sp.encode(t, out_type=out_type) for t in text]

        if out_type is int:
            return self.processor.encode(text, output_id=True)
        else:
            assert (
                out_type is str
            ), f"The out_type could only be int or str, given {out_type}"
            # google's sentencepiece uses NFKC normalization.
            if isinstance(text, str):
                text = unicodedata.normalize("NFKC", text)
            else:
                text = [unicodedata.normalize("NFKC", x) for x in text]
            return self.processor.encode(text, output_id=False)

    def decode(
        self, ids: Union[List[int], List[List[int]]]
    ) -> Union[str, List[str]]:
        """
        Decode the token ids into string, almost the same as the decode interface in sentencepiece.

        Args:
          ids:
            The input ids, could be a list of ints of a list of a list of ints.

        Return:
          If the ids is a list of ints, outputs a single string.
          If the ids is a list of a list of ints, outputs a list of strings.
        """
        if self.use_sentencepiece:
            if isinstance(ids[0], list):
                result = [self.sp.decode(x) for x in ids]
            else:
                result = self.sp.decode(ids)
        else:
            result = self.processor.decode(ids)

        if self.is_byte_bpe:
            if isinstance(result, str):
                return smart_byte_decode(result)
            else:
                return [smart_byte_decode(r) for r in result]
        return result

    def vocab_size(self) -> int:
        """
        Return the vocabulary size.`
        """
        if self.use_sentencepiece:
            return self.sp.get_piece_size()

        return self.processor.vocab_size()

    def get_piece_size(self) -> int:
        return self.vocab_size()

    def id_to_piece(self, ids: Union[int, List[int]]) -> Union[str, List[str]]:
        """
        Convert the token id into piece.

        Args:
          ids:
            The input ids, could be a int of list of ints.

        Return:
          If the ids is a a int, outputs a single string.
          If the ids is a list of ints, outputs a list of strings.
        """
        if self.use_sentencepiece:
            if isinstance(ids, int):
                return self.sp.id_to_piece(ids)
            else:
                return [self.sp.id_to_piece(i) for i in ids]

        return self.processor.id_to_piece(ids)

    def piece_to_id(
        self, pieces: Union[str, List[str]]
    ) -> Union[int, List[int]]:
        """
        Convert the piece into token id.

        Args:
          pieces:
            The input pieces, could be a list of strings of a list of a list of ints.

        Return:
          If the ids is a string, outputs a single int.
          If the ids is a list of string, outputs a list of ints.
        """
        if self.use_sentencepiece:
            if isinstance(pieces, str):
                return self.sp.piece_to_id(pieces)
            else:
                return [self.sp.piece_to_id(p) for p in pieces]

        # google's sentencepiece uses NFKC normalization.
        if isinstance(pieces, str):
            pieces = unicodedata.normalize("NFKC", pieces)
        else:
            pieces = [unicodedata.normalize("NFKC", x) for x in pieces]
        return self.processor.piece_to_id(pieces)
