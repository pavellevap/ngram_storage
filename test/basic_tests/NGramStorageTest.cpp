//
// Created by pavel on 31.12.17.
//

#include "gtest/gtest.h"
#include "NGramStorage.h"

#include <sstream>

using namespace std;

uint32_t get_ngram_count(vector<pair<vector<uint32_t>, uint32_t>>& ngrams,
                         vector<uint32_t> ngram) {
    uint32_t res = 0;
    for (uint32_t i = 0; i < ngrams.size(); i++) {
        if (ngrams[i].first.size() >= ngram.size()) {
            bool same = true;
            for (uint32_t j = 0; j < ngram.size(); j++)
                same &= ngrams[i].first[j] == ngram[j];
            if (same)
                res += ngrams[i].second;
        }
    }

    return res;
}

uint32_t get_continuations_count(vector<pair<vector<uint32_t>, uint32_t>>& ngrams,
                                 vector<uint32_t> ngram) {
    uint32_t res = 0;
    for (uint32_t i = 0; i < ngrams.size(); i++) {
        if (ngrams[i].first.size() > ngram.size()) {
            bool same = true;
            for (uint32_t j = 0; j < ngram.size(); j++)
                same &= ngrams[i].first[j] == ngram[j];
            if (same)
                res += ngrams[i].second;
        }
    }

    return res;
}

uint32_t get_unique_continuations_count(vector<pair<vector<uint32_t>, uint32_t>>& ngrams,
                                        vector<uint32_t> ngram) {
    set<uint32_t> continuations;
    for (uint32_t i = 0; i < ngrams.size(); i++) {
        if (ngrams[i].first.size() > ngram.size()) {
            bool same = true;
            for (uint32_t j = 0; j < ngram.size(); j++)
                same &= ngrams[i].first[j] == ngram[j];
            if (same)
                continuations.insert(ngrams[i].first[ngram.size()]);
        }
    }

    return uint32_t(continuations.size());
}

uint64_t seed = 0;
uint64_t prng() {
    seed = (seed * 123456789 + 12345);
    return seed;
}


TEST(ngram_storage_check, content_check) {
    vector<pair<vector<uint32_t>, uint32_t>> ngrams;

    for (int i = 0; i < 10000; i++) {
        vector<uint32_t> ngram;
        for (int j = 0; j < 3; j++)
            ngram.push_back(uint32_t(prng() % 26));
        ngrams.push_back(make_pair(ngram, prng() % 10 + 1));
    }

    NGramStorage storage(ngrams);
    NGramStorage storage2;
    storage2.loads(storage.dumps());

    for (uint32_t i = 0; i < ngrams.size(); i++) {
        vector<uint32_t> ngram = ngrams[i].first;
        for (uint32_t j = 0; j < ngrams[i].first.size(); j++) {
            ASSERT_EQ(storage2.get_ngram_count(ngram), get_ngram_count(ngrams, ngram));
            ASSERT_EQ(storage2.get_continuations_count(ngram), get_continuations_count(ngrams, ngram));
            ASSERT_EQ(storage2.get_unique_continuations_count(ngram), get_unique_continuations_count(ngrams, ngram));
            ngram.pop_back();
        }
    }

    for (int i = 0; i < 10000; i++) {
        vector<uint32_t> ngram;
        for (int j = 0; j < 3; j++) {
            ngram.push_back(uint32_t(prng() % 26));
            ASSERT_EQ(storage2.get_ngram_count(ngram), get_ngram_count(ngrams, ngram));
            ASSERT_EQ(storage2.get_continuations_count(ngram), get_continuations_count(ngrams, ngram));
            ASSERT_EQ(storage2.get_unique_continuations_count(ngram), get_unique_continuations_count(ngrams, ngram));
        }
    }
}

TEST(ngram_storage_check, iterator_check) {
    vector<pair<vector<uint32_t>, uint32_t>> ngrams;
    unordered_set<vector<uint32_t>, IntegerVectorHasher> source_ngrams;
    for (int i = 0; i < 10000; i++) {
        vector<uint32_t> ngram;
        for (int j = 0; j < 3; j++)
            ngram.push_back(uint32_t(prng() % 26));
        ngrams.push_back(make_pair(ngram, prng() % 10 + 1));
        source_ngrams.insert(ngram);
    }

    int i = 0;
    NGramStorage storage(ngrams);
    unordered_set<vector<uint32_t>, IntegerVectorHasher> target_ngrams;
    for (auto it = storage.begin(3); it != storage.end(3); it++) {
        target_ngrams.insert(it->first);
        i++;
    }

    ASSERT_TRUE(source_ngrams == target_ngrams);
}






