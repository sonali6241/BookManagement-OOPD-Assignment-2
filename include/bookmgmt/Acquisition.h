#pragma once
// AcquisitionManager applies pricing and tax, selects vendors, and records orders.

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;
    std::string department;
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money cost;  // post-tax amount paid; cancellation rows hold a negative refund
    bool approved;
    std::string reason;
    std::string department;
    std::string vendor;
    bool cancellation = false;
    int relatedOrderNo = 0;
    bool cancelled = false;
};

struct VendorOffer {
    std::string vendor;
    Money unitPrice;
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget);

    void setTaxRates(int printPercent, int electronicPercent);
    int printTaxRate() const { return printTaxPercent_; }
    int electronicTaxRate() const { return electronicTaxPercent_; }

    Budget& addDepartmentBudget(const std::string& department, Money total);
    Budget& departmentBudget(const std::string& department);
    const Budget& departmentBudget(const std::string& department) const;

    void addVendorOffer(const std::string& resourceId, std::string vendor, Money unitPrice);
    std::vector<VendorOffer> vendorsFor(const std::string& resourceId) const;

    Money quote(const std::string& id, int quantity) const;
    Money quoteAfterTax(const std::string& id, int quantity) const;
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr,
                     const std::string& department = {}) const;
    const PurchaseRecord& purchase(const std::string& id, int quantity,
                                   const std::string& department = {});
    const PurchaseRecord& cancel(int orderNo);
    std::vector<PurchaseRecord> processBatch(
        const std::vector<PurchaseRequest>& requests, bool allOrNothing = false);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;
    void printReport(std::ostream& os) const;

private:
    PurchaseRecord& record(const Resource* resource, const std::string& id, int quantity,
                           Money preTax, Money postTax, bool approved, std::string reason,
                           const std::string& department, const std::string& vendor,
                           bool cancellation = false, int relatedOrderNo = 0);
    Money taxInclusiveCost(const Resource& resource, Money preTax) const;
    Budget& budgetFor(const std::string& department);
    const Budget& budgetFor(const std::string& department) const;
    std::string selectedVendor(const std::string& id) const;
    std::vector<PurchaseRecord> processSequential(
        const std::vector<PurchaseRequest>& requests);

    Catalog& catalog_;
    Budget& budget_;
    std::map<std::string, Budget> departments_;
    std::map<std::string, std::vector<VendorOffer>> vendorOffers_;
    std::map<std::string, std::string> cheapestVendor_;
    std::vector<PurchaseRecord> history_;
    int printTaxPercent_ = 0;
    int electronicTaxPercent_ = 0;
    int nextOrderNo_ = 1;
};

}  // namespace bookmgmt
