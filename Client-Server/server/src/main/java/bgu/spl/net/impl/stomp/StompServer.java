package bgu.spl.net.impl.stomp;
import bgu.spl.net.srv.Server;

public class StompServer {
    public static void main(String[] args) {
        // TODO: implement this
        

        // you can use any server... 
        if (args[1].equals("tpc")){
            Server.threadPerClient(
                    7777, //port
                    () -> new StompMessagingProtocolImpl(), //protocol factory
                    StompMessageEncoderDecoder::new //message encoder decoder factory
            ).serve();
        }

        else if (args[1].equals("reactor")){
            Server.reactor(
                    Runtime.getRuntime().availableProcessors(),
                    7777, //port
                    () ->  new StompMessagingProtocolImpl(), //protocol factory
                    StompMessageEncoderDecoder::new //message encoder decoder factory
            ).serve();
        }
        else{
            throw new IllegalArgumentException();
        }
    }
    
}
