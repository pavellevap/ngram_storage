//
// Created by pavel on 26.12.17.
//

#include "CompressedArray.h"

const uint32_t CompressedArray::max_block_size = 1024;

CompressedArray::CompressedArray() {}

CompressedArray::CompressedArray(vector<Record> sorted_records) {
    store_values(sorted_records);
    record_count = uint32_t(sorted_records.size());
    find_best_radix_parameters(sorted_records);

    uint32_t record_index = 0;
    while (record_index < record_count) {
        BlockHeader header;
        header.key = sorted_records[record_index].key;
        header.record_index = record_index;
        header.offset = uint32_t(data.size());
        headers.push_back(header);

        size_t old_size = data.size();
        record_index = fill_block(sorted_records, record_index);
        size_t new_size = data.size();
        assert(new_size - old_size <= max_block_size);
    }
    vector<BlockHeader>(headers).swap(headers);
    vector<bool>(data).swap(data);
}

void CompressedArray::store_values(vector<Record> records) {
    vector<uint32_t> ngram_counts;
    vector<uint32_t> continuations_counts;
    vector<uint32_t> unique_continuations_counts;
    for (const Record &record : records) {
        ngram_counts.push_back(record.value.ngram_count);
        continuations_counts.push_back(record.value.continuations_count);
        unique_continuations_counts.push_back(record.value.unique_continuations_count);
    }
    ngram_count_values = Vocabulary<uint32_t>(ngram_counts);
    continuations_count_values = Vocabulary<uint32_t>(continuations_counts);
    unique_continuations_count_values = Vocabulary<uint32_t>(unique_continuations_counts);
}

uint32_t CompressedArray::fill_block(const vector<Record>& sorted_records, uint32_t record_index) {
    const Record& firstRecord = sorted_records[record_index];

    uint32_t block_size = 0;
    block_size += calculate_value_size(firstRecord.value);
    block_size += 1;

    uint32_t same_word_block_size = block_size;
    uint32_t first_index = record_index;
    uint32_t last_index = record_index + 1;
    bool same_word = true;
    while (last_index < sorted_records.size()) {
        const Record& last_record = sorted_records[last_index];
        const Record& prev_record = sorted_records[last_index - 1];
        uint32_t last_record_size = calculate_record_size(last_record, prev_record, false);
        uint32_t same_word_last_record_size = calculate_record_size(last_record, prev_record, true);
        if (block_size + last_record_size > max_block_size)
            break;

        block_size += last_record_size;
        same_word_block_size += same_word_last_record_size;

        same_word &= prev_record.key.word_index == last_record.key.word_index;
        last_index++;
    }

    if (same_word) {
        block_size = same_word_block_size;
        while (last_index != sorted_records.size()) {
            const Record& last_record = sorted_records[last_index];
            const Record& prev_record = sorted_records[last_index - 1];
            uint32_t last_record_size = calculate_record_size(last_record, prev_record, true);
            if (block_size + last_record_size > max_block_size ||
                prev_record.key.word_index != last_record.key.word_index)
                break;

            block_size += last_record_size;
            last_index++;
        }
    }

    add_value(firstRecord.value);
    add_bit(same_word);
    for (record_index = first_index + 1; record_index < last_index; record_index++)
        add_record(sorted_records[record_index], sorted_records[record_index - 1], same_word);

    return record_index;
}

uint32_t CompressedArray::size() const {
    return record_count;
}

CompressedArray::const_iterator CompressedArray::begin() const {
    CompressedArray::const_iterator it(this);
    it.switch_to_block(0);
    return it;
}

CompressedArray::const_iterator CompressedArray::end() const {
    CompressedArray::const_iterator it(this);
    it.switch_to_block(uint32_t(headers.size()));
    return it;
}

CompressedArray::const_iterator CompressedArray::find(Key key) const {
    BlockHeader header;
    header.key = key;

    auto it = upper_bound(headers.begin(), headers.end(), header);
    if (it == headers.begin())
        return end();
    it--;
    uint32_t block_index = uint32_t(it - headers.begin());

    CompressedArray::const_iterator res(this);
    res.switch_to_block(block_index);
    while (res.block_index == block_index && key != res->key)
        res++;

    if (res.block_index != block_index)
        return end();

    return res;
}

