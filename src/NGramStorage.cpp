//
// Created by pavel on 26.11.17.
//

#include "NGramStorage.h"


NGramStorage::NGramStorage() : max_ngram_size(0), cache(128) {}

NGramStorage::NGramStorage(vector<pair<vector<uint32_t>, uint32_t>> &ngrams): cache(128) {
    init(ngrams);
}

NGramStorage::NGramStorage(string filename): cache(128) {
    ifstream fin(filename, std::ios::in | std::ios::binary);
    vector<pair<vector<uint32_t>, uint32_t>> ngrams;
    uint64_t ngrams_count;
    fin.read((char*)&ngrams_count, sizeof(ngrams_count));
    ngrams.resize(ngrams_count);
    for (uint64_t i = 0; i < ngrams_count; i++) {
        fin.read((char*)&ngrams[i].second, sizeof(ngrams[i].second));
        uint8_t ngram_size;
        fin.read((char*)&ngram_size, sizeof(ngram_size));
        ngrams[i].first.resize(ngram_size);
        for (uint8_t j = 0; j < ngram_size; j++)
            fin.read((char*)&ngrams[i].first[j], sizeof(ngrams[i].first[j]));
    }
    init(ngrams);
}

void NGramStorage::init(vector<pair<vector<uint32_t>, uint32_t>>& ngrams) {
    assert(ngrams.size() < (~uint32_t(0)));
    store_empty_ngram_values(ngrams);
    store_max_ngram_size(ngrams);
    sort_ngrams(ngrams);
    build_storage(ngrams);
}

void NGramStorage::load(istream& in) {
    in.read((char*)(&empty_ngram_count), sizeof(empty_ngram_count));
    in.read((char*)(&empty_ngram_continuations_count), sizeof(empty_ngram_continuations_count));
    in.read((char*)(&empty_ngram_unique_continuations_count), sizeof(empty_ngram_unique_continuations_count));

    in.read((char*)(&max_ngram_size), sizeof(max_ngram_size));
    storage.resize(max_ngram_size);
    for (uint32_t i = 0; i < max_ngram_size; i++)
        storage[i].load(in);
}

void NGramStorage::dump(ostream& out) const {
    out.write((char*)(&empty_ngram_count), sizeof(empty_ngram_count));
    out.write((char*)(&empty_ngram_continuations_count), sizeof(empty_ngram_continuations_count));
    out.write((char*)(&empty_ngram_unique_continuations_count), sizeof(empty_ngram_unique_continuations_count));

    out.write((char*)(&max_ngram_size), sizeof(max_ngram_size));
    for (uint32_t i = 0; i < max_ngram_size; i++)
        storage[i].dump(out);
}

Record NGramStorage::find_record(const vector<uint32_t>& ngram) {
    if (ngram.size() == 0)
        throw NotFoundException("empty ngram");

    vector<uint32_t> context(ngram);
    context.pop_back();

    uint32_t context_index = get_context_index(context);
    uint32_t word_index = ngram.back();
    auto it = storage[context.size()].find(Key(word_index, context_index));
    if (it == storage[context.size()].end())
        throw NotFoundException("ngram");
    return *it;
}

uint32_t NGramStorage::get_ngram_count(const vector<uint32_t>& ngram) {
    if (ngram.size() == 0)
        return empty_ngram_count;
    else {
        try {
            return find_record(ngram).value.ngram_count;
        } catch (NotFoundException exception) {
            return 0;
        }
    }
}

uint32_t NGramStorage::get_continuations_count(const vector<uint32_t>& ngram) {
    if (ngram.size() == 0)
        return empty_ngram_continuations_count;
    else {
        try {
            return find_record(ngram).value.continuations_count;
        } catch (NotFoundException exception) {
            return 0;
        }
    }
}

uint32_t NGramStorage::get_unique_continuations_count(const vector<uint32_t>& ngram) {
    if (ngram.size() == 0)
        return empty_ngram_unique_continuations_count;
    else {
        try {
            return find_record(ngram).value.unique_continuations_count;
        } catch (NotFoundException exception) {
            return 0;
        }
    }
}

