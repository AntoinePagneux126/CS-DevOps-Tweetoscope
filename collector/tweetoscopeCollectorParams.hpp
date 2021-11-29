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

#define DURATION_END_CASCADE 1000

namespace tweetoscope
{
    namespace params
    {
        namespace section
        {
            /**
             * @brief Kafka class
             * 
             */
            struct Kafka
            {
                std::string brokers;
            };
            /**
             * @brief Topic class
             * 
             */
            struct Topic
            {
                std::string in, out_series, out_properties;
            };

            /**
             * @brief Times class
             * 
             */
            struct Times
            {
                std::vector<std::size_t> observation;
                std::size_t terminated;
            };

            /**
             * @brief Cascade class
             * 
             */
            struct Cascade
            {

                std::size_t min_cascade_size;
            };
        }

        /**
         * @brief collector class
         * 
         */
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


            /**
             * @brief Construct a new collector object
             * 
             * @param config_filename 
             */
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

        /**
         * @brief Overload operator << to show collector parameters
         * 
         * @param os 
         * @param c 
         * @return std::ostream& 
         */
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

    using timestamp = std::size_t; // unsigned long

    namespace source
    {
        using idf = std::size_t;
    }

    namespace cascade
    {
        using idf = std::size_t;
    }

    /**
     * @brief Structure tweet, objet with tweet parameters
     * 
     */
    struct tweet
    {
        std::string type = "";
        std::string msg = "";
        timestamp time = 0;
        double magnitude = 0;
        source::idf source = 0;
        std::string info = "";
    };

    /**
     * @brief Get the string val object
     * 
     * @param is 
     * @return std::string 
     */
    inline std::string get_string_val(std::istream &is)
    {
        char c;
        is >> c; // eats  "
        std::string value;
        std::getline(is, value, '"'); // eats tweet", but value has tweet
        return value;
    }

    /**
     * @brief Overload >> operator to add a tweet to an istream
     * 
     * @param is 
     * @param t 
     * @return std::istream& 
     */
    inline std::istream &operator>>(std::istream &is, tweet &t)
    {
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
        while (c != '}')
        {
            std::string tag;
            std::getline(is, tag, '"'); // Eats until next ", that is eaten but not stored into tag.
            is >> c;                    // eats ":"
            if (tag == "type")
                t.type = get_string_val(is);
            else if (tag == "msg")
                t.msg = get_string_val(is);
            else if (tag == "info")
                t.info = get_string_val(is);
            else if (tag == "t")
                is >> t.time;
            else if (tag == "m")
                is >> t.magnitude;
            else if (tag == "source")
                is >> t.source;

            is >> c; // eats either } or ,
            if (c == ',')
                is >> c; // eats '"'
        }
        return is;
    }

    namespace cascade
    {

        // Definition of the two classes
        /**
         * @brief Processor class to manage Cascades
         * 
         */
        class Processor;
        /**
         * @brief Cascade class to handle a casdade
         * 
         */
        class Cascade;               // Class for storing cascade information.

        /**
         * @brief CascadeRefComparator class to compare Cascade from a sharepointer that references a cascade
         * 
         */
        struct CascadeRefComparator; // Definition of a class of comparison functor for boost queues.

        // Definition of types like
        using cascade_ref = std::shared_ptr<Cascade>;
        using cascade_wref = std::weak_ptr<Cascade>;
        using priority_queue = boost::heap::binomial_heap<cascade_ref, boost::heap::compare<CascadeRefComparator> >;
        using idf = std::size_t;

        // overloading of << operator
        /**
         * @brief Overload operator << to add time and magnitude to an output to print
         * 
         * @param os 
         * @param time_magnitude 
         * @return std::ostream& 
         */
        std::ostream &operator<<(std::ostream &os, std::vector<std::pair<timestamp, int> > &time_magnitude)
        {
            os << "[";
            auto it_time_magnitude = time_magnitude.begin();
            while (it_time_magnitude != time_magnitude.end() - 1)
            {
                os << " [" << it_time_magnitude->first << ',' << it_time_magnitude->second << "] ,";
                ++it_time_magnitude;
            }
            os << " [" << it_time_magnitude->first << ',' << it_time_magnitude->second << "] ";
            os << "]";
            return os;
        }

