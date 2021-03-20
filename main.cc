/*

   Copyright (C) 2011 by The Department of Computer Science and
   Technology, Tsinghua University

   Redistribution of this file is permitted under the terms of
   the BSD license.

Author   : Dong Deng
Created  : 2014-09-05 11:42:44 
Modified : 2014-11-02 23:03:54
Contact  : dd11@mails.tsinghua.edu.cn

*/

#include "Trie.h"
#include "ActiveNode.h"
#include "header/Experiment.h"
#include "header/utils.h"
#include "header/Directives.h"

#include <algorithm>
#include <map>
#include <fstream>
#include <string>
#include <iostream>
#include <tr1/unordered_map>
#include <sstream>

using namespace tr1;

vector<string> recs;
vector<string> queries;
vector<string> relevantQueries;

void readData(string& filename, vector<string>& recs) {
	string str;
	ifstream input(filename, ios::in);
	while (getline(input, str)) {
//		for (auto i = 0; i < str.length(); i++) {
//		    str[i] = tolower(str[i]);
//		}
//
//		str = utils::normalize(str);
		recs.push_back(str);
	}
}

map<string, string> loadConfig() {

    std::ifstream is_file("./path.cfg");
    std::string line;

    map<string, string> config;

    if (is_file.is_open()) {
        while( std::getline(is_file, line) )
        {
            std::istringstream is_line(line);
            std::string key;
            if( std::getline(is_line, key, '=') )
            {
                std::string value;
                if( std::getline(is_line, value) )
                    config[key] = value;
            }
        }
        is_file.close();
    }

    return config;
}

void processQuery(Trie<char> *trie, Experiment *experiment, int queryId, int tau) {
    PrefixActiveNodeSet<char>* pset = new PrefixActiveNodeSet<char>(trie, tau);
    string currentQuery = "";

    for (auto j = 1; j <= queries[queryId].length(); j++) {
        #ifdef IS_COLLECT_TIME_H
            experiment->initQueryProcessingTime();
        #endif

        char ch = queries[queryId][j - 1]; // current key stroke
        currentQuery += ch;

        PrefixActiveNodeSet<char>* temp = pset->computeActiveNodeSetIncrementally(ch);
        delete pset;
        pset = temp;

        map<TrieNode<char>*, unsigned> minActiveNodes;
        pset->computeMinimizedTrieNodesInRange(0, tau, minActiveNodes);

        #ifdef IS_COLLECT_TIME_H
            experiment->endQueryProcessingTime(pset->getNumberOfActiveNodes(), currentQuery);
        #endif

        unordered_map<int, string> outputs;
        vector<int> prefixQuerySizeToFetching = { 5, 9, 13, 17 };

        if (std::find(prefixQuerySizeToFetching.begin(), prefixQuerySizeToFetching.end(), currentQuery.size()) !=
            prefixQuerySizeToFetching.end()) {
            #ifdef IS_COLLECT_TIME_H
                experiment->initQueryFetchingTime();
            #endif

            int prev_last = -1;
            auto tit = trie->ids.begin();
            for (auto mit = pset->PANMap.begin(); mit != pset->PANMap.end(); mit++) {
                if (mit->first->last <= prev_last) continue;
                prev_last = mit->first->last;
                tit = lower_bound(trie->ids.begin(), trie->ids.end(), make_pair(mit->first->id, -1));
                while (tit != trie->ids.end() && tit->first <= mit->first->last) {
                    outputs[tit->second] = recs[tit->second];
                    ++tit;
                }
            }

            #ifdef IS_COLLECT_TIME_H
                experiment->endQueryFetchingTime(currentQuery, outputs.size());
            #endif
        }

        if (currentQuery.length() == queries[queryId].length()) {
            #ifdef IS_COLLECT_TIME_H
                experiment->compileQueryProcessingTimes(queryId);
                experiment->saveQueryProcessingTime(currentQuery, queryId, prefixQuerySizeToFetching);
            #endif

            #ifdef IS_COLLECT_MEMORY_H
                experiment->getMemoryUsedInProcessing();
            #endif
        }
    }
}

void processFullQuery(Trie<char> *trie, Experiment *experiment, int queryId, int tau) {
    PrefixActiveNodeSet<char>* pset = new PrefixActiveNodeSet<char>(trie, tau);

    #ifdef IS_COLLECT_TIME_H
        experiment->initQueryProcessingTime();
    #endif

    for (auto j = 1; j <= queries[queryId].length(); j++) {
        char ch = queries[queryId][j - 1]; // current key stroke

        PrefixActiveNodeSet<char>* temp = pset->computeActiveNodeSetIncrementally(ch);
        delete pset;
        pset = temp;

        map<TrieNode<char>*, unsigned> minActiveNodes;
        pset->computeMinimizedTrieNodesInRange(0, tau, minActiveNodes);
    }

    #ifdef IS_COLLECT_TIME_H
        experiment->endSimpleQueryProcessingTime(pset->getNumberOfActiveNodes());
        experiment->initQueryFetchingTime();
    #endif

    unordered_map<int, string> outputs;

    int prev_last = -1;
    auto tit = trie->ids.begin();

    for (auto mit = pset->PANMap.begin(); mit != pset->PANMap.end(); mit++) {
        if (mit->first->last <= prev_last) continue;
        prev_last = mit->first->last;
        tit = lower_bound(trie->ids.begin(), trie->ids.end(), make_pair(mit->first->id, -1));

        while (tit != trie->ids.end() && tit->first <= mit->first->last) {
            outputs[tit->second] = recs[tit->second];
            ++tit;
        }
    }

    #ifdef IS_COLLECT_TIME_H
        experiment->endSimpleQueryFetchingTime(outputs.size());
        experiment->compileSimpleQueryProcessingTimes(queries[queryId]);
    #endif

    #ifdef IS_COLLECT_MEMORY_H
        experiment->getMemoryUsedInProcessing();
    #endif
}

