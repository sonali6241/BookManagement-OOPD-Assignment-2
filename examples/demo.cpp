// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static void demonstrateAssignmentExtensions() {
    std::cout << "\n=== Taxes, departments, vendors, and cancellation ===\n";
    Catalog catalog;
    catalog.emplace<Book>("P1", "Algorithms", std::vector<std::string>{"A. Author"},
                          "978-0001", "Press", 2022, Money::of(100));
    catalog.emplace<ElectronicResource>("D1", "Digital Library", "Publisher", 2024,
                                        Money::of(10), "https://example.test");
    Budget general(Money::of(2000));
    general.setQuota(ResourceCategory::Book, {10, Money::of(1000), 2});
    general.setQuota(ResourceCategory::ElectronicResource, {10, Money::of(1000), 2});
    AcquisitionManager manager(catalog, general);
    manager.setTaxRates(5, 18);
    manager.addVendorOffer("P1", "Vendor A", Money::of(95));
    manager.addVendorOffer("P1", "Vendor B", Money::of(90));
    Budget& department = manager.addDepartmentBudget("Computer Science", Money::of(500));
    department.setQuota(ResourceCategory::Book, {5, Money::of(500), 1});
    std::cout << "P1 pre-tax quote for 2: " << manager.quote("P1", 2)
              << "; after tax: " << manager.quoteAfterTax("P1", 2) << "\n";
    const int order = manager.purchase("P1", 2, "Computer Science").orderNo;
    std::cout << "Selected vendor: " << manager.history().back().vendor << "\n";
    manager.cancel(order);
    std::cout << "Cancellation refunded the department budget: "
              << manager.departmentBudget("Computer Science").spent() << " spent\n";

    const auto atomic = manager.processBatch({{"P1", 1, "Computer Science"},
                                               {"D1", 11}}, true);
    std::cout << "Atomic batch: " << atomic.size() << " requests recorded; "
              << "holdings after rejected batch: P1=" << catalog.holdings("P1")
              << ", D1=" << catalog.holdings("D1") << "\n";

    std::cout << "Author search for 'author': "
              << catalog.searchAuthor("author").size() << " result(s); "
              << "ISBN search: " << catalog.searchIsbnIssn("978-0001").size()
              << " result(s); years 2020-2024: "
              << catalog.byPublicationYearRange(2020, 2024).size() << " result(s)\n";
    catalog.addHoldings("P1", 1);
    catalog.borrowCopy("P1", "patron-1");
    catalog.addHoldings("D1", 1);
    catalog.openSession("D1", "patron-2");
    std::cout << "Borrowed print copies: " << catalog.borrowedCopies("P1")
              << "; active digital sessions: " << catalog.activeSessions("D1") << "\n";
    catalog.returnCopy("P1", "patron-1");
    catalog.closeSession("D1", "patron-2");

    Budget thisYear(Money::of(1000));
    thisYear.setQuota(ResourceCategory::Book, {10, Money::of(500), 3});
    thisYear.commit(ResourceCategory::Book, 2, Money::of(200), "P1");
    std::cout << "Next-year budget after 50% rollover: "
              << thisYear.rollover(50).total() << "\n";
}

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<Journal>("J001", "Nature Weekly", "1234-5678", 52,
                             "Nature Publishing", 2026, Money::of(30), 2);
    catalog.emplace<EBook>("E001", "Modern C++", "Example Press", 2025,
                            Money::of(20), "https://example.test/modern-cpp",
                            std::vector<std::string>{"A. Author"},
                            "978-1234567890", EBookFormat::EPUB, true,
                            LicenseModel::AnnualSubscription, Money::of(50));
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);
    catalog.emplace<AudioBook>("A001", "C++ in Sound", "Sound Press", 2024,
                               Money::of(15), "https://example.test/audio",
                               "N. Narrator", 360,
                               LicenseModel::AnnualSubscription, Money::of(30));
    catalog.emplace<Thesis>("T001", "Compiler Thesis", "IIIT Delhi",
                            "M.Tech", "Dr. Advisor", 2025);

    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");

    Budget budget(Money::of(25000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(9400)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(12000)});
    budget.setQuota(ResourceCategory::Journal, {5, Money::of(5000)});
    budget.setQuota(ResourceCategory::EBook, {10, Money::of(2000)});
    budget.setQuota(ResourceCategory::AudioBook, {10, Money::of(2000)});
    budget.setQuota(ResourceCategory::Thesis, {10, Money::of(1000)});
    AcquisitionManager acq(catalog, budget);

    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";
    std::cout << "2 copies of J001  = " << acq.quote("J001", 2) << "\n";
    std::cout << "2 seats of E001   = " << acq.quote("E001", 2) << "\n";
    std::cout << "10 copies of B001 = " << acq.quote("B001", 10) << "\n";
    std::cout << "51 seats of R001  = " << acq.quote("R001", 51) << "\n";
    acq.processBatch({
        {"A001", 1},
        {"T001", 1},
        {"E001", 2},   // 2 e-book seats    
        {"J001", 2},  // 2 copies for 2 years
                {"B001", 4},   // 1800  ok
        {"B002", 5},   // 7200  ok  -> book spend 9000
        {"B001", 1},   // 450   rejected: book spend quota (400 left)
        {"R001", 20},  // 5000  ok
        {"R002", 25},  // 10000 rejected: e-resource unit quota (20 seats left)
        {"R002", 15},  // 6000  ok  -> e-resource spend 11000
        {"R002", 5},   // 2000  rejected: e-resource spend quota (1000 left)
        {"X999", 1},   // rejected: unknown id
    });

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    // Direct purchase: errors are reported with exceptions
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try {
        acq.purchase("B002", 1);
    } catch (const QuotaExceededError& e) {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }
    demonstrateAssignmentExtensions();
    return 0;
}
