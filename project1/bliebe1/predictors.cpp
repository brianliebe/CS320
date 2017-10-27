#include <iostream>
#include <fstream>
#include <time.h>
#include <utility>
#include <bitset>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

struct PredictorResult 
{
    int correct;
    int total;
    PredictorResult() 
    {
        correct = 0;
        total = 0;
    }
};

unsigned getSpecificBitLength(unsigned long a, unsigned b)
{
    unsigned mask = 0;
    for(unsigned i = 0; i < b; i++)
    {
        mask |= 1 << i;
    } 
    return a & mask;
}

PredictorResult sameValuePredictor(string filename, bool useTaken)
{
    fstream infile(filename);
    string address, state;
    PredictorResult result;

    while (infile >> address >> state) 
    {
        result.total++;
        if (state == (useTaken ? "T" : "NT")) result.correct++;
    }
    return result;
}

PredictorResult singleBitBimodalPredictor(string filename, int size)
{
    fstream infile(filename);
    string address, state;
    PredictorResult result;
    
    pair<int, string> table[size];
    for (int i = 0; i < size; i++) table[i] = pair<int, string>(i, "T");
    int factor = log2(size);
    while (infile >> address >> state) 
    {
        result.total++;
        int addr = stoi(address.substr(3), nullptr, 16);
        int matchingValue = addr & ((1 << factor) - 1);

        if (table[matchingValue].second == state) result.correct++;
        table[matchingValue].second = state;
    }
    return result;
}

PredictorResult twoBitBimodalPredictor(string filename, int size)
{
    fstream infile(filename);
    string address, state;
    PredictorResult result;
    
    pair<int, int> table[size];
    for (int i = 0; i < size; i++) table[i] = pair<int, int>(i, 3);
    int factor = log2(size);

    while (infile >> address >> state) 
    {
        result.total++;
        int addr = stoi(address.substr(3), nullptr, 16);
        int matchingValue = addr & ((1 << factor) - 1);

        int tableTakenValue = table[matchingValue].second;

        if(state == "T" && (tableTakenValue == 2 || tableTakenValue == 3)) result.correct++;
        if(state == "NT" && (tableTakenValue == 0 || tableTakenValue == 1)) result.correct++;

        if (state == "T" && table[matchingValue].second < 3)
        {
            table[matchingValue].second++;
        }
        else if (state == "NT" && table[matchingValue].second > 0)
        {
            table[matchingValue].second--;
        }
    }
    return result;
}

PredictorResult gsharePredictor (string filename, int historyLength)
{
    fstream infile(filename);
    string state;
    unsigned long address;
    PredictorResult result;
    
    int history = 0;
    int table[2048];
    for (int i = 0; i < 2048; i++) table[i] = 3;

    while (infile >> hex >> address >> state) 
    {
        result.total++;
        unsigned addr = getSpecificBitLength(address, 11);
        unsigned index = addr ^ history;
        history = (history << 1) + (state == "T" ? 1 : 0);
        if (state == "T") 
        {
            if (table[index] > 1) result.correct++;
            if (table[index] < 3) table[index]++;
        }
        else if (state == "NT") 
        {
            if (table[index] < 2) result.correct++;
            if (table[index] > 0) table[index]--;
        }
        history = getSpecificBitLength(history, historyLength);
    }
    return result;
}

PredictorResult tournamentPredictor (string filename) 
{
    fstream infile(filename);
    string state;
    unsigned long address;
    PredictorResult result;
    int history = 0;

    int bimodalTable[2048];
    int gshareTable[2048];
    int selectorTable[2048];
    for (int i = 0; i < 2048; i++) 
    {
        bimodalTable[i] = 3;
        gshareTable[i] = 3;
        selectorTable[i] = 0;
    }

    while (infile >> hex >> address >> state) 
    {
        result.total++;
        unsigned addr = getSpecificBitLength(address, 11);
        unsigned index = addr ^ history;
        history = (history << 1) + (state == "T" ? 1 : 0);
        bool gshareCorrect = false;
        bool bimodalCorrect = false;

        // Gshare code
        if (state == "T") 
        {
            if (gshareTable[index] > 1) gshareCorrect = true;
            if (gshareTable[index] < 3) gshareTable[index]++;
        }
        else if (state == "NT") 
        {
            if (gshareTable[index] < 2) gshareCorrect = true;
            if (gshareTable[index] > 0) gshareTable[index]--;
        }
        history = getSpecificBitLength(history, 11);

        // Bimodal code
        int tableTakenValue = bimodalTable[addr];
        if(state == "T" && (tableTakenValue == 2 || tableTakenValue == 3)) bimodalCorrect = true;
        if(state == "NT" && (tableTakenValue == 0 || tableTakenValue == 1)) bimodalCorrect = true;
        
        if (state == "T" && bimodalTable[addr] < 3)
        {
            bimodalTable[addr]++;
        }
        else if (state == "NT" && bimodalTable[addr] > 0)
        {
            bimodalTable[addr]--;
        }

        // Check correctness
        if (selectorTable[addr] > 1) // use bimodal
        {
            if (bimodalCorrect) result.correct++;
        }
        else if (selectorTable[addr] < 2) // use gshare
        {
            if (gshareCorrect) result.correct++;
        }

        // Decide how to affect the selector table (based on results).
        if (gshareCorrect && !bimodalCorrect && selectorTable[addr] > 0) selectorTable[addr]--;
        else if (!gshareCorrect && bimodalCorrect && selectorTable[addr] < 3) selectorTable[addr]++;
    }
    return result;
}

