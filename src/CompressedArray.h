//
// Created by pavel on 26.12.17.
//

#ifndef NGRAMSTORAGE_COMPRESSEDARRAY_H
#define NGRAMSTORAGE_COMPRESSEDARRAY_H

#include "Record.h"
#include "Vocabulary.h"
#include "Serializable.h"

#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

using std::vector;
using std::unique;
using std::max;
using std::min;
using std::cout;
using std::endl;
using std::move;


class CompressedArray: public Serializable {
public:
    CompressedArray();
    CompressedArray(vector<Record> sorted_records);

    uint32_t size() const;

    void dump(ostream& out) const override;
    void load(istream& in) override;

    class const_iterator;

    const_iterator begin() const;
    const_iterator end() const;

    const_iterator find(Key key) const;

    class const_iterator {
    public:
        const_iterator(const CompressedArray* array);

        const_iterator operator++();
        const_iterator operator++(int);
        const_iterator operator+=(uint32_t n);
        const_iterator operator+(uint32_t n) const;
        const_iterator operator-=(uint32_t n);
        const_iterator operator-(uint32_t n) const;
        int32_t operator-(const const_iterator& other) const;
        const Record& operator*() const;
        const Record* operator->() const;
        bool operator==(const const_iterator& other) const;
        bool operator!=(const const_iterator& other) const;
        bool operator>(const const_iterator& other) const;
        bool operator>=(const const_iterator& other) const;
        bool operator<(const const_iterator& other) const;
        bool operator<=(const const_iterator& other) const;

        friend class CompressedArray;

    private:
        const CompressedArray* array;
        uint32_t block_index;
        uint32_t record_index;
        uint32_t offset;
        bool same_word;
        Record record;

        bool read_bit();
        uint32_t read_number(uint32_t log_radix);
        void read_key();
        void read_value();
        void read_record();

        void switch_to_block(uint32_t block_index);
        void switch_to_record(uint32_t record_index);
    };

private:
    struct BlockHeader {
        Key key;
        uint32_t record_index;
        uint32_t offset;

        bool operator < (const BlockHeader& other) const;
    };

    static const uint32_t max_block_size;

    uint32_t word_index_diff_log_radix;
    uint32_t context_index_diff_log_radix;
    uint32_t context_index_log_radix;
    uint32_t ngram_count_index_log_radix;
    uint32_t continuations_count_index_log_radix;
    uint32_t unique_continuations_count_index_log_radix;

    vector<bool> data;
    vector<BlockHeader> headers;
    Vocabulary<uint32_t> ngram_count_values;
    Vocabulary<uint32_t> continuations_count_values;
    Vocabulary<uint32_t> unique_continuations_count_values;
    uint32_t record_count;

    uint32_t fill_block(const vector<Record>& sorted_records, uint32_t record_index);
    void find_best_radix_parameters(const vector<Record>& records);
    void store_values(vector<Record> records);

    uint32_t calculate_number_size(uint32_t number, uint32_t log_radix) const;
    uint32_t calculate_key_size(Key key, Key prev_key, bool same_word) const;
    uint32_t calculate_value_size(Value value) const;
    uint32_t calculate_record_size(Record record, Record prev_record, bool same_word) const;

    void add_bit(bool bit);
    void add_number(uint32_t number, uint32_t log_radix);
    void add_key(Key key, Key prev_key, bool same_word);
    void add_value(Value value);
    void add_record(Record record, Record prev_record, bool same_word);
};


#endif //NGRAMSTORAGE_COMPRESSEDARRAY_H