int main(int argc, char ** argv) {
    map<string, string> config = loadConfig();

    Experiment* experiment = new Experiment(config);

    cout << "indexing... \n";
    string sizeSufix = "";
    int sizeType = stoi(config["size_type"]);

    switch (sizeType) {
        case 0:
            sizeSufix = "_25";
            break;
        case 1:
            sizeSufix = "_50";
            break;
        case 2:
            sizeSufix = "_75";
            break;
        case 3:
            sizeSufix = "";
    }

    auto start = chrono::high_resolution_clock::now();
    #ifdef IS_COLLECT_TIME_H
        experiment->initIndexingTime();
    #endif

    string datasetFile = config["dataset_basepath"];
    string queryFile = config["query_basepath"];
    string relevantQueryFile = config["query_basepath"];

    int queriesSize = stoi(config["queries_size"]);

    string datasetSuffix = queriesSize == 10 ? "_10" : "";
    const int tau = stoi(config["edit_distance"]);
    string editDistanceThreshold = to_string(tau);

    int dataset = stoi(config["dataset"]);
    switch (dataset) {
        case 0:
            datasetFile += "aol/aol" + sizeSufix + ".txt";
            queryFile += "aol/q17_" + editDistanceThreshold + datasetSuffix + ".txt";
            break;
        case 1:
            datasetFile += "medline/medline" + sizeSufix + ".txt";
            queryFile += "medline/q13" + datasetSuffix + ".txt";
            break;
        case 2:
            datasetFile += "usaddr/usaddr" + sizeSufix + ".txt";
            queryFile += "usaddr/q17_" + editDistanceThreshold + datasetSuffix + ".txt";
            break;
        case 3:
            datasetFile += "medline19/medline19" + sizeSufix + ".txt";
            queryFile += "medline19/q17_" + editDistanceThreshold + datasetSuffix + ".txt";
            break;
        case 4:
            datasetFile += "dblp/dblp" + sizeSufix + ".txt";
            queryFile += "dblp/q17_" + editDistanceThreshold + datasetSuffix + ".txt";
            break;
        case 5:
            datasetFile += "umbc/umbc" + sizeSufix + ".txt";
            queryFile += "umbc/q17_" + editDistanceThreshold + datasetSuffix + ".txt";
            break;
        case 6:
            datasetFile += "jusbrasil/jusbrasil" + sizeSufix + ".txt";
            queryFile += "jusbrasil/q.txt";
            relevantQueryFile += "jusbrasil/relevant_answers.txt";
            break;
        default:
            datasetFile += "aol/aol" + sizeSufix + ".txt";
            queryFile += "aol/q17_" + editDistanceThreshold + datasetSuffix + ".txt";
            break;
    }

	readData(datasetFile, recs);
	readData(queryFile, queries);
    if (config["has_relevant_queries"] == "1") {
        readData(relevantQueryFile, relevantQueries);
    }

	Trie<char>* trie = new Trie<char>(experiment);
	for (auto i = 0; i < recs.size(); i++) {
	    trie->append(recs[i].c_str(), i);
	}

	trie->buildIdx();
    #ifdef IS_COLLECT_MEMORY_H
        experiment->getMemoryUsedInIndexing();
    #endif

    #ifdef IS_COLLECT_TIME_H
        experiment->compileProportionOfBranchingSize();
        experiment->endIndexingTime();
        experiment->compileNumberOfNodes();
    #endif

    auto done = chrono::high_resolution_clock::now();
    cout << "<<<Index time: "<< chrono::duration_cast<chrono::milliseconds>(done - start).count() << " ms>>>\n";

    int startIndex = stoi(config["qry_number_start"]);
    int endIndex = stoi(config["qry_number_end"]);

    #ifdef IS_COLLECT_MEMORY_H
        endIndex = 100;
    #endif

    cout << "processing..." << endl;
    if (config["is_full_query_instrumentation"] == "0") {
        for (auto i = startIndex; i < endIndex; i++) {
            processQuery(trie, experiment, i, tau);
        }
    } else {
        for (auto i = startIndex; i < endIndex; i++) {
            processFullQuery(trie, experiment, i, tau);
        }
    }

	return 0;
}