void CompressedArray::dump(ostream& out) const {
    out.write((char*)(&word_index_diff_log_radix), sizeof(word_index_diff_log_radix));
    out.write((char*)(&context_index_diff_log_radix), sizeof(context_index_diff_log_radix));
    out.write((char*)(&context_index_log_radix), sizeof(context_index_log_radix));
    out.write((char*)(&ngram_count_index_log_radix), sizeof(ngram_count_index_log_radix));
    out.write((char*)(&continuations_count_index_log_radix),
              sizeof(continuations_count_index_log_radix));
    out.write((char*)(&unique_continuations_count_index_log_radix),
              sizeof(unique_continuations_count_index_log_radix));

    out.write((char*)(&record_count), sizeof(record_count));

    ngram_count_values.dump(out);
    continuations_count_values.dump(out);
    unique_continuations_count_values.dump(out);

    uint32_t blocks_count = uint32_t(headers.size());
    out.write((char*)(&blocks_count), sizeof(blocks_count));
    for (uint32_t i = 0; i < blocks_count; i++) {
        out.write((char*)(&headers[i].key.word_index), sizeof(headers[i].key.word_index));
        out.write((char*)(&headers[i].key.context_index), sizeof(headers[i].key.context_index));
        out.write((char*)(&headers[i].offset), sizeof(headers[i].offset));
        out.write((char*)(&headers[i].record_index), sizeof(headers[i].record_index));
    }

    uint8_t buffer = 0;
    uint8_t idx = 0;
    uint32_t size = uint32_t(data.size());
    out.write((char*)(&size), sizeof(size));
    for (uint32_t i = 0; i < data.size(); i++) {
        buffer |= data[i] << idx;
        idx++;
        if (idx == 8) {
            out.write((char*)(&buffer), sizeof(buffer));
            idx = 0;
            buffer = 0;
        }
    }
    if (idx > 0)
        out.write((char*)(&buffer), sizeof(buffer));
}

void CompressedArray::load(istream &in) {
    in.read((char*)(&word_index_diff_log_radix), sizeof(word_index_diff_log_radix));
    in.read((char*)(&context_index_diff_log_radix), sizeof(context_index_diff_log_radix));
    in.read((char*)(&context_index_log_radix), sizeof(context_index_log_radix));
    in.read((char*)(&ngram_count_index_log_radix), sizeof(ngram_count_index_log_radix));
    in.read((char*)(&continuations_count_index_log_radix),
              sizeof(continuations_count_index_log_radix));
    in.read((char*)(&unique_continuations_count_index_log_radix),
              sizeof(unique_continuations_count_index_log_radix));

    in.read((char*)(&record_count), sizeof(record_count));

    ngram_count_values.load(in);
    continuations_count_values.load(in);
    unique_continuations_count_values.load(in);

    uint32_t nblocks;
    in.read((char*)(&nblocks), sizeof(nblocks));
    headers.resize(nblocks);
    for (uint32_t i = 0; i < nblocks; i++) {
        in.read((char*)(&headers[i].key.word_index), sizeof(headers[i].key.word_index));
        in.read((char*)(&headers[i].key.context_index), sizeof(headers[i].key.context_index));
        in.read((char*)(&headers[i].offset), sizeof(headers[i].offset));
        in.read((char*)(&headers[i].record_index), sizeof(headers[i].record_index));
    }

    uint32_t size;
    in.read((char*)(&size), sizeof(size));
    data.resize(size);
    uint8_t buffer = 0;
    uint8_t idx = 8;
    for (uint32_t i = 0; i < size; i++) {
        if (idx == 8) {
            in.read((char*)(&buffer), sizeof(buffer));
            idx = 0;
        }
        data[i] = bool(buffer & (1 << idx));
        idx++;
    }
}

uint32_t CompressedArray::calculate_number_size(uint32_t number, uint32_t log_radix) const {
    uint32_t len = 0;
    while (number >= (uint64_t(1) << len * log_radix))
        len++;
    return len + 1 + len * log_radix;
}

uint32_t CompressedArray::calculate_key_size(Key key, Key prev_key, bool same_word) const {
    uint32_t size = 0;
    if (!same_word)
        size += calculate_number_size(key.word_index - prev_key.word_index,
                                      word_index_diff_log_radix);
    if (key.word_index == prev_key.word_index)
        size += calculate_number_size(key.context_index - prev_key.context_index,
                                      context_index_diff_log_radix);
    else
        size += calculate_number_size(key.context_index, context_index_log_radix);
    return size;
}

uint32_t CompressedArray::calculate_value_size(Value value) const {
    uint32_t size = 0;
    uint32_t index = ngram_count_values.get_index(value.ngram_count);
    size += calculate_number_size(index, ngram_count_index_log_radix);
    index = continuations_count_values.get_index(value.continuations_count);
    size += calculate_number_size(index, continuations_count_index_log_radix);
    index = unique_continuations_count_values.get_index(value.unique_continuations_count);
    size += calculate_number_size(index, unique_continuations_count_index_log_radix);
    return size;
}

