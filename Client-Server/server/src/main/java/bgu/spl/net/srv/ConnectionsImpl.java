package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.io.IOException;

public class ConnectionsImpl<T> implements Connections<T> {

    private static class ConnectionsImplHolder{
        private static ConnectionsImpl connections = new ConnectionsImpl<>();
        
    }

    protected ConcurrentHashMap <Integer, ConnectionHandler<T>> connectionsIDs = new ConcurrentHashMap<Integer, ConnectionHandler<T>>();
    

    private ConnectionsImpl() {}

    public static ConnectionsImpl getConnectionsImpl(){
        return ConnectionsImplHolder.connections;
    }

    public boolean send(int connectionId, T msg){
        // TODO
        connectionsIDs.get(connectionId).send(msg);
        return true;
    }

    public void send(String channel, T msg){
    }

    public void disconnect(int connectionId) {
        synchronized (connectionsIDs){
            connectionsIDs.remove(connectionId);
        }
    }

    public void addConnection(ConnectionHandler<T> connectionHandler, int connectionId){
        synchronized (connectionsIDs){
            connectionsIDs.put(connectionId, connectionHandler);
        }
    }
    
}
