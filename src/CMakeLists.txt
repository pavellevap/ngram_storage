cmake_minimum_required(VERSION 3.7)

include_directories(BBHash)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wsign-compare -O2")
set(CMAKE_CXX_LINKER_FLAGS "-Wall -Wextra -Wsign-compare -O2")
set(SOURCE_FILES NGramStorage.cpp NGramStorage.h CompressedArray.cpp CompressedArray.h
        Record.h Serializable.h Cache.h Vocabulary.h)

add_library(ngram_storage ${SOURCE_FILES})