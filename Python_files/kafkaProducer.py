import argparse                   # To parse command line arguments
import json                       # To parse and dump JSON
from kafka import KafkaProducer   # Import Kafka producder


import time

topic_writing="tweets"

parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
parser.add_argument('--broker-list', type=str, required=True, help="the broker list")
args = parser.parse_args()  # Parse arguments

producer = KafkaProducer(
  bootstrap_servers = args.broker_list,                     # List of brokers passed from the command line
  value_serializer=lambda v: json.dumps(v).encode('utf-8'), # How to serialize the value to a binary buffer
  key_serializer=str.encode                                 # How to serialize the key
)

msg = {
    'dst': 'Metz',
    'temp': 2,
    'type': 'rain',
    'comment': 'Nothing special'
}
for _ in range(5000):
  print("Message : ",_, ", temps(s) : ",_*5)
  producer.send(topic_writing, key = msg['dst'], value = msg) # Send a new message to topic
  time.sleep(5)
    
producer.flush() # Flush: force purging intermediate buffers before leaving
