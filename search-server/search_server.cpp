#include <numeric>
#include <utility>
#include <iostream>
#include <iterator>
#include "search_server.h"

using std::string_literals::operator""s;

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer::SearchServer(SplitIntoWords(stop_words_text)) {}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count; //new index id->words
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

//remove document
void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }
    auto iter = std::find(policy,
                          document_ids_.begin(), document_ids_.end(),
                          document_id); // параллельно быстро ищем document_id
    if (iter == document_ids_.end()) {
        return;
    }
    documents_.erase(document_id);
    document_ids_.erase(iter);

    // удаляем упоминания в word_to_document_freqs_
    // сохраним слова, по которым надо итерироваться
    auto& words_to_freqs = document_to_word_freqs_.at(document_id);
    std::vector<const std::string*> words; // все ради итераторов произвольного доступа
    words.reserve(words_to_freqs.size());

    std::transform(
            policy,
            words_to_freqs.begin(), words_to_freqs.end(),
            words.begin(),
            [&](const auto& word) {
                return &word.first;
            });
    std::for_each(policy,
                  words.begin(), words.end(),
                  [&](const auto& word){
            word_to_document_freqs_[*word].erase(document_id);
    });

    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }
    auto iter = document_ids_.find(document_id); // параллельно быстро ищем document_id
    if (iter == document_ids_.end()) {
        return;
    }
    documents_.erase(document_id);
    document_ids_.erase(iter);

    std::map<std::string, double>& words_to_freqs = document_to_word_freqs_.at(document_id);
    for (const auto& word: words_to_freqs) {
        word_to_document_freqs_[word.first].erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id) {
    RemoveDocument(document_id);
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string, double> dummy;
    if (document_to_word_freqs_.count(document_id) == 0) {
        return dummy;
    }
    return document_to_word_freqs_.at(document_id);
}



std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
        std::execution::parallel_policy policy,
        const std::string& raw_query,
        int document_id) const {
    const Query query = ParseQuery(policy, raw_query);
    const auto& docwords = document_to_word_freqs_.at(document_id);
    const auto& mw = query.minus_words;
    bool hasMinusWord = std::any_of(
            policy, docwords.begin(), docwords.end(),
            [&](const auto& par){
                return std::find(mw.begin(), mw.end(),par.first) != mw.end();
            });
    if (hasMinusWord){
        return {{}, documents_.at(document_id).status};
    }
    const auto& pw = query.plus_words;

    std::vector<std::string> matched_words(pw.size());
    auto cop = std::copy_if(policy,
                   pw.begin(), pw.end(),
                   matched_words.begin(),
                   [&](const auto& w){
                        return docwords.count(w) > 0;
    });
    matched_words.erase(cop, matched_words.end());
    std::sort(matched_words.begin(), matched_words.end());
    cop = std::unique(policy, matched_words.begin(), matched_words.end());
    matched_words.erase(cop, matched_words.end());
    matched_words.shrink_to_fit();
    return {matched_words, documents_.at(document_id).status};
}


std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
        const std::string& raw_query,
        int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) > 0) {
            if (word_to_document_freqs_.at(word).count(document_id) > 0) {
                return {matched_words, documents_.at(document_id).status};
            }
        }
    }
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) > 0) {
            if (word_to_document_freqs_.at(word).count(document_id) > 0) {
                matched_words.push_back(word);
            }
        }
    }

    auto cop = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(cop, matched_words.end());
    matched_words.shrink_to_fit();
    std::sort(matched_words.begin(), matched_words.end());
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
        std::execution::sequenced_policy policy,
        const std::string& raw_query,
        int document_id) const {
    return MatchDocument(raw_query, document_id);
}
//private methods

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + text + " is invalid"s);
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;
    const auto words = SplitIntoWords(text);
    for (const std::string& word : words) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    std::sort(result.minus_words.begin(), result.minus_words.end());
    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto cop = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(cop, result.minus_words.end());
    result.minus_words.shrink_to_fit();
    cop = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(cop, result.plus_words.end());
    result.plus_words.shrink_to_fit();
    return result;
}

SearchServer::Query SearchServer::ParseQuery(std::execution::parallel_policy policy,
                                             const std::string& text) const {
    std::vector<std::string> words = SplitIntoWords(text);
    std::vector<QueryWord> qwords(words.size());
    std::transform(policy,
                   words.begin(), words.end(),
                   qwords.begin(),
                   [&](const auto& w){
                        return ParseQueryWord(w);
    });
    Query result;
    result.minus_words.reserve(words.size());
    result.plus_words.reserve(words.size());
    for (const QueryWord& qw: qwords) {
        if (!qw.is_stop) {
            if (qw.is_minus) {
                result.minus_words.emplace_back(qw.data);
            } else {
                result.plus_words.emplace_back(qw.data);
            }
        }
    }
    return result;
}

SearchServer::Query SearchServer::ParseQuery(std::execution::sequenced_policy policy,
                                             const std::string& text) const {
    return ParseQuery(text);
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

//Functions out of class
void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::invalid_argument& e) {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = index;
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }
}