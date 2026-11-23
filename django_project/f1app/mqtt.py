# import paho.mqtt.client as mqtt
# import django_project.settings as settings
# import json
# from f1app.models import sensor_data
#
#
# def on_connect(mqtt_client, userdata, flags, rc):
#     if rc == 0:
#         print('Connected to MQTT broker')
#         mqtt_client.subscribe('iotmsg/#')
#     else:
#         print('Connection failed with code', rc)
#
#
# # def on_message(mqtt_client, userdata, msg):
# #     try:
# #         # 完整解析JSON数据
# #         payload = json.loads(msg.payload.decode('utf-8'))
# #         print("Received MQTT message:")
# #         print(json.dumps(payload, indent=2))  # 格式化输出JSON
# #
# #         # 提取各字段数据
# #         pin_id = payload.get('PIN_ID', '')
# #         device_id = payload.get('ID', '')
# #         devices = payload.get('Devices', {})
# #
# #         # 创建数据库记录
# #         sensor_data.objects.create(
# #             pin_id=pin_id,
# #             device_id=device_id,
# #             ds18b20_temp=devices.get('DS18B20-Temp'),
# #             dht11_temp=devices.get('DHT11-Temp'),
# #             dht11_humidity=devices.get('DHT11-Humidity'),
# #             timestamp=devices.get('Time')
# #         )
# #
# #     except json.JSONDecodeError:
# #         print("Error: Invalid JSON format")
# #     except Exception as e:
# #         print(f"Unexpected error: {str(e)}")
#
# def on_message(mqtt_client, userdata, msg):
#     try:
#         payload = json.loads(msg.payload.decode('utf-8'))
#         print("Received MQTT message:")
#         print(json.dumps(payload, indent=2))  # 格式化输出JSON
#         devices = payload.get('Devices', {})
#
#         # 类型转换处理
#         dht11_temp = float(devices.get('DHT11-Temp', 0)) if devices.get('DHT11-Temp') else None
#         dht11_humidity = int(devices.get('DHT11-Humidity', 0)) if devices.get('DHT11-Humidity') else None
#
#         # 创建数据库记录
#         sensor_data.objects.create(
#             pin_id=payload.get('PIN_ID', ''),
#             device_id=payload.get('ID', ''),
#             ds18b20_temp=devices.get('DS18B20-Temp'),
#             dht11_temp=dht11_temp,
#             dht11_humidity=dht11_humidity,
#             timestamp=devices.get('Time', '')
#         )
#
#     except json.JSONDecodeError:
#         print("Error: Invalid JSON format")
#     except Exception as e:
#         print(f"Unexpected error: {str(e)}")
#
# client = mqtt.Client()
# client.on_connect = on_connect
# client.on_message = on_message
# client.username_pw_set(settings.MQTT_USER, settings.MQTT_PASSWORD)
# client.connect(host=settings.MQTT_SERVER, port=settings.MQTT_PORT,keepalive=settings.MQTT_KEEPALIVE)
#
# client.loop_start()

# import paho.mqtt.client as mqtt
# import json
# from django.conf import settings
# from openpyxl.utils.datetime import time_to_days
#
# from f1app.models import Test1
# from datetime import datetime
#
# from channels.layers import get_channel_layer
# from asgiref.sync import async_to_sync
#
#
# def on_connect(mqtt_client, userdata, flags, rc):
#     if rc == 0:
#         print('Connected to MQTT broker')
#         mqtt_client.subscribe('iotmsg/#')
#     else:
#         print(f'Connection failed with code {rc}')
#
# def on_message(mqtt_client, userdata, msg):
#     # payload = json.loads(msg.payload.decode('utf-8'))
#     # print("Received MQTT message:")
#     # print(json.dumps(payload, indent=2))
#     payload = json.loads(msg.payload.decode('utf-8'))
#     print("Received MQTT message:")
#     print(json.dumps(payload, indent=2))
#
#
#     ss = str(msg.payload.decode("utf-8"))
#     print("接收到的消息:", ss)
#
#     try:
#         data = json.loads(ss)
#
#         # 从数据中提取字段
#         pin_id = data.get("PIN_ID")
#         node_id = data.get("ID")
#         n_ss = data.get("Devices")
#         ds18b20_temp = n_ss.get("DS18B20-Temp")
#         dht11_temp = n_ss.get("DHT11-Temp")
#         dht11_humi = n_ss.get("DHT11-Humidity")
#
#         # 获取当前日期时间对象
#         current_datetime = datetime.now()
#
#         # 分别存储日期和时间
#         current_date = current_datetime.strftime('%Y年%m月%d日')  # 日期部分
#         current_time = current_datetime.strftime('%H时%M分%S秒')  # 时间部分
#
#         # 验证输出
#         print(f"当前日期：{current_date}")
#         print(f"当前时间：{current_time}")
#
#         print("PIN_ID:", pin_id)
#         print("ID:", node_id)
#         print("DS18B20-Temp:", ds18b20_temp)
#         print("DHT11-Temp:", dht11_temp)
#         print("DHT11-Humidity:", dht11_humi)
#
#         Test1.objects.create(pin_id = pin_id,
#                              node_id = node_id,
#                              ds18b20_temp = ds18b20_temp,
#                              dht11_temp = int(dht11_temp),
#                              dht11_humi = int(dht11_humi),
#                              time_day = current_date,
#                              time = current_time)
#
#     except json.JSONDecodeError as e:
#         print("错误: 无法解析 JSON 数据", e)
#     except Exception as e:
#         print("错误: 无法处理消息", e)
#     try:
#         # 获取channel_layer
#         channel_layer = get_channel_layer()
#
#         # 准备要发送的数据
#         ws_data = {
#             "pin_id": pin_id,
#             "node_id": node_id,
#             "ds18b20_temp": ds18b20_temp,
#             "dht11_temp": dht11_temp,
#             "dht11_humidity": dht11_humi,
#             "timestamp": current_datetime.isoformat(),
#             "date": current_date,
#             "time": current_time,
#             "topic": msg.topic,
#             "message_type": "sensor_data"
#         }
#
#         # 方式1：发送到特定的组（适用于群组广播）
#         async_to_sync(channel_layer.group_send)(
#             'data_group',  # 组名，与WebSocket消费者的组名对应
#             {
#                 'type': 'send_data',  # 对应WebSocket消费者的方法名
#                 'data': ws_data,  # 发送的数据
#                 'source': 'mqtt'  # 数据来源
#             }
#         )
#         print("WebSocket message sent to group 'data_group'")
#         logger.info("WebSocket message sent to group 'data_group'")
#
#
#     except Exception as ws_error:
#         print(f"WebSocket send error: {ws_error}")
#         logger.error(f"WebSocket send error: {ws_error}")
#         # ============ WebSocket 推送部分结束 ============
#
#
#
#
# client = mqtt.Client()
# client.on_connect = on_connect
# client.on_message = on_message
# client.username_pw_set(settings.MQTT_USER, settings.MQTT_PASSWORD)
# client.connect(host=settings.MQTT_SERVER, port=settings.MQTT_PORT,keepalive=settings.MQTT_KEEPALIVE)
#
# client.loop_start()

