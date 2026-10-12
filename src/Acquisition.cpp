#include "bookmgmt/Acquisition.h"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}

void AcquisitionManager::setTaxRates(int printPercent, int electronicPercent) {
    if (printPercent < 0 || electronicPercent < 0 ||
        printPercent > 100 || electronicPercent > 100)
        throw std::invalid_argument("tax rates must be in 0..100 percent");
    printTaxPercent_ = printPercent;
    electronicTaxPercent_ = electronicPercent;
}

Budget& AcquisitionManager::addDepartmentBudget(const std::string& department,
                                                 Money total) {
    if (department.empty()) throw std::invalid_argument("department name must not be empty");
    (void)(total + budget_.total());
    auto result = departments_.emplace(department, Budget(total));
    if (!result.second) throw std::invalid_argument("department already exists: " + department);
    return result.first->second;
}

Budget& AcquisitionManager::departmentBudget(const std::string& department) {
    auto it = departments_.find(department);
    if (it == departments_.end()) throw std::invalid_argument("unknown department: " + department);
    return it->second;
}

const Budget& AcquisitionManager::departmentBudget(const std::string& department) const {
    auto it = departments_.find(department);
    if (it == departments_.end()) throw std::invalid_argument("unknown department: " + department);
    return it->second;
}

Budget& AcquisitionManager::budgetFor(const std::string& department) {
    return department.empty() ? budget_ : departmentBudget(department);
}

const Budget& AcquisitionManager::budgetFor(const std::string& department) const {
    return department.empty() ? budget_ : departmentBudget(department);
}

void AcquisitionManager::addVendorOffer(const std::string& resourceId,
                                       std::string vendor, Money unitPrice) {
    Resource& resource = catalog_.get(resourceId);
    if (vendor.empty()) throw std::invalid_argument("vendor name must not be empty");
    if (unitPrice.isNegative()) throw std::invalid_argument("vendor price must not be negative");
    (void)(unitPrice + resource.unitPrice());
    auto& offers = vendorOffers_[resourceId];
    auto existing = std::find_if(offers.begin(), offers.end(), [&](const VendorOffer& offer) {
        return offer.vendor == vendor;
    });
    if (existing == offers.end()) offers.push_back({std::move(vendor), unitPrice});
    else existing->unitPrice = unitPrice;
    const auto cheapest = std::min_element(offers.begin(), offers.end(),
        [](const VendorOffer& left, const VendorOffer& right) {
            return left.unitPrice < right.unitPrice;
        });
    resource.setUnitPrice(cheapest->unitPrice);
    cheapestVendor_[resourceId] = cheapest->vendor;
}

std::vector<VendorOffer> AcquisitionManager::vendorsFor(const std::string& resourceId) const {
    auto it = vendorOffers_.find(resourceId);
    return it == vendorOffers_.end() ? std::vector<VendorOffer>{} : it->second;
}

std::string AcquisitionManager::selectedVendor(const std::string& id) const {
    auto it = cheapestVendor_.find(id);
    return it == cheapestVendor_.end() ? std::string{} : it->second;
}

Money AcquisitionManager::taxInclusiveCost(const Resource& resource, Money preTax) const {
    const int rate = resource.isDigital() ? electronicTaxPercent_ : printTaxPercent_;
    const std::int64_t taxMinor = (preTax.minorUnits() * rate + 50) / 100;
    return preTax + Money::fromMinor(taxMinor, preTax.currencyCode());
}

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    return catalog_.get(id).costFor(quantity);
}

