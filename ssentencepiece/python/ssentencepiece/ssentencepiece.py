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


class Ssentencepiece:
    def __init__(
        self, vocab: str = "librispeech-500", num_threads: int = os.cpu_count()
    ):
        """
        Construct a Ssentencepiece object.

        Args:
          vocab:
            The path of vocab file (this vocab file must be created by google's
            sentencepiece trainer.
          num_threads:
            The number of worker threads when encode/decode multiple sequences.
            Default `os.cpu_count()`.
        """
        if not os.path.exists(vocab):
            ref = (
                importlib_resources.files("ssentencepiece")
                / f"resources/{vocab}.vocab"
            )
            with importlib_resources.as_file(ref) as path:
                vocab = str(path)
            assert os.path.exists(vocab), f"Cannot find {vocab} file."

        self.processor = _ssentencepiece.ssentencepiece(vocab, num_threads)

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
        return self.processor.decode(ids)

    def vocab_size(self) -> int:
        """
        Return the vocabulary size.`
        """
        return self.processor.vocab_size()

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

        # google's sentencepiece uses NFKC normalization.
        if isinstance(pieces, str):
            pieces = unicodedata.normalize("NFKC", pieces)
        else:
            pieces = [unicodedata.normalize("NFKC", x) for x in pieces]
        return self.processor.piece_to_id(pieces)
