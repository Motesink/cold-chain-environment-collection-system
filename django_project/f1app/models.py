# Create your models here.
# from django.db import models
#
# class sensor_data(models.Model):
#     id = models.AutoField(primary_key=True, auto_created=True, blank=False)
#     data = models.FloatField(blank=True)
# class latest_sensor_data(models.Model):
#     id = models.AutoField(primary_key=True, auto_created=True, blank=False)
#     data = models.FloatField(blank=True)

from django.db import models

class SensorData(models.Model):
    id = models.AutoField(primary_key=True, auto_created=True, blank=False)
    pin_id = models.CharField(max_length=4)
    node_id = models.CharField(max_length=12)
    ds18b20_temp = models.FloatField()
    dht11_temp = models.IntegerField()
    dht11_humi = models.IntegerField()
    time_day = models.CharField(max_length=12)
    time = models.CharField(max_length=12)

class Test1(models.Model):
    id = models.AutoField(primary_key=True, auto_created=True, blank=False)
    pin_id = models.CharField(max_length=4)
    node_id = models.CharField(max_length=12)
    ds18b20_temp = models.FloatField()
    dht11_temp = models.IntegerField()
    dht11_humi = models.IntegerField()
    time_day = models.CharField(max_length=12)
    time = models.CharField(max_length=12)
