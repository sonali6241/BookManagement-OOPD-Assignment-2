#pragma once
// Catalog owns resources, tracks holdings, and provides search and lending operations.

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class Catalog {
public:
    Resource& add(std::unique_ptr<Resource> resource);

    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        auto resource = std::make_unique<T>(std::forward<Args>(args)...);
        T& result = *resource;
        add(std::move(resource));
        return result;
    }

    bool contains(const std::string& id) const;
    Resource* find(const std::string& id);
    const Resource* find(const std::string& id) const;
    Resource& get(const std::string& id);
    const Resource& get(const std::string& id) const;
    void remove(const std::string& id);

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    int holdings(const std::string& id) const;
    void addHoldings(const std::string& id, int units);

    std::vector<const Resource*> all() const;
    std::vector<const Resource*> byCategory(ResourceCategory category) const;
    std::vector<const Resource*> searchTitle(const std::string& text) const;
    std::vector<const Resource*> searchAuthor(const std::string& text) const;
    std::vector<const Resource*> searchIsbnIssn(const std::string& text) const;
    std::vector<const Resource*> byPublicationYearRange(int firstYear, int lastYear) const;
    std::vector<const Resource*> where(
        const std::function<bool(const Resource&)>& predicate) const;

    void borrowCopy(const std::string& id, const std::string& patronId);
    void returnCopy(const std::string& id, const std::string& patronId);
    void openSession(const std::string& id, const std::string& patronId);
    void closeSession(const std::string& id, const std::string& patronId);
    int borrowedCopies(const std::string& id) const;
    int activeSessions(const std::string& id) const;

private:
    struct Entry {
        std::unique_ptr<Resource> resource;
        int holdings = 0;
    };
    std::map<std::string, Entry> items_;
    std::map<std::string, std::map<std::string, int>> loans_;
    std::map<std::string, std::map<std::string, int>> sessions_;
};

}  // namespace bookmgmt
