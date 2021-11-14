/*

  The boost library has clever tools for handling program
  parameters. Here, for the sake of code simplification, we use a
  custom class.

*/

#pragma once

#include <tuple>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <memory>
#include <map>
#include <queue>
#include <iomanip>
#include <boost/heap/binomial_heap.hpp>


namespace tweetoscope
{
    namespace params
    {
        namespace section
        {
            struct Kafka
            {
                std::string brokers;
            };

            struct Topic
            {
                std::string in, out_series, out_properties;
            };

            struct Times
            {
                std::vector<std::size_t> observation;
                std::size_t terminated;
            };

            struct Cascade
            {
                
                std::size_t min_cascade_size;
            };
        }

        struct collector
        {
        private:
            std::string current_section;

            std::pair<std::string, std::string> parse_value(std::istream &is)
            {
                char c;
                std::string buf;
                is >> std::ws >> c;
                while (c == '#' || c == '[')
                {
                    if (c == '[')
                        std::getline(is, current_section, ']'); //getline reads characters from an input stream and places them into a string
                    std::getline(is, buf, '\n');                //input-the stream to get data from   str	-  the string to put the data into    delim-the delimiter character
                    is >> std::ws >> c;
                }
                is.putback(c); // recupere le dernier input stream utilisé
                std::string key, val;
                is >> std::ws; // supprime les esapces
                std::getline(is, key, '=');
                is >> val;
                std::getline(is, buf, '\n');
                return {key, val};
            }

        public:
            section::Kafka kafka;
            section::Topic topic;
            section::Times times;
            section::Cascade cascade;

            collector(const std::string &config_filename)
            {
                std::ifstream ifs(config_filename.c_str()); //.c_str renvoie A pointer to the c-string representation of the string object's value.
                if (!ifs)
                    throw std::runtime_error(std::string("Cannot open \"") + config_filename + "\" for reading parameters.");
                ifs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
                try
                {
                    while (true)
                    {
                        auto [key, val] = parse_value(ifs);
                        if (current_section == "kafka")
                        {
                            if (key == "brokers")
                                kafka.brokers = val;
                        }
                        else if (current_section == "topic")
                        {
                            if (key == "in")
                                topic.in = val;
                            else if (key == "out_series")
                                topic.out_series = val;
                            else if (key == "out_properties")
                                topic.out_properties = val;
                        }
                        else if (current_section == "times")
                        {
                            if (key == "observation")
                                times.observation.push_back(std::stoul(val));
                            else if (key == "terminated")
                                times.terminated = std::stoul(val);
                        }
                        else if (current_section == "cascade")
                        {
                            if (key == "min_cascade_size")
                                cascade.min_cascade_size = std::stoul(val);
                        }
                    }
                }
                catch (const std::exception &e)
                { /* nope, end of file occurred. */
                }
            }
        };

        inline std::ostream &operator<<(std::ostream &os, const collector &c)
        {
            os << "[kafka]" << std::endl
               << "  brokers=" << c.kafka.brokers << std::endl
               << std::endl
               << "[topic]" << std::endl
               << "  in=" << c.topic.in << std::endl
               << "  out_series=" << c.topic.out_series << std::endl
               << "  out_properties=" << c.topic.out_properties << std::endl
               << std::endl
               << "[times]" << std::endl;
            for (auto &o : c.times.observation)
                os << "  observation=" << o << std::endl;
            os << "  terminated=" << c.times.terminated << std::endl
               << std::endl
               << "[cascade]" << std::endl
               << "  min_cascade_size=" << c.cascade.min_cascade_size << std::endl;
            return os;
        }
    }



    using       timestamp   =   std::size_t; // unsigned long
    
    namespace   source  {
        using   idf =   std::size_t;
    }

    namespace   cascade {
        using   idf =   std::size_t;
    }

    struct      tweet
    {
        std::string type        =   "";
        std::string msg         =   "";
        timestamp   time        =   0;
        double      magnitude   =   0;
        source::idf source      =   0;
        std::string info        =   "";
    };

    inline  std::string     get_string_val(std::istream& is)    {
        char    c;
        is   >> c; // eats  "
        std::string value;
        std::getline(is, value, '"'); // eats tweet", but value has tweet
        return value; 
    }

    inline  std::istream&   operator>>(std::istream& is, tweet& t) {
        // A tweet is  : {"type" : "tweet"|"retweet", 
        //                "msg": "...", 
        //                "time": timestamp,
        //                "magnitude": 1085.0,
        //                "source": 0,
        //                "info": "blabla"}
        std::string buf;
        char c;
        is >> c; // eats '{'
        is >> c; // eats '"'
        while(c != '}') { 
        std::string tag;
        std::getline(is, tag, '"'); // Eats until next ", that is eaten but not stored into tag.
        is >> c;  // eats ":"
        if     (tag == "type")    t.type = get_string_val(is);
        else if(tag == "msg")     t.msg  = get_string_val(is);
        else if(tag == "info")    t.info = get_string_val(is);
        else if(tag == "t")       is >> t.time;
        else if(tag == "m")       is >> t.magnitude;
        else if(tag == "source")  is >> t.source;
                
        is >> c; // eats either } or ,
        if(c == ',')
            is >> c; // eats '"'
        } 
        return is;
    }