        // Implementation of CascadeRefComparator class
        /**
         * @brief CascadeRefComparator class
         * 
         */
        struct CascadeRefComparator
        {
            /**
             * @brief Overload () operator
             * 
             * @param ref_c1 
             * @param ref_c2 
             * @return true 
             * @return false 
             */
            bool operator()(cascade_ref ref_c1, cascade_ref ref_c2) const;
        };

        inline bool CascadeRefComparator::operator()(cascade_ref ref_c1, cascade_ref ref_c2) const
        {
            return ref_c1 > ref_c2;
        }

        // Implementation of the Cascade class
        class Cascade
        {
        private:
            // Attributes
            std::string m_id;
            std::string m_msg = "";
            timestamp m_timeOfFirstTweet;
            timestamp m_timeOfLastTweet;
            std::vector<std::pair<timestamp, int> > m_pairsOfTimesAndMagnitudes;
            source::idf m_source;

            // Declare friend classes
            friend class Processor;
            friend struct CascadeRefComaparator;
            // Declare friend operators
            friend std::ostream &operator<<(std::ostream &os, std::vector<std::pair<timestamp, int> > &time_magnitude);

        public:
            // Constructors
            /**
             * @brief Construct a new Cascade object from a tweet (std::string) and a key (std::string)
             * 
             * @param twt 
             * @param key 
             */
            Cascade(const tweet &twt, const std::string &key);
            /**
             * @brief Construct a new Cascade object by copy
             * 
             * @param process 
             */
            Cascade(const Cascade &process) = default;
            /**
             * @brief Construct a new Cascade object by displacement (move)
             * 
             * @param process 
             */
            Cascade(Cascade &&process) = default;
            
            /**
             * @brief Overload operateur = to construct a new Cascade object by recopy
             * 
             * @param process 
             * @return Cascade& 
             */
            Cascade &operator=(const Cascade &process) = default;
            /**
             * @brief Overload operateur = to construct a new Cascade object by recopying the right value
             * 
             * @param process 
             * @return Cascade& 
             */
            Cascade &operator=(Cascade &&process) = default;

            // Destructor
            /**
             * @brief Destroy the Cascade object
             * 
             */
            ~Cascade();

            // Methods
            // Assessors
            /**
             * @brief Get the Id object
             * 
             * @return std::string 
             */
            std::string getId() const;
            /**
             * @brief Get the Msg object
             * 
             * @return std::string 
             */
            std::string getMsg() const;
            /**
             * @brief Get the Time Of First Tweet object
             * 
             * @return timestamp 
             */
            timestamp getTimeOfFirstTweet() const;
            /**
             * @brief Get the Time Of Last Tweet object
             * 
             * @return timestamp 
             */
            timestamp getTimeOfLastTweet() const;
            /**
             * @brief Get the pair (Times, magnitude) from a Cascade object
             * 
             * @return std::vector<std::pair<timestamp, int> > 
             */
            std::vector<std::pair<timestamp, int> > getpairsOfTimesAndMagnitudes() const;
            /**
             * @brief Get the Source object
             * 
             * @return source::idf 
             */
            source::idf getSource() const;
            // Others
            /**
             * @brief Add a tweet to a cascade object
             * 
             * @param twt 
             * @param key 
             */
            void addTweetToCascade(const tweet &twt, const std::string &key);
            /**
             * @brief Overload operator += to add a tweet and its key to a cascade
             * 
             * @param elt 
             */
            void operator+=(std::pair<tweet, std::string> &elt);
            /**
             * @brief Overload operator < to compare the cascade object from its sharepointer reference with another reference of a cascade
             * 
             * @param ref_other_cascade 
             * @return true 
             * @return false 
             */
            bool operator<(const cascade_ref &ref_other_cascade) const;
        };