import paho.mqtt.client as mqtt
import json
from django.conf import settings
from datetime import datetime
from channels.layers import get_channel_layer
from asgiref.sync import async_to_sync
from f1app.models import Test1
import logging

logger = logging.getLogger(__name__)


def on_connect(mqtt_client, userdata, flags, rc):
    if rc == 0:
        print('MQTT连接成功')
        mqtt_client.subscribe('iotmsg/#')
    else:
        print(f'MQTT连接失败: {rc}')


def on_message(mqtt_client, userdata, msg):
    try:
        # 解析消息
        payload = json.loads(msg.payload.decode('utf-8'))
        print(f"收到MQTT消息: {msg.topic}")

        # 提取数据
        pin_id = payload.get("PIN_ID")
        node_id = payload.get("ID")
        devices = payload.get("Devices", {})

        ds18b20_temp = devices.get("DS18B20-Temp")
        dht11_temp = devices.get("DHT11-Temp")
        dht11_humi = devices.get("DHT11-Humidity")

        # 获取当前时间
        current_datetime = datetime.now()
        current_date = current_datetime.strftime('%Y年%m月%d日')
        current_time = current_datetime.strftime('%H时%M分%S秒')

        # 验证输出
        print(f"当前日期：{current_date}")
        print(f"当前时间：{current_time}")

        print("PIN_ID:", pin_id)
        print("ID:", node_id)
        print("DS18B20-Temp:", ds18b20_temp)
        print("DHT11-Temp:", dht11_temp)
        print("DHT11-Humidity:", dht11_humi)

        # 保存到数据库
        Test1.objects.create(
            pin_id=pin_id,
            node_id=node_id,
            ds18b20_temp=ds18b20_temp,
            dht11_temp=int(dht11_temp) if dht11_temp else None,
            dht11_humi=int(dht11_humi) if dht11_humi else None,
            time_day=current_date,
            time=current_time
        )

        # 准备 WebSocket 数据
        ws_data = {
            "pin_id": pin_id,
            "node_id": node_id,
            "ds18b20_temp": ds18b20_temp,
            "dht11_temp": dht11_temp,
            "dht11_humidity": dht11_humi,
            "timestamp": current_datetime.isoformat(),
            "date": current_date,
            "time": current_time,
            "topic": msg.topic
        }

        # 发送到 WebSocket - 确保在 Django 已启动的情况下
        try:
            from channels.layers import get_channel_layer
            from asgiref.sync import async_to_sync

            channel_layer = get_channel_layer()
            if channel_layer:
                async_to_sync(channel_layer.group_send)(
                    'data_group',
                    {
                        'type': 'send_data',
                        'data': ws_data
                    }
                )
                print("WebSocket 消息发送成功")
            else:
                print("Channel layer 未初始化")
        except Exception as e:
            print(f"WebSocket 发送失败: {e}")

    except Exception as e:
        print(f"处理消息失败: {e}")


def send_to_websocket(data):
    """发送数据到WebSocket组"""
    try:
        channel_layer = get_channel_layer()
        async_to_sync(channel_layer.group_send)(
            'data_group',
            {
                'type': 'send_data',
                'data': data
            }
        )
        print("WebSocket消息发送成功")
    except Exception as e:
        print(f"WebSocket发送失败: {e}")


# MQTT客户端配置
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set(settings.MQTT_USER, settings.MQTT_PASSWORD)
client.connect(
    host=settings.MQTT_SERVER,
    port=settings.MQTT_PORT,
    keepalive=settings.MQTT_KEEPALIVE
)

client.loop_start()