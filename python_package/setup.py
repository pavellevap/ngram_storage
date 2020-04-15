from Cython.Build import cythonize
from setuptools import setup, Extension

extension = Extension('ngram_storage', sources=['ngram_storage.pyx', '../src/NGramStorage.cpp', '../src/CompressedArray.cpp'],
                      language='c++', extra_compile_args=['--std=c++11'], extra_link_args=['--std=c++11'])
setup(name='ngram_storage', ext_modules=cythonize(extension, language_level="3"))

extension = Extension('vocabulary', sources=['vocabulary.pyx'],
                      language='c++', extra_compile_args=['--std=c++11'], extra_link_args=['--std=c++11'])
setup(name='vocabulary', ext_modules=cythonize(extension, language_level="3"))
