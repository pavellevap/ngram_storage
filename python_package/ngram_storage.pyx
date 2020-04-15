from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.pair cimport pair
from libcpp cimport bool
from cython.operator cimport dereference, preincrement

import sys
import tempfile


ctypedef unsigned int uint
ctypedef unsigned char uchar

cdef extern from "../src/Vocabulary.h":
    cdef cppclass Serializable:
        pass

    cdef cppclass Vocabulary[string](Serializable):
        Vocabulary()
        Vocabulary(const vector[string]& words) nogil

        void loads(const string& state) nogil
        string dumps() nogil const

        uint get_index(const string& word) const
        const string& get_word(uint index) const
        const string& operator [] (uint index) const

        cppclass const_iterator:
            const_iterator operator++(int)
            const_iterator operator++()
            const_iterator operator--()
            const_iterator operator--(int)
            const_iterator operator + (uint n) const
            const_iterator operator - (uint n) const
            uint operator - (const const_iterator& other) const
            string operator*() const
            bool operator==(const const_iterator& other) const
            bool operator!=(const const_iterator& other) const
            bool operator > (const const_iterator& other) const
            bool operator >= (const const_iterator& other) const
            bool operator < (const const_iterator& other) const
            bool operator <= (const const_iterator& other) const

        const_iterator begin() const
        const_iterator end() const
        const_iterator find(const string& word) const


cdef extern from "../src/NGramStorage.h":
    cdef cppclass Serializable:
        pass

    cdef cppclass NGramStorage(Serializable):
        NGramStorage()
        NGramStorage(vector[pair[vector[uint], uint]]& ngrams) nogil
        NGramStorage(string filename) nogil

        void loads(const string& state) nogil
        string dumps() nogil const

        uint get_ngram_count(const vector[uint]& ngram) const
        uint get_continuations_count(const vector[uint]& ngram) const
        uint get_unique_continuations_count(const vector[uint]& ngram) const

        uchar get_max_ngram_size() const

        cppclass const_iterator:
            const_iterator operator++()
            const_iterator operator++(int)
            const pair[vector[uint], uint]& operator*() const
            bool operator==(const const_iterator& other) const
            bool operator!=(const const_iterator& other) const

        const_iterator begin(int ngram_size) const;
        const_iterator end(int ngram_size) const;


cdef class CStorage:
    cdef NGramStorage storage
    cdef Vocabulary[string] vocabulary
    cdef object encoding

    def __init__(self, filename):
        self.encoding = 'utf-8'

        ngrams_count = 0
        words = set()
        with open(filename, 'r') as infile:
            for line in infile:
                ngrams_count += 1
                count, ngram = line.strip().split(' ', 1)
                for word in ngram.split(' '):
                    words.add(word.encode(self.encoding))

        cdef vector[string] cwords = list(words)
        with nogil:
            self.vocabulary = Vocabulary[string](cwords)

        cdef string cfilename
        with tempfile.TemporaryDirectory() as tmpdir:
            with open(tmpdir + '/encoded_ngrams', 'wb') as outfile:
                with open(filename, 'r') as infile:
                    outfile.write(ngrams_count.to_bytes(length=8, byteorder=sys.byteorder))
                    for line in infile:
                        count, ngram = line.strip().split(' ', 1)
                        count = int(count)
                        ngram = ngram.split(' ')
                        outfile.write(count.to_bytes(length=4, byteorder=sys.byteorder))
                        outfile.write(len(ngram).to_bytes(length=1, byteorder=sys.byteorder))
                        for index in self._encode_ngram(ngram):
                            outfile.write(index.to_bytes(length=4, byteorder=sys.byteorder))

            cfilename = (tmpdir + '/encoded_ngrams').encode(self.encoding)
            with nogil:
                self.storage = NGramStorage(cfilename)

    def get_ngram_count(self, ngram):
        try:
            return self.storage.get_ngram_count(self._encode_ngram(ngram))
        except KeyError:
            return 0

    def get_continuations_count(self, ngram):
        try:
            return self.storage.get_continuations_count(self._encode_ngram(ngram))
        except KeyError:
            return 0

    def get_unique_continuations_count(self, ngram):
        try:
            return self.storage.get_unique_continuations_count(self._encode_ngram(ngram))
        except KeyError:
            return 0

    def get_max_ngram_size(self):
        return self.storage.get_max_ngram_size()

    def get_ngrams(self, ngram_size, return_count=False):
        if ngram_size <= 0 or ngram_size > self.get_max_ngram_size():
            return 0
        begin = self.storage.begin(ngram_size)
        end = self.storage.end(ngram_size)
        while begin != end:
            ngram, count = dereference(begin)
            if return_count:
                yield self._decode_ngram(ngram), count
            else:
                yield self._decode_ngram(ngram)
            preincrement(begin)

    def get_words(self):
        begin = self.vocabulary.begin()
        end = self.vocabulary.end()
        while begin != end:
            yield bytes(dereference(begin)).decode(self.encoding)
            preincrement(begin)

    def _encode_ngram(self, ngram):
        encoded_ngram = []
        for word in ngram:
            encoded_word = word.encode(self.encoding)
            it = self.vocabulary.find(encoded_word)
            if it == self.vocabulary.end():
                raise KeyError()
            encoded_ngram.append(it - self.vocabulary.begin())
        return encoded_ngram

    def _decode_ngram(self, ngram):
        decoded_ngram = []
        for index in ngram:
            decoded_ngram.append(bytes(self.vocabulary.get_word(index)).decode(self.encoding))
        return decoded_ngram

    def __getitem__(self, ngram):
        return self.get_ngram_count(ngram)

    def __contains__(self, ngram):
        if type(ngram) == str:
            return self.vocabulary.find(ngram.encode(self.encoding)) != self.vocabulary.end()
        else:
            return self.get_ngram_count(ngram) > 0

    def __getstate__(self):
        cdef string storage_dump
        cdef string vocabulary_dump
        with nogil:
            storage_dump = self.storage.dumps()
            vocabulary_dump = self.vocabulary.dumps()
        state = {'storage': storage_dump,
                 'vocabulary': vocabulary_dump,
                 'encoding': self.encoding}
        return state

    def __setstate__(self, state):
        cdef string storage_dump = state['storage']
        cdef string vocabulary_dump = state['vocabulary']
        self.encoding = state['encoding']
        with nogil:
            self.storage.loads(storage_dump)
            self.vocabulary.loads(vocabulary_dump)
