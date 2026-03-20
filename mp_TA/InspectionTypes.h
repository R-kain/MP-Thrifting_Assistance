#pragma once

#include <string>
#include <vector>

struct ImageInspectionMetrics
{
    double scratchSeverity = 0.0;
    double stainSeverity = 0.0;
    double discolorationSeverity = 0.0;
    double edgeDamageSeverity = 0.0;
    double shapeDeformationSeverity = 0.0;
    double analysisConfidence = 0.0;
    std::vector<std::string> detectedIssues;

};

struct ProductScoreReport
{
    double score = 0.0;

    std::string grade;
    std::vector<std::string> detectedIssues;
    std::string summary;
};