uint32_t CompressedArray::calculate_record_size(Record record, Record prev_record, bool same_word) const {
    return (calculate_key_size(record.key, prev_record.key, same_word) +
            calculate_value_size(record.value));
}

void CompressedArray::add_bit(bool bit) {
    data.push_back(bit);
}

void CompressedArray::add_number(uint32_t number, uint32_t log_radix) {
    uint32_t len = 0;
    while (number >= (uint64_t(1) << len * log_radix))
        len++;

    for (uint32_t i = 0; i < len;  i++)
        data.push_back(1);
    data.push_back(0);

    for (uint32_t i = 0; i < len * log_radix; i++) {
        data.push_back(bool(number & 1));
        number >>= 1;
    }
}

void CompressedArray::add_key(Key key, Key prev_key, bool same_word) {
    if (!same_word)
        add_number(key.word_index - prev_key.word_index, word_index_diff_log_radix);
    if (key.word_index != prev_key.word_index)
        add_number(key.context_index, context_index_log_radix);
    else
        add_number(key.context_index - prev_key.context_index, context_index_diff_log_radix);
}

void CompressedArray::add_value(Value value) {
    uint32_t index = ngram_count_values.get_index(value.ngram_count);
    add_number(index, ngram_count_index_log_radix);
    index = continuations_count_values.get_index(value.continuations_count);
    add_number(index, continuations_count_index_log_radix);
    index = unique_continuations_count_values.get_index(value.unique_continuations_count);
    add_number(index, unique_continuations_count_index_log_radix);
}

void CompressedArray::add_record(Record record, Record prev_record, bool same_word) {
    add_key(record.key, prev_record.key, same_word);
    add_value(record.value);
}

void CompressedArray::find_best_radix_parameters(const vector<Record>& records) {
    vector<uint64_t> context_index_diff_size(8, 0);
    vector<uint64_t> context_index_size(8, 0);
    vector<uint64_t> ngram_count_index_size(8, 0);
    vector<uint64_t> continuations_count_index_size(8, 0);
    vector<uint64_t> unique_continuations_count_index_size(8, 0);

    for (size_t i = 1; i < records.size(); i++) {
        const Record& record = records[i];
        const Record& prev_record = records[i - 1];

        for (uint32_t j = 0; j < 8; j++) {
            if (prev_record.key.word_index == record.key.word_index) {
                uint32_t diff = record.key.context_index - prev_record.key.context_index;
                context_index_diff_size[j] += calculate_number_size(diff, j + 1);
            } else {
                context_index_size[j] += calculate_number_size(record.key.context_index, j + 1);
            }

            uint32_t index = ngram_count_values.get_index(record.value.ngram_count);
            ngram_count_index_size[j] += calculate_number_size(index, j + 1);
            index = continuations_count_values.get_index(record.value.continuations_count);
            continuations_count_index_size[j] += calculate_number_size(index, j + 1);
            index = unique_continuations_count_values.get_index(record.value.unique_continuations_count);
            unique_continuations_count_index_size[j] += calculate_number_size(index, j + 1);
        }
    }

    word_index_diff_log_radix = 2;
    context_index_diff_log_radix = 1;
    context_index_log_radix = 1;
    ngram_count_index_log_radix = 1;
    continuations_count_index_log_radix = 1;
    unique_continuations_count_index_log_radix = 1;
    for (uint32_t j = 0; j < 8; j++) {
        if (context_index_diff_size[context_index_diff_log_radix - 1] >
                context_index_diff_size[j])
            context_index_diff_log_radix = j + 1;
        if (context_index_size[context_index_log_radix - 1] >
                context_index_size[j])
            context_index_log_radix = j + 1;
        if (ngram_count_index_size[ngram_count_index_log_radix - 1] >
                ngram_count_index_size[j])
            ngram_count_index_log_radix = j + 1;
        if (continuations_count_index_size[continuations_count_index_log_radix - 1] >
                continuations_count_index_size[j])
            continuations_count_index_log_radix = j + 1;
        if (unique_continuations_count_index_size[unique_continuations_count_index_log_radix - 1] >
                unique_continuations_count_index_size[j])
            unique_continuations_count_index_log_radix = j + 1;
    }
}

bool CompressedArray::BlockHeader::operator<(const CompressedArray::BlockHeader &other) const {
    return key < other.key;
}

CompressedArray::const_iterator::const_iterator(const CompressedArray* array): array(array) {}

CompressedArray::const_iterator CompressedArray::const_iterator::operator++(int) {
    CompressedArray::const_iterator res(*this);
    ++(*this);
    return res;
}

