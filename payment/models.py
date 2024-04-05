from django.db import models

class User(models.Model):
    user_id = models.AutoField(primary_key=True)
    balance = models.FloatField()
    device_id = models.IntegerField(default=0)
    
class Transaction(models.Model):
    transaction_id = models.AutoField(primary_key=True)
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    amount = models.FloatField()
    start_at = models.DateTimeField(auto_now_add=True)
    approved_at = models.DateTimeField(auto_now=False, null=True)
    finished = models.BooleanField(default=False)