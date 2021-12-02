"""
The aim of this code is to estimate the cascade's parameters.
"""



import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaConsumer   # Import Kafka consumer
from kafka import KafkaProducer   # Import Kafka producer
import numpy as np
import time

import hawkes_tools as HT
import logger



if __name__=="__main__" :
    
    ################################################
    #######         Kafka Part              ########
    ################################################
    topic_reading="cascadeseries"
    topic_writing="cascadeproperties"


    ## default value without typing anything in the terminal
    parser = argparse.ArgumentParser()
    parser.add_argument('--broker-list', type=str, help="the broker list", default="kafka-service:9092")
    args = parser.parse_args() # Parse arguments


    consumer = KafkaConsumer(topic_reading,                   # Topic name
      bootstrap_servers = args.broker_list,                      # List of brokers passed from the command line
      value_deserializer=lambda v: json.loads(v.decode('utf-8')))  # How to deserialize the value from a binary buffer
    
    print("consumer created")

    producer = KafkaProducer(
      bootstrap_servers = args.broker_list,                     # List of brokers passed from the command line
      value_serializer=lambda v: json.dumps(v).encode('utf-8'), # How to serialize the value to a binary buffer
      key_serializer=str.encode                                 # How to serialize the key
    )
    producer_log = KafkaProducer(
      bootstrap_servers = args.broker_list,                     # List of brokers passed from the command line
      value_serializer=lambda v: json.dumps(v).encode('utf-8'), # How to serialize the value to a binary buffer
    )
    print("producers created")

    ################################################
    #######         Stats part              ########
    ################################################

    # Constants given by Mishra et al 
    mu,alpha=1 , 2.016

    for msg in consumer : 
        cid=msg.value["cid"]
        history=np.array(msg.value['tweets'])
        
        starting_date=history[0,0]## origine date
        for i in range(len(history[:,0])):
          history[i,0]-=starting_date
        print(history)
        MAP_res=HT.compute_MAP(history=history,t=float(history[-1,0]),alpha=alpha, mu=mu)
        p,beta=MAP_res[-1]
        my_params=[p,beta]

        send ={
            'type': 'parameters',
            'n_obs' : msg.value["T_obs"],
            'n_tot' : 0,## sended by Matthieu and Antoine once the cascade is ended
            'params' : my_params,
            'cid': cid,
            'tweets':history.tolist(),
        }
        producer.send(topic_writing, key = str(msg.value['T_obs']), value = send)
        msg_log={
            't': round(time.time(),3),
            'level' : "DEBUG",
            'source' : "estimator",
            'message': f"sended messages for {cid}",
        }
        producer_log.send("logs",value=msg_log)
    producer_log.flush()
    producer.flush()


