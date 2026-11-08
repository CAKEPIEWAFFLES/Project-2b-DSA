#ifndef PROJECT2_HARNESS_H
#define PROJECT2_HARNESS_H

#include <string>
#include <vector>

#include "data.h"

using namespace std;

struct BenchmarkRow {
    string approach;
    size_t numRatings;
    size_t numLikes;
    double buildMs;
    size_t numQueries;
    double avgQueryMs;
    double p95QueryMs;
    double matchBruteForcePct;
};

struct BenchmarkConfig {
    vector<int> percents = {25, 50, 75, 100};
    int queriesPerRun = 200;
    int minLikesForQuery = 20;
};

vector<BenchmarkRow> runBenchmark(const Dataset& data, const vector<string>& approaches,
                                  const BenchmarkConfig& config);

bool writeBenchmarkCsv(const vector<BenchmarkRow>& rows, const string& path);

void printBenchmark(const vector<BenchmarkRow>& rows);

int runSelfTests();

#endif
