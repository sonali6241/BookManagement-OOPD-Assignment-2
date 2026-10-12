// Tests use standard assert instead of a custom CHECK macro system.

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

template <typename Exception, typename Callable>
static void assertThrows(Callable&& callable) {
    bool thrown = false;
    try {
        callable();
    } catch (const Exception&) {
        thrown = true;
    } catch (...) {
    }
    assert(thrown);
}

static void testMoney() {
    assert(Money::of(12, 5).toString() == "12.05");
    assert(Money::of(-3, 50).toString() == "-3.50");
    assert(Money::fromMinor(7).toString() == "0.07");
    assert(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    assert(Money::of(3) * 4 == Money::of(12));
    assert(Money::of(1) < Money::of(2));
    assert(Money::of(2, 50, "USD").currencyCode() == "USD");
    assertThrows<std::invalid_argument>([] { (void)Money::of(1, 100); });
    assertThrows<std::invalid_argument>([] { (void)Money::of(1, 0, "US"); });
    assertThrows<std::invalid_argument>([] { (void)(Money::of(1) + Money::of(1, 0, "USD")); });
    assertThrows<std::invalid_argument>([] { (void)(Money::of(1) - Money::of(1, 0, "USD")); });
    assertThrows<std::invalid_argument>([] { (void)(Money::of(1) < Money::of(1, 0, "USD")); });
    assertThrows<std::invalid_argument>([] { (void)(Money::of(1) == Money::of(1, 0, "USD")); });

    Catalog catalog;
    catalog.emplace<ElectronicResource>("USD1", "USD Database", "Publisher", 2026,
                                        Money::of(10, 0, "USD"), "url");
    Budget usdBudget(Money::of(1000, 0, "USD"));
    usdBudget.setQuota(ResourceCategory::ElectronicResource,
                       {10, Money::of(500, 0, "USD")});
    AcquisitionManager manager(catalog, usdBudget);
    assert(manager.quote("USD1", 2) == Money::of(20, 0, "USD"));
    manager.purchase("USD1", 2);
    assert(usdBudget.spent() == Money::of(20, 0, "USD"));
}

static void testResourceTypesAndPricing() {
    Book paperback("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020,
                   Money::of(100));
    assert(paperback.category() == ResourceCategory::Book);
    assert(!paperback.isDigital());
    assert(paperback.costFor(3) == Money::of(300));
    assert(paperback.costFor(10) == Money::of(900));
    assert(joinAuthors(paperback.authors()) == "A, B and C");
    assertThrows<std::invalid_argument>([&] { (void)paperback.costFor(0); });

    Book hardcover("BH", "Hardcover", {"A"}, "isbn", "P", 2025,
                   Money::of(100), 1, Binding::Hardcover);
    assert(hardcover.costFor(2) == Money::of(240));
    assert(hardcover.costFor(10) == Money::of(1080));

    Journal journal("J1", "Science Today", "1234-5678", 12, "Publisher",
                    2025, Money::of(20), 3);
    assert(journal.category() == ResourceCategory::Journal);
    assert(journal.costFor(2) == Money::of(120));
    assert(journal.costFor(10) == Money::of(540));
    assertThrows<std::invalid_argument>([&] { (void)journal.costFor(0); });
    assertThrows<std::invalid_argument>([] {
        (void)Journal("J2", "Old Journal", "8765-4321", 12,
                      "Publisher", 2025, Money::of(20), 0);
    });
    std::ostringstream journalDetails;
    journal.print(journalDetails);
    assert(journalDetails.str().find("subscription years: 3") != std::string::npos);

    ElectronicResource electronic("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(100));
    assert(electronic.isDigital());
    assert(electronic.costFor(5) == Money::of(150));
    assert(electronic.costFor(50) == Money::of(600));
    assert(electronic.costFor(51) == Money::of(605));
    assert(electronic.category() == ResourceCategory::ElectronicResource);

    EBook ebook("E1", "C++ Guide", "Example Press", 2025, Money::of(10),
                "https://example.test/ebook", {"A. Author"}, "978-1234567890",
                EBookFormat::EPUB, true, LicenseModel::AnnualSubscription,
                Money::of(50));
    assert(ebook.category() == ResourceCategory::EBook);
    assert(ebook.isDigital());
    assert(ebook.costFor(5) == Money::of(100));
    assert(ebook.costFor(51) == Money::of(555));
    std::ostringstream ebookDetails;
    ebook.print(ebookDetails);
    assert(ebookDetails.str().find("platform fee: 50.00") != std::string::npos);
    assert(ebookDetails.str().find("file format: EPUB") != std::string::npos);
    assert(ebookDetails.str().find("drm protected: yes") != std::string::npos);

    AudioBook audio("A1", "C++ Audio", "Sound Press", 2024,
                    Money::of(15), "https://example.test/audio", "N. Narrator",
                    360, LicenseModel::AnnualSubscription, Money::of(30));
    assert(audio.category() == ResourceCategory::AudioBook);
    assert(audio.isDigital());
    assert(audio.costFor(2) == Money::of(60));
    std::ostringstream audioDetails;
    audio.print(audioDetails);
    assert(audioDetails.str().find("narrator: N. Narrator") != std::string::npos);

    Thesis thesis("T1", "Compiler Thesis", "IIIT Delhi", "M.Tech",
                  "Dr. Advisor", 2025);
    assert(thesis.category() == ResourceCategory::Thesis);
    assert(thesis.costFor(1) == Money{});
    std::ostringstream thesisDetails;
    thesis.print(thesisDetails);
    assert(thesisDetails.str().find("degree: M.Tech") != std::string::npos);

    assertThrows<std::invalid_argument>([] {
        (void)Book("", "T", {}, "", "", 2000, Money::of(1));
    });
    assertThrows<std::invalid_argument>([] {
        (void)Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1));
    });
}

