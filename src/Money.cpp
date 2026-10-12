#include "bookmgmt/Money.h"

#include <cctype>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

namespace {
void validateCurrencyCode(const std::string& code) {
    if (code.size() != 3)
        throw std::invalid_argument("currency code must contain three uppercase letters");
    for (unsigned char ch : code) {
        if (!std::isupper(ch))
            throw std::invalid_argument("currency code must contain three uppercase letters");
    }
}
}  // namespace

Money Money::fromMinor(std::int64_t minor, std::string currencyCode) {
    validateCurrencyCode(currencyCode);
    return Money(minor, std::move(currencyCode));
}

Money Money::of(std::int64_t major, int minor, std::string currencyCode) {
    if (minor < 0 || minor > 99)
        throw std::invalid_argument("minor part must be in 0..99");
    validateCurrencyCode(currencyCode);
    const std::int64_t sign = major < 0 ? -1 : 1;
    return Money(major * 100 + sign * minor, std::move(currencyCode));
}

void Money::requireSameCurrency(Money other) const {
    if (currencyCode_ != other.currencyCode_)
        throw std::invalid_argument("currency mismatch: " + currencyCode_ + " and " +
                                    other.currencyCode_);
}

Money& Money::operator+=(Money other) {
    requireSameCurrency(other);
    minor_ += other.minor_;
    return *this;
}

Money& Money::operator-=(Money other) {
    requireSameCurrency(other);
    minor_ -= other.minor_;
    return *this;
}

bool operator==(Money a, Money b) {
    a.requireSameCurrency(b);
    return a.minor_ == b.minor_;
}

bool operator<(Money a, Money b) {
    a.requireSameCurrency(b);
    return a.minor_ < b.minor_;
}

std::string Money::toString() const {
    const std::int64_t absoluteMinor = minor_ < 0 ? -minor_ : minor_;
    std::string result = std::to_string(absoluteMinor / 100);
    const auto fraction = absoluteMinor % 100;
    result += '.';
    if (fraction < 10) result += '0';
    result += std::to_string(fraction);
    return minor_ < 0 ? "-" + result : result;
}

std::ostream& operator<<(std::ostream& os, Money amount) {
    return os << amount.toString() << " " << amount.currencyCode();
}

}  // namespace bookmgmt
