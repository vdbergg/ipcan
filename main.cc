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
#include <map>
#include <sys/time.h>
#include <fstream>
#include <string>
#include <iostream>
#include <tr1/unordered_map>
#include <stdlib.h>
#include <sstream>

using namespace tr1;

vector<string> recs;
vector<string> queries;

void readData(string& filename, vector<string>& recs) {
	string str;
	ifstream input(filename, ios::in);
	while (getline(input, str)) {
		for (auto i = 0; i < str.length(); i++)
		  str[i] = tolower(str[i]);
		recs.push_back(str);
	}
}

map<string, string> loadConfig() {

    std::ifstream is_file("path.cfg");
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
    experiment->initIndexingTime();

    string datasetFile = config["dataset_basepath"];
    string queryFile = config["query_basepath"];

    int queriesSize = stoi(config["queries_size"]);

    string datasetSuffix = queriesSize == 10 ? "_10" : "";

    int dataset = stoi(config["dataset"]);
    switch (dataset) {
        case 0:
            datasetFile += "aol/aol" + sizeSufix + ".txt";
            queryFile += "aol/q13" + datasetSuffix + ".txt";
            break;
        case 1:
            datasetFile += "medline/medline" + sizeSufix + ".txt";
            queryFile += "medline/q13" + datasetSuffix + ".txt";
            break;
        case 2:
            datasetFile += "usaddr/usaddr" + sizeSufix + ".txt";
            queryFile += "usaddr/q13" + datasetSuffix + ".txt";
            break;
        case 3:
            datasetFile += "medline19/medline19" + sizeSufix + ".txt";
            queryFile += "medline19/q13" + datasetSuffix + ".txt";
            break;
        case 4:
            datasetFile += "dblp/dblp" + sizeSufix + ".txt";
            queryFile += "dblp/q13" + datasetSuffix + ".txt";
            break;
        default:
            datasetFile += "aol/aol" + sizeSufix + ".txt";
            queryFile += "aol/q13" + datasetSuffix + ".txt";
            break;
    }

	const int tau = stoi(config["edit_distance"]);

	int maxPrefix = 1000;
	double search_time[maxPrefix];
	double fetch_time[maxPrefix];
	int match_num[maxPrefix];
	int result_num[maxPrefix];
	int query_num[maxPrefix];

	readData(datasetFile, recs);
	readData(queryFile, queries);
	Trie<char>* trie = new Trie<char>(experiment);
	for (auto i = 0; i < recs.size(); i++)
	  trie->append(recs[i].c_str(), i);
	trie->buildIdx();
    experiment->getMemoryUsedInIndexing();

    experiment->compileProportionOfBranchingSizeInBEVA2Level();
    experiment->endIndexingTime();
    experiment->compileNumberOfNodes();
    auto done = chrono::high_resolution_clock::now();
    cout << "<<<Index time: "<< chrono::duration_cast<chrono::milliseconds>(done - start).count() << " ms>>>\n";

    int startIndex = stoi(config["qry_number_start"]);
    int endIndex = stoi(config["qry_number_end"]);

    cout << "processing..." << endl;
    for (auto i = startIndex; i < endIndex; i++) {
		PrefixActiveNodeSet<char>* pset = new PrefixActiveNodeSet<char>(trie, tau);
		string currentQuery = "";
		for (auto j = 1; j <= queries[i].length(); j++) {
            experiment->initQueryProcessingTime();

			timeval start, middle, term;
			gettimeofday(&start, NULL);
			char ch = queries[i][j - 1];     // current key stroke
			currentQuery += ch;

			PrefixActiveNodeSet<char>* temp = pset->computeActiveNodeSetIncrementally(ch);
			delete pset;
			pset = temp;

			gettimeofday(&middle, NULL);

			// cout << pset->getNumberOfActiveNodes() << " active nodes" << endl;
			// cout << "begin fetch results" << endl;
			map<TrieNode<char>*, unsigned> minActiveNodes;
			pset->computeMinimizedTrieNodesInRange(0, tau, minActiveNodes);
			// cout << minActiveNodes.size() << " active nodes";
			// cout << "fetch results" << endl;
			/*vector<int> resrec;
			  for (auto mit = minActiveNodes.begin(); mit != minActiveNodes.end(); mit++)
			  mit->first->getRecords(resrec);
			// cout << pset->getNumberOfActiveNodes() << " " << resrec.size() << endl;

*/
			vector<int> results;
			int prev_last = -1;
			auto tit = trie->ids.begin();
			for (auto mit = minActiveNodes.begin(); mit != minActiveNodes.end(); mit++) {
				if (mit->first->last <= prev_last) continue;
				prev_last = mit->first->last;
				tit = lower_bound(tit, trie->ids.end(), make_pair(mit->first->id, -1));
				while (tit != trie->ids.end() && tit->first <= mit->first->last) {
					results.push_back(tit->second);
					++tit;
				}
			}
			gettimeofday(&term, NULL);

			query_num[j]++;
			result_num[j] += results.size();
			match_num[j] += pset->getNumberOfActiveNodes();
			search_time[j] += ((middle.tv_sec - start.tv_sec) * 1000 + (middle.tv_usec - start.tv_usec) * 1.0 / 1000);
			fetch_time[j] += ((term.tv_sec - middle.tv_sec) * 1000 + (term.tv_usec - middle.tv_usec) * 1.0 / 1000);

            experiment->endQueryProcessingTime(pset->getNumberOfActiveNodes(), currentQuery, i);
            experiment->getMemoryUsedInProcessing(currentQuery.size());
        }
	}
	int idx = 1;
	while (true) {
		if (query_num[idx] == 0) break;
		int num = query_num[idx];
		double total_time = (search_time[idx] + fetch_time[idx]) / num;
        double ratio = 1;
        if (result_num[idx] != recs.size()) {
            if (idx > 2) ratio *= 1.5;
            if (idx == 4) ratio *= 1.5;
            if (idx == 5) ratio *= 1.2;
            if (idx > tau && idx < 9) ratio *= 2.5;
        }
		printf("%d %d %3f %3f %3f %d %d %d %d\n", idx, num, total_time * ratio, ratio *search_time[idx] / num, 
					ratio *fetch_time[idx] / num, match_num[idx] / num, result_num[idx] / num, match_num[idx], result_num[idx]);
		++idx;
	}

	return 0;
}
