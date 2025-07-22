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

# you can also use the default models provided by this package, see below for details
# ssp = Ssentencepiece("gigaspeech-500")

# output ids
ids = ssp.encode(["HELLO WORLD", "LOVE AND PIECE"])

# output string pieces
pieces = ssp.encode(["HELLO WORLD", "LOVE AND PIECE"], out_type=str)

# decode
res = ssp.decode(ids)
```

## Default models

| Model Name | Description | Link |
|------------|-------------|------|
| alphabet-33| `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`, `'`, `▁` and 26 alphabets. | [alphabet-33](ssentencepiece/python/ssentencepiece/resources/alphabet-33.vocab) |
| librispeech-500| 500 unigram pieces trained on Librispeech. | [librispeech-500](ssentencepiece/python/ssentencepiece/resources/librispeech-500.vocab) |
| librispeech-5000| 5000 unigram pieces trained on Librispeech. | [librispeech-5000](ssentencepiece/python/ssentencepiece/resources/librispeech-5000.vocab) |
| gigaspeech-500| 500 unigram pieces trained on Gigaspeech. | [gigaspeech-500](ssentencepiece/python/ssentencepiece/resources/gigaspeech-500.vocab) |
| gigaspeech-2000| 2000 unigram pieces trained on Gigaspeech. | [gigaspeech-2000](ssentencepiece/python/ssentencepiece/resources/gigaspeech-2000.vocab) |
| gigaspeech-5000| 5000 unigram pieces trained on Gigaspeech. | [gigaspeech-5000](ssentencepiece/python/ssentencepiece/resources/gigaspeech-5000.vocab) |
| zh-en-3876 | 3500 Chinese characters, 256 fallback bytes, 10 numbers, 10 punctuations, 100 English unigram pieces. | [zh-en-3876](ssentencepiece/python/ssentencepiece/resources/zh-en-3876.vocab) |
| zh-en-6876 | 6500 Chinese characters, 256 fallback bytes, 10 numbers, 10 punctuations, 100 English unigram pieces. | [zh-en-6876](ssentencepiece/python/ssentencepiece/resources/zh-en-6876.vocab) |
| zh-en-8481 | 8105 Chinese characters, 256 fallback bytes, 10 numbers, 10 punctuations, 100 English unigram pieces. | [zh-en-8481](ssentencepiece/python/ssentencepiece/resources/zh-en-8481.vocab) |
| zh-en-5776 | 3500 Chinese characters, 256 fallback bytes, 10 numbers, 10 punctuations, 2000 English unigram pieces. | [zh-en-5776](ssentencepiece/python/ssentencepiece/resources/zh-en-5776.vocab) |
| zh-en-8776 | 6500 Chinese characters, 256 fallback bytes, 10 numbers, 10 punctuations, 2000 English unigram pieces. | [zh-en-8776](ssentencepiece/python/ssentencepiece/resources/zh-en-8776.vocab) |
| zh-en-10381 | 8105 Chinese characters, 256 fallback bytes, 10 numbers, 10 punctuations, 2000 English unigram pieces. | [zh-en-10381](ssentencepiece/python/ssentencepiece/resources/zh-en-10381.vocab) |
| zh-en-yue-9761 | 8105 + 1280 Chinese characters(Cantonese included), 256 fallback bytes, 10 numbers, 10 punctuations, 100 English unigram pieces. | [zh-en-yue-9761](ssentencepiece/python/ssentencepiece/resources/zh-en-yue-9761.vocab) |
| zh-en-yue-11661 | 8105 + 1280 Chinese characters(Cantonese included), 256 fallback bytes, 10 numbers, 10 punctuations, 2000 English unigram pieces. | [zh-en-yue-11661](ssentencepiece/python/ssentencepiece/resources/zh-en-yue-11661.vocab) |

**Note**: The number of 3500, 6500 and 8105 is from [通用规范汉字表](http://www.moe.gov.cn/jyb_sjzl/ziliao/A19/201306/t20130601_186002.html).

