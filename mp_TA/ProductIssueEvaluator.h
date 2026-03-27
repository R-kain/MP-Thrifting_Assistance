#pragma once

#include "InspectionTypes.h"

class ProductIssueEvaluator
{
public:
    [[nodiscard]] ProductScoreReport Evaluate(const ImageInspectionMetrics& metrics) const;

private:
    [[nodiscard]] static double Clamp(double value, double minValue, double maxValue);
    [[nodiscard]] static std::string CalculateGrade(double score);
    [[nodiscard]] static std::string BuildSummary(const ImageInspectionMetrics& metrics, double score);
};
