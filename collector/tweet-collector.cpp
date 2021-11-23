#include    <cppkafka/cppkafka.h>
#include    <iostream>
#include    <sstream>
#include    "tweetoscopeCollectorParams.hpp"

/* --- Informations --- */
/* ----------
                                    

Compile the collector : g++ -o tweet-collector tweet-collector.cpp -O3 -lcppkafka

Check to see if the ZooKeeper daemon is already installed and running : systemctl status zookeeper
Start a kafka broker :  $KAFKA_HOME/bin/kafka-server-start.sh $KAFKA_HOME/config/server.properties &
Create a topic named tweets : $KAFKA_HOME/bin/kafka-topics.sh --create --bootstrap-server localhost:9092 --replication-factor 1 --partitions 1 --topic tweets


Written from : https://gitlab.com/Virgitlabnie/sae_demo03/-/blob/master/Cpp/consumer.cpp
And          : https://gitlab.com/Virgitlabnie/sae_demo03/-/blob/master/Cpp/producer.cpp
        

----------- */
/* --- ############# --- */

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        std::cout << "Usage : " << argv[0] << " <config-filename>" << std::endl;
        return 0;
    }
    tweetoscope::params::collector params(argv[1]);
    std::cout << std::endl
              << "Parameters : " << std::endl
              << "----------" << std::endl
              << std::endl
              << params << std::endl
              << std::endl;


    /* --- Implementation of the Consumer --- */
    // Configuration of the consumer 
    cppkafka::Configuration     ConsumerConfig  =   {
                                                        { "bootstrap.servers", params.kafka.brokers },
                                                        { "auto.offset.reset", "earliest" },
                                                        { "group.id", "myOwnPrivateCppGroup" }
                                                    };
    // Implementation of a Consumer
    cppkafka::Consumer          Consumer(ConsumerConfig);
    Consumer.subscribe({params.topic.in});
    std::cout<< "-------------------------------" << std::endl;
    std::cout<< "Consumer Created" << std::endl;
    std::cout<< "-------------------------------" << std::endl;


    /* --- Implementation of the Producers --- */
    // Configuration of the producer
    cppkafka::Configuration     ProducerConfig  =   {
                                                        {"bootstrap.servers", params.kafka.brokers}
                                                    };
    // Implementation of a Producer which write on partial Cascades : series
    cppkafka::MessageBuilder    PartialMessageBuilder       {params.topic.out_series};
    cppkafka::Producer          Producer(ProducerConfig);
    // Implementation of a Producer which write on terminated Cascades : properties
    cppkafka::MessageBuilder    TerminatedMessageBuilder    {params.topic.out_properties};
    std::cout<< "-------------------------------" << std::endl;
    std::cout<< "Producers Created" << std::endl;
    std::cout<< "-------------------------------" << std::endl;


    /* --- --- */

    std::map<tweetoscope::source::idf,tweetoscope::cascade::Processor>          mapIdfProcessor;
    std::map<std::string, tweetoscope::cascade::priority_queue::handle_type>    mapKeyHandle; 
    std::vector<std::size_t>                                                    obs         =   params.times.observation;
    tweetoscope::timestamp                                                      end_time    =   params.times.terminated;


    /* --- ---*/
    std::cout<< "-------------------------------" << std::endl;
    std::cout<< "Start reading tweets" << std::endl;
    std::cout<< "-------------------------------" << std::endl;
    // To handle an "endless" loop
    bool continueLoop = true;
    while(continueLoop){
        // Reading of the topic
        auto    msg =   Consumer.poll();

        // Assert msg is not empty and there no errors 
        if (msg && ! msg.get_error()){
            // Instanciation of a tweet
            tweetoscope::tweet Twt;
            auto init_key = tweetoscope::cascade::idf(std::stoi(msg.get_key()));
            auto istr = std::istringstream(std::string(msg.get_payload()));
            istr >> Twt;
            //std::cout << Twt.source<<"  "<< Twt.type<<std::endl;

            //  Creating processor of the source if not already created
            auto key = std::to_string(init_key);
            if (mapIdfProcessor.find(Twt.source)==mapIdfProcessor.end()){
                //std::cout<<"coukar's"<<std::endl;
                std::cout<< "-------------------------------" << std::endl;
                std::cout<< "Creating processor" << std::endl;
                std::cout<< "-------------------------------" << std::endl;
                tweetoscope::cascade::Processor processor(Twt);
                mapIdfProcessor.insert(std::make_pair(Twt.source,processor));
            }

            // Instanciation of a processor from the tweets source
            tweetoscope::cascade::Processor* Processor = &mapIdfProcessor.at(Twt.source);
            if(Twt.type=="tweet"){
                //td::cout<<"coukars'ss"<<std::endl;
                // Set the source time
                Processor->setSourceTime(Twt.time);
                // Create a shared pointer on cascade
                tweetoscope::cascade::cascade_ref refCascade = tweetoscope::cascade::makeRef(Twt, key);             
                // Add a weak pointer to the cascade
                tweetoscope::cascade::cascade_wref weakRefCascade = refCascade;
                // Add the reference to the priority queue
                auto pos = Processor->addToPriorityQueue(refCascade);
                mapKeyHandle.insert(std::make_pair(key,pos));
                // Add the weakRef of the cascade to FIFO
                if (!(obs.empty())){
                    for(auto& t_obs : obs){
                        Processor -> addToFIFO(t_obs, weakRefCascade);
                    }
                }
                // Add weakRef to the symbole table
                Processor -> addToSymbolTable(key, weakRefCascade);
            }
            else{
                //std::cout<<"in the else"<<std::endl;
                tweetoscope::cascade::cascade_wref weakRefCascade = Processor->getSymbolTable()[key];
                if (auto refCascade = weakRefCascade.lock(); refCascade){
                    refCascade -> addTweetToCascade(Twt, key);
                    Processor  -> setSourceTime(Twt.time);
                    Processor  -> decreasePriorityQueue(mapKeyHandle[key], refCascade);
                }
                if(weakRefCascade.use_count()>1){
                    throw std::invalid_argument( "Cascade has too many shared pointers");
                }
            }
            //std::cout<<"out of the else"<<std::endl;
            // Partial Cascades : series
            //std::cout<<"before serie tosend "<<std::endl;
            std::vector<std::string>    seriesToSend = Processor->sendPartialCascade(obs);
            //std::cout<<"after series tosend"<<std::endl;
            for(auto& msg_series : seriesToSend){  
                std::cout<< "Sending Partial Cascades : " << msg_series << std::endl;
                PartialMessageBuilder.payload(msg_series); 
                try{
                    // Try to send message
                    Producer.produce(PartialMessageBuilder);
                } 
                catch (const cppkafka::HandleException& e) {
                    std::ostringstream ostr2;
                    ostr2 << e.what();
                    std::string error {ostr2.str()};
                    if (error.compare("Queue full") != 0) { 
                        std::chrono::milliseconds timeout(3000);	
                        Producer.flush(timeout);
                        Producer.produce(PartialMessageBuilder);
                    } 
                    else {
                        std::cout << "Something went wrong: " << e.what() << std::endl;
                    }
                }
            }
            // Terminated Cascades : properties
            //std::cout<<"before properties tosend "<<std::endl;
            std::vector<std::string>  propertiesToSend = Processor-> sendTerminatedCascade(end_time, params.cascade.min_cascade_size);
            //std::cout<<"after properties tosend"<<std::endl;
            for(auto& msg_properties : propertiesToSend){
                //std::cout<<"1"<<std::endl;
                std::cout<< "Sending Terminated Cascades : " << msg_properties <<std::endl;
                //std::cout<<"2"<<std::endl;
                TerminatedMessageBuilder.payload(msg_properties);
                //std::cout<<"3"<<std::endl;
                int i = 0;
                for(auto T_obs : obs){
                    //std::cout<<"4 for"<<std::endl;
                    auto T_obs_key = std::to_string(T_obs);
                    TerminatedMessageBuilder.partition(i);
                    i++;
                    TerminatedMessageBuilder.key(T_obs_key);
                    //std::cout<<"5"<<std::endl;
                    try{
                        // Try to send message
                        //std::cout<<"6 try"<<std::endl;
                        Producer.produce(TerminatedMessageBuilder);
                        //std::cout<<"7 try"<<std::endl;
                    } 
                    catch (const cppkafka::HandleException& e) {
                        //std::cout<<"8 catch"<<std::endl;
                        std::ostringstream ostr2;
                        ostr2 << e.what();
                        std::string error {ostr2.str()};
                        //std::cout<<"9"<<std::endl;
                        if (error.compare("Queue full") != 0) { 
                            //std::cout<<"10 if"<<std::endl;
                            std::chrono::milliseconds timeout(3000);	
                            Producer.flush(timeout);
                            //Producer.produce(TerminatedMessageBuilder);
                            //std::cout<<"11 end if"<<std::endl;
                        } 
                        else {
                            //std::cout<<"12 else"<<std::endl;
                            std::cout << "Something went wrong: " << e.what() << std::endl;
                            //std::cout<<"13 end else"<<std::endl;
                        }
                    }
                }
            }
            //std::cout<<"after properties for"<<std::endl;
        }
    }   
    return 0;
}
