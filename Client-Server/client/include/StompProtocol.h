#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/Game.h"

#include <map>
#include <string>
#include <tuple>
#include <mutex>
#include <fstream>
#include <condition_variable>

using std::string;
using std::vector;
using std::map;
using std::tuple;

// TODO: implement the STOMP protocol
class StompProtocol
{

private:
	ConnectionHandler &mHandler;
    volatile bool isLoggedIn;
	string mUserName;
	map<string, int> mTopics;
	int nextSubId;
	int nextRecId;
	vector<tuple<string, string, string>> mReciepts;
	map<string,vector<Game>> gamePerUser;
	std::mutex mTopics_mutex;
	std::condition_variable logout_cond;
	std::mutex logout_mutex;

public:
    StompProtocol(ConnectionHandler &connectionHandler);

	virtual ~StompProtocol();

	bool getIsLoggedIn();

	// Logs in to the server
	bool handleLogIn(string &lineContent);

	void handleServerResponse();

	void handleErrorResponse(string &frameContent);

	void handleReceiptResponse(string &frameContent);

	void handleMessageResponse(string &frameContent);

	// Process input command
	void process(string &line, string &pendingCommand);

	// Returns seperated user command
	void seperateUserCommand(string lineContent, vector<string> &seperateLines);

	// Returns seperated error frame
	string extraxtErrorMessage(string &lineContent);

	// Process join command
	void join(vector<string> &lineContent);

	// Process exit command
	void exit(vector<string> &lineContent);

	// Process report command
	void report(vector<string> &lineContent);

	// Process summary command
	void summary(vector<string> &lineContent);

	// Process logout command
	void logout(vector<string> &lineContent);

	void deactivateUserGame(string &topic);

	bool activateUserGame(string &topic);

	void buildUpdatesBody(string &updatesBody, Event &event);

	int getGamePos(string &topic, vector<Game> &userGames);
};

// Returns seperated message frame
	map<string,string> seperateMessageFrame(string lineContent);

