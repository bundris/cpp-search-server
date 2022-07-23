#include <numeric>
#include <execution>
#include <iostream>
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(
            std::execution::par,
            queries.begin(), queries.end(),
            result.begin(),
            [&search_server](const std::string& query) {
                return search_server.FindTopDocuments(query);
            });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {

    auto docs_by_queries = ProcessQueries(search_server, queries);
    size_t k = std::accumulate(docs_by_queries.begin(), docs_by_queries.end(),
                                   0,
                                   [](const size_t lhs, const std::vector<Document>& rhs) {
                                       return lhs + rhs.size();
                                   });
    std::vector<Document> result(k);
    auto start = result.begin();
    for (const auto& vect: docs_by_queries) {
        std::move(vect.begin(), vect.end(), start);
        start = next(start, vect.size());
    }
    return result;
}