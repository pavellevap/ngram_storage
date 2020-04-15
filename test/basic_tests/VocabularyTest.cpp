//
// Created by pavel on 07.08.18.
//

#include "gtest/gtest.h"
#include "NGramStorage.h"

#include <sstream>

using namespace std;

uint64_t seed = 0;
uint64_t prng() {
    seed = (seed * 123456789 + 12345);
    return seed;
}

TEST(vocabulary_test, content_check) {
    vector<string> words;
    unordered_set<string> source_words;
    for (int i = 0; i < 10000; i++) {
        string word;
        for (int j = 0; j < 3; j++)
            word.push_back(char('a' + prng() % 26));
        words.push_back(word);
        source_words.insert(word);
    }

    Vocabulary<string> vocab(words);
    Vocabulary<string> vocab2;
    vocab2.loads(vocab.dumps());
    unordered_set<string> target_words;
    for (string word : vocab2)
        target_words.insert(word);

    ASSERT_TRUE(source_words == target_words);
};

TEST(vocabulary_test, index_check) {
    vector<string> words;
    for (int i = 0; i < 10000; i++) {
        string word;
        for (int j = 0; j < 3; j++)
            word.push_back(char('a' + prng() % 26));
        words.push_back(word);
    }

    Vocabulary<string> vocab(words);
    Vocabulary<string> vocab2;
    vocab2.loads(vocab.dumps());
    unordered_set<uint32_t> indices;
    for (string word : vocab2) {
        uint32_t index = vocab2.get_index(word);
        ASSERT_TRUE(indices.count(index) == 0);
        indices.insert(index);
        ASSERT_TRUE(word == vocab2[index]);
    }
};

TEST(vocabulary_test, serialization_test) {
    vector<string> words;
    for (int i = 0; i < 10000; i++) {
        string word;
        for (int j = 0; j < 3; j++)
            word.push_back(char('a' + prng() % 26));
        words.push_back(word);
    }

    Vocabulary<string> vocab(words);
    Vocabulary<string> vocab2;
    vocab2.loads(vocab.dumps());
    auto it1 = vocab.begin();
    auto it2 = vocab2.begin();
    while (it1 != vocab.end()) {
        ASSERT_TRUE(*it1 == *it2);
        it1++;
        it2++;
    }
    ASSERT_TRUE(it2 == vocab2.end());

    words.clear();
    vocab = Vocabulary<string>(words);
    vocab2.loads(vocab.dumps());
    ASSERT_TRUE(vocab2.begin() == vocab2.end());
};
