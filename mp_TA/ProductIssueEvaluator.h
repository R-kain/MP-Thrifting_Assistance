#pragma once

#include "InspectionTypes.h"

class ProductIssueEvaluator
{
public:
    [[nodiscard]] ProductScoreReport Evaluate(const ImageInspectionMetrics& metrics) const;

private:
    [[nodiscard]] static double Clamp(double value, double minValue, double maxValue);
    [[nodiscard]] static std::wstring CalculateGrade(double score);
    [[nodiscard]] static std::wstring BuildSummary(const ImageInspectionMetrics& metrics, double score);

};
