#include <iostream>

#include "patch_tree_key_comparator.h"

// TODO: see if we can use the built-in comparator instead, then we can avoid deserializing each time
int32_t PatchTreeKeyComparator::compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    PatchTreeKey element1;
    PatchTreeKey element2;
    element1.deserialize(akbuf, aksiz);
    element2.deserialize(bkbuf, bksiz);
    int comp_subject = element1.get_subject().compare(element2.get_subject());
    //cout << "comp: " << element1.to_string() << " ? " << element2.to_string() << endl; // TODO
    if(!comp_subject) {
        int comp_predicate = element1.get_predicate().compare(element2.get_predicate());
        if(!comp_predicate) {
            return element1.get_object().compare(element2.get_object());
        }
        return comp_predicate;
    }
    return comp_subject;
};

int32_t PatchTreeKeyComparator::compare(PatchTreeKey key1, PatchTreeKey key2) {
    size_t key1_size, key2_size;
    const char* key1_data = key1.serialize(&key1_size);
    const char* key2_data = key2.serialize(&key2_size);
    int ret = compare(key1_data, key1_size, key2_data, key2_size);
    free((char*) key1_data);
    free((char*) key2_data);
    return ret;
}
