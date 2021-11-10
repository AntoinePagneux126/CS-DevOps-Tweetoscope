  """
The aim of this code is to estimate the cascade's parameters. 
  """



import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaConsumer   # Import Kafka consumer
from kafka import KafkaProducer   # Import Kafka producer
import os

import Hawkes_tools as Haw

if __name__=="__main__" : 

    ################################################
    #######         Kafka Part              ########
    ################################################

    topic_reading="cascade_series"
    topic_writing="cascade_properties"


    ## we have to put in the terminal the port 
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--broker-list', type=str, required=True, help="the broker list")
    args = parser.parse_args()  # Parse arguments

    consumer = KafkaConsumer(topic_reading,                   # Topic name
      bootstrap_servers = args.broker_list,                        # List of brokers passed from the command line
      value_deserializer=lambda v: json.loads(v.decode('utf-8')),  # How to deserialize the value from a binary buffer
      key_deserializer= lambda v: v.decode()                       # How to deserialize the key (if any)
    )

    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--broker-list', type=str, required=True, help="the broker list")
    args = parser.parse_args()  # Parse arguments

    producer = KafkaProducer(
      bootstrap_servers = args.broker_list,                     # List of brokers passed from the command line
      value_serializer=lambda v: json.dumps(v).encode('utf-8'), # How to serialize the value to a binary buffer
      key_serializer=str.encode                                 # How to serialize the key
    )

    ################################################
    #######         Stats part              ########
    ################################################

    # Constants given by Mishra et al 
    mu,alpha=1 , 2.016
    
    for msg in consumer : 
      MAP_res=Haw.compute_MAP(history=msg['tweets'],t=msg['T_obs'],alpha=alpha, mu=mu)
      p,beta=MAP_res[-1]

    producer.send(topic_writing, key = msg['dst'], value = msg)


