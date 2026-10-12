#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

// A thesis is an academic work with its own metadata, not a print book
// or a seat-licensed electronic resource, so it derives directly from Resource.
class Thesis : public Resource {
public:
    Thesis(std::string id, std::string title, std::string university,
           std::string degree, std::string supervisor, int year,
           Money unitPrice = Money{});

    const std::string& university() const { return university_; }
    const std::string& degree() const { return degree_; }
    const std::string& supervisor() const { return supervisor_; }

    ResourceCategory category() const override {
        return ResourceCategory::Thesis;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

}  // namespace bookmgmt