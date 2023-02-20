package bgu.spl.net.impl.stomp;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.api.MessagingProtocol;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;

public class StompMessagingProtocolImpl implements MessagingProtocol<String> {

    protected volatile boolean shouldTerminate = false;
    protected Connections<String> connections;
    protected int connectionId;
    protected ConcurrentHashMap <String, LinkedBlockingQueue<String>> TopicToSubIds = new ConcurrentHashMap<String, LinkedBlockingQueue<String>>();
    protected ConcurrentHashMap <String, String> subIdToTopic = new ConcurrentHashMap<String, String>();
    protected StompDataBase dataBase;
    protected String userName;
    private volatile boolean connected = false;


    public void process(String msg) {
        int index = 0;
        while (index < msg.length() && msg.charAt(index) != '\n'){
            index++;
        }
        String command = msg.substring(0, index);

        String content = msg.substring(index+1);
        if (command.equals("CONNECT")) { connect(content); }
        else if (connected){
            if (command.equals("SEND")) { send(content); }
            else if (command.equals("SUBSCRIBE")) { subscribe(content); }
            else if (command.equals("UNSUBSCRIBE")) { unsubscribe(content); }
            else if (command.equals("DISCONNECT")) { disconnect(content); }
            else { error("-1", "Illegal Command", content); } 
        }
    }
 
    public boolean shouldTerminate(){
        return shouldTerminate;
    }

    @Override
    public void start(int connectionId, Connections<String> connections){
        this.connections = connections;
        this.connectionId = connectionId;
        dataBase = StompDataBase.getStompDataBase();
        //...
    }

    public void connect(String content){
        HashMap <String, String> frameFields = seperate(content);
        String acceptVersion = frameFields.get("accept-version");
        String host = frameFields.get("host");
        String login = frameFields.get("login");
        String passcode = frameFields.get("passcode");
        String body = frameFields.get("body");
        if (acceptVersion == null || !acceptVersion.equals("1.2")
            | host == null || !host.equals("stomp.cs.bgu.ac.il")
            | login == null | passcode == null | body != null)
            error("-1", "Malformed connect frame received", content);
        else{
            userName = login;
            String loginResponse = dataBase.loginResponse(login, passcode); 
            if (loginResponse.equals("Login is legal")){
                String frame = "CONNECTED\nversion:1.2\n\n";
                connected = true;
                connections.send(connectionId, frame);
            }
            else{
                error("-1", loginResponse, content);
            }
        }
    }

    public void send(String content){
        HashMap <String, String> frameFields = seperate(content);
        String receipt = frameFields.get("receipt");
        String destination = frameFields.get("destination");
        String body = frameFields.get("body");
        if (receipt == null)
            receipt = "-1";
        if (destination == null | body == null || !messageIsValid(body))
            error(receipt, "Malformed frame received", "");
        if (!TopicToSubIds.containsKey(destination))
            error(receipt, "Message can't be sent to an unsubscribed topic", "");
        else{
            dataBase.handleSendProcess(destination, body);
            if (!receipt.equals("-1"))
                receipt(receipt);
        }
    }

    public void subscribe(String content){
        HashMap <String, String> frameFields = seperate(content);
        String receipt = frameFields.get("receipt");
        String id = frameFields.get("id");
        String destination = frameFields.get("destination");
        String body = frameFields.get("body");
        if (receipt == null)
            error("-1", "Malformed frame received - missing receipt id", "");
        else if (id == null | destination == null | body != null)
            error(receipt, "Malformed frame received", "");
            //should we add if receipt-id is not unique?
        else if (subIdToTopic.containsKey(id))
                error(receipt, "Subscribe id already in use", "");
        else{
            dataBase.addSubscription(destination, connectionId, id);
            LinkedBlockingQueue<String> subscriptionsIds = TopicToSubIds.get(destination);
            if (subscriptionsIds == null) //topic didn't exist
                subscriptionsIds = new LinkedBlockingQueue<String>();
            subscriptionsIds.add(id);
            TopicToSubIds.put(destination, subscriptionsIds);
            subIdToTopic.put(id, destination);
            if (!receipt.equals("-1"))
                receipt(receipt);
        }
    }

