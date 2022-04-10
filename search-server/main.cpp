#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cstdlib>
#include <cassert>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ACCURACY_THRESHOLD = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

struct Document {
    int id;
    double relevance;
    int rating;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / static_cast<int>(words.size());
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }

    template <typename FilterPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, FilterPredicate filter_condition) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter_condition);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < ACCURACY_THRESHOLD) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus required_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [required_status](int document_id, DocumentStatus status, int rating) { return status == required_status; });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename FilterPredicate>
    vector<Document> FindAllDocuments(const Query& query, FilterPredicate filter_condition) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                auto& document_data = documents_.at(document_id);
                if (filter_condition(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating,
                                        });
        }
        return matched_documents;
    }
};

template <typename DataType>
ostream& operator<<(ostream& out, set<DataType>& container){
    out << "{"s;
    bool isFirst = true;
    for (DataType& el: container) {
        if (isFirst){
            out << el;
            isFirst = false;
        } else {
            out << ", " << el;
        }
    }
    out << "}"s;
    return out;
}
template <typename DataType>
ostream& operator<<(ostream& out, vector<DataType>& container){
    out << "["s;
    bool isFirst = true;
    for (DataType& el: container) {
        if (isFirst){
            out << el;
            isFirst = false;
        } else {
            out << ", " << el;
        }
    }
    out << "]"s;
    return out;
}

template <typename DataType1, typename DataType2>
ostream& operator<<(ostream& out, map<DataType1, DataType2>& container){
    out << "{"s;
    bool isFirst = true;
    for (const auto& [d1, d2]: container) {
        if (isFirst){
            out << d1 << ": " << d2;
            isFirst = false;
        } else {
            out << ", " << d1 << ": " << d2;
        }
    }
    out << "}"s;
    return out;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func)  RunTestImpl(func, #func)

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

template <typename T>
void RunTestImpl(const T& t, const string& t_str) {
    t();
    cerr << t_str << " OK"s << endl;
}

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    /*
     * Ситуации:
     * Документ есть; Запрос содержит стопслово из документа
     * Документ пуст, но есть стопслова
     * Документ есть, нет стопслов
     * Документ не возвращает ничего, если все слова документа - стопслова
    */
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        // тест без стопслов
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        // теперь проверка с стопсловами и поиск по стоп слову
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
    {
        //все слова - стопслова
        SearchServer server;
        server.SetStopWords("cat in the city"s);
        server.AddDocument(doc_id, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat in the city"s);
        ASSERT_HINT(found_docs.empty(), "Stop words must be excluded even query consist only of stop words"s);
    }
    {
        //Документ пуст
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, ""s, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat in the city"s);
        ASSERT_HINT(found_docs.empty(), "Empty document shouldn't return any data"s);
    }
}


void TestAddDocuments(){
    //Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.

    /* Ситуации:
     * Документ можно найти по слову
     * Документ можно найти по нескольким словам
     * Документ нельзя найти по неправильному слову или подстроке
     * Документ пуст
     * Поисковый запрос пуст или состоит из спецсимволов
     */
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // проверим что сервер изначально создается пустой
    {
        SearchServer server;
        //server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 0, "New instance of SearchServer doesn't include any documents"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        //док должен быть найден по 1 и по нескольким словам; Пустая строка или пробел должны игнорироваться,
        //лишнее находить не должны
        ASSERT_EQUAL(server.FindTopDocuments("the"s).size(), 1); //тест на 1 слово
        ASSERT_EQUAL(server.FindTopDocuments("cat city"s).size(), 1); // тест на 2 слова
        ASSERT_HINT(server.FindTopDocuments(" "s).empty(),
                    "No documents should be found by special symbols and whitespace"s); //тест на некорректный непустой запрос
        ASSERT_HINT(server.FindTopDocuments(""s).empty(),
                    "No documents should be found if query is empty"s); // тест на пустую строку
        ASSERT_HINT(server.FindTopDocuments("catdog at a cit"s).empty(),
                    "No documents should be found by substring intersections"s); // тест на запрос без пересечения
    }
}


void TestMinusWords(){
    //Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
    /*Ситуации:
     * Минус-слово есть в документе
     * Минус-слово совпадает с стоп-словом
     * минус-слово совпадает с словом запроса
     */
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        //проверим работу минус-слов когда нет стоп-слов
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("cat -in"s).empty(), "Minus word should remove document from response"s); // в запросе есть минус-слово из документа
        ASSERT_EQUAL(server.FindTopDocuments("cat"s).size(), 1); // нет минус слова
        ASSERT_EQUAL_HINT(server.FindTopDocuments("cat -random"s).size(), 1, "Documents don't contain minus words"s); // минус слово не встречается в документе
    }
    {
        SearchServer server;
        server.SetStopWords("the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL_HINT(server.FindTopDocuments("city -the"s).size(), 1, "Minus words should be ignored, if they are in stop words list"s);
    }
}

