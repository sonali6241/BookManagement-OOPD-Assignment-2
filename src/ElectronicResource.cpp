#include "bookmgmt/ElectronicResource.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

const char* licenseName(LicenseModel m) {
    switch (m) {
        case LicenseModel::Perpetual: return "Perpetual";
        case LicenseModel::AnnualSubscription: return "Annual subscription";
    }
    return "Unknown";
}

ElectronicResource::ElectronicResource(std::string id, std::string title,
                                       std::string publisher, int year,
                                       Money pricePerSeat, std::string accessUrl,
                                       LicenseModel license, Money platformFee)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, pricePerSeat),
      accessUrl_(std::move(accessUrl)),
      license_(license),
      platformFee_(platformFee) {
    if (platformFee_.isZero() &&
        platformFee_.currencyCode() != pricePerSeat.currencyCode())
        platformFee_ = Money::fromMinor(0, pricePerSeat.currencyCode());
    if (platformFee_.isNegative())
        throw std::invalid_argument("platform fee must not be negative");
}

Money ElectronicResource::costFor(int seats) const {
    requirePositive(seats);

    const int fullPriceSeats = seats < 50 ? seats : 50;
    const int halfPriceSeats = seats > 50 ? seats - 50 : 0;

    Money seatCost = unitPrice() * fullPriceSeats;
    seatCost += Money::fromMinor(
        (unitPrice().minorUnits() * halfPriceSeats) / 2, unitPrice().currencyCode());

    return platformFee_ + seatCost;
}

void ElectronicResource::printDetails(std::ostream& os) const {
    os << "  access url: " << accessUrl_ << "\n"
       << "  license: " << licenseName(license_) << "\n"
       << "  platform fee: " << platformFee_ << "\n";
}

}  // namespace bookmgmt