    public void unsubscribe(String content) {
        HashMap <String, String> frameFields = seperate(content);
        String receipt = frameFields.get("receipt");
        String id = frameFields.get("id");
        String body = frameFields.get("body");
        if (receipt == null)
            receipt = "-1";
        if (body != null || id == null)
            error(receipt, "Malformed frame received - body should be empty", "");
        if (id == "null" || !subIdToTopic.containsKey(id))
            error(receipt, "Illegal subscription id", "");
            //should we add if receipt-id is not unique?
        else{
            String currTopic = subIdToTopic.get(id);
            dataBase.removeSubscription(currTopic, connectionId, id);
            LinkedBlockingQueue<String> subscriptionsIds = TopicToSubIds.get(currTopic);
            subscriptionsIds.remove(id);
            if (subscriptionsIds.size() == 0) //no other subscription id
                TopicToSubIds.remove(currTopic);
            else
                TopicToSubIds.put(currTopic, subscriptionsIds);
            subIdToTopic.remove(id);
            if (!receipt.equals("-1"))
                receipt(receipt);
        }
    }

    public void disconnect(String content){
        // TODO
        HashMap <String, String> frameFields = seperate(content);
        String receipt = frameFields.get("receipt");
        String body = frameFields.get("body");
        if (body != null)
            error(receipt, "Malformed frame received - body should be empty", "");
        if (receipt == null)
            error("-1", "Malformed disconnect frame received - missing receipt-id header ", "");
            //should we add if receipt-id is not unique?
        else{
            dataBase.disconnect(userName);
            shouldTerminate = true;
            connected = false;
            dataBase.removeSubscriptions(connectionId);
            receipt(receipt);
            connections.disconnect(connectionId);
        }
    }


    public void error(String receiptId, String error, String frameContent){
        // TODO
        String frame;
        if (receiptId.equals("-1"))
            frame = "ERROR\nmessage:"+error+"\n\nError was caused by the following message-\n"+frameContent+"\n";
        else
            frame = "ERROR\nreceipt-id:"+receiptId+"\nmessage:"+error+"\n\n";
        connections.send(connectionId, frame);
        //close connection
        dataBase.disconnect(userName);
        shouldTerminate = true;
        dataBase.removeSubscriptions(connectionId);
        connections.disconnect(connectionId);
        connected = false;
    }

    public void receipt(String receiptId){
        String frame = "RECEIPT\nreceipt-id:"+receiptId+"\n\n";
        connections.send(connectionId, frame);
    }

    public HashMap <String, String> seperate(String content){
        HashMap <String, String> frameFields = new HashMap<String, String>();
        String currString = content;
        int index = content.indexOf(":");
        while (index < currString.length() & index >= 0){
            String key = currString.substring(0, index);
            currString = currString.substring(index+1);
            index = currString.indexOf("\n");
            String value = currString.substring(0, index);
            frameFields.put(key, value);
            currString = currString.substring(index+1);
            index = 0;
            if (currString.charAt(index) == '\n'){
                if (currString.length() > 2){
                    currString = currString.substring(index+1);
                    frameFields.put("body", currString );
                }
                //index = currString.length();
                break;
            }
            else
                index = currString.indexOf(":");
        }
        return frameFields;
    }

    public boolean messageIsValid(String body){
        // TODO
        HashMap <String, String> frameFields = seperate(body);
        if (!frameFields.containsKey("event name") || !frameFields.containsKey("description")
         || !frameFields.containsKey("time") || !frameFields.containsKey("general game updates")
         || !frameFields.containsKey("team a updates") || !frameFields.containsKey("team b updates"))
            return false;
        else 
            return true;
    }


    public LinkedBlockingQueue<String> getTopicSubscritionIds(String topic){
        return TopicToSubIds.get(topic);
    }
    
}
