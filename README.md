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
| alphabet-30| `<blk>`,`<unk>`, `'`, `▁` and 26 alphabets (upper case) | [alphabet-30](ssentencepiece/python/ssentencepiece/resources/alphabet-30.vocab) |
| alphabet-32| `<blk>`,`<unk>`, `<sos>`, `<eos>`, `'`, `▁` and 26 alphabets (upper case) | [alphabet-32](ssentencepiece/python/ssentencepiece/resources/alphabet-32.vocab) |
| librispeech-500| 500 unigram pieces trained on Librispeech. (special tokens: `<blk>`,`<unk>` and `<sos/eos>`) | [librispeech-500](ssentencepiece/python/ssentencepiece/resources/librispeech-500.vocab) |
| librispeech-5000| 5000 unigram pieces trained on Librispeech. (special tokens: `<blk>`,`<unk>` and `<sos/eos>`) | [librispeech-5000](ssentencepiece/python/ssentencepiece/resources/librispeech-5000.vocab) |
| gigaspeech-500| 500 unigram pieces trained on Gigaspeech. (special tokens: `<blk>`,`<unk>` and `<sos/eos>`) | [gigaspeech-500](ssentencepiece/python/ssentencepiece/resources/gigaspeech-500.vocab) |
| gigaspeech-2000| 2000 unigram pieces trained on Gigaspeech. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [gigaspeech-2000](ssentencepiece/python/ssentencepiece/resources/gigaspeech-2000.vocab) |
| gigaspeech-5000| 5000 unigram pieces trained on Gigaspeech. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [gigaspeech-5000](ssentencepiece/python/ssentencepiece/resources/gigaspeech-5000.vocab) |
| zh-en-4258 | 3500 Chinese characters, 256 fallback bytes, 502 English unigram pieces. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [zh-en-4258](ssentencepiece/python/ssentencepiece/resources/zh-en-4258.vocab) |
| zh-en-7258 | 6500 Chinese characters, 256 fallback bytes, 502 English unigram pieces. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [zh-en-7258](ssentencepiece/python/ssentencepiece/resources/zh-en-7258.vocab) |
| zh-en-8756 | 6500 Chinese characters, 256 fallback bytes, 2000 English unigram pieces. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [zh-en-8756](ssentencepiece/python/ssentencepiece/resources/zh-en-8756.vocab) |
| zh-en-8863 | 8105 Chinese characters, 256 fallback bytes, 502 English unigram pieces. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [zh-en-8863](ssentencepiece/python/ssentencepiece/resources/zh-en-8863.vocab) |
| zh-en-13361 | 8105 Chinese characters, 256 fallback bytes, 5000 English unigram pieces. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [zh-en-13361](ssentencepiece/python/ssentencepiece/resources/zh-en-13361.vocab) |
| zh-en-yue-11641 | 8105 + 1280 Chinese characters(Cantonese included), 256 fallback bytes, 2000 English unigram pieces. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [zh-en-yue-11641](ssentencepiece/python/ssentencepiece/resources/zh-en-yue-11641.vocab) |
| zh-en-yue-14641 | 8105 + 1280 Chinese characters(Cantonese included), 256 fallback bytes, 5000 English unigram pieces. (special tokens: `<blk>`,`<unk>`, `<sos>`, `<eos>`, `<pad>`) | [zh-en-yue-14641](ssentencepiece/python/ssentencepiece/resources/zh-en-yue-14641.vocab) |

**Note**: The number of 3500, 6500 and 8105 is from [通用规范汉字表](http://www.moe.gov.cn/jyb_sjzl/ziliao/A19/201306/t20130601_186002.html).


