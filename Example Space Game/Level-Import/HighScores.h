#include <iostream>
#include<fstream>
#include<string>
#include "../gateware-main/Gateware.h"
#include "../Source/Systems/EnemyLogic.h"
#include <map>
#pragma once

using namespace std;

struct HighScores {
    string name;
    int score;
};

void createFileIfNotExists(const string& filename) {
    ifstream file(filename);
    if (!file.good()) {
        std::ofstream createFile(filename);
        createFile.close();
    }
}

void updateScores(const string& player_name, int player_score) {
    const string filename = "scores.txt";
    createFileIfNotExists(filename);

    ifstream inFile(filename);
    vector<HighScores> scores;

    // Read existing scores from the file
    if (inFile) {
        string name;
        int score;
        while (inFile >> name >> score) {
            scores.push_back({ name, score });
        }
        inFile.close();
    }

    // Add new player's score
    scores.push_back({ player_name, player_score });

    // Sort scores based on score (descending order)
    sort(scores.begin(), scores.end(), [](const HighScores& p1, const HighScores& p2) {
        return p1.score > p2.score;
        });

    // Take top 5 scores
    scores.resize(min(scores.size(), 5u));

    // Write updated scores back to the file
    ofstream outFile(filename);
    for (const auto& player : scores) {
        outFile << player.name << " " << player.score << "\n";
    }
}


void printTop5Scores() {
    const string filename = "scores.txt";
    ifstream inFile(filename);
    vector<HighScores> scores;

    // Read existing scores from the file
    if (inFile) {
        string name;
        int score;
        while (inFile >> name >> score) {
            scores.push_back({ name, score });
        }
        inFile.close();
    }

    // Sort scores based on score (descending order)
    sort(scores.begin(), scores.end(), [](const HighScores& p1, const HighScores& p2) {
        return p1.score > p2.score;
        });

    // Print top 5 scores
    cout << "Top 5 High Scores:\n";
    for (size_t i = 0; i < min(scores.size(), 5u); ++i) {
        cout << i + 1 << ". " << scores[i].name << ": " << scores[i].score << "\n";
    }
}

//void UpdateTopScore(const char* scoreFilePath, int newScore/*, string& name*/) {
//    int topScore = std::stoi(ReadHighScore(scoreFilePath)); // Read top score from file
//
//    if (newScore > topScore) {
//        
//        std::string newTopScore = std::to_string(newScore); // Convert new score to string
//        WriteStringToFile(scoreFilePath, newTopScore/*, name*/); // Write new top score to file
//        std::cout << "Congrats : "  << "\n " << "New top score is: " << newScore << std::endl;
//    }
//    else {
//        std::cout << "Score not higher than top score." << std::endl;
//    }
//}





