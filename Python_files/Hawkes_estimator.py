"""
The aim of this code is to estimate the cascade's parameters. 
"""



import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaConsumer   # Import Kafka consumer
from kafka import KafkaProducer   # Import Kafka producer
import os
import numpy as np

import hawkes_tools as HT
import logger



if __name__=="__main__" : 

    logger = logger.get_logger('estimator', broker_list="localhost::9092",debug=True)
    
    ################################################
    #######         Kafka Part              ########
    ################################################

    topic_reading="cascade_series"
    topic_writing="cascade_properties"


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

    # Constants given by Mishra et al 
    mu,alpha=1 , 2.016
    logger.info("Start reading in cascade serie topic...")
    # for i in range(0,10): 
    #     cascade=np.load(f"Python_files/Cascades/test_cascade_{i}.npy")

    #     dico= {
    #       "cid": i ,
    #       "tweets" : cascade,
    #       "T_obs" : cascade[-1,0],
    #     }

    for msg in consumer : 
        # I'll construct a cascade object thanks to msg
        cid=msg.value["cid"]
        logger.info(f"Map computation for {cid} ...")
        MAP_res=HT.compute_MAP(history=msg.value['tweets'],t=msg.value['T_obs'],alpha=alpha, mu=mu)
        p,beta=MAP_res[-1]
        my_params=np.array([p,beta])

        send ={
            'type': 'parameters',
            'n_obs' : msg.value["T_obs"],
            'n_supp' : 0,## error in the subject
            'params' : my_params,
        }
        logger.info(f"Sending estimated parameter for {cid}...")
        producer.send(topic_writing, key = msg.value['T_obs'], value = send)


