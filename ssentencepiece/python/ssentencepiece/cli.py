# Copyright      2025 Wei Kang (wkang@pku.edu.cn)
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


import click
import logging
import sys

from pathlib import Path
from ssentencepiece import Ssentencepiece

try:
    import sentencepiece as spm
except ImportError:
    print("Please run")
    print("  pip install sentencepiece")
    print("before you continue")
    raise


@click.group()
def cli():
    """
    The shell entry point to ssentencepiece.
    """
    logging.basicConfig(
        format="%(asctime)s %(levelname)s [%(filename)s:%(lineno)d] %(message)s",
        level=logging.INFO,
    )


@cli.command(name="export")
@click.option(
    "--bpe-model", type=str, required=True, help="The path to the bpe model."
)
def export(bpe_model):
    """
    Export the vocabulary (needed by simple-sentencepiece) from a BPE model (trained with google's sentencepiece).
    The vocabulary is written to a file with the same name as the model but with a .vocab extension.
    """
    model_file = bpe_model
    vocab_file = model_file.replace(".model", ".vocab")

    sp = spm.SentencePieceProcessor()
    sp.Load(model_file)
    vocabs = [sp.IdToPiece(id) for id in range(sp.GetPieceSize())]
    with open(vocab_file, "w") as vfile:
        for v in vocabs:
            id = sp.PieceToId(v)
            vfile.write(f"{v}\t{sp.GetScore(id)}\n")
    logging.info(f"Vocabulary file is written to {vocab_file}")


@cli.command(name="train")
@click.option(
    "--output-dir",
    type=str,
    required=True,
    help="The directory to store the trained model",
)
@click.option(
    "--texts",
    type=str,
    required=True,
    help="The path to the texts that will be used to train the model.",
)
@click.option(
    "--vocab-size",
    type=int,
    default=5000,
    help="The size of the vocabulary.",
)
@click.option(
    "--character-coverage",
    type=float,
    default=1.0,
    help="The character coverage for the model.",
)
@click.option(
    "--model-type",
    type=click.Choice(["unigram", "char", "word"]),
    default="unigram",
    help="The type of the model.",
)
@click.option(
    "--input-sentence-size",
    type=int,
    default=100000000,
    help="The number of sentences to use for training.",
)
def train_bpe_model(
    output_dir,
    texts,
    vocab_size,
    character_coverage,
    model_type,
    input_sentence_size,
):
    """
    Use the sentencepiece library to train a BPE model.
    The first 5 tokens are fixed to <blk>, <sos>, <eos>, <pad> and <unk>.
    """
    model_prefix = f"{output_dir}/{model_type}_{vocab_size}"

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    user_defined_symbols = ["<blk>", "<sos>", "<eos>", "<pad>"]
    unk_id = len(user_defined_symbols)
    # Note: unk_id is fixed to 4.
    # If you change it, you should also change other
    # places that are using it.

    model_file = Path(model_prefix + ".vocab")
    if not model_file.is_file():
        spm.SentencePieceTrainer.train(
            input=texts,
            vocab_size=vocab_size,
            model_type=model_type,
            model_prefix=model_prefix,
            input_sentence_size=input_sentence_size,
            character_coverage=character_coverage,
            user_defined_symbols=user_defined_symbols,
            unk_id=unk_id,
            bos_id=-1,
            eos_id=-1,
        )
    else:
        print(f"{model_file} exists - skipping")
        return


@cli.command(name="encode")
@click.option(
    "--bpe-model",
    type=str,
    help="""
    The path to the bpe model, if provided will use standard sentencepiece
    toolkit (from google) to encode the text.
    """,
)
@click.option(
    "--bpe-vocab",
    type=str,
    help="""The path to the bpe model, if provided will use simple-sentencepiece
    (current one) to encode the text. If provided it will override the bpe-model.
    """,
)
@click.option(
    "--texts",
    type=str,
    help="The path to the texts to encode. If not provided, will read from stdin.",
)
@click.option(
    "--output-format",
    type=click.Choice(["piece", "id"]),
    default="piece",
    help="The output format, either piece or id.",
)
@click.option(
    "--output",
    type=str,
    help="The path to the output file. If not provided, will write to stdout.",
)
def encode_texts(bpe_model, bpe_vocab, texts, output_format, output):
    """
    Encode the texts to tokens or token ids.
    """
    if bpe_vocab:
        sp = Ssentencepiece(bpe_vocab)
    else:
        assert bpe_model, "Either bpe-model or bpe-vocab should be provided"
        sp = spm.SentencePieceProcessor()
        sp.Load(bpe_model)
    if output:
        output = open(output, "w")
    else:
        output = sys.stdout

    out_type = int if output_format == "id" else str

    if texts:
        with open(texts, "r") as f:
            for sen in f:
                sen = sen.strip()
                r = sp.encode(sen, out_type=out_type)
                output.write(" ".join([str(rr) for rr in r]) + "\n")
    else:
        for sen in sys.stdin:
            sen = sen.strip()
            r = sp.encode(sen, out_type=out_type)
            output.write(" ".join([str(rr) for rr in r]) + "\n")


