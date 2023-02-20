package bgu.spl.net.api;
import java.io.IOException;
import bgu.spl.net.srv.Connections;

public interface MessagingProtocol<T> {
 
    /**
     * process the given message 
     * @param msg the received message
     */
    void process(T msg) throws IOException;
 
    /**
     * @return true if the connection should be terminated
     */
    boolean shouldTerminate();

    /**
	 * Used to initiate the current client protocol with it's personal connection ID and the connections implementation
	**/
    void start(int connectionId, Connections<T> connections);
 
}