# Generated by Django 4.1 on 2024-04-05 08:38

from django.db import migrations, models


class Migration(migrations.Migration):
    dependencies = [
        ("payment", "0003_alter_transaction_start_at"),
    ]

    operations = [
        migrations.AddField(
            model_name="transaction",
            name="approved",
            field=models.BooleanField(default=False),
        ),
    ]
