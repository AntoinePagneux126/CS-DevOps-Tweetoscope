"""
The aim of this code is predict the number of retweet thanks to the estimated 
parameters.
"""


import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaConsumer   # Import Kafka consumer
from kafka import KafkaProducer   # Import Kafka producer
import pickle
import numpy as np 
import time

import predictor_tools as prd

if __name__ == "__main__":

    ################################################
    #######         Kafka Part              ########
    ################################################

    topic_reading = "cascadeproperties"
    topic_reading_2 = "models"
    topic_writing_sample = "samples"
    topic_writing_alert = "alerts"
    topic_writing_stats = "stats"


    parser = argparse.ArgumentParser()
    parser.add_argument('--broker-list', type=str,
                        help="the broker list", default="kafka-service:9092")
    args = parser.parse_args()  # Parse arguments

    consumer = KafkaConsumer(topic_reading,                   # Topic name
                             # List of brokers passed from the command line
                             bootstrap_servers=args.broker_list,
                             # How to deserialize the value from a binary buffer
                             value_deserializer=lambda v: json.loads(
                                 v.decode('utf-8')),
                             # How to deserialize the key (if any)
                             key_deserializer=lambda v: v.decode()
                             )
    consumer_model = KafkaConsumer(topic_reading_2,                   # Topic name
                                   # List of brokers passed from the command line
                                   bootstrap_servers=args.broker_list,
                                   # How to deserialize the value from a binary buffer
                                   value_deserializer=lambda v: pickle.loads(
                                       v.decode('utf-8')),
                                   # How to deserialize the key (if any)
                                   key_deserializer=lambda v: v.decode()
                                   )

    producer = KafkaProducer(
        # List of brokers passed from the command line
        bootstrap_servers=args.broker_list,
        # How to serialize the value to a binary buffer
        value_serializer=lambda v: json.dumps(v).encode('utf-8'),
        # How to serialize the key
        key_serializer=str.encode
    )
    producer_log = KafkaProducer(
      bootstrap_servers = args.broker_list,                     # List of brokers passed from the command line
      value_serializer=lambda v: json.dumps(v).encode('utf-8'), # How to serialize the value to a binary buffer
    )

    ################################################
    #####         Prediction Part              #####
    ################################################
    for msg in consumer:            
        msg = msg.value 
        my_params = msg["params"]
        cid = msg["cid"]

        N, N_star, G1 = prd.predictions(
            params=np.array(my_params), history=msg["tweets"], alpha=2.016, mu=1)
        
        send_sample = {
            'type': 'sample',
            'cid': cid,
            'params': my_params,
            'X': [msg["beta"], N_star, G1],
            # based on predicted result
            'W': (msg["n_supp"] - msg["n_obs"]) * (1 - N_star) / G1,
        }
        producer.send(topic_writing_sample,
                      key=msg["T_obs"], value=send_sample)
        
        for msg_model in consumer_model:
            if msg_model.key == msg["T_obs"] and msg_model[msg["T_obs"]] != None:
                model = msg_model[msg["T_obs"]]
        omega = model.predict([msg["beta"], N_star, G1])
        N_forest = msg["n_obs"] + omega * (1 - N_star) / G1
        error = abs(N-N_forest)/N_forest

        send_alert = {
            'type': 'alert',
            'to display': 'very hot topic, follow up with it',
            'cid': cid,
            'n_tot': N,
        }

        producer.send(topic_writing_alert, key=msg["T_obs"], value=send_alert)

        
        send_stats = {
            'type': 'stats',
            'cid': cid,
            'T_obs': msg["T_obs"],
            'ARE': error,
        }
        msg_log={
            't': round(time.time(),3),
            'level' : "DEBUG",
            'source' : "predictor",
            'message': f"sended messages for {cid}",
        }
        producer_log.send("logs",value=msg_log)
        producer.send(topic_writing_stats, key=None, value=send_stats)
    producer.flush()
    producer_log.flush()
