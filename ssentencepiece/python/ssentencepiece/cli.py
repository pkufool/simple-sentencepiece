import click
import logging

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
@click.option('--bpe-model', type=str, required=True, help='The path to the bpe model.')
def export_vocab(bpe_model):
    """
    Export the vocabulary from a BPE model.
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
