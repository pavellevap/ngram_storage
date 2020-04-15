include_directories("${gtest_SOURCE_DIR}/include"
                    "${gtest_SOURCE_DIR}")

add_executable(run_compressed_array_test CompressedArrayTest.cpp)
target_link_libraries(run_compressed_array_test gtest gtest_main)
target_link_libraries(run_compressed_array_test ngram_storage)

add_executable(run_ngram_storage_test NGramStorageTest.cpp)
target_link_libraries(run_ngram_storage_test gtest gtest_main)
target_link_libraries(run_ngram_storage_test ngram_storage)

add_executable(run_vocabulary_test VocabularyTest.cpp)
target_link_libraries(run_vocabulary_test gtest gtest_main)
target_link_libraries(run_vocabulary_test ngram_storage)
