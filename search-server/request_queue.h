//
// Created by BORIS KARPOV on 21/05/2022.
//
#pragma once
#include <deque>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
            : server_(search_server),
              no_results_(0),
              current_time_(0)
    {}

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        ++current_time_;
        std::vector<Document> results = server_.FindTopDocuments(raw_query, document_predicate);
        if (!requests_.empty()) {
            while(current_time_ - requests_.front().request_time >= min_in_day_) {
                if (requests_.front().docs == 0){
                    --no_results_;
                }
                requests_.pop_front();
            }
        }
        requests_.push_back({static_cast<int>(results.size()), current_time_});
        if (results.empty()) {
            ++no_results_;
        }
        return results;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return RequestQueue::AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        return RequestQueue::AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const {
        return no_results_;
    }
private:
    struct QueryResult {
        int docs;
        int request_time;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    int no_results_;
    int current_time_;
};