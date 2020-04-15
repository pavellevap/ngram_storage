//
// Created by pavel on 06.08.18.
//

#ifndef VOCABULARY_H
#define VOCABULARY_H

#include "Serializable.h"
#include "BBHash/BooPHF.h"

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>
#include <iostream>

using std::string;
using std::vector;
using std::sort;
using std::unique;
using std::lower_bound;
using std::unordered_set;
using std::unordered_map;
using std::pair;
using std::distance;
using std::make_pair;
using std::cout;
using std::endl;
using std::copy;

typedef boomphf::SingleHashFunctor<uint64_t>  Hasher;
typedef boomphf::mphf<uint64_t, Hasher> BooPHF;


class NotFoundException: public std::exception {
public:
    NotFoundException(string name) {
        this->error_message = name + " is not found";
    }

    virtual const char* what() const throw() {
        return error_message.c_str();
    }

private:
    string error_message;
};


template <class PrimitiveType>
class Vocabulary: public Serializable {
public:
    Vocabulary() {}

    Vocabulary(const vector<PrimitiveType>& words): words(words) {
        assert(words.size() < (~uint32_t(0)));
        sort(this->words.begin(), this->words.end());
        auto words_end = unique(this->words.begin(), this->words.end());
        vector<PrimitiveType>(this->words.begin(), words_end).swap(this->words);
    }

    void dump(ostream& out) const override {
        uint32_t size = uint32_t(words.size());
        out.write((char*)(&size), sizeof(size));
        for (uint32_t i = 0; i < size; i++)
            out.write((char*)(&words[i]), sizeof(words[i]));
    }

    void load(istream& in) override {
        uint32_t size;
        in.read((char*)(&size), sizeof(size));
        words.resize(size);
        for (uint32_t i = 0; i < size; i++)
            in.read((char*)(&words[i]), sizeof(words[i]));
    }

    uint32_t get_index(const PrimitiveType& word) const {
        auto it = lower_bound(words.begin(), words.end(), word);
        if (it == words.end() || *it != word)
            throw NotFoundException("word");
        return uint32_t(distance(words.begin(), it));
    }

    const PrimitiveType& get_word(uint32_t index) const {
        return words[index];
    }

    uint32_t size() const {
        return words.size();
    }

    const PrimitiveType& operator [] (uint32_t index) const {
        return get_word(index);
    }

    typename vector<PrimitiveType>::const_iterator begin() const {
        return words.begin();
    }

    typename vector<PrimitiveType>::const_iterator end() const {
        return words.end();
    }

    typename vector<PrimitiveType>::const_iterator find(const PrimitiveType& word) const {
        auto it = lower_bound(begin(), end(), word);
        if (it != end() && *it == word)
            return it;
        else
            return end();
    }

private:
    vector<PrimitiveType> words;
};


template <>
class Vocabulary<string>: public Serializable {
public:
    Vocabulary() {}

    Vocabulary(const vector<string> &words) {
        vector<string> unique_words(words);
        sort(unique_words.begin(), unique_words.end());
        auto it = unique(unique_words.begin(), unique_words.end());
        unique_words.resize(size_t(it - unique_words.begin()));

        assert(unique_words.size() < (~uint32_t(0)));

        std::hash<string> hasher;
        vector<uint64_t> hashes(unique_words.size());
        for (size_t i = 0; i < unique_words.size(); i++)
            hashes[i] = hasher(unique_words[i]);
        if (hashes.size() == 0) // mphf cannot be serialized if key set is empty
            hashes.push_back(ULLONG_MAX);
        mphf = BooPHF(hashes.size(), hashes, 8, 2.0, true, false);

        uint64_t data_size = 0;
        vector<string> reordered_words(unique_words.size());
        for (size_t i = 0; i < unique_words.size(); i++) {
            reordered_words[mphf.lookup(hashes[i])] = unique_words[i];
            data_size += unique_words[i].length();
        }
        unique_words.clear();
        hashes.clear();

        assert(data_size < (~uint32_t(0)));

        data.resize(data_size);
        auto data_end = data.begin();
        offsets.resize(reordered_words.size() + 1);
        for (size_t i = 0; i < reordered_words.size(); i++) {
            offsets[i] = uint32_t(data_end - data.begin());
            data_end = copy(reordered_words[i].begin(), reordered_words[i].end(), data_end);
        }
        offsets[reordered_words.size()] = uint32_t(data_end - data.begin());
    }

