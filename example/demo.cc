/**
 * Copyright      2026    Wei Kang (wkang@pku.edu.cn)
 *
 * Licensed under the Apache License, Version 2.0
 *
 * ---------------------------------------------------------------------------
 * A minimal, self-contained demo showing how to integrate
 * simple-sentencepiece (no protobuf dependency!) into a C++ project.
 *
 * Build: see ./README.md and ./CMakeLists.txt
 * ---------------------------------------------------------------------------
 */

#include "ssentencepiece/csrc/ssentencepiece.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void PrintPieces(const std::vector<std::string> &pieces) {
  std::cout << "  pieces: [";
  for (size_t i = 0; i < pieces.size(); ++i) {
    std::cout << "\"" << pieces[i] << "\"";
    if (i + 1 < pieces.size()) std::cout << ", ";
  }
  std::cout << "]\n";
}

void PrintIds(const std::vector<int32_t> &ids) {
  std::cout << "  ids   : [";
  for (size_t i = 0; i < ids.size(); ++i) {
    std::cout << ids[i];
    if (i + 1 < ids.size()) std::cout << ", ";
  }
  std::cout << "]\n";
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr
        << "Usage: " << argv[0] << " <bpe.vocab> [text ...]\n"
        << "\n"
        << "  <bpe.vocab>  Path to a sentencepiece .vocab file.\n"
        << "               You can generate one via google's sentencepiece,\n"
        << "               or use any of the default vocabs under\n"
        << "               ssentencepiece/python/ssentencepiece/resources/.\n"
        << "\n"
        << "Example:\n"
        << "  " << argv[0]
        << " ../ssentencepiece/python/tests/testdata/bpe.vocab \\\n"
        << "      \"HELLO WORLD\" \"I LOVE BEIJING\"\n";
    return EXIT_FAILURE;
  }

  const std::string vocab_path = argv[1];

  // ---------------------------------------------------------------------
  // Step 1. Load the vocab and build the processor.
  //         The second argument is the number of worker threads used for
  //         batched encoding/decoding; pass 1 if you don't want threads.
  // ---------------------------------------------------------------------
  ssentencepiece::Ssentencepiece processor(vocab_path, /*num_threads=*/2);
  std::cout << "Loaded vocab from: " << vocab_path << "\n";
  std::cout << "Vocab size       : " << processor.VocabSize() << "\n\n";

  // ---------------------------------------------------------------------
  // Step 2. Collect input sentences. Default to a couple of built-in demos
  //         if the user didn't supply any.
  // ---------------------------------------------------------------------
  std::vector<std::string> sentences;
  if (argc > 2) {
    for (int i = 2; i < argc; ++i) sentences.emplace_back(argv[i]);
  } else {
    sentences = {"HELLO WORLD", "I LOVE BEIJING", "LOVE YOU AMERICAN"};
  }

  // ---------------------------------------------------------------------
  // Step 3. Single-sentence encode to pieces and ids.
  // ---------------------------------------------------------------------
  std::cout << "=== Single-sentence Encode ===\n";
  for (const auto &s : sentences) {
    std::cout << "input : \"" << s << "\"\n";

    std::vector<std::string> pieces;
    processor.Encode(s, &pieces);
    PrintPieces(pieces);

    std::vector<int32_t> ids;
    processor.Encode(s, &ids);
    PrintIds(ids);

    std::cout << "  decode: \"" << processor.Decode(ids) << "\"\n\n";
  }

  // ---------------------------------------------------------------------
  // Step 4. Batched encode — multiple sentences are tokenized in parallel
  //         using the internal threadpool.
  // ---------------------------------------------------------------------
  std::cout << "=== Batched Encode ===\n";
  std::vector<std::vector<int32_t>> batch_ids;
  processor.Encode(sentences, &batch_ids);
  for (size_t i = 0; i < sentences.size(); ++i) {
    std::cout << "[" << i << "] \"" << sentences[i] << "\"\n";
    PrintIds(batch_ids[i]);
  }
  std::cout << "\n";

  // ---------------------------------------------------------------------
  // Step 5. Batched decode — symmetric to batched encode.
  // ---------------------------------------------------------------------
  std::cout << "=== Batched Decode ===\n";
  std::vector<std::string> decoded = processor.Decode(batch_ids);
  for (size_t i = 0; i < decoded.size(); ++i) {
    std::cout << "[" << i << "] \"" << decoded[i] << "\"\n";
  }
  std::cout << "\n";

  // ---------------------------------------------------------------------
  // Step 6. PieceToId / IdToPiece — useful when you need to look up
  //         special tokens such as <sos>, <eos>, <blk>, etc.
  // ---------------------------------------------------------------------
  std::cout << "=== Piece <-> Id lookup ===\n";
  const std::vector<std::string> probe = {"<unk>", "<s>", "</s>"};
  for (const auto &p : probe) {
    int32_t id = processor.PieceToId(p);
    std::cout << "  PieceToId(\"" << p << "\") = " << id;
    if (id >= 0) {
      std::cout << "  IdToPiece(" << id << ") = \""
                << processor.IdToPiece(id) << "\"";
    }
    std::cout << "\n";
  }

  return EXIT_SUCCESS;
}
