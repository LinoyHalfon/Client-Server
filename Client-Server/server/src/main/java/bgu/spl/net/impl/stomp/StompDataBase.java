package bgu.spl.net.impl.stomp;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;

public class StompDataBase {

    private static class StompDataBaseHolder{
        private static StompDataBase dataBase = new StompDataBase();
        
    }

    protected ConcurrentHashMap <String, String> usersLogin = new ConcurrentHashMap<String, String>();
    protected LinkedBlockingQueue<String> activeUsers = new LinkedBlockingQueue<String>();
    protected ConcurrentHashMap <String, ConcurrentHashMap <Integer,LinkedBlockingQueue<String>>> topics = new ConcurrentHashMap <String, ConcurrentHashMap <Integer,LinkedBlockingQueue<String>>>();
    private Object usersLock = new Object();
    protected int nextMessageId = 0; 
    protected Connections<String> connections;

    private StompDataBase(){
        connections = (Connections<String>) ConnectionsImpl.getConnectionsImpl();
    }

    public static StompDataBase getStompDataBase(){
        return StompDataBaseHolder.dataBase;
    }

    public void addUser(String login, String passcode){
        usersLogin.put(login, passcode);
        activeUsers.add(login);  
    }

    public String loginResponse(String login, String passcode){
        synchronized (usersLock){
            if (!activeUsers.contains(login)){
                if (usersLogin.containsKey(login)){
                    if (usersLogin.get(login).equals(passcode)){
                        return "Login is legal";
                    }
                    else{
                        return "Wrong Password";
                    }
                }
                else{ //!usersLogin.containsKey(login)
                    addUser(login, passcode);
                    return "Login is legal";
                }
            }
            else //activeUsers.contains(login)
                return "User already logged in";
        }   
    }

    public void addSubscription(String topicName, int connectionId, String subscriptionId){
        synchronized (topics){
            ConcurrentHashMap <Integer,LinkedBlockingQueue<String>> subscribedClients = topics.computeIfAbsent(topicName, k -> new ConcurrentHashMap <Integer,LinkedBlockingQueue<String>>());
            LinkedBlockingQueue<String> subscriptionIds = subscribedClients.computeIfAbsent(connectionId, k -> new LinkedBlockingQueue<String>());
            subscriptionIds.add(subscriptionId);
        }
    }

    public void removeSubscriptions(int connectionId){
        synchronized (topics){
            for (String topic : topics.keySet()){
                ConcurrentHashMap <Integer,LinkedBlockingQueue<String>> subscribedClients = topics.get(topic);  
                subscribedClients.remove(connectionId);
            }                
        }
    }

    public void removeSubscription(String topicName, int connectionId, String subscriptionId){
        synchronized (topics){
            ConcurrentHashMap <Integer,LinkedBlockingQueue<String>> subscribedClients = topics.get(topicName);  
            subscribedClients.get(connectionId).remove(subscriptionId);
            }                
        }

    public void handleSendProcess(String topic, String body){
        synchronized (topics){
            String frame;
            ConcurrentHashMap<Integer,LinkedBlockingQueue<String>> subscribedClients = topics.get(topic);
            for (Integer connectionId : subscribedClients.keySet()){   
                LinkedBlockingQueue<String> subIds = subscribedClients.get(connectionId);
                for (String subId : subIds){
                    frame = "MESSAGE"+"\nsubscription:"+subId+"\nmessage-id:"+nextMessageId+"\ndestination:"+topic+"\n\n"+body+"\n\u0000";
                    nextMessageId++;
                    connections.send(connectionId, frame);
                }
            }
        }
    }

    public void disconnect(String userName){
        synchronized (usersLock){
            activeUsers.remove(userName);
        }
    }
}
