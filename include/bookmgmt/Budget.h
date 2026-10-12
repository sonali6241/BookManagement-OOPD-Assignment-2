#pragma once
// Budget tracks total spending and per-category unit, spend, and title limits.

#include <iosfwd>
#include <map>
#include <optional>
#include <string>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct Quota {
    int maxUnits;
    Money maxSpend;
    int maxTitles = -1;  // -1 means no distinct-title limit.
};

struct Usage {
    int units = 0;
    Money spent;
    int titles = 0;
};

class Budget {
public:
    explicit Budget(Money total);

    Money total() const { return total_; }
    Money spent() const { return spent_; }
    Money remaining() const { return total_ - spent_; }

    void setQuota(ResourceCategory category, Quota quota);
    void removeQuota(ResourceCategory category);
    std::optional<Quota> quotaFor(ResourceCategory category) const;
    Usage usageFor(ResourceCategory category) const;

    std::optional<int> unitsRemaining(ResourceCategory category) const;
    std::optional<int> titlesRemaining(ResourceCategory category) const;
    std::optional<Money> spendRemaining(ResourceCategory category) const;

    std::string check(ResourceCategory category, int units, Money cost,
                      const std::string& titleId = {}) const;
    void commit(ResourceCategory category, int units, Money cost,
                const std::string& titleId = {});
    void refund(ResourceCategory category, int units, Money cost,
                const std::string& titleId = {});

    Budget rollover(int carryForwardPercent) const;
    void print(std::ostream& os) const;

private:
    enum class Failure { None, BadInput, Quota, Overall };
    Failure evaluate(ResourceCategory category, int units, Money cost,
                     const std::string& titleId, std::string& why) const;

    Money total_;
    Money spent_;
    std::map<ResourceCategory, Quota> quotas_;
    std::map<ResourceCategory, Usage> usage_;
    std::map<ResourceCategory, std::map<std::string, int>> titleUnits_;
};

}  // namespace bookmgmt