        // Inlining methods of the Cascade class
        inline std::string Cascade::getId() const { return m_id; }
        inline std::string Cascade::getMsg() const { return m_msg; }
        inline timestamp Cascade::getTimeOfFirstTweet() const { return m_timeOfFirstTweet; }
        inline timestamp Cascade::getTimeOfLastTweet() const { return m_timeOfLastTweet; }
        inline std::vector<std::pair<timestamp, int> > Cascade::getpairsOfTimesAndMagnitudes() const { return m_pairsOfTimesAndMagnitudes; }
        inline source::idf Cascade::getSource() const { return m_source; }

        inline Cascade::Cascade(const tweet &twt, const std::string &key) : m_id(key),
                                                                            m_msg(twt.msg),
                                                                            m_timeOfFirstTweet(twt.time),
                                                                            m_timeOfLastTweet(twt.time),
                                                                            m_pairsOfTimesAndMagnitudes({std::make_pair(twt.time, twt.magnitude)}),
                                                                            m_source(twt.source)
        {
        }

        inline Cascade::~Cascade() {}

        inline void Cascade::addTweetToCascade(const tweet &twt, const std::string &key)
        {

            this->m_pairsOfTimesAndMagnitudes.push_back(std::make_pair(twt.time, twt.magnitude));
            this->m_timeOfLastTweet = twt.time;
        }

        inline void Cascade::operator+=(std::pair<tweet, std::string> &elt)
        {
            tweet &twt = elt.first;
            std::string &key = elt.second;

            if (key == this->m_id)
            {
                this->m_pairsOfTimesAndMagnitudes.push_back(std::make_pair(twt.time, twt.magnitude));

                if (twt.source == this->m_source && twt.source > this->m_timeOfLastTweet)
                {
                    this->m_timeOfLastTweet = twt.time;
                }
            }
        }

        inline bool Cascade::operator<(const cascade_ref &ref_other_cascade) const
        {
            return m_timeOfLastTweet < ref_other_cascade->getTimeOfLastTweet();
        }

        inline cascade_ref makeRef(tweet &twt, std::string &key)
        {
            return std::make_shared<Cascade>(twt, key);
        }

        // Implementation of the Processor class
        class Processor
        {
        private:
            // Attributes
            source::idf m_source;
            timestamp m_sourceTime;
            priority_queue m_priorityQueue;
            std::map<timestamp, std::queue<cascade_wref> > m_FIFO;
            std::map<std::string, cascade_wref> m_symbolTable;

            // Declare friend classes
            friend class Cascade;
            // Declare friend operators
            friend std::ostream &operator<<(std::ostream &os, std::vector<std::pair<timestamp, int> > &time_magnitude);

        public:
            // Constructor
            /**
             * @brief Construct a new Processor object from a tweet
             * 
             * @param twt 
             */
            Processor(const tweet &twt);
            /**
             * @brief Construct a new Processor object by copy
             * 
             * @param process 
             */
            Processor(const Processor &process) = default;
            /**
             * @brief Construct a new Processor object by displacement
             * 
             * @param process 
             */
            Processor(Processor &&process) = default;
            /**
             * @brief Overload operateur = to construct a new Cascade object by recopy
             * 
             * @param process 
             * @return Processor& 
             */
            Processor &operator=(const Processor &process) = default;
            /**
             * @brief Overload operateur = to construct a new Cascade object by recopying its right value
             * 
             * @param process 
             * @return Processor& 
             */
            Processor &operator=(Processor &&process) = default;

            // Destructor
            /**
             * @brief Destroy the Processor object
             * 
             */
            ~Processor();

