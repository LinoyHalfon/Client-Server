#include "../include/StompProtocol.h"

using boost::asio::ip::tcp;

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ofstream;
using std::ios;


StompProtocol::StompProtocol(ConnectionHandler &connectionHandler) : mHandler(connectionHandler), isLoggedIn(false), mUserName(""), mTopics(), nextSubId(0), nextRecId(0), mReciepts(), gamePerUser(), mTopics_mutex(), logout_cond(), logout_mutex()
{
}

StompProtocol::~StompProtocol() {}

bool StompProtocol::getIsLoggedIn(){
    return isLoggedIn;
}

void StompProtocol::process(string &line, string &pendingCommand) {
    if (!isLoggedIn)
        pendingCommand = line;
    else{ //user is logged in 
        int index = line.find(' ');
        string command = line.substr(0,index);
        string lineContent = line.substr(index+1);
        vector<string> seperatedLines;
        seperateUserCommand(lineContent, seperatedLines);
        if (command == "login") 
            std::cout << "The client is already logged in, log out before trying again" << std::endl;
        else if (command == "exit") { exit(seperatedLines); }
        else if (command == "join") { join(seperatedLines); }
        else if (command == "report") { report(seperatedLines); }
        else if (command == "summary") { summary(seperatedLines); }
        else if (command == "logout") { logout(seperatedLines); }
        else 
            std::cout << "Invalid command" << std::endl;
    }
}

void StompProtocol::seperateUserCommand(string lineContent, vector<string> &seperateLines){
    int index = 0;
    string currString = lineContent; 
    string stringToAdd;
    int size = currString.size();
    while (index < size){
        currString = currString.substr(index);
        index = currString.find(' ');
        if (index != -1){ //if found
            stringToAdd = currString.substr(0, index);
            seperateLines.push_back(stringToAdd);
            index = index+1;
        } 
        else{
            seperateLines.push_back(currString);
            break;
        } 
        size = currString.size();    
    }
}

string StompProtocol::extraxtErrorMessage(string &lineContent){   
    string errorMsg;
    int index = 0;
    string key;
    string currString = lineContent; 
    int size = currString.size();
    while (index < size){
        index = currString.find(':');
        key = currString.substr(0, index);
        if(key == "message"){
            currString = currString.substr(index+1);
            index = currString.find('\n');
            errorMsg = currString.substr(0, index);
            break;
        }
        else{
            index = currString.find('\n');
            currString = currString.substr(index+1);
            size = currString.size();
            index = 0;
        }
    }
    return errorMsg;
}

map<string,string> seperateMessageFrame(string lineContent){ 
    map<string,string> seperatedLines;
    int index = 0;
    string key;
    string value; 
    string currString = lineContent; 
    int size = currString.size();
    while (index < size){
        currString = currString.substr(index);
        index = currString.find(':');
        if (index != -1){ //if found
            key = currString.substr(0, index);
            currString = currString.substr(index+1);
            index = currString.find('\n');
            value = currString.substr(0, index);
            seperatedLines.insert(make_pair(key,value));
            index = index+1;
            if (currString.at(index) == '\n'){ //if body started
                index = currString.find(':');
                currString = currString.substr(index+1);
                index = currString.find('\n');
                seperatedLines.insert(make_pair("userName",currString.substr(0,index)));
                currString = currString.substr(index+1);
                index = currString.find('\0');
                currString = currString.substr(0, currString.size()-1);
                seperatedLines.insert(make_pair("body", currString));
                break;
            }
            size = currString.size();
        }
    }
    return seperatedLines;
}
    
bool StompProtocol::handleLogIn(string &lineContent) {
    vector<string> seperatedLine;
    seperateUserCommand(lineContent, seperatedLine);
    string userName = seperatedLine.at(1);
    string password = seperatedLine.at(2);
    string frame = "CONNECT\naccept-version:1.2\nhost:stomp.cs.bgu.ac.il\nlogin:"+userName+"\npasscode:"+password+"\n\n";
    if (mHandler.sendFrameAscii(frame, '\0')){
       // connect frame was sent to server
        string receivedFrame;
        if (mHandler.getFrameAscii(receivedFrame, '\0')){
            // frame was received
            int index = receivedFrame.find('\n');
            string response = receivedFrame.substr(0,index);
            if (response == "CONNECTED"){
                mUserName = mUserName + userName;
                isLoggedIn = true;
                std::cout << "Login successful" << std::endl;
                return true;
            }
            else if (response == "ERROR"){
                int index2 = receivedFrame.find(':');
                string err = receivedFrame.substr(index2+1);
                int index3 = err.find('\n');
                err = err.substr(0, index3);
                std::cout << err + "\nConnection disconnected" << std::endl;
                return true;
            }
            else{
                std::cout << "Could not connect to server" << std::endl;
                return false;
            }
        }
    }
    std::cout << "Could not connect to server" << std::endl;
    return false;
} 

