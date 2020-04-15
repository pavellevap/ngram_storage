//
// Created by pavel on 26.12.17.
//

#ifndef NGRAMSTORAGE_RECORD_H
#define NGRAMSTORAGE_RECORD_H

#include <cstdint>


struct Key {
    Key() {}
    Key(uint32_t word_index, uint32_t context_index): word_index(word_index),
                                                      context_index(context_index) {}

    uint32_t word_index;
    uint32_t context_index;

    bool operator < (const Key& other) const {
        return ((word_index < other.word_index) ||
                (word_index == other.word_index && context_index < other.context_index));
    }

    bool operator == (const Key& other) const {
        return (word_index == other.word_index) && (context_index == other.context_index);
    }

    bool operator != (const Key& other) const {
        return (word_index != other.word_index) || (context_index != other.context_index);
    }
};


struct Value {
    Value() {}
    Value(uint32_t ngram_count, uint32_t continuations_count, uint32_t unique_continuations_count):
            ngram_count(ngram_count), continuations_count(continuations_count),
            unique_continuations_count(unique_continuations_count) {}

    uint32_t ngram_count;
    uint32_t continuations_count;
    uint32_t unique_continuations_count;
};


struct Record {
    Record() {}
    Record(Key key, Value value): key(key), value(value) {}

    Key key;
    Value value;

    bool operator < (const Record& other) const {
        return key < other.key;
    }
};

#endif //NGRAMSTORAGE_RECORD_H
