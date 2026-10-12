#include "bookmgmt/Catalog.h"

#include <cctype>
#include <stdexcept>
#include <utility>

#include "bookmgmt/Book.h"
#include "bookmgmt/EBook.h"
#include "bookmgmt/Exceptions.h"
#include "bookmgmt/Journal.h"

namespace bookmgmt {

namespace {
std::string lower(std::string value) {
    for (std::size_t i = 0; i < value.size(); ++i)
        value[i] = static_cast<char>(std::tolower(
            static_cast<unsigned char>(value[i])));
    return value;
}

int totalFor(const std::map<std::string, int>& counts) {
    int total = 0;
    for (const auto& entry : counts) total += entry.second;
    return total;
}
}  // namespace

Resource& Catalog::add(std::unique_ptr<Resource> resource) {
    if (!resource) throw std::invalid_argument("cannot add a null resource");
    const std::string id = resource->id();
    if (items_.count(id)) throw DuplicateIdError(id);
    Entry& entry = items_[id];
    entry.resource = std::move(resource);
    return *entry.resource;
}

bool Catalog::contains(const std::string& id) const { return items_.count(id) != 0; }

Resource* Catalog::find(const std::string& id) {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

const Resource* Catalog::find(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

Resource& Catalog::get(const std::string& id) {
    if (Resource* resource = find(id)) return *resource;
    throw NotFoundError(id);
}

const Resource& Catalog::get(const std::string& id) const {
    if (const Resource* resource = find(id)) return *resource;
    throw NotFoundError(id);
}

void Catalog::remove(const std::string& id) {
    if (items_.erase(id) == 0) throw NotFoundError(id);
    loans_.erase(id);
    sessions_.erase(id);
}

int Catalog::holdings(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.holdings;
}

void Catalog::addHoldings(const std::string& id, int units) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    const int inUse = borrowedCopies(id) + activeSessions(id);
    if (it->second.holdings + units < inUse)
        throw std::invalid_argument("holdings cannot be reduced below units in use");
    if (it->second.holdings + units < 0)
        throw std::invalid_argument("holdings cannot become negative");
    it->second.holdings += units;
}

std::vector<const Resource*> Catalog::where(
    const std::function<bool(const Resource&)>& predicate) const {
    std::vector<const Resource*> results;
    for (const auto& entry : items_)
        if (predicate(*entry.second.resource)) results.push_back(entry.second.resource.get());
    return results;
}

std::vector<const Resource*> Catalog::all() const {
    std::vector<const Resource*> results;
    for (std::map<std::string, Entry>::const_iterator item = items_.begin();
         item != items_.end(); ++item)
        results.push_back(item->second.resource.get());
    return results;
}

std::vector<const Resource*> Catalog::byCategory(ResourceCategory category) const {
    std::vector<const Resource*> results;
    for (std::map<std::string, Entry>::const_iterator item = items_.begin();
         item != items_.end(); ++item) {
        const Resource* resource = item->second.resource.get();
        if (resource->category() == category) results.push_back(resource);
    }
    return results;
}

std::vector<const Resource*> Catalog::searchTitle(const std::string& text) const {
    const std::string needle = lower(text);
    std::vector<const Resource*> results;
    for (std::map<std::string, Entry>::const_iterator item = items_.begin();
         item != items_.end(); ++item) {
        const Resource* resource = item->second.resource.get();
        if (lower(resource->title()).find(needle) != std::string::npos)
            results.push_back(resource);
    }
    return results;
}

std::vector<const Resource*> Catalog::searchAuthor(const std::string& text) const {
    const std::string needle = lower(text);
    std::vector<const Resource*> results;
    for (std::map<std::string, Entry>::const_iterator item = items_.begin();
         item != items_.end(); ++item) {
        const Resource* resource = item->second.resource.get();
        const std::vector<std::string>* authors = nullptr;
        const Book* book = dynamic_cast<const Book*>(resource);
        const EBook* ebook = dynamic_cast<const EBook*>(resource);
        if (book != nullptr) authors = &book->authors();
        else if (ebook != nullptr) authors = &ebook->authors();

        bool authorMatches = false;
        if (authors != nullptr) {
            for (std::size_t i = 0; i < authors->size(); ++i) {
                if (lower((*authors)[i]).find(needle) != std::string::npos) {
                    authorMatches = true;
                    break;
                }
            }
        }
        if (authorMatches) results.push_back(resource);
    }
    return results;
}

std::vector<const Resource*> Catalog::searchIsbnIssn(const std::string& text) const {
    const std::string needle = lower(text);
    std::vector<const Resource*> results;
    for (std::map<std::string, Entry>::const_iterator item = items_.begin();
         item != items_.end(); ++item) {
        const Resource* resource = item->second.resource.get();
        std::string identifier;
        const Book* book = dynamic_cast<const Book*>(resource);
        const EBook* ebook = dynamic_cast<const EBook*>(resource);
        const Journal* journal = dynamic_cast<const Journal*>(resource);
        if (book != nullptr)
            identifier = book->isbn();
        else if (ebook != nullptr)
            identifier = ebook->isbn();
        else if (journal != nullptr)
            identifier = journal->issn();
        if (!identifier.empty() &&
            lower(identifier).find(needle) != std::string::npos)
            results.push_back(resource);
    }
    return results;
}

std::vector<const Resource*> Catalog::byPublicationYearRange(int firstYear,
                                                             int lastYear) const {
    if (firstYear > lastYear)
        throw std::invalid_argument("first year must not exceed last year");
    std::vector<const Resource*> results;
    for (std::map<std::string, Entry>::const_iterator item = items_.begin();
         item != items_.end(); ++item) {
        const Resource* resource = item->second.resource.get();
        if (resource->year() >= firstYear && resource->year() <= lastYear)
            results.push_back(resource);
    }
    return results;
}

int Catalog::borrowedCopies(const std::string& id) const {
    auto it = loans_.find(id);
    return it == loans_.end() ? 0 : totalFor(it->second);
}

int Catalog::activeSessions(const std::string& id) const {
    auto it = sessions_.find(id);
    return it == sessions_.end() ? 0 : totalFor(it->second);
}

void Catalog::borrowCopy(const std::string& id, const std::string& patronId) {
    const Resource& resource = get(id);
    if (resource.isDigital()) throw std::invalid_argument("electronic resources use sessions");
    if (patronId.empty()) throw std::invalid_argument("patron id must not be empty");
    if (holdings(id) - borrowedCopies(id) <= 0)
        throw std::invalid_argument("no available print copies");
    ++loans_[id][patronId];
}

void Catalog::returnCopy(const std::string& id, const std::string& patronId) {
    const Resource& resource = get(id);
    if (resource.isDigital()) throw std::invalid_argument("electronic resources use sessions");
    auto item = loans_.find(id);
    if (item == loans_.end() || !item->second.count(patronId))
        throw std::invalid_argument("patron has no copy to return");
    auto patron = item->second.find(patronId);
    if (--patron->second == 0) item->second.erase(patron);
    if (item->second.empty()) loans_.erase(item);
}

void Catalog::openSession(const std::string& id, const std::string& patronId) {
    const Resource& resource = get(id);
    if (!resource.isDigital()) throw std::invalid_argument("print resources use copy lending");
    if (patronId.empty()) throw std::invalid_argument("patron id must not be empty");
    if (activeSessions(id) >= holdings(id))
        throw std::invalid_argument("all licensed seats are in use");
    ++sessions_[id][patronId];
}

void Catalog::closeSession(const std::string& id, const std::string& patronId) {
    const Resource& resource = get(id);
    if (!resource.isDigital()) throw std::invalid_argument("print resources use copy lending");
    auto item = sessions_.find(id);
    if (item == sessions_.end() || !item->second.count(patronId))
        throw std::invalid_argument("patron has no active session");
    auto patron = item->second.find(patronId);
    if (--patron->second == 0) item->second.erase(patron);
    if (item->second.empty()) sessions_.erase(item);
}

}  // namespace bookmgmt
