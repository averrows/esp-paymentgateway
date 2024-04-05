from django.contrib import admin
from django.apps import apps

# Register your models here.
admin.site.register(apps.all_models['payment'].values())
