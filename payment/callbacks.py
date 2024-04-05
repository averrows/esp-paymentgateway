from paho.mqtt.client import MQTTMessage, Client
import json
from .models import Transaction, User
import datetime
import random

APPROVAL_TIMEOUT = 10
INITIAL_RESPONSE_TIMEOUT = 10

def payment_callback(user_id: str, mqtt_client: Client, userdata, msg: MQTTMessage):    
    print(f'[PAYMENT] Starting transaction for user: {user_id} with payload: {json.loads(msg.payload)}')
    
    
    payload = json.loads(msg.payload)
    
    # timestamp = datetime.datetime.fromtimestamp(payload['timestamp'])
    # if (datetime.datetime.now() - timestamp).seconds > INITIAL_RESPONSE_TIMEOUT:
    #     # no need to respond
    #     # the client has already timed out
    #     return
    
    try:
        amount = int(payload['amount'])
    except ValueError:
        print(f'[PAYMENT] Amount is not a valid number')
        mqtt_client.publish(f'payment/{user_id}/status', json.dumps({'transaction_status': 'failed'}))
        return
    mqtt_client.publish(f'payment/{user_id}/status', json.dumps({'transaction_status': 'onprogress'}))
    
    if amount < 0:
        mqtt_client.publish(f'payment/{user_id}/status', json.dumps({'transaction_status': 'failed'}))
        return
    
    # payment will fail 10% of the time due to random cause
    random_number = random.random()
    if random_number < 0.1:
        mqtt_client.publish(f'payment/{user_id}/status', json.dumps({'transaction_status': 'failed'}))
        print(f'[PAYMENT] Transaction failed for user {user_id}')
        return
    
    if amount > User.objects.get(user_id=user_id).balance:
        mqtt_client.publish(f'payment/{user_id}/status', json.dumps({'transaction_status': 'failed'}))
        print(f'[PAYMENT] User {user_id} does not have enough balance to make the transaction')
        return
    
    transaction = Transaction(user=User.objects.get(user_id=user_id), amount=-amount)
    transaction.save()
    
    mqtt_client.publish(f'payment/{user_id}/status', json.dumps({'transaction_status': 'finished', 'transaction_id': transaction.transaction_id}))
    print(f'[PAYMENT] Asking for approval for transaction {transaction.transaction_id} for user {user_id}')
    
def payment_approval_callback(user_id: str, mqtt_client: Client, userdata, msg: MQTTMessage):
    print(f'[PAYMENT APPROVAL] Execute the transaction for user: {user_id} with payload: {json.loads(msg.payload)}')
    
    payload = json.loads(msg.payload)
    transaction_id = payload['transaction_id']
    approval = payload['approval']
    
    transaction = Transaction.objects.get(transaction_id=transaction_id)
    
    if approval:
        # the timezone is still an issue
        now = datetime.datetime.now().astimezone()
        # check timeout
        if (now - transaction.start_at).seconds > 10:
            transaction.end_at = now
            transaction.save()
            print(f'[PAYMENT APPROVAL] Transaction {transaction_id} for user {user_id} is expired')
            return
        transaction.end_at = now
        transaction.save()
        
        user = User.objects.get(user_id=user_id)
        user.balance += transaction.amount
        user.save()
        print(f'[PAYMENT APPROVAL] Transaction {transaction_id} for user {user_id} is approved')
        print(f'[PAYMENT APPROVAL] User {user_id} balance is now {user.balance}')

def register_callback(mqtt_client: Client, userdata, msg: MQTTMessage):
    msg = json.loads(msg.payload)
    try:
        device_id = msg['device_id']
    except KeyError:
        print(f'[REGISTER] Device ID is not provided')
        return
    # find the user
    try:
        user = User.objects.get(device_id=device_id)
    except User.DoesNotExist:
        user = None
    if user:
        mqtt_client.publish(f'register/status/{device_id}', json.dumps({'user_id': user.user_id}))
        print(f'[REGISTER] Device {device_id} has been registered  to {user.user_id} with balance {user.balance}')
        return
    user = User.objects.create(balance=200000, device_id=device_id)
    mqtt_client.publish(f'register/status/{device_id}', json.dumps({'user_id': user.user_id}))
    print(f'[REGISTER] Successfully registering {device_id} to {user.user_id} with balance {user.balance}')
    
    

    