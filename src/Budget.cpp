#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
const ResourceCategory kAllCategories[] = {
    ResourceCategory::Book,
    ResourceCategory::ElectronicResource,
    ResourceCategory::Journal,
    ResourceCategory::EBook,
    ResourceCategory::AudioBook,
    ResourceCategory::Thesis
};
}  // namespace

Budget::Budget(Money total)
    : total_(total), spent_(Money::fromMinor(0, total.currencyCode())) {
    if (total_.isNegative()) throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory category, Quota quota) {
    if (quota.maxUnits < 0 || quota.maxSpend.isNegative() || quota.maxTitles < -1)
        throw std::invalid_argument("quota limits must not be negative");
    (void)(quota.maxSpend + total_);
    quotas_[category] = quota;
}

void Budget::removeQuota(ResourceCategory category) { quotas_.erase(category); }

std::optional<Quota> Budget::quotaFor(ResourceCategory category) const {
    auto it = quotas_.find(category);
    if (it == quotas_.end()) return std::nullopt;
    return it->second;
}

Usage Budget::usageFor(ResourceCategory category) const {
    auto it = usage_.find(category);
    return it == usage_.end() ? Usage{} : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory category) const {
    auto quota = quotaFor(category);
    if (!quota) return std::nullopt;
    return quota->maxUnits - usageFor(category).units;
}

std::optional<int> Budget::titlesRemaining(ResourceCategory category) const {
    auto quota = quotaFor(category);
    if (!quota || quota->maxTitles < 0) return std::nullopt;
    return quota->maxTitles - usageFor(category).titles;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory category) const {
    auto quota = quotaFor(category);
    if (!quota) return std::nullopt;
    Usage usage = usageFor(category);
    if (usage.units == 0 && usage.spent.isZero())
        usage.spent = Money::fromMinor(0, quota->maxSpend.currencyCode());
    return quota->maxSpend - usage.spent;
}

Budget::Failure Budget::evaluate(ResourceCategory category, int units, Money cost,
                                 const std::string& titleId, std::string& why) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }
    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }
    try {
        (void)(cost + total_);  // A budget has one currency throughout.
        if (const auto quota = quotaFor(category)) (void)(cost + quota->maxSpend);
    } catch (const std::invalid_argument& error) {
        why = error.what();
        return Failure::BadInput;
    }
    if (auto left = unitsRemaining(category); left && units > *left) {
        why = std::string(categoryName(category)) + " unit quota exceeded: requested " +
              std::to_string(units) + ", " + std::to_string(*left) + " remaining";
        return Failure::Quota;
    }
    if (!titleId.empty() && titlesRemaining(category)) {
        const auto categoryTitles = titleUnits_.find(category);
        const bool alreadyCounted = categoryTitles != titleUnits_.end() &&
                                    categoryTitles->second.count(titleId) != 0;
        if (!alreadyCounted) {
            if (auto left = titlesRemaining(category); left && *left <= 0) {
                why = std::string(categoryName(category)) +
                      " distinct-title quota exceeded";
                return Failure::Quota;
            }
        }
    }
    if (auto left = spendRemaining(category); left && cost > *left) {
        why = std::string(categoryName(category)) + " spend quota exceeded: cost " +
              cost.toString() + ", " + left->toString() + " remaining";
        return Failure::Quota;
    }
    if (cost > remaining()) {
        why = "overall budget exceeded: cost " + cost.toString() + ", " +
              remaining().toString() + " remaining";
        return Failure::Overall;
    }
    why.clear();
    return Failure::None;
}

std::string Budget::check(ResourceCategory category, int units, Money cost,
                          const std::string& titleId) const {
    std::string why;
    evaluate(category, units, cost, titleId, why);
    return why;
}

void Budget::commit(ResourceCategory category, int units, Money cost,
                    const std::string& titleId) {
    std::string why;
    switch (evaluate(category, units, cost, titleId, why)) {
        case Failure::None: break;
        case Failure::BadInput: throw std::invalid_argument(why);
        case Failure::Quota: throw QuotaExceededError(why);
        case Failure::Overall: throw BudgetExceededError(why);
    }
    Usage& usage = usage_[category];
    if (usage.units == 0 && usage.spent.isZero())
        usage.spent = Money::fromMinor(0, cost.currencyCode());
    usage.units += units;
    usage.spent += cost;
    spent_ += cost;
    if (!titleId.empty()) {
        auto& counts = titleUnits_[category];
        if (counts[titleId] == 0) ++usage.titles;
        counts[titleId] += units;
    }
}

void Budget::refund(ResourceCategory category, int units, Money cost,
                    const std::string& titleId) {
    if (units <= 0 || cost.isNegative())
        throw std::invalid_argument("refund values must be positive");
    auto usageIt = usage_.find(category);
    if (usageIt == usage_.end() || units > usageIt->second.units ||
        cost > usageIt->second.spent)
        throw std::invalid_argument("refund exceeds recorded usage");
    if (!titleId.empty()) {
        auto categoryIt = titleUnits_.find(category);
        if (categoryIt == titleUnits_.end())
            throw std::invalid_argument("title has no recorded usage");
        auto titleIt = categoryIt->second.find(titleId);
        if (titleIt == categoryIt->second.end() || units > titleIt->second)
            throw std::invalid_argument("refund exceeds title usage");
        titleIt->second -= units;
        if (titleIt->second == 0) {
            categoryIt->second.erase(titleIt);
            --usageIt->second.titles;
        }
    }
    usageIt->second.units -= units;
    usageIt->second.spent -= cost;
    spent_ -= cost;
}

Budget Budget::rollover(int carryForwardPercent) const {
    if (carryForwardPercent < 0 || carryForwardPercent > 100)
        throw std::invalid_argument("carry-forward percentage must be in 0..100");
    const auto carryMinor = remaining().minorUnits() * carryForwardPercent / 100;
    Budget next(Money::fromMinor(carryMinor, remaining().currencyCode()));
    next.quotas_ = quotas_;
    return next;
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
       << remaining() << "\n";
    os << std::left << std::setw(22) << "  Category" << std::setw(18) << "Units used/max"
       << std::setw(18) << "Titles used/max" << "Spend used/max\n";
    for (ResourceCategory category : kAllCategories) {
        const Usage usage = usageFor(category);
        const auto quota = quotaFor(category);
        const std::string units = std::to_string(usage.units) + "/" +
                                  (quota ? std::to_string(quota->maxUnits) : "-");
        const std::string titles = std::to_string(usage.titles) + "/" +
                                   (quota && quota->maxTitles >= 0
                                        ? std::to_string(quota->maxTitles) : "-");
        const std::string spend = usage.spent.toString() + "/" +
                                 (quota ? quota->maxSpend.toString() : "-");
        os << "  " << std::setw(20) << categoryName(category) << std::setw(18) << units
           << std::setw(18) << titles << spend << "\n";
    }
}

}  // namespace bookmgmt