PredictorResult extraCredit (string filename, int tableSize)
{
    fstream infile(filename);
    string state, address;
    PredictorResult result;
    int index = 0;
    
    vector<pair<int, int>> table;

    while (infile >> address >> state) 
    {
        result.total++;
        int addr = stoi(address.substr(3), nullptr, 16);
        int matchingValue = addr & ((1 << 10) - 1);
        int change = (state == "T" ? 1 : -1);

        bool found = false;

        for (int i = 0; i < table.size(); i++, index++)
        {
            if (index >= tableSize) index = 0;
            pair<int, int> p = table[i];
            pair<int, int> m1 = pair<int, int>(matchingValue, 0);
            pair<int, int> m2 = pair<int, int>(matchingValue, 1);
            pair<int, int> m3 = pair<int, int>(matchingValue, 2);
            pair<int, int> m4 = pair<int, int>(matchingValue, 3);
            if (p == m1 || p == m2 || p == m3 || p == m4)
            {
                found = true;
                if ((p.second == 0 || p.second == 1) && state == "NT") result.correct++;
                if ((p.second == 2 || p.second == 3) && state == "T") result.correct++;

                p = pair<int, int>(matchingValue, (state == "T" ? (p.second + 1) : (p.second - 1)));
                if (p.second < 0) p.second = 0;
                if (p.second > 3) p.second = 3;
                // table[i] = p;
                table[index] = p;
                break;
            }
        }
        if (!found) 
        {
            if (table.size() < tableSize) table.push_back(pair<int, int>(matchingValue, 3));
            else table[index] = pair<int, int>(matchingValue, 3);
            if (state == "T") result.correct++;
        }

    }
    return result;
}

