//
// Created by pavel on 07.02.18.
//

#ifndef COMPACTNGRAMSTORAGE_CACHE_H
#define COMPACTNGRAMSTORAGE_CACHE_H

#include <list>
#include <unordered_map>

using namespace std;

template <class Key, class Value, class Hash=std::hash<Key>> class LRUCache {
public:
    LRUCache(size_t cache_size): cache_size(cache_size) {}

    void put(const Key& key, const Value& val) {
        auto it = item_map.find(key);
        if (it != item_map.end()) {
            item_list.erase(it->second);
            item_map.erase(it);
        }
        item_list.push_front(make_pair(key, val));
        item_map.insert(make_pair(key, item_list.begin()));
        clean();
    };

    bool exist(const Key& key) const {
        return item_map.count(key) > 0;
    };

    Value get(const Key& key) {
        auto it = item_map.find(key);
        item_list.splice(item_list.begin(), item_list, it->second);
        return it->second->second;
    };

private:
    list<pair<Key, Value>> item_list;
    unordered_map<Key, decltype(item_list.begin()), Hash> item_map;
    size_t cache_size;

    void clean() {
        while (item_map.size() > cache_size) {
            auto last_it = item_list.end(); last_it --;
            item_map.erase(last_it->first);
            item_list.pop_back();
        }
    };
};

#endif //COMPACTNGRAMSTORAGE_CACHE_H