CompressedArray::const_iterator CompressedArray::const_iterator::operator++() {
    if (record_index == array->record_count)
        return *this;
    if (record_index + 1 == array->record_count)
        switch_to_block(uint32_t(array->headers.size()));
    else if (block_index + 1 == array->headers.size())
        read_record();
    else if (record_index + 1 == array->headers[block_index + 1].record_index)
        switch_to_block(block_index + 1);
    else
        read_record();
    return *this;
}

CompressedArray::const_iterator CompressedArray::const_iterator::operator+=(uint32_t n) {
    switch_to_record(record_index + n);
    return *this;
}

CompressedArray::const_iterator CompressedArray::const_iterator::operator+(uint32_t n) const {
    CompressedArray::const_iterator res(*this);
    res += n;
    return res;
}

CompressedArray::const_iterator CompressedArray::const_iterator::operator-=(uint32_t n) {
    if (n > record_index)
        switch_to_record(0);
    else
        switch_to_record(record_index - n);
    return *this;
}

CompressedArray::const_iterator CompressedArray::const_iterator::operator-(uint32_t n) const {
    CompressedArray::const_iterator res(*this);
    res -= n;
    return res;
}

int32_t CompressedArray::const_iterator::operator-(const CompressedArray::const_iterator& other) const {
    return record_index - other.record_index;
}

const Record& CompressedArray::const_iterator::operator*() const {
    return record;
}

const Record* CompressedArray::const_iterator::operator->() const {
    return &record;
}

bool CompressedArray::const_iterator::operator==(const CompressedArray::const_iterator &other) const {
    return record_index == other.record_index;
}

bool CompressedArray::const_iterator::operator!=(const CompressedArray::const_iterator &other) const {
    return record_index != other.record_index;
}

bool CompressedArray::const_iterator::operator>(const const_iterator& other) const {
    return record_index > other.record_index;
}

bool CompressedArray::const_iterator::operator>=(const const_iterator& other) const {
    return record_index >= other.record_index;
}

bool CompressedArray::const_iterator::operator<(const const_iterator& other) const {
    return record_index < other.record_index;
}

bool CompressedArray::const_iterator::operator<=(const const_iterator& other) const {
    return record_index <= other.record_index;
}

bool CompressedArray::const_iterator::read_bit() {
    return array->data[offset++];
}

uint32_t CompressedArray::const_iterator::read_number(uint32_t log_radix) {
    uint32_t len = 0;
    while (array->data[offset]) {
        len++;
        offset++;
    }
    offset++;

    uint32_t number = 0;
    for (uint32_t i = 0; i < len * log_radix; i++) {
        number |= (array->data[offset] << i);
        offset++;
    }

    return number;
}

void CompressedArray::const_iterator::read_key() {
    uint32_t word_index_delta = 0;

    if (!same_word) {
        word_index_delta = read_number(array->word_index_diff_log_radix);
        record.key.word_index += word_index_delta;
    }

    if (word_index_delta > 0)
        record.key.context_index = read_number(array->context_index_log_radix);
    else
        record.key.context_index += read_number(array->context_index_diff_log_radix);
}

void CompressedArray::const_iterator::read_value() {
    uint32_t index = read_number(array->ngram_count_index_log_radix);
    record.value.ngram_count = array->ngram_count_values[index];
    index = read_number(array->continuations_count_index_log_radix);
    record.value.continuations_count = array->continuations_count_values[index];
    index = read_number(array->unique_continuations_count_index_log_radix);
    record.value.unique_continuations_count = array->unique_continuations_count_values[index];
}

void CompressedArray::const_iterator::read_record() {
    read_key();
    read_value();
    record_index++;
}

void CompressedArray::const_iterator::switch_to_block(uint32_t block_index) {
    if (block_index >= array->headers.size()) {
        this->block_index = uint32_t(array->headers.size());
        record_index = array->record_count;
        offset = uint32_t(array->data.size());
    } else {
        this->block_index = block_index;
        record_index = array->headers[block_index].record_index;
        offset = array->headers[block_index].offset;

        record.key = array->headers[block_index].key;
        read_value();

        same_word = read_bit();
    }
}

void CompressedArray::const_iterator::switch_to_record(uint32_t record_index) {
    if (record_index > array->record_count)
        record_index = array->record_count;

    BlockHeader header;
    header.record_index = record_index;
    auto it = upper_bound(array->headers.begin(), array->headers.end(), header,
                          [](const BlockHeader& first, const BlockHeader& second) {
                              return first.record_index < second.record_index;
                          });
    --it;

    uint32_t block_index = uint32_t(it - array->headers.begin());
    switch_to_block(block_index);
    while (this->block_index == block_index && this->record_index < record_index)
        (*this)++;
}