static void testCatalogSearchAndLending() {
    Catalog catalog;
    catalog.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"Robert Martin"},
                          "978-111", "Press", 2008, Money::of(100));
    catalog.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"Robert Martin"},
                          "978-222", "Press", 2017, Money::of(100));
    catalog.emplace<EBook>("E1", "Modern C++", "Press", 2021, Money::of(10), "url",
                           std::vector<std::string>{"Bjarne Stroustrup"}, "978-333",
                           EBookFormat::PDF, false);
    catalog.emplace<Journal>("J1", "Science", "1234-5678", 12, "Publisher",
                             2020, Money::of(20));
    catalog.emplace<ElectronicResource>("R1", "Database", "Publisher", 2026,
                                        Money::of(5), "url");

    assert(catalog.size() == 5);
    assert(catalog.searchTitle("clean").size() == 2);
    assert(catalog.searchAuthor("robert").size() == 2);
    assert(catalog.searchAuthor("bjarne").size() == 1);
    assert(catalog.searchIsbnIssn("978-333").size() == 1);
    assert(catalog.searchIsbnIssn("1234-5678").size() == 1);
    assert(catalog.byPublicationYearRange(2017, 2021).size() == 3);
    assertThrows<std::invalid_argument>([&] {
        (void)catalog.byPublicationYearRange(2022, 2020);
    });

    catalog.addHoldings("B1", 1);
    catalog.borrowCopy("B1", "P1");
    assert(catalog.borrowedCopies("B1") == 1);
    assertThrows<std::invalid_argument>([&] { catalog.borrowCopy("B1", "P2"); });
    catalog.returnCopy("B1", "P1");
    assert(catalog.borrowedCopies("B1") == 0);

    catalog.addHoldings("R1", 2);
    catalog.openSession("R1", "P1");
    catalog.openSession("R1", "P2");
    assert(catalog.activeSessions("R1") == 2);
    assertThrows<std::invalid_argument>([&] { catalog.openSession("R1", "P3"); });
    catalog.closeSession("R1", "P1");
    assert(catalog.activeSessions("R1") == 1);
    assertThrows<std::invalid_argument>([&] { catalog.borrowCopy("R1", "P1"); });
    assertThrows<std::invalid_argument>([&] { catalog.closeSession("R1", "P3"); });
}

static void testBudgetTitlesAndRollover() {
    Budget budget(Money::of(1000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(1000), 1});
    budget.commit(ResourceCategory::Book, 2, Money::of(200), "B1");
    assert(budget.usageFor(ResourceCategory::Book).titles == 1);
    assert(budget.check(ResourceCategory::Book, 1, Money::of(10), "B2").find("title") !=
           std::string::npos);
    assert(budget.check(ResourceCategory::Book, 1, Money::of(10), "B1").empty());
    budget.commit(ResourceCategory::Book, 1, Money::of(100), "B1");
    budget.refund(ResourceCategory::Book, 3, Money::of(300), "B1");
    assert(budget.usageFor(ResourceCategory::Book).titles == 0);

    Budget current(Money::of(1000));
    current.setQuota(ResourceCategory::Book, {10, Money::of(500), 3});
    current.commit(ResourceCategory::Book, 2, Money::of(300), "B1");
    Budget next = current.rollover(50);
    assert(next.total() == Money::of(350));
    assert(next.spent() == Money{});
    assert(next.quotaFor(ResourceCategory::Book)->maxTitles == 3);
    assertThrows<std::invalid_argument>([&] { (void)current.rollover(101); });
}