int main (int argc, char * argv[])
{
    ofstream outfile(argv[2]);
    
    // Always Taken Predictor
    cout << "DEBUG: ALWAYS TAKEN" << endl;
    cout << "       Starting always taken..." << endl;
    PredictorResult resultAlwaysTaken = sameValuePredictor(argv[1], true);
    outfile << to_string(resultAlwaysTaken.correct) + "," + to_string(resultAlwaysTaken.total) + ";" << endl;

    // Always Not Taken Predictor
    cout << "DEBUG: ALWAYS NOT TAKEN" << endl;
    cout << "       Starting always not taken..." << endl;
    PredictorResult resultAlwaysNotTaken = sameValuePredictor(argv[1], false);
    outfile << to_string(resultAlwaysNotTaken.correct) + "," + to_string(resultAlwaysNotTaken.total) + ";" << endl;

    // Bimodal Predictor (single bit)
    cout << "DEBUG: BIMODAL SINGLE BIT" << endl;
    cout << "       Starting 16 entry..." << endl;
    PredictorResult resultSingleBitBimodalPredictor = singleBitBimodalPredictor(argv[1], 16);
    outfile << to_string(resultSingleBitBimodalPredictor.correct) + "," + to_string(resultSingleBitBimodalPredictor.total) + "; ";
    cout << "       Starting 32 entry..." << endl;
    resultSingleBitBimodalPredictor = singleBitBimodalPredictor(argv[1], 32);
    outfile << to_string(resultSingleBitBimodalPredictor.correct) + "," + to_string(resultSingleBitBimodalPredictor.total) + "; ";
    cout << "       Starting 128 entry..." << endl;
    resultSingleBitBimodalPredictor = singleBitBimodalPredictor(argv[1], 128);
    outfile << to_string(resultSingleBitBimodalPredictor.correct) + "," + to_string(resultSingleBitBimodalPredictor.total) + "; ";
    cout << "       Starting 256 entry..." << endl;
    resultSingleBitBimodalPredictor = singleBitBimodalPredictor(argv[1], 256);
    outfile << to_string(resultSingleBitBimodalPredictor.correct) + "," + to_string(resultSingleBitBimodalPredictor.total) + "; ";
    cout << "       Starting 512 entry..." << endl;
    resultSingleBitBimodalPredictor = singleBitBimodalPredictor(argv[1], 512);
    outfile << to_string(resultSingleBitBimodalPredictor.correct) + "," + to_string(resultSingleBitBimodalPredictor.total) + "; ";
    cout << "       Starting 1024 entry..." << endl;
    resultSingleBitBimodalPredictor = singleBitBimodalPredictor(argv[1], 1024);
    outfile << to_string(resultSingleBitBimodalPredictor.correct) + "," + to_string(resultSingleBitBimodalPredictor.total) + "; ";
    cout << "       Starting 2048 entry..." << endl;
    resultSingleBitBimodalPredictor = singleBitBimodalPredictor(argv[1], 2048);
    outfile << to_string(resultSingleBitBimodalPredictor.correct) + "," + to_string(resultSingleBitBimodalPredictor.total) + ";" << endl;

    // Bimodal Predictor (two bits)
    cout << "DEBUG: BIMODAL TWO BIT" << endl;
    cout << "       Starting 16 entry..." << endl;
    PredictorResult resultTwoBitBimodalPredictor = twoBitBimodalPredictor(argv[1], 16);
    outfile << to_string(resultTwoBitBimodalPredictor.correct) + "," + to_string(resultTwoBitBimodalPredictor.total) + "; ";
    cout << "       Starting 32 entry..." << endl;
    resultTwoBitBimodalPredictor = twoBitBimodalPredictor(argv[1], 32);
    outfile << to_string(resultTwoBitBimodalPredictor.correct) + "," + to_string(resultTwoBitBimodalPredictor.total) + "; ";
    cout << "       Starting 128 entry..." << endl;
    resultTwoBitBimodalPredictor = twoBitBimodalPredictor(argv[1], 128);
    outfile << to_string(resultTwoBitBimodalPredictor.correct) + "," + to_string(resultTwoBitBimodalPredictor.total) + "; ";
    cout << "       Starting 256 entry..." << endl;
    resultTwoBitBimodalPredictor = twoBitBimodalPredictor(argv[1], 256);
    outfile << to_string(resultTwoBitBimodalPredictor.correct) + "," + to_string(resultTwoBitBimodalPredictor.total) + "; ";
    cout << "       Starting 512 entry..." << endl;
    resultTwoBitBimodalPredictor = twoBitBimodalPredictor(argv[1], 512);
    outfile << to_string(resultTwoBitBimodalPredictor.correct) + "," + to_string(resultTwoBitBimodalPredictor.total) + "; ";
    cout << "       Starting 1024 entry..." << endl;
    resultTwoBitBimodalPredictor = twoBitBimodalPredictor(argv[1], 1024);
    outfile << to_string(resultTwoBitBimodalPredictor.correct) + "," + to_string(resultTwoBitBimodalPredictor.total) + "; ";
    cout << "       Starting 2048 entry..." << endl;
    resultTwoBitBimodalPredictor = twoBitBimodalPredictor(argv[1], 2048);
    outfile << to_string(resultTwoBitBimodalPredictor.correct) + "," + to_string(resultTwoBitBimodalPredictor.total) + ";" << endl;

    // Gshare Predictor
    cout << "DEBUG: GSHARE" << endl;
    cout << "       Starting 3 bit..." << endl;
    PredictorResult resultGsharePredictor = gsharePredictor(argv[1], 3);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 4 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 4);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 5 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 5);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 6 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 6);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 7 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 7);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 8 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 8);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 9 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 9);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 10 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 10);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + "; ";
    cout << "       Starting 11 bit..." << endl;
    resultGsharePredictor = gsharePredictor(argv[1], 11);
    outfile << to_string(resultGsharePredictor.correct) + "," + to_string(resultGsharePredictor.total) + ";" << endl;

    // Tournament Predictor
    cout << "DEBUG: TOURNAMENT" << endl;
    cout << "       Starting tournament..." << endl;
    PredictorResult resultTournamentPredictor = tournamentPredictor(argv[1]);
    outfile << to_string(resultTournamentPredictor.correct) + "," + to_string(resultTournamentPredictor.total) + ";" << endl;

    // Extra Credit Predictor
    cout << "DEBUG: EXTRA CREDIT" << endl;
    cout << "       Starting extra credit..." << endl;
    PredictorResult resultExtraCredit = extraCredit(argv[1], 16);
    cout << "       Result: " + to_string(resultExtraCredit.correct) + "," + to_string(resultExtraCredit.total) + ";" << endl;

    outfile.close();
    return 0;
}