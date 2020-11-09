//
// Created by berg on 12/07/19.
//

#ifndef BEVA_EXPERIMENT_H
#define BEVA_EXPERIMENT_H

#include <vector>
#include <map>
#include <chrono>
#include "QueryResult.h"

#include "sys/types.h"
#include "sys/sysinfo.h"

using namespace std;

class Experiment {
public:
    Experiment(map<string, string>);

    map<string, string>  config;
    int editDistanceThreshold;
    long numberOfNodes;
    vector<float> memoryUsedInProcessing;
    vector<long> processingTimes;
    vector<long> fetchingTimes;
    vector<float> resultsSize;
    vector<long> currentQueryProcessingTime;
    vector<long> currentQueryFetchingTime;
    vector<float> activeNodesSizes;
    vector<long> currentActiveNodesSize;
    vector<long> currentResultsSize;
    map<int, int> branchSize;

    bool recoveryMode = false;

    chrono::time_point<std::chrono::system_clock> startIndexingTime;
    chrono::time_point<std::chrono::system_clock> finishIndexingTime;

    chrono::time_point<std::chrono::system_clock> startQueryProcessingTime;
    chrono::time_point<std::chrono::system_clock> finishQueryProcessingTime;

    chrono::time_point<std::chrono::system_clock> startQueryFetchingTime;
    chrono::time_point<std::chrono::system_clock> finishQueryFetchingTime;

    void initIndexingTime();
    void endIndexingTime();
    void initQueryProcessingTime();
    void initQueryFetchingTime();
    void endQueryProcessingTime(long, string&);
    void endQueryFetchingTime(string&, long);
    void compileQueryProcessingTimes(int);
    void proportionOfBranchingSizeInBEVA2Level(int);
    void incrementNumberOfNodes();
    void compileNumberOfNodes();
    void compileProportionOfBranchingSizeInBEVA2Level();
    void readQueryProcessingTime(string& filename);
    void saveQueryProcessingTime(string&, int);
    void getMemoryUsedInIndexing();
    void getMemoryUsedInProcessing(int);

    void writeFile(const string&, const string&, bool writeInTheEnd=false);
};


#endif //BEVA_EXPERIMENT_H