    namespace   cascade {

        // Definition of the two classes
        class   Processor;
        class   Cascade; // Class for storing cascade information.
        struct  CascadeRefComparator; // Definition of a class of comparison functor for boost queues.


        // Definition of types like
        using   cascade_ref             =   std::shared_ptr<Cascade>;
        using   cascade_wref            =   std::weak_ptr<Cascade>;
        using   priority_queue          =   boost::heap::binomial_heap<cascade_ref, boost::heap::compare< CascadeRefComparator>>;
        using   idf                     =   std::size_t;


        // Implementation of CascadeRefComparator class
        struct  CascadeRefComparator {
            bool operator()(cascade_ref ref_c1, cascade_ref ref_c2) const;
        };

        inline  bool    CascadeRefComparator::operator()(cascade_ref ref_c1, cascade_ref ref_c2) const{
            return ref_c1 > ref_c2;
        }

        // Implementation of the Cascade class
        class Cascade
        {
            private:
                // Attributes
                std::string                             m_id;
                std::string                             m_msg;
                timestamp                               m_timeOfFirstTweet;
                timestamp                               m_timeOfLastTweet;
                std::vector<std::pair<timestamp, int>>  m_pairsOfTimesAndMagnitudes; 
                source::idf                             m_source; 
                

            public:
                // Constructor
                Cascade(const tweet& twt, const std::string& key);
                Cascade(const Cascade& process)                     =   default;
                Cascade(Cascade&& process)                          =   default;
                Cascade& operator=(const Cascade& process)          =   default;
                Cascade& operator=(Cascade&& process)               =   default;

                // Destructor
                ~Cascade();

                // Methods
                void operator+=(const std::pair<tweet, std::string>& elt)           ;
                bool operator<(const cascade_ref& ref_other_cascade)           const;
                cascade_ref makeRef(tweet& twt, std::string&key)                    ;

        };
            
        // Inlining methods of the Cascade class    
        inline  Cascade::Cascade(const tweet& twt, const std::string& key)  :   m_id(key),
                                                                                m_msg(twt.msg),
                                                                                m_timeOfFirstTweet(twt.time),
                                                                                m_timeOfLastTweet(twt.time),
                                                                                m_pairsOfTimesAndMagnitudes{std::make_pair(twt.time, twt.magnitude)},
                                                                                m_source(twt.source)
        {}
            
        inline  Cascade::~Cascade() {}

        inline  void        Cascade::operator+=(const std::pair<tweet, std::string>& elt)   { 
            /* to dev */
        }

        inline  bool        Cascade::operator<(const cascade_ref& ref_other_cascade) const  {
            /* to dev */
            return true;
        }
        
        inline  cascade_ref Cascade::makeRef(tweet& twt, std::string&key)                   {
            return std::make_shared<Cascade>(twt,key);
        }




        // Implementation of the Processor class
        class Processor
        {
        private:
            // Attributes
            source::idf                                     m_source;
            timestamp                                       m_sourceTime;
            priority_queue                                  m_priorityQueue;
            std::map<timestamp, std::queue<cascade_wref>>   m_FIFO;
            std::map<idf, cascade_wref>                     m_symbolTable;
        

        public:
            // Constructor
            Processor(const tweet& twt);
            Processor(const Processor& process)             =   default;
            Processor(Processor&& process)                  =   default;
            Processor& operator=(const Processor& process)  =   default;
            Processor& operator=(Processor&& process)       =   default;

            // Destructor
            ~Processor();

            // Methods
            std::vector<std::string>    sendPartialCascade(const std::vector<std::size_t>& obs)                 const;
            std::vector<std::string>    sendTerminatedCascade(timestamp& end_time, const std::size_t& min_size) const;
        };
        
        // Inlining methods of the Processor class 
        inline  Processor::Processor(const tweet& twt)      :   m_source(twt.source),
                                                                m_sourceTime(twt.time),
                                                                m_priorityQueue{}, 
                                                                m_FIFO{}, 
                                                                m_symbolTable{} 
        {}
        
        inline  Processor::~Processor() {}

        std::vector<std::string>    sendPartialCascade(const std::vector<std::size_t>& obs)                 {
            /* to dev */
            return {"to dev"};
        }

        std::vector<std::string>    sendTerminatedCascade(timestamp& end_time, const std::size_t& min_size) {
            /* to dev */
            return {"to dev"};
        }
        
        
    } //End of namespace cascade




}