void TestMatchingWordsInDocument(){
    //Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
    /*
     * Ситуации -
     * в запросе только плюс слова
     * в запросе есть и плюс и минус-слова
     * в запросе плюс-слово совпадает с минус словом
     * в запросе стоп-слово
     */
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.SetStopWords("the"s);
    vector<string> words;
    DocumentStatus status;
    {
        tie(words, status) = server.MatchDocument("cat city"s, 42);
        ASSERT(words[0] == "cat"s && words[1] == "city"s); // слова входят в документ
    }
    {
        tie(words, status) = server.MatchDocument("cat -in"s, 42);
        ASSERT_HINT(words.empty(), "Minus word should mismatch document"); // в документе есть минус-слово
    }
    {
        tie(words, status) = server.MatchDocument("in -in"s, 42);
        ASSERT_HINT(words.empty(), "Minus word should filter documents , even if this word is plus too"); // плюс и минус слова совпадают
    }
    {
        tie(words, status) = server.MatchDocument("cat -the"s, 42);
        ASSERT_EQUAL_HINT(words.at(0), "cat"s, "Document should be matched and properly processed if minus word is on stop list"); // стоп-слово будет отсутствовать в документе, поэтому минус-слово не должно влиять
    }
}

void TestSortByRelevance(){
    //Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
    SearchServer server;
    server.SetStopWords("the"s);
    server.AddDocument(42, "cat in the big city city dog"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "cat in the small garden in little city"s, DocumentStatus::ACTUAL, {2, 1, 4});
    server.AddDocument(44, "cat in the small garden"s, DocumentStatus::ACTUAL, {2, 1, 4});
    {
        const auto& docs = server.FindTopDocuments("big city"s);
        ASSERT(docs.at(0).id == 42 &&
               docs.at(1).id == 43); // поиск по 2 словам

    }
    {
        const auto& docs = server.FindTopDocuments("cat in the small garden"s);
        ASSERT(docs.at(0).id == 44 &&
               docs.at(1).id == 43 &&
               docs.at(2).id == 42 ); // поиск по нескольким словам
    }
    {
        const auto& docs = server.FindTopDocuments("cat in the -small garden"s);
        ASSERT(docs.at(0).id == 42 && docs.size() == 1); // минус-слово отсекает 2 документа из 3х
    }
    {
        const auto& docs = server.FindTopDocuments("the"s);
        ASSERT_HINT(docs.empty(), "No relevance for empty response (by stop word)"s); // стоп-слово не будет найдено ни в 1 документе
    }
}

