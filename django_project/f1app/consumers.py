# f1app/consumers.py
import json
import asyncio
from channels.generic.websocket import AsyncWebsocketConsumer
from channels.db import database_sync_to_async
from .models import Test1
import time


class DataConsumer(AsyncWebsocketConsumer):
    async def connect(self):
        # 直接使用固定组名，不需要 room_name
        self.room_group_name = 'data_group'

        # 加入组
        await self.channel_layer.group_add(
            self.room_group_name,
            self.channel_name
        )

        # 接受连接
        await self.accept()

        # 发送连接成功消息
        await self.send(text_data=json.dumps({
            'type': 'connection_established',
            'message': 'WebSocket连接已建立'
        }))

        # 发送默认历史数据（今天的数据）
        await self.send_historical_data()

    async def disconnect(self, close_code):
        await self.channel_layer.group_discard(
            self.room_group_name,
            self.channel_name
        )

    async def receive(self, text_data):
        """接收客户端消息"""
        try:
            data = json.loads(text_data)
            message_type = data.get('type', '')

            if message_type == 'ping':
                await self.send_pong()
            elif message_type == 'subscribe':
                await self.send_subscription_confirmation(data)
            elif message_type == 'unsubscribe':
                await self.send_unsubscription_confirmation(data)
            elif message_type == 'request_date_data':
                # 处理请求特定日期的数据
                date_str = data.get('date', '')
                await self.send_date_data(date_str)

        except json.JSONDecodeError:
            await self.send_error('无效的JSON格式')

    async def send_data(self, event):
        """发送传感器数据到WebSocket"""
        sensor_data = event.get('data', {})

        await self.send(text_data=json.dumps({
            "type": "sensor_data",
            "data": sensor_data,
            "timestamp": sensor_data.get('timestamp')
        }))

    async def send_pong(self):
        """响应心跳"""
        await self.send(json.dumps({
            'type': 'pong',
            'timestamp': int(time.time() * 1000)
        }))

    async def send_subscription_confirmation(self, data):
        """确认订阅"""
        subscription_type = data.get('subscription_type', 'all')
        await self.send(json.dumps({
            'type': 'subscription_confirmation',
            'subscription_type': subscription_type,
            'message': f'已订阅 {subscription_type} 数据'
        }))

    async def send_unsubscription_confirmation(self, data):
        """确认取消订阅"""
        subscription_type = data.get('subscription_type', 'all')
        await self.send(json.dumps({
            'type': 'unsubscription_confirmation',
            'subscription_type': subscription_type,
            'message': f'已取消订阅 {subscription_type} 数据'
        }))

    async def send_error(self, message):
        """发送错误信息"""
        await self.send(json.dumps({
            'type': 'error',
            'message': message
        }))

    async def send_historical_data(self, date_str=None):
        """发送历史数据"""
        try:
            historical_data = await self.get_historical_data(date_str)
            await self.send(text_data=json.dumps({
                'type': 'historical_data',
                'data': historical_data,
                'count': len(historical_data),
                'date': date_str if date_str else 'today'
            }))
        except Exception as e:
            print(f"获取历史数据失败: {e}")

    async def send_date_data(self, date_str):
        """发送特定日期的数据"""
        try:
            historical_data = await self.get_historical_data(date_str)
            await self.send(text_data=json.dumps({
                'type': 'date_data',
                'data': historical_data,
                'count': len(historical_data),
                'date': date_str
            }))
        except Exception as e:
            print(f"获取日期数据失败: {e}")

    @database_sync_to_async
    def get_historical_data(self, date_str=None):
        """从数据库获取历史数据"""
        if date_str:
            # 如果有日期参数，获取该日期的所有数据
            # date_str格式可能是 "2026-01-04" 或 "2026年01月04日"
            try:
                # 尝试不同的日期格式
                if '-' in date_str:
                    # 格式 "2026-01-04" 转换为 "2026年01月04日"
                    parts = date_str.split('-')
                    if len(parts) == 3:
                        chinese_date = f"{parts[0]}年{parts[1]}月{parts[2]}日"
                    else:
                        chinese_date = date_str
                else:
                    chinese_date = date_str

                # 按时间正序排列，获取该日期的所有数据
                data = Test1.objects.filter(time_day=chinese_date).order_by('time')
                return [
                    {
                        'pin_id': item.pin_id,
                        'node_id': item.node_id,
                        'ds18b20_temp': item.ds18b20_temp,
                        'dht11_temp': item.dht11_temp,
                        'dht11_humidity': item.dht11_humi,
                        'date': item.time_day,
                        'time': item.time,
                    }
                    for item in data
                ]
            except Exception as e:
                print(f"按日期查询失败: {e}, date_str: {date_str}")
                # 如果按日期查询失败，返回空数组
                return []
        else:
            # 没有日期参数，获取最近50条数据
            data = Test1.objects.order_by('-id')[:50]
            return [
                {
                    'pin_id': item.pin_id,
                    'node_id': item.node_id,
                    'ds18b20_temp': item.ds18b20_temp,
                    'dht11_temp': item.dht11_temp,
                    'dht11_humidity': item.dht11_humi,
                    'date': item.time_day,
                    'time': item.time,
                }
                for item in data
            ]