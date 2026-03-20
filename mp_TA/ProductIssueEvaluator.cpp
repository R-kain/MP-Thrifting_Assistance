#include "ProductIssueEvaluator.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace
{
    constexpr double kBaseScore = 100.0;
    constexpr double kScratchWeight = 25.0;
    constexpr double kStainWeight = 20.0;
    constexpr double kDiscolorationWeight = 18.0;
    constexpr double kEdgeDamageWeight = 22.0;
    constexpr double kShapeDeformationWeight = 15.0;
    constexpr double kConfidenceBonusWeight = 5.0;
}

double ProductIssueEvaluator::Clamp(double value, double minValue, double maxValue)
{
    return std::clamp(value, minValue, maxValue);
}

std::wstring ProductIssueEvaluator::CalculateGrade(double score)
{
    if (score >= 90.0)
    {
        return L"A";
    }

    if (score >= 75.0)
    {
        return L"B";
    }

    if (score >= 60.0)
    {
        return L"C";
    }

    return L"D";
}

std::wstring ProductIssueEvaluator::BuildSummary(const ImageInspectionMetrics& metrics, double score)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(1);
    stream << L"Appearance score: " << score << L". ";

    if (metrics.analysisConfidence < 0.4)
    {
        stream << L"Analysis confidence is low, so more photos are recommended.";
        return stream.str();
    }

    if (metrics.scratchSeverity >= 0.6 || metrics.edgeDamageSeverity >= 0.6)
    {
        stream << L"Surface scratches and edge damage are the main reasons for the score reduction.";
    }
    else if (metrics.stainSeverity >= 0.5 || metrics.discolorationSeverity >= 0.5)
    {
        stream << L"Stains or discoloration are the main factors lowering the quality score.";
    }
    else
    {
        stream << L"The overall exterior condition looks good.";
    }

    return stream.str();
}

ProductScoreReport ProductIssueEvaluator::Evaluate(const ImageInspectionMetrics& metrics) const
{
    const double scratchPenalty = Clamp(metrics.scratchSeverity, 0.0, 1.0) * kScratchWeight;
    const double stainPenalty = Clamp(metrics.stainSeverity, 0.0, 1.0) * kStainWeight;
    const double discolorationPenalty = Clamp(metrics.discolorationSeverity, 0.0, 1.0) * kDiscolorationWeight;
    const double edgePenalty = Clamp(metrics.edgeDamageSeverity, 0.0, 1.0) * kEdgeDamageWeight;
    const double deformationPenalty = Clamp(metrics.shapeDeformationSeverity, 0.0, 1.0) * kShapeDeformationWeight;
    const double confidenceBonus = Clamp(metrics.analysisConfidence, 0.0, 1.0) * kConfidenceBonusWeight;

    const double finalScore = Clamp(
        kBaseScore - scratchPenalty - stainPenalty - discolorationPenalty - edgePenalty - deformationPenalty + confidenceBonus,
        0.0,
        100.0);

    ProductScoreReport report;
    report.score = finalScore;
    report.grade = CalculateGrade(finalScore);
    report.detectedIssues = metrics.detectedIssues;
    report.summary = BuildSummary(metrics, finalScore);
    return report;
}
