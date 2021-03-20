//
// Created by berg on 12/07/19.
//

#include <algorithm>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <zconf.h>

#include "../header/Experiment.h"
#include "../header/utils.h"
#include "../header/QueryResult.h"

using namespace std;

string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

Experiment::Experiment(map<string, string> config) {
    this->config = std::move(config);
    this->editDistanceThreshold = stoi(this->config["edit_distance"]);
}

void setVector(int position, unsigned long value, vector<long> &v) {
    if (position < v.size()) {
        v[position] += value;
    } else {
        for (int i = v.size(); i <= position; i++) {
            if (i == position) {
                v.push_back(value);
            } else {
                v.push_back(0);
            }
        }
    }
}

void Experiment::writeFile(const string& name, const string& value, bool writeInTheEnd) {
    ofstream myfile;
    string newName = config["experiments_basepath"] + name;
    newName += "_data_set_" + config["dataset"] + "_size_type_" + config["size_type"] +
            "_tau_" + to_string(this->editDistanceThreshold) + "_alg_" + config["alg"] + ".txt";
//    cout << newName << "\n";
    if (writeInTheEnd) {
        myfile.open(newName, std::ios::app);
    } else {
        myfile.open(newName);
    }

    if (myfile.is_open()) {
        myfile << value;
        myfile.close();
    } else {
        cout << "Unable to open file.\n";
    }
}

void Experiment::initIndexingTime() {
    this->startIndexingTime = chrono::high_resolution_clock::now();
}

void Experiment::endIndexingTime() {
    this->finishIndexingTime = chrono::high_resolution_clock::now();

    long result = chrono::duration_cast<chrono::nanoseconds>(
            this->finishIndexingTime - this->startIndexingTime
            ).count();


    writeFile("indexing_time", "indexing_time\n" + to_string(result) + "\n");
}

void Experiment::initQueryFetchingTime() {
    this->startQueryFetchingTime = chrono::high_resolution_clock::now();
}

void Experiment::endQueryFetchingTime(string &query, long resultsSize_) {
    this->finishQueryFetchingTime = chrono::high_resolution_clock::now();

    int currentQueryLength = query.size();

    long result = chrono::duration_cast<chrono::nanoseconds>(
            this->finishQueryFetchingTime - this->startQueryFetchingTime
    ).count();

    setVector(currentQueryLength - 1, result, this->currentQueryFetchingTime);
    setVector(currentQueryLength - 1, resultsSize_, this->currentResultsSize);
    setVector(currentQueryLength - 1, result, this->fetchingTimes);
    setVector(currentQueryLength - 1, resultsSize_, this->resultsSize);
}

void Experiment::endSimpleQueryFetchingTime(unsigned long resultsSize_) {
    this->finishQueryFetchingTime = chrono::high_resolution_clock::now();

    this->simpleFetchingTimes = chrono::duration_cast<chrono::nanoseconds>(
            this->finishQueryFetchingTime - this->startQueryFetchingTime
    ).count();

    this->simpleResultsSize = resultsSize_;
}

void Experiment::initQueryProcessingTime() {
    this->startQueryProcessingTime = chrono::high_resolution_clock::now();
}

void Experiment::endQueryProcessingTime(long activeNodesSize, string &query) {
    this->finishQueryProcessingTime = chrono::high_resolution_clock::now();

    long currentQueryLength = query.size();

    long result = chrono::duration_cast<chrono::nanoseconds>(
            this->finishQueryProcessingTime - this->startQueryProcessingTime
    ).count();

    setVector(currentQueryLength - 1, result, this->currentQueryProcessingTime);
    setVector(currentQueryLength - 1, activeNodesSize, this->currentActiveNodesSize);
    setVector(currentQueryLength - 1, result, this->processingTimes);
    setVector(currentQueryLength - 1, activeNodesSize, this->activeNodesSizes);
}

void Experiment::endSimpleQueryProcessingTime(long activeNodesSize) {
    this->finishQueryProcessingTime = chrono::high_resolution_clock::now();

    this->simpleProcessingTimes = chrono::duration_cast<chrono::nanoseconds>(
            this->finishQueryProcessingTime - this->startQueryProcessingTime
    ).count();

    this->simpleActiveNodesSizes = activeNodesSize;
}

