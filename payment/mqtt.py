import paho.mqtt.client as mqtt
from paho.mqtt.client import MQTTMessage, Client
from .callbacks import payment_callback, payment_approval_callback, register_callback
import json
from django.conf import settings

PAYMENT_TOPIC = 'payment/+'
PAYMENT_APPROVAL_TOPIC = 'payment/+/approval'

global is_callback_onrun
is_callback_onrun = False

def on_connect(mqtt_client, userdata, flags, rc):
    if rc == 0:
        print('[MQTT] Connected successfully to the mqtt broker')
        mqtt_client.subscribe(PAYMENT_TOPIC)
        print('[MQTT] Successfully subscribed to : ', PAYMENT_TOPIC)
        mqtt_client.subscribe(PAYMENT_APPROVAL_TOPIC)
        print('[MQTT] Successfully subscribed to : ', PAYMENT_APPROVAL_TOPIC)
        mqtt_client.subscribe("register")
        print('[MQTT] Successfully subscribed to : ', "register")
    else:
        print('[MQTT][ERROR] Bad connection. Code:', rc)


def on_message(mqtt_client: Client, userdata, msg: MQTTMessage):
    global is_callback_onrun, counter
    if is_callback_onrun:
        return
    is_callback_onrun = True
    print(f'[MQTT] Received message on topic {msg.topic} with payload {json.loads(msg.payload)}')
    topic_parts = msg.topic.split('/')
    if topic_parts[0] == 'payment' and len(topic_parts) == 2:
        user_id = topic_parts[1]
        payment_callback(user_id, mqtt_client, userdata, msg)
    elif topic_parts[0] == 'payment' and topic_parts[2] == 'approval':
        user_id = topic_parts[1]
        payment_approval_callback(user_id, mqtt_client, userdata, msg)
    elif topic_parts[0] == "register":
        register_callback(mqtt_client, userdata, msg)
    is_callback_onrun = False
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
# client.username_pw_set(settings.MQTT_USER, settings.MQTT_PASSWORD)
client.connect(
    host=settings.MQTT_SERVER,
    port=settings.MQTT_PORT,
    keepalive=settings.MQTT_KEEPALIVE
)