            // Methods
            // Assessors
            // Get
            /**
             * @brief Get the Source object
             * 
             * @return source::idf 
             */
            source::idf getSource() const;
            /**
             * @brief Get the Source Time object
             * 
             * @return timestamp 
             */
            timestamp getSourceTime() const;
            /**
             * @brief Get the Priority Queue object
             * 
             * @return priority_queue 
             */
            priority_queue getPriorityQueue() const;
            /**
             * @brief Get the FIFO object
             * 
             * @return std::map<timestamp, std::queue<cascade_wref> > 
             */
            std::map<timestamp, std::queue<cascade_wref> > getFIFO() const;
            /**
             * @brief Get the Symbol Table object
             * 
             * @return std::map<std::string, cascade_wref> 
             */
            std::map<std::string, cascade_wref> getSymbolTable() const;
            // Set
            /**
             * @brief Set the Source Time object
             * 
             * @param src_time 
             */
            void setSourceTime(const timestamp &src_time);
            // Others
            /**
             * @brief Add a cascade of a tweet to the FIFO from its weak reference
             * 
             * @param pos 
             * @param weak_ref_cascade 
             */
            void addToFIFO(const int &pos, const cascade_wref &weak_ref_cascade);
            /**
             * @brief Add a cascade of a tweet to the Symbole Table from its weak reference and the key of the tweet
             * 
             * @param key 
             * @param weak_ref_cascade 
             */
            void addToSymbolTable(const std::string &key, const cascade_wref &weak_ref_cascade);
            /**
             * @brief Add a cascade of a tweet to the Priority Queue from its the share pointer of the reference
             * 
             * @param sh_ref_cascade 
             * @return auto 
             */
            auto addToPriorityQueue(const cascade_ref &sh_ref_cascade);
            /**
             * @brief Remove a Cascade of the Priority Queue from the share pointer that references the cascade object
             * 
             * @param elt 
             * @param sh_ref_cascade 
             */
            void decreasePriorityQueue(const priority_queue::handle_type &elt, const cascade_ref &sh_ref_cascade);
            /**
             * @brief Method to send a serie which is a partial cascade
             * 
             * @param obs 
             * @return std::vector<std::string> 
             */
            std::vector<std::string> sendPartialCascade(const std::vector<std::size_t> &obs);
            /**
             * @brief Method to send properties which correspond to a terminated cascade
             * 
             * @param end_time 
             * @param min_size 
             * @return std::vector<std::string> 
             */
            std::vector<std::string> sendTerminatedCascade(timestamp &end_time, const std::size_t &min_size);
        };

        // Inlining methods of the Processor class

        inline Processor::Processor(const tweet &twt) : m_source(twt.source),
                                                        m_sourceTime(twt.time),
                                                        m_priorityQueue{},
                                                        m_FIFO{},
                                                        m_symbolTable{}
        {
        }

        inline Processor::~Processor() {}

        inline source::idf Processor::getSource() const { return this->m_source; }
        inline timestamp Processor::getSourceTime() const { return this->m_sourceTime; }
        inline priority_queue Processor::getPriorityQueue() const { return this->m_priorityQueue; }
        inline std::map<timestamp, std::queue<cascade_wref> > Processor::getFIFO() const { return this->m_FIFO; }
        inline std::map<std::string, cascade_wref> Processor::getSymbolTable() const { return this->m_symbolTable; }

        inline void Processor::setSourceTime(const timestamp &src_time) { this->m_sourceTime = src_time; }

        inline void Processor::addToFIFO(const int &t_obs, const cascade_wref &weak_ref_cascade)
        {
            this->m_FIFO[t_obs].push(weak_ref_cascade);
        }

        inline void Processor::addToSymbolTable(const std::string &key, const cascade_wref &weak_ref_cascade)
        {
            this->m_symbolTable.insert(std::make_pair(key, weak_ref_cascade));
        }

        inline auto Processor::addToPriorityQueue(const cascade_ref &sh_ref_cascade)
        {
            return this->m_priorityQueue.push(sh_ref_cascade);
        }

        inline void Processor::decreasePriorityQueue(const priority_queue::handle_type &elt, const cascade_ref &sh_ref_cascade)
        {
            this->m_priorityQueue.decrease(elt, sh_ref_cascade);
        }

