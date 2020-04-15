//
// Created by pavel on 28.12.17.
//

#include "gtest/gtest.h"
#include "CompressedArray.h"

#include <sstream>

using namespace std;

bool check_same(const Record& record, const CompressedArray::const_iterator& it) {
    bool same = true;
    same &= it->key.context_index == record.key.context_index;
    same &= it->key.word_index == record.key.word_index;
    same &= it->value.ngram_count == record.value.ngram_count;
    return same;
}

bool check_same(const vector<Record>& records, const CompressedArray& array) {
    bool same = true;
    for (auto it = array.begin(); it != array.end(); it++) {
        uint32_t index = uint32_t(it - array.begin());
        same &= check_same(records[index], it);
    }
    return same;
}

bool search(const vector<Record>& records, const CompressedArray& array) {
    bool found = true;
    for (uint32_t i = 0; i < records.size(); i++)
        found &= (array.find(records[i].key) - array.begin()) == i;
    return found;
}

vector<Record> create_records_1() {
    vector<Record> records;
    for (uint32_t i = 0; i < 2000; i += 2)
        records.push_back(Record(Key(i, i), Value(i, i, i)));
    return records;
}

vector<Record> create_records_2() {
    vector<Record> records;
    for (uint32_t i = 0; i < 2000; i += 2)
        records.push_back(Record(Key(0, i), Value(i, i, i)));
    return records;
}

vector<Record> create_records_3() {
    vector<Record> records;
    for (uint32_t i = 0; i < 200; i += 2)
        for (uint32_t j = 0; j < 20; j += 2)
            records.push_back(Record(Key(i, j), Value(i, i, i)));
    return records;
}

TEST(compressed_array_check, size_check) {
    vector<Record> records;

    records = create_records_1();
    ASSERT_EQ(records.size(), CompressedArray(records).size());

    records = create_records_2();
    ASSERT_EQ(records.size(), CompressedArray(records).size());

    records = create_records_3();
    ASSERT_EQ(records.size(), CompressedArray(records).size());
}

TEST(compressed_array_check, content_check) {
    vector<Record> records;

    records = create_records_1();
    ASSERT_TRUE(check_same(records, CompressedArray(records)));

    records = create_records_2();
    ASSERT_TRUE(check_same(records, CompressedArray(records)));

    records = create_records_3();
    ASSERT_TRUE(check_same(records, CompressedArray(records)));
}

TEST(compressed_array_check, find_check) {
    vector<Record> records;

    records = create_records_1();
    CompressedArray array1(records);
    ASSERT_TRUE(search(records, array1));
    ASSERT_TRUE(array1.find(Key(0, 1)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1000, 1)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1998, 1)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1, 1)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1001, 1)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1999, 1)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1, 0)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1001, 0)) == array1.end());
    ASSERT_TRUE(array1.find(Key(1999, 0)) == array1.end());

    records = create_records_2();
    CompressedArray array2(records);
    ASSERT_TRUE(search(records, array2));
    ASSERT_TRUE(array2.find(Key(0, 1)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1000, 1)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1998, 1)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1, 1)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1001, 1)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1999, 1)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1, 0)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1001, 0)) == array2.end());
    ASSERT_TRUE(array2.find(Key(1999, 0)) == array2.end());

    records = create_records_3();
    CompressedArray array3(records);
    ASSERT_TRUE(search(records, array3));
    ASSERT_TRUE(array3.find(Key(0, 1)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1000, 1)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1998, 1)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1, 1)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1001, 1)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1999, 1)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1, 0)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1001, 0)) == array3.end());
    ASSERT_TRUE(array3.find(Key(1999, 0)) == array3.end());
}

TEST(compressed_array_check, save_load_check) {
    vector<Record> records;

    records = create_records_1();
    CompressedArray array1(records);
    array1.loads(array1.dumps());
    ASSERT_TRUE(check_same(records, array1));

    records = create_records_2();
    CompressedArray array2(records);
    array2.loads(array2.dumps());
    ASSERT_TRUE(check_same(records, array2));

    records = create_records_3();
    CompressedArray array3(records);
    array3.loads(array3.dumps());
    ASSERT_TRUE(check_same(records, array3));
}

TEST(compressed_array_check, random_access_check) {
    vector<Record> records;

    records = create_records_1();
    CompressedArray array1(records);
    for (uint32_t i = 0; i < records.size(); i++)
        ASSERT_TRUE(check_same(records[i], array1.begin() + i));

    records = create_records_2();
    CompressedArray array2(records);
    for (uint32_t i = 0; i < records.size(); i++)
        ASSERT_TRUE(check_same(records[i], array2.begin() + i));

    records = create_records_3();
    CompressedArray array3(records);
    for (uint32_t i = 0; i < records.size(); i++)
        ASSERT_TRUE(check_same(records[i], array3.begin() + i));
}