void Experiment::saveQueryProcessingTime(string& query, int queryId) {
    string value = query + "\t" + to_string(queryId) + "\n";

    long accum = 0;
    for (int j = 0; j < this->currentQueryProcessingTime.size(); j++) {
        accum += this->currentQueryProcessingTime[j];

        value += to_string(j + 1) + "\t" + to_string(this->currentQueryProcessingTime[j]) + "\t" +
                 to_string(accum) + "\t" + to_string(this->currentQueryFetchingTime[j]) + "\t" +
                 to_string(this->currentResultsSize[j]) + "\t" + to_string(this->currentActiveNodesSize[j]) + "\n";
    }

    this->currentResultsSize.clear();
    this->currentActiveNodesSize.clear();
    this->currentQueryProcessingTime.clear();
    this->currentQueryFetchingTime.clear();

    writeFile("all_time_values", value, true);
}

void Experiment::compileQueryProcessingTimes(int queryId) {
    string value = to_string(queryId) + "\n";
    value += "query_size\tquery_processing_time\taccumulated_query_processing_time\tfetching_time\tresults_size\t"
             "active_nodes_size\n";

    int accum = 0;
    for (int i = 0; i < this->processingTimes.size(); i++) {
        long processingTime = this->processingTimes[i] / (queryId + 1);
        long fetchingTime = this->fetchingTimes[i] / (queryId + 1);
        float _resultsSize = this->resultsSize[i] / (queryId + 1);
        float activeNodesSize = this->activeNodesSizes[i] / (queryId + 1);
        stringstream streamResultSize;
        streamResultSize << std::fixed << std::setprecision(1) << _resultsSize;
        stringstream stream;
        stream << std::fixed << std::setprecision(1) << activeNodesSize;
        accum += processingTime;
        value += to_string(i + 1) + "\t" + to_string(processingTime) +
                "\t" + to_string(accum) + "\t" + to_string(fetchingTime) + "\t" + streamResultSize.str() +
                "\t" + stream.str() + "\n";
    }

    writeFile("query_processing_time", value);
}

void Experiment::compileSimpleQueryProcessingTimes(string &query, bool relevantReturned) {
    string value = query + "\t" + to_string(this->simpleProcessingTimes) + "\t" +
                   to_string(this->simpleFetchingTimes) + "\t" + to_string(this->simpleResultsSize) + "\t" +
                   to_string(this->simpleActiveNodesSizes) + "\t" + to_string(int(relevantReturned)) + "\n";

    writeFile("all_time_values", value, true);
}

void Experiment::proportionOfBranchingSize(int size) {
    if (this->branchSize.find(size) == this->branchSize.end() ) {
        this->branchSize[size] = 1;
    } else {
        this->branchSize[size]++;
    }
}

void Experiment::compileProportionOfBranchingSize() {
    string value = "branch_size\tnumber_of_branches\n";
    for (map<int, int>::iterator it = this->branchSize.begin(); it != this->branchSize.end(); ++it) {
        value += to_string(it->first) + "\t" + to_string(it->second) + "\n";
    }
    writeFile("proportion_branch_size", value);
}

void Experiment::incrementNumberOfNodes() {
    this->numberOfNodes++;
}

void Experiment::compileNumberOfNodes() {
    string value = "number_of_nodes\n";
    value += to_string(this->numberOfNodes) + "\n";
    writeFile("number_of_nodes", value);
}

void Experiment::getMemoryUsedInProcessing() {
    pid_t pid = getpid();
    string cmd = "/bin/ps -p " + to_string(pid) + " -o size";
    string output = exec(cmd.c_str());

    vector<string> tokens = utils::split(output, '\n');

    float memoryUsed = stof(tokens[1]) / 1000;

    string value = to_string(memoryUsed) + "\n";

    writeFile("memory_used_in_processing", value, true);
}

void Experiment::getMemoryUsedInIndexing() {
    pid_t pid = getpid();
    string cmd = "/bin/ps -p " + to_string(pid) + " -o size";
    string output = exec(cmd.c_str());

    vector<string> tokens = utils::split(output, '\n');

    float memoryUsed = stof(tokens[1]) / 1000;

    string value = to_string(memoryUsed) + "\n";

    writeFile("memory_used_in_indexing", "memory_total_mb\tmemory_used_mb\n" + value);
}
