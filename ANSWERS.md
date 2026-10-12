# Assignment 2 answers

## Questions attempted

Q1–Q16 are implemented. The test program exercises the new resource types,
pricing rules, acquisition operations, searches, lending, and currency checks.

## Design decisions

- **Q2:** `EBook` derives from `ElectronicResource`, so its platform fee,
  license, and per-seat pricing behavior are inherited. `printDetails()` calls
  `ElectronicResource::printDetails()` before printing ebook metadata.
- **Q3:** `AudioBook` derives from `ElectronicResource` because it is licensed
  by concurrent-user seats. `Thesis` derives directly from `Resource` because
  its academic metadata and usual zero price do not make it a print book or a
  seat-licensed resource.
- **Q6:** Tax rates are configurable as integer percentages. Tax is rounded to
  the nearest minor currency unit; quota checks use the tax-inclusive amount.
- **Q7:** A title is identified by its resource ID. Buying the same ID again
  uses units and spending but does not consume another distinct-title slot.
- **Q8:** Cancellation adds a negative refund record and keeps the original
  purchase in history. It is rejected if the units being cancelled are still
  borrowed or in active electronic sessions.
- **Q9:** Department names identify separate `Budget` objects. An empty
  department in a request uses the original overall budget.
- **Q10:** `rollover(percent)` returns a new budget whose total is the selected
  percentage of the current unspent amount and whose category quotas carry over.
- **Q11:** Atomic batches are preflighted against copies of all affected
  budgets. If one request fails, each request is recorded as rejected and no
  holdings or real budget usage is changed.
- **Q12:** Vendor offers are stored per resource ID. The resource price is set
  to the least expensive offer and the selected vendor is recorded on orders.
- **Q14:** Lending and sessions are tracked by resource ID and patron ID.
  Holdings represent print copies or licensed digital seats.
- **Q15:** `Money` defaults to INR and validates three uppercase currency
  letters. Addition, subtraction, equality, and ordering reject mismatched
  currencies.
- **Q16:** The test program uses standard `assert` checks and a small
  `assertThrows` helper for exception cases.

## Written answer for Q2

`Book` and `EBook` both store and display authors and an ISBN. A shared
`BibliographicInfo` value type (or a common bibliographic base class) could own
that metadata and formatting, allowing both resource types to reuse it.
