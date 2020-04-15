from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp cimport bool
from cython.operator cimport dereference, preincrement


ctypedef unsigned int uint

cdef extern from "../src/Vocabulary.h":
    cdef cppclass Serializable:
        pass

    cdef cppclass Vocabulary[string](Serializable):
        Vocabulary()
        Vocabulary(const vector[string]& words) nogil

        void loads(const string& state) nogil
        string dumps() nogil const

        void loadf(const string& filename) nogil
        void dumpf(const string& filename) nogil const

        uint get_index(const string& word) const
        const string& get_word(uint index) const
        uint size() const
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


cdef class CVocabulary:
    cdef Vocabulary[string] vocabulary
    cdef object encoding

    def __init__(self, words, encoding='utf-8'):
        self.encoding=encoding
        cdef vector[string] cwords = [word.encode(encoding) for word in words]
        with nogil:
            self.vocabulary = Vocabulary[string](cwords)

    def get_index(self, word):
        index = self.vocabulary.find(word.encode(self.encoding)) - self.vocabulary.begin()
        return index if index < self.vocabulary.size() else -1

    def get_word(self, index):
        return bytes(self.vocabulary.get_word(index)).decode(self.encoding)

    def get_words(self):
        begin = self.vocabulary.begin()
        end = self.vocabulary.end()
        while begin != end:
            yield bytes(dereference(begin)).decode(self.encoding)
            preincrement(begin)

    def size(self):
        return self.vocabulary.size()

    def save(self, filename):
        self.vocabulary.dumpf(filename.encode(self.encoding))

    def load(self, filename):
        self.vocabulary.loadf(filename.encode(self.encoding))

    def __getitem__(self, index):
        return bytes(self.vocabulary[index]).decode(self.encoding)

    def __contains__(self, word):
        return self.vocabulary.find(word.encode(self.encoding)) != self.vocabulary.end()

    def __getstate__(self):
        cdef string vocabulary_dump
        with nogil:
            vocabulary_dump = self.vocabulary.dumps()
        state = {'vocabulary': vocabulary_dump,
                 'encoding': self.encoding}
        return state

    def __setstate__(self, state):
        cdef string vocabulary_dump = state['vocabulary']
        self.encoding = state['encoding']
        with nogil:
            self.vocabulary.loads(vocabulary_dump)
