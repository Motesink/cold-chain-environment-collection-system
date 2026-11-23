from django.shortcuts import render
from django.http import JsonResponse

# Create your views here.

from f1app.models import Test1  # 确保f1app已注册到INSTALLED_APPS

# def get_test1_data(request):
#     # 查询所有数据并转换为字典
#     data = list(Test1.objects.values())
#     return JsonResponse({"test1_data": data})

# def get_test1_data(request):
#     latest_data = Test1.objects.order_by('-time_day', '-time')[:10]
#     data = [
#         {
#             'id': item.id,
#             'pin_id': item.pin_id,
#             'node_id': item.node_id,
#             'ds18b20_temp': float(item.ds18b20_temp),
#             'dht11_temp': int(item.dht11_temp),
#             'dht11_humi': int(item.dht11_humi),
#             'time_day': item.time_day,
#             'time': item.time
#         }
#         for item in latest_data
#     ]
#     return JsonResponse({'test1_data': data})

def get_test1_data(request):
    latest_data = Test1.objects.order_by('-time_day', '-time').first()
    data = {
        'id': latest_data.id,
        'pin_id': latest_data.pin_id,
        'node_id': latest_data.node_id,
        'ds18b20_temp': float(latest_data.ds18b20_temp),
        'dht11_temp': int(latest_data.dht11_temp),
        'dht11_humi': int(latest_data.dht11_humi),
        'time_day': latest_data.time_day,
        'time': latest_data.time
    } if latest_data else {}
    return JsonResponse({'latest_data': data})


def get_test1_alldata(request):
    # 获取所有数据并按时间倒序排列
    all_data = Test1.objects.order_by('-time_day', '-time')

    # 构建包含所有数据的列表
    data_list = []
    for item in all_data:
        data_list.append({
            'id': item.id,
            'pin_id': item.pin_id,
            'node_id': item.node_id,
            'ds18b20_temp': float(item.ds18b20_temp),
            'dht11_temp': int(item.dht11_temp),
            'dht11_humi': int(item.dht11_humi),
            'time_day': item.time_day,
            'time': item.time
        })

    return JsonResponse({'data': data_list})


def get_test1_onedaydata(request):
    # 从URL参数获取time_day
    time_day = request.GET.get('time_day')

    # 根据参数存在与否构建查询集
    if time_day:
        # 验证日期格式
        try:
            # 确保日期格式正确
            filtered_data = Test1.objects.filter(time_day=time_day).order_by('-time')
        except ValueError:
            return JsonResponse({'error': 'Invalid date format'}, status=400)
    else:
        filtered_data = Test1.objects.all().order_by('-time_day', '-time')

    # 构建数据列表
    data_list = []
    for item in filtered_data:
        data_list.append({
            'id': item.id,
            'pin_id': item.pin_id,
            'node_id': item.node_id,
            'ds18b20_temp': float(item.ds18b20_temp),
            'dht11_temp': int(item.dht11_temp),
            'dht11_humi': int(item.dht11_humi),
            'time_day': item.time_day,
            'time': item.time
        })

    return JsonResponse({'data': data_list})

def index(request):
    if request.method =='GET':
        return render(request, 'index.html')