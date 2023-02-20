#include "../include/event.h"
#include "../include/json.hpp"
#include "../include/StompProtocol.h"


using json = nlohmann::json;

Event::Event(std::string team_a_name, std::string team_b_name, std::string name, int time,
             std::map<std::string, std::string> game_updates, std::map<std::string, std::string> team_a_updates,
             std::map<std::string, std::string> team_b_updates, std::string discription)
    : team_a_name(team_a_name), team_b_name(team_b_name), name(name),
      time(time), game_updates(game_updates), team_a_updates(team_a_updates),
      team_b_updates(team_b_updates), description(discription)
{
}

Event::~Event()
{
}

const std::string &Event::get_team_a_name() const
{
    return this->team_a_name;
}

const std::string &Event::get_team_b_name() const
{
    return this->team_b_name;
}

const std::string &Event::get_name() const
{
    return this->name;
}

int Event::get_time() const
{
    return this->time;
}

const std::map<std::string, std::string> &Event::get_game_updates() const
{
    return this->game_updates;
}

const std::map<std::string, std::string> &Event::get_team_a_updates() const
{
    return this->team_a_updates;
}

const std::map<std::string, std::string> &Event::get_team_b_updates() const
{
    return this->team_b_updates;
}

const std::string &Event::get_discription() const
{
    return this->description;
}

Event::Event(const std::string &frame_body) : team_a_name(""), team_b_name(""), name(""), time(0), game_updates(), team_a_updates(), team_b_updates(), description("")
{
    int index; int i=0;
    std:: stringstream body(frame_body);
    string currline;
    vector <string> seperateLines;

    while (getline(body, currline)){
        seperateLines.push_back(currline);
    } 

    index = seperateLines[i].find(':');
    team_a_name = seperateLines[i].substr(index+1);
    i++;

    index = seperateLines[i].find(':');
    team_b_name = seperateLines[i].substr(index+1);
    i++;

    index = seperateLines[i].find(':');
    name = seperateLines[i].substr(index+1);
    i++;

    index = seperateLines[i].find(':');
    seperateLines[i].erase(0,index+1);
    time = std::stoi(seperateLines[i]);
    i++; i++;
    while (seperateLines[i] != "team a updates:") {      //general game updates
        string trimmedStat = seperateLines[i].substr(4);   //removing space
        index = trimmedStat.find(':');
        string key = trimmedStat.substr(0,index);
        string value = trimmedStat.substr(index+1);
        game_updates[key]=value;
        i++;
    }
    i++;

    while (seperateLines[i] != "team b updates:") {      //maybe without the newLine?? //team a updates
        string trimmedStat = seperateLines[i].substr(4);
        index = trimmedStat.find(':');
        string key = trimmedStat.substr(0,index);
        string value = trimmedStat.substr(index+1);
        team_a_updates[key]=value;
        i++;
    }
    i++;

    while (seperateLines[i] != "description:") {         //team b updates
        string trimmedStat = seperateLines[i].substr(4);
        index = trimmedStat.find(':');
        string key = trimmedStat.substr(0,index);
        string value = trimmedStat.substr(index+1);
        team_b_updates[key]=value;
        i++;
    } 
    i++;

    int size = seperateLines.size();
    while (i < size) {                  //description
        description = description + seperateLines[i];
        i++;
    } 
}


names_and_events parseEventsFile(std::string json_path)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    std::string team_a_name = data["team a"];
    std::string team_b_name = data["team b"];

    // run over all the events and convert them to Event objects
    std::vector<Event> events;
    for (auto &event : data["events"])
    {
        std::string name = event["event name"];
        int time = event["time"];
        std::string description = event["description"];
        std::map<std::string, std::string> game_updates;
        std::map<std::string, std::string> team_a_updates;
        std::map<std::string, std::string> team_b_updates;
        for (auto &update : event["general game updates"].items())
        {
            if (update.value().is_string())
                game_updates[update.key()] = update.value();
            else
                game_updates[update.key()] = update.value().dump();
        }

        for (auto &update : event["team a updates"].items())
        {
            if (update.value().is_string())
                team_a_updates[update.key()] = update.value();
            else
                team_a_updates[update.key()] = update.value().dump();
        }

        for (auto &update : event["team b updates"].items())
        {
            if (update.value().is_string())
                team_b_updates[update.key()] = update.value();
            else
                team_b_updates[update.key()] = update.value().dump();
        }
        
        events.push_back(Event(team_a_name, team_b_name, name, time, game_updates, team_a_updates, team_b_updates, description));
    }
    names_and_events events_and_names{team_a_name, team_b_name, events};

    return events_and_names;
}