uint8_t NGramStorage::get_max_ngram_size() const {
    return max_ngram_size;
}

uint32_t NGramStorage::get_context_index(const vector<uint32_t>& ngram) {
    uint32_t context_index = 0;
    uint32_t i = 0;

    vector<uint32_t> ngram_copy(ngram);
    while (ngram_copy.size() > 0) {
        if (cache.exist(ngram_copy)) {
            context_index = cache.get(ngram_copy);
            i = uint32_t(ngram_copy.size());
            break;
        }
        ngram_copy.pop_back();
    }

    while (i < ngram.size()) {
        uint32_t word_index = ngram[i];
        auto it = storage[i].find(Key(word_index, context_index));
        if (it == storage[i].end())
            throw NotFoundException("context");
        context_index = uint32_t(it - storage[i].begin());
        ngram_copy.push_back(ngram[i]);
        cache.put(ngram_copy, context_index);
        i++;
    }

    return context_index;
}

void NGramStorage::store_empty_ngram_values(const vector<pair<vector<uint32_t>, uint32_t>> &ngrams) {
    set<uint32_t> continuations;
    empty_ngram_count = 0;
    empty_ngram_continuations_count = 0;
    for (const auto& ngram : ngrams) {
        empty_ngram_count += ngram.second;
        empty_ngram_continuations_count += ngram.second;
        continuations.insert(ngram.first[0]);
    }
    empty_ngram_unique_continuations_count = uint32_t(continuations.size());
}

void NGramStorage::store_max_ngram_size(const vector<pair<vector<uint32_t>, uint32_t>> &ngrams) {
    max_ngram_size = 0;
    for (const auto& ngram : ngrams)
        max_ngram_size = max(max_ngram_size, uint8_t(ngram.first.size()));
}

void NGramStorage::sort_ngrams(vector<pair<vector<uint32_t>, uint32_t>> &ngrams) const {
    sort(ngrams.begin(), ngrams.end(), [] (const pair<vector<uint32_t>, uint32_t>& ngram1,
                                           const pair<vector<uint32_t>, uint32_t>& ngram2)  {
             for (size_t i = 0; i < min(ngram1.first.size(), ngram2.first.size()); i++)
                 if (ngram1.first[i] < ngram2.first[i])
                     return true;
                 else if (ngram1.first[i] > ngram2.first[i])
                     return false;
             return ngram1.first.size() < ngram2.first.size();
         });
}

void NGramStorage::build_storage(const vector<pair<vector<uint32_t>, uint32_t>> &sorted_ngrams) {
    vector<uint32_t> contexts(sorted_ngrams.size(), 0);
    for (uint32_t i = 0; i < max_ngram_size; i++) {
        vector<Record> records;

        uint32_t prev_word_index = ~uint32_t(0);
        uint32_t prev_context_index = ~uint32_t(0);
        uint32_t prev_continuation_index = ~uint32_t(0);
        uint32_t ngram_count = 0;
        uint32_t continuations_count = 0;
        uint32_t unique_continuations_count = 0;

        for (uint32_t j = 0; j < sorted_ngrams.size(); j++) {
            if (i < sorted_ngrams[j].first.size()) {
                uint32_t word_index = sorted_ngrams[j].first[i];
                uint32_t context_index = contexts[j];
                if (prev_word_index == ~uint32_t(0)) {
                    prev_word_index = word_index;
                    prev_context_index = context_index;
                }
                if (word_index != prev_word_index || context_index != prev_context_index) {
                    Key key(prev_word_index, prev_context_index);
                    Value value(ngram_count, continuations_count, unique_continuations_count);
                    Record record(key, value);
                    records.push_back(record);
                    prev_word_index = word_index;
                    prev_context_index = context_index;
                    prev_continuation_index = ~uint32_t(0);
                    ngram_count = 0;
                    continuations_count = 0;
                    unique_continuations_count = 0;
                }
                ngram_count += sorted_ngrams[j].second;
                if (i + 1 < sorted_ngrams[j].first.size()) {
                    continuations_count += sorted_ngrams[j].second;
                    uint32_t continuation_index = sorted_ngrams[j].first[i + 1];
                    if (continuation_index != prev_continuation_index) {
                        unique_continuations_count += 1;
                        prev_continuation_index = continuation_index;
                    }
                }
            }
        }

        if (prev_word_index != ~uint32_t(0)) {
            Key key(prev_word_index, prev_context_index);
            Value value(ngram_count, continuations_count, unique_continuations_count);
            Record record(key, value);
            records.push_back(record);
        }

        sort(records.begin(), records.end());
        storage.push_back(CompressedArray(move(records)));

        if (i + 1 < max_ngram_size) {
            Key prev_key = Key(~uint32_t(0), ~uint32_t(0));
            uint32_t prev_key_index = ~uint32_t(0);
            for (uint32_t j = 0; j < sorted_ngrams.size(); j++)
                if (i < sorted_ngrams[j].first.size()) {
                    Key key(sorted_ngrams[j].first[i], contexts[j]);
                    if (prev_key != key) {
                        prev_key = key;
                        prev_key_index = uint32_t(storage[i].find(key) - storage[i].begin());
                    }
                    contexts[j] = prev_key_index;
                }
        }
    }
}

