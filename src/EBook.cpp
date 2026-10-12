#include "bookmgmt/EBook.h"

#include <ostream>
#include <utility>

#include "bookmgmt/Book.h"

namespace bookmgmt {

namespace {
const char* formatName(EBookFormat format) {
    switch (format) {
        case EBookFormat::PDF: return "PDF";
        case EBookFormat::EPUB: return "EPUB";
        case EBookFormat::HTML: return "HTML";
    }
    return "Unknown";
}
}  // namespace

EBook::EBook(std::string id, std::string title, std::string publisher,
             int year, Money pricePerSeat, std::string accessUrl,
             std::vector<std::string> authors, std::string isbn,
             EBookFormat format, bool drmProtected,
             LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title), std::move(publisher),
                         year, pricePerSeat, std::move(accessUrl), license,
                         platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drmProtected_(drmProtected) {}

void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  file format: " << formatName(format_) << "\n"
       << "  drm protected: " << (drmProtected_ ? "yes" : "no") << "\n";
}

// Book and EBook both store authors and ISBN. A shared BibliographicInfo
// component could hold that metadata and its formatting for both classes.

}  // namespace bookmgmt
