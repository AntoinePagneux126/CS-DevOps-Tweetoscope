"""
The aim of this code is predict the number of retweet thanks to the estimated 
parameters.
"""


import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaConsumer   # Import Kafka consumer
from kafka import KafkaProducer   # Import Kafka producer
import Predictor_tools as prd

if __name__=="__main__": 
    ################################################
    #######         Kafka Part              ########
    ################################################

    topic_reading="cascade_properties"
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
    #####         Prediction Part              #####
    ################################################
    for msg in consumer : 
      msg=msg.value # which will be remplaced by our object in a near future 
      my_params=[msg["p"],msg["beta"]]
      cid=msg["cid"]
      # modifier predictions afin d'avoir G1 en valeur de sortire aussi 
      N,G1= prd.predictions(params=my_params, history = msg["tweets"], alpha=2.016,mu=1)

      send= {
        'type': 'sample',
        'cid': cid,
        'params': my_params,
        'X': [msg["beta"],N,G1],
        'W' : 1,
        }

    producer.send(topic_writing, key =msg["T_obs"], value = send)