NGramStorage::const_iterator::const_iterator(const NGramStorage* storage, uint8_t ngram_size):
        storage(storage), ngram_size(ngram_size) {
    assert(ngram_size > 0);
    cursor.push_back(storage->storage[ngram_size - 1].begin());
    ngram.first.resize(ngram_size);
    ngram.first[ngram_size - 1] = cursor.back()->key.word_index;
    ngram.second = cursor.back()->value.ngram_count;
    for (uint8_t i = uint8_t(ngram_size - 1); i > 0; i--) {
        cursor.push_back(storage->storage[i - 1].begin() + cursor.back()->key.context_index);
        ngram.first[i - 1] = cursor.back()->key.word_index;
    }
}

NGramStorage::const_iterator NGramStorage::begin(uint8_t ngram_size) {
    return const_iterator(this, ngram_size);
}

NGramStorage::const_iterator NGramStorage::end(uint8_t ngram_size) {
    const_iterator res(this, ngram_size);
    res.cursor.clear();
    res.cursor.push_back(storage[ngram_size - 1].end());
    return res;
}

NGramStorage::const_iterator NGramStorage::const_iterator::operator++() {
    if (cursor[0] == storage->storage[ngram_size - 1].end())
        return *this;
    CompressedArray::const_iterator it(++cursor[0]);
    cursor.clear();
    cursor.push_back(it);
    ngram.first[ngram_size - 1] = cursor.back()->key.word_index;
    ngram.second = cursor.back()->value.ngram_count;
    for (uint8_t i = uint8_t(ngram_size - 1); i > 0; i--) {
        cursor.push_back(storage->storage[i - 1].begin() + cursor.back()->key.context_index);
        ngram.first[i - 1] = cursor.back()->key.word_index;
    }
    return *this;
}

NGramStorage::const_iterator NGramStorage::const_iterator::operator++(int) {
    const_iterator res(*this);
    ++(*this);
    return res;
}

const pair<vector<uint32_t>, uint32_t>& NGramStorage::const_iterator::operator*() const {
    return ngram;
}

const pair<vector<uint32_t>, uint32_t>* NGramStorage::const_iterator::operator->() const {
    return &ngram;
}

bool NGramStorage::const_iterator::operator==(const NGramStorage::const_iterator& other) const {
    return ngram_size == other.ngram_size && cursor[0] == other.cursor[0];
}

bool NGramStorage::const_iterator::operator!=(const NGramStorage::const_iterator& other) const {
    return ngram_size != other.ngram_size || cursor[0] != other.cursor[0];
}
