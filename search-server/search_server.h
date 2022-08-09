#pragma once
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <algorithm>
#include <execution>
#include <stdexcept>
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ACCURACY_THRESHOLD = 1e-6;

class SearchServer {
public:
    //constructors
    template <class StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);
    //document operation methods
    void AddDocument(int document_id, std::string_view document,
                     DocumentStatus status, const std::vector<int>& ratings);
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);
    void RemoveDocument(int document_id);

    //search documents
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    //iterators and getters
    int GetDocumentCount() const;
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
            std::execution::parallel_policy policy,
            const std::string_view& raw_query,
            int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
            std::execution::sequenced_policy policy,
            const std::string_view& raw_query,
            int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
            const std::string_view& raw_query,
            int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::set<std::string> doc_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;

    bool IsStopWord(const std::string_view& word) const;
    static bool IsValidWord(const std::string_view& word);
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);
    QueryWord ParseQueryWord(const std::string_view& text) const;

    Query ParseQuery(std::execution::parallel_policy policy, const std::string_view& text) const;
    Query ParseQuery(std::execution::sequenced_policy policy, const std::string_view& text) const;
    Query ParseQuery(const std::string_view& text) const;
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

//class template methods/constructors
template <class StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    using std::string_literals::operator""s;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < ACCURACY_THRESHOLD) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

//out of class functions

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings);
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query);
void MatchDocuments(const SearchServer& search_server, const std::string& query);
