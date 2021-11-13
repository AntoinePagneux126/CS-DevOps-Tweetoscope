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




}