        inline std::vector<std::string> Processor::sendPartialCascade(const std::vector<std::size_t> &obs)
        {
            //obs is a vector of time to send the cascade
            std::vector<std::string> seriesToSend;
            for (auto &t_obs : obs)
            {
                std::vector<std::string> ids;
                if (!this->m_FIFO[t_obs].empty())
                {
                    cascade_wref wRefCascade = this->m_FIFO[t_obs].front(); // Take the last element of the FIFO
                    auto currentCascade = wRefCascade.lock();               // Take a weak pointer on it to be sure the shared pointer exists
                    // loop while time beetwen the source time and time of the last tweet
                    // is still higher than observation time
                    if (currentCascade == 0)
                    {
                        break;
                    }
                    while ((this->m_sourceTime - currentCascade->m_timeOfFirstTweet) >= t_obs)
                    {
                        auto it = currentCascade->m_pairsOfTimesAndMagnitudes.begin();
                        std::vector<std::pair<timestamp, int> > partialPairsTimesMagnitudes;
                        while ((it->first - currentCascade->m_timeOfFirstTweet <= t_obs) & (it != currentCascade->m_pairsOfTimesAndMagnitudes.end()))
                        {
                            partialPairsTimesMagnitudes.push_back(*it);
                            ++it;
                        }
                        std::ostringstream os;
                        os << "{"
                           << "\"type\" : \"serie\""
                           << ", \"cid\" : " << currentCascade->getId()
                           << ", \"msg\": \"" << currentCascade->getMsg() << '"'
                           << ", \"T_obs\" : " << t_obs
                           << ",\"tweets\" :" << partialPairsTimesMagnitudes
                           << '}';

                        std::string msg_series = os.str();
                        // Check if the key is not duplicated

                        if (std::count(ids.begin(), ids.end(), currentCascade->m_id))
                        {
                            std::cout << "Duplicated key : " << currentCascade->getId() << " , T_obs : " << t_obs << std::endl;
                        }
                        else
                        {
                            std::cout << msg_series << std::endl;
                            seriesToSend.push_back(msg_series);
                            ids.push_back(currentCascade->m_id);
                        }
                        this->m_FIFO[t_obs].pop();
                        if (!(this->m_FIFO[t_obs].empty()))
                        {
                            wRefCascade = this->m_FIFO[t_obs].front();
                            auto currentCascade = wRefCascade.lock();
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            return seriesToSend;
        }

        inline std::vector<std::string> Processor::sendTerminatedCascade(timestamp &end_time, const std::size_t &min_size)
        {
            std::vector<std::string> propertiesToSend;
            // First check that priorityqueue is not empty
            if (!(this->m_priorityQueue.empty()))
            {
                auto topCascade = this->m_priorityQueue.top();
                while ((this->m_sourceTime - topCascade->m_timeOfLastTweet) > end_time)
                {
                    // Check the size of the cascade is greater than the min size required
                    // So, it can determine if the cascade should be considered
                    if (topCascade->m_pairsOfTimesAndMagnitudes.size() > min_size)
                    {
                        std::ostringstream os;
                        os << "{"
                           << "\"type\" : \"size\""
                           << ", \"cid\" : " << topCascade->m_id
                           << ", \"n_tot\": \"" << topCascade->m_pairsOfTimesAndMagnitudes.size() << '"'
                           << ", \"t_end\" : " << topCascade->m_timeOfLastTweet
                           << '}';
                        // Add porperties to the whole message and pop the last element of the queue
                        std::string msg_properties = os.str();
                        propertiesToSend.push_back(msg_properties);
                    }
                    this->m_priorityQueue.pop();
                    // if priority queue is not empty, one affects another one cascade
                    // which is at the top of que priority queue
                    if (!(this->m_priorityQueue.empty()))
                    {
                        topCascade = m_priorityQueue.top();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            return propertiesToSend;
        }

    } //End of namespace cascade

}
