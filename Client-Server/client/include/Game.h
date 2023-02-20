#pragma once

#include "../include/event.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>


using std::string;
using std::vector;
using std::map;


class Game {

private:
    string gameName; 
    string team_a_name;
    string team_b_name;
    map<string,string> generalStats;
    map<string,string> teamAStats;
    map<string,string> teamBStats;
    vector<Event> gameEventReports;
    volatile bool active;

public:

    Game(string gameName);
    virtual ~Game();
    string getName();
    string makeSummary();
    void addReport(Event &event);
    bool isActive();
    void setActive(bool val);

};