void TestRatingComputation(){
    //Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
    SearchServer server;
    server.AddDocument(42, "cat"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "dog"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(44, "frog"s, DocumentStatus::ACTUAL, {2, 1, 4, -5, 0});
    ASSERT_EQUAL(server.FindTopDocuments("cat"s).at(0).rating, 2);
    ASSERT_EQUAL(server.FindTopDocuments("dog"s).at(0).rating, 0);
    ASSERT_EQUAL(server.FindTopDocuments("frog"s).at(0).rating, 0);
}

void TestPredicateFiltering(){
    //Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
    SearchServer server;
    server.AddDocument(42, "cat in the big city city dog"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "cat in the small garden in little city"s, DocumentStatus::ACTUAL, {2, 2, 5});
    server.AddDocument(44, "cat in the small garden"s, DocumentStatus::REMOVED, {2, 2, 5});
    {
        const auto& docs = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating)
        { return document_id%2 != 0; }); // нечетные номера индексов
        ASSERT_EQUAL(docs.at(0).id, 43);
    }
    {
        const auto& docs = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating)
        { return status == DocumentStatus::ACTUAL; });
        ASSERT(docs.at(0).id == 43 && docs.at(1).id == 42 && docs.size() == 2);
    }
    {
        const auto& docs = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating)
        { return rating == 2; });
        ASSERT(docs.size() == 1 && docs.at(0).id == 42);
    }
}

void TestSearchDocumentsByStatus(){
    SearchServer server;
    server.AddDocument(42, "cat"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "dog"s, DocumentStatus::IRRELEVANT, {2, 1, 4});
    server.AddDocument(44, "frog"s, DocumentStatus::BANNED, {2, 1, 4});
    server.AddDocument(45, "horse"s, DocumentStatus::REMOVED, {2, 1, 4});
    server.AddDocument(46, "hedgehog"s, DocumentStatus::ACTUAL, {2, 1, 4});
    {
        const auto& docs = server.FindTopDocuments("cat hedgehog"s,[](int document_id, DocumentStatus status, int rating)
        { return status == DocumentStatus::ACTUAL; });
        ASSERT(docs.at(0).id == 42 && docs.at(1).id == 46 && docs.size() == 2);
    }
    {
        const auto& docs = server.FindTopDocuments("dog"s,[](int document_id, DocumentStatus status, int rating)
        { return status == DocumentStatus::IRRELEVANT; });
        ASSERT(docs.at(0).id == 43 && docs.size() == 1);
    }
    {
        const auto& docs = server.FindTopDocuments("frog"s,[](int document_id, DocumentStatus status, int rating)
        { return status == DocumentStatus::BANNED; });
        ASSERT(docs.at(0).id == 44 && docs.size() == 1);
    }
    {
        const auto& docs = server.FindTopDocuments("horse"s,[](int document_id, DocumentStatus status, int rating)
        { return status == DocumentStatus::REMOVED; });
        ASSERT(docs.at(0).id == 45 && docs.size() == 1);
    }
}

void TestRelevanceComputation(){
    //Корректное вычисление релевантности найденных документов.
    SearchServer server;
    server.AddDocument(42, "cat in the city cat"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(44, "cat at the town"s, DocumentStatus::ACTUAL, {1, 2, 3});

    //assert(server.FindTopDocuments("city"s).at(0).relevance < ACCURACY_THRESHOLD);
    ASSERT(abs(server.FindTopDocuments("cat"s).at(0).relevance - 0.162186) < ACCURACY_THRESHOLD);
    ASSERT(abs(server.FindTopDocuments("dog"s).at(0).relevance - 0.274653) < ACCURACY_THRESHOLD);
    ASSERT(abs(server.FindTopDocuments("cat at the town"s).at(0).relevance - 0.650672) < ACCURACY_THRESHOLD); //проверяем суммирование
    const auto& docs = server.FindTopDocuments("cat at the -town"s);
    ASSERT(docs.at(0).relevance - 0.162186 < ACCURACY_THRESHOLD && docs.at(1).relevance < ACCURACY_THRESHOLD); //проверяем запрос с минус-словом
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocuments);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchingWordsInDocument);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestRatingComputation);
    RUN_TEST(TestPredicateFiltering);
    RUN_TEST(TestSearchDocumentsByStatus);
    RUN_TEST(TestRelevanceComputation);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}