Money AcquisitionManager::quoteAfterTax(const std::string& id, int quantity) const {
    const Resource& resource = catalog_.get(id);
    return taxInclusiveCost(resource, resource.costFor(quantity));
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason,
                                     const std::string& department) const {
    std::string why;
    const Resource* resource = catalog_.find(id);
    if (!resource) {
        why = "resource not found: " + id;
    } else if (quantity <= 0) {
        why = "quantity must be positive";
    } else {
        try {
            const Money preTax = resource->costFor(quantity);
            const Money postTax = taxInclusiveCost(*resource, preTax);
            why = budgetFor(department).check(resource->category(), quantity, postTax, id);
        } catch (const std::invalid_argument& error) {
            why = error.what();
        }
    }
    if (reason) *reason = why;
    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(const Resource* resource,
                                           const std::string& id, int quantity,
                                           Money preTax, Money postTax, bool approved,
                                           std::string reason,
                                           const std::string& department,
                                           const std::string& vendor,
                                           bool cancellation, int relatedOrderNo) {
    PurchaseRecord result{
        nextOrderNo_++, id, resource ? resource->title() : std::string("(unknown)"),
        resource ? resource->category() : ResourceCategory::Book, quantity,
        preTax, postTax, approved, std::move(reason), department, vendor,
        cancellation, relatedOrderNo, false};
    history_.push_back(std::move(result));
    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int quantity,
                                                    const std::string& department) {
    const Resource& resource = catalog_.get(id);
    const Money preTax = resource.costFor(quantity);
    const Money postTax = taxInclusiveCost(resource, preTax);
    budgetFor(department).commit(resource.category(), quantity, postTax, id);
    catalog_.addHoldings(id, quantity);
    return record(&resource, id, quantity, preTax, postTax, true, {}, department,
                  selectedVendor(id));
}

const PurchaseRecord& AcquisitionManager::cancel(int orderNo) {
    auto it = std::find_if(history_.begin(), history_.end(), [orderNo](const PurchaseRecord& rec) {
        return rec.orderNo == orderNo;
    });
    if (it == history_.end()) throw std::invalid_argument("order not found");
    if (!it->approved || it->cancellation)
        throw std::invalid_argument("only approved purchase orders can be cancelled");
    if (it->cancelled) throw std::invalid_argument("order is already cancelled");

    const PurchaseRecord original = *it;
    const Resource& resource = catalog_.get(original.resourceId);
    const int inUse = resource.isDigital() ? catalog_.activeSessions(original.resourceId)
                                           : catalog_.borrowedCopies(original.resourceId);
    if (catalog_.holdings(original.resourceId) - original.quantity < inUse)
        throw std::invalid_argument("cannot cancel units currently in use");

    budgetFor(original.department).refund(original.category, original.quantity,
                                          original.cost, original.resourceId);
    catalog_.addHoldings(original.resourceId, -original.quantity);
    auto originalIt = std::find_if(history_.begin(), history_.end(), [orderNo](const PurchaseRecord& rec) {
        return rec.orderNo == orderNo;
    });
    originalIt->cancelled = true;
    const Money refundPreTax = Money::fromMinor(-original.preTaxCost.minorUnits(),
                                                 original.preTaxCost.currencyCode());
    const Money refundPostTax = Money::fromMinor(-original.cost.minorUnits(),
                                                  original.cost.currencyCode());
    return record(&resource, original.resourceId, -original.quantity,
                  refundPreTax, refundPostTax, true, {}, original.department,
                  original.vendor, true, original.orderNo);
}

std::vector<PurchaseRecord> AcquisitionManager::processSequential(
    const std::vector<PurchaseRequest>& requests) {
    std::vector<PurchaseRecord> results;
    results.reserve(requests.size());
    for (const auto& request : requests) {
        const Resource* resource = catalog_.find(request.resourceId);
        Money preTax;
        Money postTax;
        std::string why;
        if (!resource) {
            why = "resource not found: " + request.resourceId;
        } else if (request.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            preTax = resource->costFor(request.quantity);
            postTax = taxInclusiveCost(*resource, preTax);
            try {
                why = budgetFor(request.department).check(resource->category(),
                    request.quantity, postTax, request.resourceId);
            } catch (const std::invalid_argument& error) {
                why = error.what();
            }
        }
        if (why.empty()) {
            results.push_back(purchase(request.resourceId, request.quantity,
                                       request.department));
        } else {
            results.push_back(record(resource, request.resourceId, request.quantity,
                preTax, postTax, false, why, request.department,
                selectedVendor(request.resourceId)));
        }
    }
    return results;
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& requests, bool allOrNothing) {
    if (!allOrNothing) return processSequential(requests);

    Budget mainShadow = budget_;
    auto departmentShadows = departments_;
    std::vector<const Resource*> resources(requests.size(), nullptr);
    std::vector<Money> preTax(requests.size());
    std::vector<Money> postTax(requests.size());
    std::vector<std::string> failures(requests.size());
    std::string firstFailure;
    std::size_t failedIndex = requests.size();

    for (std::size_t i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        resources[i] = catalog_.find(request.resourceId);
        if (!resources[i]) failures[i] = "resource not found: " + request.resourceId;
        else if (request.quantity <= 0) failures[i] = "quantity must be positive";
        else {
            try {
                preTax[i] = resources[i]->costFor(request.quantity);
                postTax[i] = taxInclusiveCost(*resources[i], preTax[i]);
                Budget* shadow = nullptr;
                if (request.department.empty()) shadow = &mainShadow;
                else {
                    auto department = departmentShadows.find(request.department);
                    if (department == departmentShadows.end())
                        throw std::invalid_argument("unknown department: " + request.department);
                    shadow = &department->second;
                }
                failures[i] = shadow->check(resources[i]->category(), request.quantity,
                                             postTax[i], request.resourceId);
                if (failures[i].empty())
                    shadow->commit(resources[i]->category(), request.quantity,
                                   postTax[i], request.resourceId);
            } catch (const std::invalid_argument& error) {
                failures[i] = error.what();
            }
        }
        if (!failures[i].empty() && failedIndex == requests.size()) {
            failedIndex = i;
            firstFailure = failures[i];
        }
    }

    if (failedIndex != requests.size()) {
        std::vector<PurchaseRecord> rejected;
        rejected.reserve(requests.size());
        for (std::size_t i = 0; i < requests.size(); ++i) {
            std::string why = failures[i];
            if (why.empty())
                why = "atomic batch rolled back because request " +
                      requests[failedIndex].resourceId + " failed: " + firstFailure;
            rejected.push_back(record(resources[i], requests[i].resourceId,
                requests[i].quantity, preTax[i], postTax[i], false, std::move(why),
                requests[i].department, selectedVendor(requests[i].resourceId)));
        }
        return rejected;
    }
    return processSequential(requests);
}

Money AcquisitionManager::totalSpent() const {
    Money sum = Money::fromMinor(0, budget_.total().currencyCode());
    for (const auto& rec : history_)
        if (rec.approved) sum += rec.cost;
    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";
    Money preTaxTotal = Money::fromMinor(0, budget_.total().currencyCode());
    for (const auto& rec : history_) {
        const char* status = rec.cancellation ? "CANCELLED" :
                             (rec.approved ? "APPROVED" : "REJECTED");
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " " << status << "  "
           << std::setw(6) << rec.resourceId << " x" << std::setw(3) << rec.quantity
           << "  pre-tax " << std::setw(12) << std::right << rec.preTaxCost.toString()
           << "  post-tax " << std::setw(12) << rec.cost.toString() << std::left
           << "  " << rec.title;
        if (!rec.department.empty()) os << " [" << rec.department << "]";
        if (!rec.vendor.empty()) os << " {" << rec.vendor << "}";
        if (rec.cancellation) os << " (refund for #" << rec.relatedOrderNo << ")";
        if (rec.cancelled) os << " (cancelled later)";
        if (!rec.approved) os << "\n        reason: " << rec.reason;
        os << "\n";
        if (rec.approved) preTaxTotal += rec.preTaxCost;
    }
    os << "Total pre-tax: " << preTaxTotal << "\n"
       << "Total post-tax: " << totalSpent() << "\n";
}

}  // namespace bookmgmt