static void testDepartmentBudgets() {
    Catalog catalog;
    catalog.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P",
                          2020, Money::of(100));
    Budget general(Money::of(1000));
    AcquisitionManager manager(catalog, general);
    Budget& cs = manager.addDepartmentBudget("Computer Science", Money::of(300));
    cs.setQuota(ResourceCategory::Book, {2, Money::of(300), 2});
    manager.addDepartmentBudget("Physics", Money::of(150));
    const auto& record = manager.purchase("B1", 2, "Computer Science");
    assert(record.department == "Computer Science");
    assert(cs.spent() == Money::of(200));
    assert(manager.departmentBudget("Physics").spent() == Money{});
    assert(general.spent() == Money{});
}

static void testAtomicBatches() {
    Catalog catalog;
    catalog.emplace<Book>("B1", "Affordable", std::vector<std::string>{"A"}, "i", "P",
                          2020, Money::of(50));
    catalog.emplace<Book>("B2", "Too costly", std::vector<std::string>{"B"}, "j", "P",
                          2021, Money::of(200));
    Budget budget(Money::of(150));
    AcquisitionManager manager(catalog, budget);
    const auto result = manager.processBatch({{"B1", 1}, {"B2", 1}}, true);
    assert(result.size() == 2);
    assert(!result[0].approved && !result[1].approved);
    assert(budget.spent() == Money{});
    assert(catalog.holdings("B1") == 0 && catalog.holdings("B2") == 0);
}

static void testVendors() {
    Catalog catalog;
    catalog.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P",
                          2020, Money::of(100));
    Budget budget(Money::of(1000));
    AcquisitionManager manager(catalog, budget);
    manager.addVendorOffer("B1", "Vendor A", Money::of(90));
    manager.addVendorOffer("B1", "Vendor B", Money::of(80));
    assert(manager.vendorsFor("B1").size() == 2);
    assert(manager.quote("B1", 2) == Money::of(160));
    const auto& record = manager.purchase("B1", 2);
    assert(record.vendor == "Vendor B");
    assert(record.cost == Money::of(160));
}

static void testTaxAndCancellation() {
    Catalog catalog;
    catalog.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P",
                          2020, Money::of(100));
    catalog.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                        LicenseModel::AnnualSubscription, Money::of(20));
    Budget budget(Money::of(5000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(219), 2});
    AcquisitionManager manager(catalog, budget);
    manager.setTaxRates(10, 20);
    assert(manager.quote("B1", 2) == Money::of(200));
    assert(manager.quoteAfterTax("B1", 2) == Money::of(220));
    std::string reason;
    assert(!manager.canPurchase("B1", 2, &reason));
    assert(reason.find("spend quota") != std::string::npos);
    auto& bookBudget = budget;
    bookBudget.setQuota(ResourceCategory::Book, {10, Money::of(220), 2});
    const int orderNo = manager.purchase("B1", 2).orderNo;
    assert(budget.spent() == Money::of(220));
    assert(manager.history().front().preTaxCost == Money::of(200));
    assert(manager.history().front().cost == Money::of(220));
    manager.purchase("R1", 2);
    assert(manager.history().back().preTaxCost == Money::of(40));
    assert(manager.history().back().cost == Money::of(48));
    std::ostringstream report;
    manager.printReport(report);
    assert(report.str().find("Total pre-tax: 240.00") != std::string::npos);
    assert(report.str().find("Total post-tax: 268.00") != std::string::npos);

    manager.cancel(orderNo);
    assert(budget.spent() == Money::of(48));
    assert(catalog.holdings("B1") == 0);
    assert(manager.history().size() == 3);
    assert(manager.history()[0].cancelled);
    assert(manager.history()[2].cancellation);
    assert(manager.totalSpent() == Money::of(48));
    assertThrows<std::invalid_argument>([&] { manager.cancel(orderNo); });
    assertThrows<std::invalid_argument>([&] { manager.setTaxRates(101, 0); });
}

int main() {
    testMoney();
    testResourceTypesAndPricing();
    testCatalogSearchAndLending();
    testBudgetTitlesAndRollover();
    testDepartmentBudgets();
    testAtomicBatches();
    testVendors();
    testTaxAndCancellation();
}
