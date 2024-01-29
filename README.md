# simple-sentencepiece
A simple sentencepiece encoder and decoder.

Note: This is not a new sentencepiece toolkit, it just uses google's sentencepiece model
as input and encode the string to ids/pieces or decode the ids to string. The advantage of
this tool is that it doesn't have any dependency (no protobuf), so it will be easier to
integrate it into a C++ project.


## Installation

```
pip install simple-sentencepiece
```


## Usage

The usage is very similar to sentencepiece, it also has `encode` and `decode` interface.

```python
from ssentencepiece import Ssentencepiece

# you can get bpe.vocab from a trained bpe model, see google's sentencepiece for details
ssp = Ssentencepiece("/path/to/bpe.vocab")

# output ids
ids = ssp.encode(["HELLO WORLD", "LOVE AND PIECE"])

# output string pieces
pieces = ssp.encode(["HELLO WORLD", "LOVE AND PIECE"], out_type=str)

# decode
res = ssp.decode(ids)
```
