from django.apps import AppConfig


class PaymentConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'
    name = 'payment'
    
    def ready(self) -> None:
        from . import mqtt
        mqtt_client = mqtt.client
        mqtt_client.loop_start()