void StompProtocol::join(vector<string> &seperatedLine) {
    string topicName = seperatedLine.at(0);
    string frame = "SUBSCRIBE\ndestination:"+topicName+"\nid:"+std::to_string(nextSubId)+"\nreceipt:"+std::to_string(nextRecId)+"\n\n";
    if (mHandler.sendFrameAscii(frame, '\0')){
        tuple<string,string, string> frameSent("join", std::to_string(nextSubId), topicName);
        mReciepts.push_back(frameSent);
        nextSubId++;
        nextRecId++;
    }   
}

void StompProtocol::exit(vector<string> &seperatedLine) {  
    string topicName = seperatedLine.at(0);
    std::unique_lock<std::mutex> lock(mTopics_mutex);
    map<string, int> :: iterator it = mTopics.find(topicName);
    int subId = it -> second;
    if (it == mTopics.end())
        subId = -1;
    lock.unlock();
    string frame = "UNSUBSCRIBE\nid:"+std::to_string(subId)+"\nreceipt:"+std::to_string(nextRecId)+"\n\n";
    if (mHandler.sendFrameAscii(frame, '\0')){
        tuple<string,string, string> frameSent("exit", std::to_string(subId), topicName);
        mReciepts.push_back(frameSent);
        nextRecId++;
    }
} 
       


void StompProtocol::report(vector<string> &seperatedLine) {
    string jsonPath = seperatedLine.at(0);
    names_and_events report = parseEventsFile(jsonPath);
    vector<Event> &events = report.events;
    for (Event event : events){
        string updatesBody;
        buildUpdatesBody(updatesBody, event);
        string frameBody = "user:"+mUserName+
        "\nteam a:"+event.get_team_a_name()+
        "\nteam b:"+event.get_team_b_name()+
        "\nevent name:"+event.get_name()+
        "\ntime:"+std::to_string(event.get_time())+
        updatesBody+"description:\n"+event.get_discription();
        string frame = "SEND\ndestination:"+ report.team_a_name+"_"+report.team_b_name+"\nreceipt:"+std::to_string(nextRecId)+"\n\n"+frameBody+"\n";
        if (mHandler.sendFrameAscii(frame, '\0')){
            tuple<string,string, string> frameSent("report", "", "");
            mReciepts.push_back(frameSent);
            nextRecId++; 
        }
    }
}

void StompProtocol::buildUpdatesBody(string &updatesBody, Event &event){
    string gameUpdates = "";
    string teamAUpdates = "";
    string teamBUpdates = "";
    map<string, string> gameUpdatesMap =  event.get_game_updates();
    map<string, string> teamAUpdatesMap = event.get_team_a_updates();
    map<string, string> teamBUpdatesMap = event.get_team_b_updates();
    for (auto const &pair: gameUpdatesMap) 
        gameUpdates = gameUpdates+"    "+pair.first+":"+pair.second+"\n";
    for (auto const &pair: teamAUpdatesMap) 
        teamAUpdates = teamAUpdates+"    "+pair.first+":"+pair.second+"\n";
    for (auto const &pair: teamBUpdatesMap) 
        teamBUpdates = teamBUpdates+"    "+pair.first+":"+pair.second+"\n";
    updatesBody = "\ngeneral game updates:\n"+gameUpdates+
    "team a updates:\n"+teamAUpdates+
    "team b updates:\n"+teamBUpdates;
        
}

void StompProtocol::summary(vector<string> &seperatedLine) {
    string topicName = seperatedLine.at(0);
    std::unique_lock<std::mutex> lock(mTopics_mutex);  
    if (mTopics.find(topicName) == mTopics.end()){ //user is not subscribed
        cout << "Invalid request - user isn't subscribed to this topic" << endl;
        lock.unlock();
    }
    else{ //user is subscribed
        lock.unlock();
        string userName = seperatedLine.at(1);
        string fileName = seperatedLine.at(2);

        map<string, vector<Game>> :: iterator userGames = gamePerUser.find(userName);
        if (userGames != gamePerUser.end()){ // user was found
            vector<Game> &games = gamePerUser[userName];
            int GamePos = getGamePos(topicName, games);
            int size = games.size();
            if (GamePos < size){ // game was found
                string summary = games.at(GamePos).makeSummary();
                ofstream file;
                file.open(fileName, ios::out);
                if (file.is_open()){ // file was opened
                    file << summary;
                    file.close();
                }
                else // failed opening file
                    std::cout << "Error on file opening" << std::endl;  //??  
            }
            else // game wasn't found
                std::cout << "No accessible reports" << std::endl;  
        }
        else // user wasn't found
            std::cout << "No accessible reports" << std::endl;  
    }
}

