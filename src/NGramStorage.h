//
// Created by pavel on 26.11.17.
//

#ifndef NGRAMSTORAGE_NGRAMSTORAGE_H
#define NGRAMSTORAGE_NGRAMSTORAGE_H

#include <set>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <queue>

#include "CompressedArray.h"
#include "Cache.h"

using std::set;
using std::ifstream;
using std::istringstream;
using std::priority_queue;
using std::unordered_set;


class IntegerVectorHasher {
public:
    size_t operator()(const vector<uint32_t> &vec) const {
        size_t seed = vec.size();
        for (auto &s : vec)
            seed ^= size_t(s) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};


class IntegerVectorComparator {
public:
    bool operator()(const vector<uint32_t>& v1, const vector<uint32_t>& v2) const {
        for (size_t i = 0; i < min(v1.size(), v2.size()); i++)
            if (v1[i] < v2[i])
                return true;
            else if (v1[i] > v2[i])
                return false;
        return v1.size() < v2.size();
    }
};


class NGramStorage: public Serializable {
public:
    NGramStorage();
    NGramStorage(vector<pair<vector<uint32_t>, uint32_t>>& ngrams);
    NGramStorage(string filename);

    void init(vector<pair<vector<uint32_t>, uint32_t>>& ngrams);

    void load(istream& in) override;
    void dump(ostream& out) const override;

    uint32_t get_ngram_count(const vector<uint32_t>& ngram);
    uint32_t get_continuations_count(const vector<uint32_t>& ngram);
    uint32_t get_unique_continuations_count(const vector<uint32_t>& ngram);

    uint8_t get_max_ngram_size() const;

    class const_iterator;

    const_iterator begin(uint8_t ngram_size);
    const_iterator end(uint8_t ngram_size);

    class const_iterator {
    public:
        const_iterator() = default;
        const_iterator(const NGramStorage* storage, uint8_t ngram_size);

        const_iterator operator++(int);
        const_iterator operator++();
        const pair<vector<uint32_t>, uint32_t>& operator*() const;
        const pair<vector<uint32_t>, uint32_t>* operator->() const;
        bool operator==(const const_iterator& other) const;
        bool operator!=(const const_iterator& other) const;

        friend class NGramStorage;

    private:
        const NGramStorage* storage;
        uint8_t ngram_size;
        vector<CompressedArray::const_iterator> cursor;
        pair<vector<uint32_t>, uint32_t> ngram;
    };

private:
    uint8_t max_ngram_size;
    vector<CompressedArray> storage;
    LRUCache<vector<uint32_t>, uint32_t, IntegerVectorHasher> cache;
    uint32_t empty_ngram_count;
    uint32_t empty_ngram_continuations_count;
    uint32_t empty_ngram_unique_continuations_count;

    void store_empty_ngram_values(const vector<pair<vector<uint32_t>, uint32_t>>& ngrams);
    void store_max_ngram_size(const vector<pair<vector<uint32_t>, uint32_t>>& ngrams);
    void sort_ngrams(vector<pair<vector<uint32_t>, uint32_t>>& ngrams) const;
    void build_storage(const vector<pair<vector<uint32_t>, uint32_t>>& sorted_ngrams);

    uint32_t get_context_index(const vector<uint32_t>& ngram);

    Record find_record(const vector<uint32_t>& ngram);
};


#endif //NGRAMSTORAGE_NGRAMSTORAGE_H
