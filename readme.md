# Description
Implementation of a compact ngram storage described in 
[Faster and Smaller N-Gram Language Models](https://www.aclweb.org/anthology/P11-1027.pdf)

##### Features:
* Very compact in-memory NGramStorage. 2.9 bytes per ngram on Web1T while gzip takes 7 bytes.
* Constant query time.
* Vocabulary class that stores list of unique string in a compact way. 
Allows constant time access as well.
* Python wrappers for NGramStorage and Vocabulary.
* Implementation of statistical language model with Kneser-Ney smoothing
* Pickling is supported.
* Testing with googletest.

# Installation
[BBHash](https://github.com/rizkg/BBHash) and [googletest](https://github.com/google/googletest) 
are used as submodules, so initialization is required

    git submodule init
    git submodule update

Cython is used for wrapping C++ classes, so 

    pip3 install cython
    
After that

    cd python_package
    python3 setup.py install
    
# Usage example
file_with_ngrams.txt:

    2 a b c
    1 a b d
    1 a b
    3 a d
    2 c b a
    

Creating storage:

    >>> from ngram_storage import CStorage
    >>> storage = CStorage('file_with_ngrams.txt')
    
Getting ngram count:

    >>> storage.get_ngram_count(('a', 'b', 'c'))
    2
    >>> storage.get_ngram_count(('a', 'b'))
    4
    >>> storage.get_continuations_count(('a', 'b'))  # 'c' 2 times, 'd' once
    3
    >>> storage.get_unique_continuations_count(('a', 'b'))  # 'c', 'd'
    2
    >>> storage.get_continuations_count(('a'))  # 'b' 4 times, 'c' 3 times
    7
    >>> storage.get_unique_continuations_count(('a'))  # 'b', 'c'
    2
    
List of stored ngrams:
    
    >>> list(storage.get_ngrams(ngram_size=3, return_count=True))
    [(['a', 'b', 'c'], 2), (['c', 'b', 'a'], 2), (['a', 'b', 'd'], 1)]
    >>> list(storage.get_ngrams(ngram_size=2, return_count=True))
    [(['c', 'b'], 2), (['a', 'b'], 4), (['a', 'd'], 3)]
    
List of words:

    >>> list(storage.get_words())
    ['c', 'b', 'a', 'd']
    
Saving/loading:

    >>> import pickle
    >>> pickle.dump(storage, open('storage.pkl', 'wb'))
    >>> storage = pickle.load(open('storage.pkl', 'rb'))
    
Additional examples could be seen in language_model.py