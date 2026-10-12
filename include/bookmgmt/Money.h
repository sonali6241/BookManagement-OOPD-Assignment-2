#pragma once
// Money: fixed-point amount in the minor units of a currency.

#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>

namespace bookmgmt {

class Money {
public:
    Money() = default;

    static Money fromMinor(std::int64_t minor, std::string currencyCode = "INR");
    static Money of(std::int64_t major, int minor = 0,
                    std::string currencyCode = "INR");

    std::int64_t minorUnits() const { return minor_; }
    const std::string& currencyCode() const { return currencyCode_; }
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(Money other);
    Money& operator-=(Money other);
    Money& operator*=(std::int64_t factor) { minor_ *= factor; return *this; }

    friend Money operator+(Money a, Money b) { return a += b; }
    friend Money operator-(Money a, Money b) { return a -= b; }
    friend Money operator*(Money amount, std::int64_t factor) { return amount *= factor; }
    friend Money operator*(std::int64_t factor, Money amount) { return amount *= factor; }

    friend bool operator==(Money a, Money b);
    friend bool operator!=(Money a, Money b) { return !(a == b); }
    friend bool operator<(Money a, Money b);
    friend bool operator<=(Money a, Money b) { return a == b || a < b; }
    friend bool operator>(Money a, Money b) { return b < a; }
    friend bool operator>=(Money a, Money b) { return a == b || b < a; }

private:
    Money(std::int64_t minor, std::string currencyCode)
        : minor_(minor), currencyCode_(std::move(currencyCode)) {}
    void requireSameCurrency(Money other) const;

    std::int64_t minor_ = 0;
    std::string currencyCode_ = "INR";
};

std::ostream& operator<<(std::ostream& os, Money amount);

}  // namespace bookmgmt


