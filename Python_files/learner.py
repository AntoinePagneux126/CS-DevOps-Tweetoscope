"""
The aim of this code is to estimate the cascade's parameters. 
"""



import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaConsumer   # Import Kafka consumer
from kafka import KafkaProducer   # Import Kafka producer
import os
import numpy as np
from sklearn.ensemble import RandomForestRegressor

import hawkes_tools as HT
import logger



if __name__=="__main__" : 

    logger = logger.get_logger('learner', broker_list="localhost::9092",debug=True)
    
    ################################################
    #######         Kafka Part              ########
    ################################################

    topic_reading="samples"
    topic_writing="models"


    ## default value without typing anything in the terminal
    parser = argparse.ArgumentParser()
    parser.add_argument('--broker-list', type=str, help="the broker list", default="localhost:9092")
    args = parser.parse_args()  # Parse arguments


    consumer = KafkaConsumer(topic_reading,                   # Topic name
      bootstrap_servers = args.broker_list,                        # List of brokers passed from the command line
      value_deserializer=lambda v: json.loads(v.decode('utf-8')),  # How to deserialize the value from a binary buffer
      key_deserializer= lambda v: v.decode()                       # How to deserialize the key (if any)
    )

    producer = KafkaProducer(
      bootstrap_servers = args.broker_list,                     # List of brokers passed from the command line
      value_serializer=lambda v: json.dumps(v).encode('utf-8'), # How to serialize the value to a binary buffer
      key_serializer=str.encode                                 # How to serialize the key
    )

    ################################################
    #######         Stats part              ########
    ################################################

    logger.info("Start reading in the samples topic...")


    for msg in consumer : 
        # I'll construct a cascade object thanks to msg
        cid=msg.value["cid"]
        X= msg.value["X"]
        # TODO data set en append le X et W et si longueur du dataset dépasse un seuil on train
        model = RandomForestRegressor.fit(X,msg.value["W"])

        send ={
            'type': 'parameters',
            'n_obs' : msg.value["T_obs"],
        }
        logger.info(f"Sending estimated parameter for {cid}...")
        producer.send(topic_writing, key = msg.value['T_obs'], value = send)


