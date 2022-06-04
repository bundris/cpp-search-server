#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    //идем по мапе слово -> индексы. Составляем сеты ind1 : ind2, ind3...., чтобы было возрастание индексов
    using namespace std;
    set<set<string>> uniq;
    set<int> duplicates;
    map<string, double> word_to_rate;
    for (const int doc_id: search_server) {
        word_to_rate = search_server.GetWordFrequencies(doc_id);   //key - word, value - rate
        set<string> mapper;
        for (auto& [word, _]: word_to_rate) {
            mapper.insert(word);
        }
        if (uniq.count(mapper)) {
            duplicates.insert(doc_id);
        } else {
            uniq.insert(mapper);
        }
    }
    for (int id: duplicates) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id "s << id << endl;
    }
}