@cli.command(name="decode")
@click.option(
    "--bpe-model",
    type=str,
    help="""
    The path to the bpe model, if provided will use standard sentencepiece
    toolkit (from google) to encode the text.
    """,
)
@click.option(
    "--bpe-vocab",
    type=str,
    help="""The path to the bpe model, if provided will use simple-sentencepiece
    (current one) to encode the text. If provided it will override the bpe-model.
    """,
)
@click.option(
    "--input",
    type=str,
    help="The path to the pieces/ids to decode. If not provided, will read from stdin.",
)
@click.option(
    "--input-format",
    type=click.Choice(["piece", "id"]),
    default="piece",
    help="The input format, either piece or id.",
)
@click.option(
    "--output",
    type=str,
    help="The path to the output file. If not provided, will write to stdout.",
)
def decode_pieces(bpe_model, bpe_vocab, input, input_format, output):
    """
    Decode the tokens or token ids to texts.
    tokens and token ids are separated by space.
    """
    if bpe_vocab:
        sp = Ssentencepiece(bpe_vocab)
    else:
        assert bpe_model, "Either bpe-model or bpe-vocab should be provided"
        sp = spm.SentencePieceProcessor()
        sp.Load(bpe_model)
    if output:
        output = open(output, "w")
    else:
        output = sys.stdout

    if input:
        with open(input, "r") as f:
            for sen in f:
                sen = sen.strip().split()
                if input_format == "piece":
                    r = "".join(sen).replace("▁", " ").strip()
                else:
                    r = sp.decode([in_type(s) for s in sen])
                output.write(r + "\n")
    else:
        for sen in sys.stdin:
            sen = sen.strip().split()
            if input_format == "piece":
                r = "".join(sen).replace("▁", " ").strip()
            else:
                r = sp.decode([in_type(s) for s in sen])
            output.write(r + "\n")


@cli.command(name="combine")
@click.argument(
    "vocabs",
    nargs=-1,
    type=click.Path(exists=True, dir_okay=False, allow_dash=True),
)
@click.argument("output_vocab", type=click.Path(allow_dash=True))
def combine_models(vocabs, output_vocab):
    """
    Combine multiple bpe models into one.

    The same token in different vocabularies will be merged into one token (maximum score is selected).
    """
    special_tokens = {}
    byte_tokens = {}
    normal_tokens = {}
    for vocab in vocabs:
        with open(vocab, "r") as f:
            for index, line in enumerate(f):
                token, score = line.strip().split("\t")
                score = float(score)
                if token.startswith("<0x") and token.endswith(">"):
                    byte_tokens[token] = score
                elif token.startswith("<") and token.endswith(">"):
                    if token not in special_tokens:
                        special_tokens[token] = index
                    else:
                        special_tokens[token] = min(
                            special_tokens[token], index
                        )
                else:
                    if token not in normal_tokens:
                        normal_tokens[token] = score
                    else:
                        normal_tokens[token] = max(normal_tokens[token], score)

    with open(output_vocab, "w") as f:
        special_tokens = sorted(special_tokens.items(), key=lambda x: x[1])
        for token, _ in special_tokens:
            f.write(f"{token}\t{0}\n")
        byte_tokens = sorted(byte_tokens.items(), key=lambda x: x[0])
        for token, _ in byte_tokens:
            f.write(f"{token}\t{0}\n")
        normal_tokens = sorted(normal_tokens.items(), key=lambda x: x[0])
        for token, score in normal_tokens:
            f.write(f"{token}\t{score}\n")