    void dump(ostream &out) const override {
        uint32_t data_size = uint32_t(data.length());
        out.write((char *) (&data_size), sizeof(data_size));
        out.write(data.data(), data_size);

        uint32_t offsets_size = uint32_t(offsets.size());
        out.write((char *) (&offsets_size), sizeof(offsets_size));
        out.write((char *) (offsets.data()), offsets_size * sizeof(decltype(*offsets.begin())));

        mphf.save(out);
    }

    void load(istream &in) override {
        uint32_t data_size;
        in.read((char *) (&data_size), sizeof(data_size));
        char *data_buffer = new char[data_size];
        in.read(data_buffer, data_size);
        data = string(data_buffer, data_size);
        delete[] data_buffer;

        uint32_t offsets_size;
        in.read((char *) (&offsets_size), sizeof(offsets_size));
        uint32_t *offsets_buffer = new uint32_t[offsets_size];
        in.read((char *) offsets_buffer, offsets_size * sizeof(uint32_t));
        offsets = vector<uint32_t>(offsets_buffer, offsets_buffer + offsets_size);
        delete[] offsets_buffer;
        offsets.shrink_to_fit();

        mphf.load(in);
    }

    uint32_t get_index(const string &word) {
        const_iterator it(find(word));
        if (it == end())
            throw NotFoundException("word \"" + word + "\"");
        return uint32_t(it - begin());
    }

    string get_word(uint32_t index) const {
        return string(data.data() + offsets[index], offsets[index + 1] - offsets[index]);
    }

    uint32_t size() const {
        return offsets.size() - 1;
    }

    string operator[](uint32_t index) const {
        return get_word(index);
    }

    class const_iterator;

    const_iterator begin() const {
        return const_iterator(this);
    }

    const_iterator end() const {
        const_iterator it(this);
        it.index = size();
        return it;
    }

    const_iterator find(const string &word) {
        uint64_t index = mphf.lookup(std::hash<string>()(word));

        if (index == ULLONG_MAX)
            return end();
        if (string(data.data() + offsets[index], offsets[index + 1] - offsets[index]) != word)
            return end();

        return begin() + index;
    }

    class const_iterator {
    public:
        const_iterator() = default;

        const_iterator(const Vocabulary *vocabulary) : vocabulary(vocabulary), index(0) {}

        const_iterator operator++(int) {
            const_iterator res(*this);
            ++(*this);
            return res;
        }

        const_iterator operator++() {
            if (index == vocabulary->offsets.size() - 1)
                return *this;
            index += 1;
            return *this;
        }

        const_iterator operator--() {
            if (index == 0)
                return *this;
            const_iterator res(*this);
            index -= 1;
            return res;
        }

        const_iterator operator--(int) {
            if (index == 0)
                return *this;
            index -= 1;
            return *this;
        }

        const_iterator operator+(uint32_t n) const {
            const_iterator it(*this);
            it.index += n;
            return it;
        }

        const_iterator operator+=(uint32_t n) {
            index += n;
            return *this;
        }

        const_iterator operator-(uint32_t n) const {
            const_iterator it(*this);
            it.index -= n;
            return it;
        }

        const_iterator operator-=(uint32_t n) {
            index -= n;
            return *this;
        }

        int32_t operator-(const const_iterator &other) const {
            return index - other.index;
        }

        string operator*() const {
            return vocabulary->get_word(index);
        }

        bool operator==(const const_iterator &other) const {
            return index == other.index;
        }

        bool operator!=(const const_iterator &other) const {
            return index != other.index;
        }

        bool operator>(const const_iterator &other) const {
            return index > other.index;
        }

        bool operator>=(const const_iterator &other) const {
            return index >= other.index;
        }

        bool operator<(const const_iterator &other) const {
            return index < other.index;
        }

        bool operator<=(const const_iterator &other) const {
            return index <= other.index;
        }

        friend class Vocabulary;

    private:
        const Vocabulary *vocabulary;
        uint32_t index;
    };

private:
    string data;
    vector<uint32_t> offsets;
    BooPHF mphf;
};


#endif //VOCABULARY_H
