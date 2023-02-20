#include "../include/Game.h"

using std::string;

Game::Game(string gameName): gameName(gameName), team_a_name(), team_b_name(), generalStats(), teamAStats(), teamBStats(), gameEventReports(), active()  {
    int index = gameName.find('_');
    team_a_name = gameName.substr(0,index);
    team_b_name = gameName.substr(index+1);
    active = true;
}

Game::~Game()
{
}

string Game::getName(){
    return gameName;
}

bool Game::isActive(){
    return active;
}

void Game::setActive(bool val){
    active = val;
}

void Game::addReport(Event &event){
    map<string,string> eventGeneralStats = event.get_game_updates();
    map<string,string>::iterator eventGeneralStatsIt = eventGeneralStats.begin();
    while(eventGeneralStatsIt != eventGeneralStats.end()){
        generalStats[eventGeneralStatsIt->first] = eventGeneralStatsIt->second;
        eventGeneralStatsIt++;
    }

    map<string,string> eventTeamAStats = event.get_team_a_updates();
    map<string,string>::iterator eventTeamAStatsIt = eventTeamAStats.begin();
    while(eventTeamAStatsIt != eventTeamAStats.end()){
        teamAStats[eventTeamAStatsIt->first] = eventTeamAStatsIt->second;
        eventTeamAStatsIt++;
    }

    map<string,string> eventTeamBStats = event.get_team_b_updates();
    map<string,string>::iterator eventTeamBStatsIt = eventTeamBStats.begin();
    while(eventTeamBStatsIt != eventTeamBStats.end()){
        teamBStats[eventTeamBStatsIt->first] = eventTeamBStatsIt->second;
        eventTeamBStatsIt++;
    }
    gameEventReports.push_back(event);
}


string Game::makeSummary(){
    string output=team_a_name + " vs " + team_b_name +"\nGame stats:\nGeneral stats:\n";
    map<string,string>::iterator generalStatsIt = generalStats.begin();
    while(generalStatsIt != generalStats.end() ){
        output = output + generalStatsIt -> first +": "+ generalStatsIt -> second +"\n";
        generalStatsIt++;
    }

    output = output + team_a_name + " stats:\n";
    map<string,string>::iterator teamAStatsIt = teamAStats.begin();
    while(teamAStatsIt != teamAStats.end() ){
        output = output + teamAStatsIt -> first +": "+ teamAStatsIt -> second +"\n";
        teamAStatsIt++;
    }

    output = output + team_b_name + " stats:\n";
    map<string,string>::iterator teamBStatsIt = teamBStats.begin();
    while(teamBStatsIt != teamBStats.end() ){
        output = output + teamBStatsIt -> first +": "+ teamBStatsIt -> second + "\n";
        teamBStatsIt++;
    }

    output = output + "Game event reports:\n";
    int size = gameEventReports.size();
    for (int i=0; i<size; i++){
        output = output + std::to_string(gameEventReports.at(i).get_time()) + " - " + gameEventReports.at(i).get_name() + ":\n\n"+gameEventReports.at(i).get_discription() + "\n\n\n";
    }
    return output;
}