void StompProtocol::logout(vector<string> &seperatedLine) {
    string frame = "DISCONNECT\nreceipt:"+std::to_string(nextRecId)+"\n\n";
    if (mHandler.sendFrameAscii(frame, '\0')){
        tuple<string,string, string> frameSent("logout","", "");
        mReciepts.push_back(frameSent); 
        nextRecId++;
        std::unique_lock<std::mutex> lock(logout_mutex);  
        logout_cond.wait(lock, [this] { return !isLoggedIn; }); 
    }
}

void StompProtocol::handleServerResponse(){
    while (isLoggedIn){
        string receivedFrame;
        if (mHandler.getFrameAscii(receivedFrame, '\0')){
            // frame was received
            int index = receivedFrame.find('\n');
            string command = receivedFrame.substr(0,index);
            string frameContent = receivedFrame.substr(index+1);
            if (command == "ERROR") { handleErrorResponse(frameContent); }
            else if (command == "RECEIPT") { handleReceiptResponse(frameContent); } 
            else if (command == "MESSAGE") { handleMessageResponse(frameContent); } 
        }
        else{
            std::cout << "No connection to the server" << std::endl;
            isLoggedIn = false;
            mHandler.close();
        }
    }
}

void StompProtocol::handleErrorResponse(string &frameContent){
    //end connect
    string errorMsg = extraxtErrorMessage(frameContent);
    std::cout << errorMsg + "\nConnection disconnected, user is logged out" << std::endl;
    isLoggedIn = false;
    mHandler.close();
}

void StompProtocol::handleReceiptResponse(string &frameContent){
    //get receipt-id
    int index1 = frameContent.find(':');
    int index2 = frameContent.find('\n');
    string receiptId = (frameContent.substr(index1+1)).substr(0,index2);
    int intReceiptId = std::stoi(receiptId); 
    //get reciept
    tuple<string,string,string> tuple = mReciepts.at(intReceiptId);
    string action = std::get<0>(tuple);
    if (action == "exit"){
        std::unique_lock<std::mutex> lock(mTopics_mutex);
        string topicName = std::get<2>(tuple);
        mTopics.erase(topicName);
        deactivateUserGame(topicName);
        std::cout << "Exited channel "+topicName << std::endl;
        lock.unlock();
    }
    if (action == "join"){ 
        std::unique_lock<std::mutex> lock(mTopics_mutex);
        string topicName = std::get<2>(tuple);
        string subId = std::get<1>(tuple);
        mTopics.insert(std::pair<string, int> (topicName, std::stoi(subId)));
        if (!activateUserGame(topicName)){
            Game newGame = Game(topicName);
            vector<Game> &userGames = gamePerUser[mUserName];
            userGames.push_back(newGame);
        }
        std::cout << "Joined channel "+topicName << std::endl;
        lock.unlock();
    }
    if (action == "logout"){ 
        std::unique_lock<std::mutex> lock(logout_mutex);  
        isLoggedIn = false;
        std::cout << "User is logged out" << std::endl;
        mHandler.close();
        logout_cond.notify_all();
    }
}

void StompProtocol::handleMessageResponse(string &frameContent){
    map<string, string> seperatedFrame = seperateMessageFrame(frameContent);
    string topic = seperatedFrame["destination"];
    string user = seperatedFrame["userName"];
    string body = seperatedFrame["body"];
    vector<Game> &userGames = gamePerUser[user];
    Event event = Event(body);
    bool found = false;
    int size  = userGames.size();
    for (int i = 0; !found & (i<size); i++){
        if (userGames.at(i).getName() == topic){
            found = true;
            userGames.at(i).addReport(event);
        }
    }
    if (!found){ //create game object
        Game newGame = Game(topic);
        newGame.addReport(event);
        userGames.push_back(newGame);
    }
}

void StompProtocol::deactivateUserGame(string &topic){
    vector<Game> &userGames = gamePerUser[mUserName];
    int posGame = getGamePos(topic, userGames);
    userGames.at(posGame).setActive(false);
}

bool StompProtocol::activateUserGame(string &topic){
    vector<Game> &userGames = gamePerUser[mUserName];
    int posGame = getGamePos(topic, userGames);
    int size = userGames.size();
    if (posGame < size){
        userGames.at(posGame).setActive(true);
        return true;
    }
    return false;
}

int StompProtocol::getGamePos(string &topic, vector<Game> &userGames){
    vector<Game> :: iterator iter = userGames.begin();
    int pos=0;
    while (iter != userGames.end()){
        if (iter->getName() == topic){
            return pos;
        }
        else{
            iter++;
            pos++;
        }
    }
    return pos;
}
