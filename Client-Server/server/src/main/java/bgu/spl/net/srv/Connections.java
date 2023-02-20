package bgu.spl.net.srv;


public interface Connections<T> {

    boolean send (int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    void addConnection(ConnectionHandler<T> connectionHandler, int connectionId);

}
