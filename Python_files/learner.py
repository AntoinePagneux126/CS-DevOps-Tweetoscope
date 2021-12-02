"""
The aim of this code is to estimate the cascade's parameters. 
"""


import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaConsumer   # Import Kafka consumer
from kafka import KafkaProducer   # Import Kafka producer
import pandas as pd
import time
import pickle
from sklearn.ensemble import RandomForestRegressor

import logger


if __name__ == "__main__":


    ################################################
    #######         Kafka Part              ########
    ################################################

    topic_reading = "samples"
    topic_writing = "models"

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

    producer = KafkaProducer(
        # List of brokers passed from the command line
        bootstrap_servers=args.broker_list,
        # How to serialize the value to a binary buffer
        value_serializer=lambda v: pickle.dumps(v).encode('utf-8'),
        # How to serialize the key
        key_serializer=str.encode
    )
    producer_log = KafkaProducer(
      bootstrap_servers = args.broker_list,                     # List of brokers passed from the command line
      value_serializer=lambda v: json.dumps(v).encode('utf-8'), # How to serialize the value to a binary buffer
    )

    ################################################
    #######         Stats part              ########
    ################################################

    df = pd.DataFrame(columns=["T_obs", "X", "W"])
    models_dict = {"600": RandomForestRegressor(
    ), "1200": RandomForestRegressor(), "Others": RandomForestRegressor()}
    threshold = {"600": 100, "1200": 200, "Others": 100}
    for msg in consumer:
        cid = msg.value["cid"]
        T_obs = msg.value["T_obs"]
        X = msg.value["X"]
        W = msg.value["W"]
        df.append(T_obs, X, W)

        models_dict[T_obs].fit(
            df[df["T_obs" == T_obs]]["X"], df[df["T_obs" == T_obs]]["W"])
        send = {
            'type': 'parameters',
            # 'n_obs' : msg.value["T_obs"],
            'model': pickle.dumps(models_dict[T_obs])
        }

        producer.send(topic_writing, key=msg.value['T_obs'], value=send)
        threshold[T_obs] += 100  # restarting counter

        msg_log={
        't': round(time.time(),3),
        'level' : "DEBUG",
        'source' : "learner",
        'message': f"sended message",
    }
        producer_log.send("logs", value=msg_log)
        producer.flush()
