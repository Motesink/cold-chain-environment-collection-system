import * as echarts from '../../components/ec-canvas/echarts';

// 修复日期格式处理
function formatDate(inputDate) {
  if (typeof inputDate === 'string' && inputDate.includes('年')) {
    const [year, month, day] = inputDate.match(/\d+/g);
    return `${year}年${month.padStart(2, '0')}月${day.padStart(2, '0')}日`;
  }
  
  const date = new Date(inputDate);
  if (isNaN(date.getTime())) return "1970年01月01日";
  
  const year = date.getFullYear();
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const day = String(date.getDate()).padStart(2, '0');
  return `${year}年${month}月${day}日`;
}

Page({
  data: {
    tempEc: null,
    humiEc: null,
    temperatureDataDS18B20: [], // DS18B20温度数据
    temperatureDataDHT11: [],    // DHT11温度数据
    humidityData: [],            // DHT11湿度数据
    timeData: [],
    isLoading: false,
    threshold: {
      tempMin: null,
      tempMax: null,
      humiMin: null,
      humiMax: null
    },
    selectedDate: '',
    endDate: '',
    chartReady: false
  },

  onLoad() {
    // 初始化日期
    const today = new Date();
    const year = today.getFullYear();
    const month = String(today.getMonth() + 1).padStart(2, '0');
    const day = String(today.getDate()).padStart(2, '0');
    
    const formattedToday = `${year}年${month}月${day}日`;
    
    this.setData({
      selectedDate: formattedToday,
      endDate: formattedToday
    }, () => {
      this.fetchData(formattedToday);
    });
  },

  onReady() {
    this.initECharts();
  },

  // 初始化echarts配置
  initECharts() {
    const initChart = (canvas, width, height) => {
      const chart = echarts.init(canvas, null, {
        width: width,
        height: height,
        devicePixelRatio: wx.getSystemInfoSync().pixelRatio
      });
      canvas.setChart(chart);
      return chart;
    };

    this.setData({
      tempEc: { onInit: initChart },
      humiEc: { onInit: initChart },
      chartReady: true
    });
  },

  fetchData(formattedDate) {
    wx.showLoading({ title: '加载中...' });
    
    wx.request({
      url: `http://192.168.198.1:8080/get_test1_onedaydata/?time_day=${formattedDate}`,
      success: res => {
        wx.hideLoading();
        
        if (res.data && res.data.data && res.data.data.length) {
          this.processData(res.data.data);
        } else {
          wx.showToast({ 
            title: '无数据', 
            icon: 'none',
            duration: 2000
          });
          this.setData({
            temperatureDataDS18B20: [],
            temperatureDataDHT11: [],
            humidityData: [],
            timeData: []
          }, () => {
            this.updateCharts();
          });
        }
      },
      fail: (err) => {
        wx.hideLoading();
        console.error('请求失败:', err);
        wx.showToast({ 
          title: '请求失败', 
          icon: 'none',
          duration: 2000
        });
      }
    });
  },

  processData(rawData) {
    console.log('原始数据长度:', rawData.length);
    
    // 复制数据并反转顺序（修复X轴时间顺序问题）
    const sortedData = [...rawData];
    
    // 按时间正序排序（从早到晚）
    sortedData.sort((a, b) => {
      // 将时间字符串转换为可比较的值
      const getTimeValue = (timeStr) => {
        const match = timeStr.match(/(\d+)时(\d+)分(\d+)秒/);
        if (match) {
          const hour = parseInt(match[1]);
          const minute = parseInt(match[2]);
          const second = parseInt(match[3]);
          return hour * 3600 + minute * 60 + second;
        }
        return 0;
      };
      
      return getTimeValue(a.time) - getTimeValue(b.time);
    });
    
    console.log('排序后的第一条数据时间:', sortedData[0]?.time);
    console.log('排序后的最后一条数据时间:', sortedData[sortedData.length - 1]?.time);
    
    const tempDataDS18B20 = [];
    const tempDataDHT11 = [];
    const humiData = [];
    const timeData = [];
    
    // 处理排序后的数据
    sortedData.forEach((item, index) => {
      const timeStr = item.time || `数据${index}`;
      
      timeData.push(timeStr);
      
      // DS18B20温度数据
      const tempValueDS18B20 = parseFloat(item.ds18b20_temp) || 0;
      tempDataDS18B20.push({
        value: tempValueDS18B20,
        time: timeStr,
        itemStyle: this.getItemStyle(tempValueDS18B20, 'temp')
      });
      
      // DHT11温度数据
      const tempValueDHT11 = parseFloat(item.dht11_temp) || 0;
      tempDataDHT11.push({
        value: tempValueDHT11,
        time: timeStr,
        itemStyle: this.getItemStyle(tempValueDHT11, 'temp')
      });
      
      // DHT11湿度数据
      const humiValue = parseFloat(item.dht11_humi) || 0;
      humiData.push({
        value: humiValue,
        time: timeStr,
        itemStyle: this.getItemStyle(humiValue, 'humi')
      });
    });
    
    console.log('时间数据顺序:', timeData);
    
    this.setData({
      temperatureDataDS18B20: tempDataDS18B20,
      temperatureDataDHT11: tempDataDHT11,
      humidityData: humiData,
      timeData: timeData
    }, () => {
      setTimeout(() => {
        this.updateCharts();
      }, 100);
    });
  },

  getItemStyle(value, type) {
    const threshold = this.data.threshold;
    
    if (type === 'temp') {
      if (threshold.tempMin !== null && value < threshold.tempMin) {
        return { color: '#ff0000' };
      }
      if (threshold.tempMax !== null && value > threshold.tempMax) {
        return { color: '#ff0000' };
      }
      return null;
    } else {
      if (threshold.humiMin !== null && value < threshold.humiMin) {
        return { color: '#ff0000' };
      }
      if (threshold.humiMax !== null && value > threshold.humiMax) {
        return { color: '#ff0000' };
      }
      return null;
    }
  },

  // 更新图表
  updateCharts() {
    if (!this.data.chartReady) return;
    
    const tempChartComponent = this.selectComponent('#temp-chart');
    const humiChartComponent = this.selectComponent('#humi-chart');
    
    if (tempChartComponent) {
      tempChartComponent.init((canvas, width, height) => {
        const tempChart = echarts.init(canvas, null, { width, height });
        this.setTempOption(tempChart);
        return tempChart;
      });
    }
    
    if (humiChartComponent) {
      humiChartComponent.init((canvas, width, height) => {
        const humiChart = echarts.init(canvas, null, { width, height });
        this.setHumiOption(humiChart);
        return humiChart;
      });
    }
  },

  // 设置温度图表选项 - 修复X轴顺序
  setTempOption(chart) {
    if (!this.data.temperatureDataDS18B20.length) return;
    
    const allTempValues = [
      ...this.data.temperatureDataDS18B20.map(item => item.value),
      ...this.data.temperatureDataDHT11.map(item => item.value)
    ];
    const minTemp = Math.min(...allTempValues);
    const maxTemp = Math.max(...allTempValues);
    
    const option = {
      title: { show: false },
      grid: {
        left: '12%',
        right: '8%',
        bottom: '18%',
        top: '8%',
        containLabel: true
      },
      tooltip: {
        trigger: 'axis',
        backgroundColor: 'rgba(50, 50, 50, 0.9)',
        borderColor: '#333',
        textStyle: { color: '#fff' },
        formatter: (params) => {
          const dataIndex = params[0].dataIndex;
          const timeStr = this.data.timeData[dataIndex] || `点${dataIndex}`;
          let result = `<div style="font-weight:bold;margin-bottom:5rpx;">${timeStr}</div>`;
          
          params.forEach(param => {
            const color = param.color;
            const marker = `<span style="display:inline-block;width:10rpx;height:10rpx;border-radius:50%;background:${color};margin-right:5rpx;"></span>`;
            result += `${marker}${param.seriesName}: <b>${param.value}℃</b><br/>`;
          });
          
          return result;
        }
      },
      legend: {
        data: ['DS18B20温度', 'DHT11温度'],
        bottom: '2%',
        left: 'center',
        itemWidth: 15,
        itemHeight: 10,
        textStyle: { fontSize: 10 }
      },
      xAxis: {
        type: 'category',
        data: this.data.timeData,
        name: '时间',
        nameLocation: 'middle',
        nameGap: 25,
        nameTextStyle: { fontSize: 11 },
        axisLabel: {
          interval: 0,
          rotate: 45,
          fontSize: 9,
          margin: 5,
          formatter: (value, index) => {
            // 每4个点显示一个标签，避免重叠
            if (index % 4 === 0 && value) {
              const match = value.match(/(\d+)时(\d+)分/);
              if (match) {
                const hour = match[1];
                const minute = match[2];
                return hour + ':' + minute;
              }
              return value.length > 6 ? value.substring(0, 5) : value;
            }
            return '';
          }
        },
        axisTick: { alignWithLabel: true, length: 3 },
        axisLine: { lineStyle: { color: '#ddd' } }
      },
      yAxis: {
        type: 'value',
        name: '温度(℃)',
        nameLocation: 'middle',
        nameGap: 35,
        nameTextStyle: { fontSize: 11 },
        min: Math.floor(minTemp) - 2,
        max: Math.ceil(maxTemp) + 2,
        axisLabel: { fontSize: 10 },
        splitLine: { lineStyle: { type: 'dashed', color: '#eee' } }
      },
      series: [
        {
          name: 'DS18B20温度',
          type: 'line',
          data: this.data.temperatureDataDS18B20.map(item => item.value),
          smooth: true,
          symbol: 'circle',
          symbolSize: 4,
          lineStyle: { width: 2, color: '#5470C6' },
          itemStyle: { color: '#5470C6', borderWidth: 1, borderColor: '#fff' },
          markLine: this.getThresholdLines('temp')
        },
        {
          name: 'DHT11温度',
          type: 'line',
          data: this.data.temperatureDataDHT11.map(item => item.value),
          smooth: true,
          symbol: 'circle',
          symbolSize: 4,
          lineStyle: { width: 2, color: '#91CC75' },
          itemStyle: { color: '#91CC75', borderWidth: 1, borderColor: '#fff' },
          markLine: this.getThresholdLines('temp')
        }
      ]
    };
    
    chart.setOption(option, true);
    
    setTimeout(() => {
      chart.resize();
    }, 50);
  },

  // 设置湿度图表选项 - 修复X轴顺序
  setHumiOption(chart) {
    if (!this.data.humidityData.length) return;
    
    const option = {
      title: { show: false },
      grid: {
        left: '12%',
        right: '8%',
        bottom: '15%',
        top: '8%',
        containLabel: true
      },
      tooltip: {
        trigger: 'axis',
        backgroundColor: 'rgba(50, 50, 50, 0.9)',
        borderColor: '#333',
        textStyle: { color: '#fff' },
        formatter: (params) => {
          const dataPoint = params[0];
          const dataIndex = dataPoint.dataIndex;
          const timeStr = this.data.timeData[dataIndex] || `点${dataIndex}`;
          const color = dataPoint.color;
          const marker = `<span style="display:inline-block;width:10rpx;height:10rpx;border-radius:50%;background:${color};margin-right:5rpx;"></span>`;
          return `<div style="font-weight:bold;">${timeStr}</div>${marker}湿度: <b>${dataPoint.value}%</b>`;
        }
      },
      xAxis: {
        type: 'category',
        data: this.data.timeData,
        name: '时间',
        nameLocation: 'middle',
        nameGap: 25,
        nameTextStyle: { fontSize: 11 },
        axisLabel: {
          interval: 0,
          rotate: 45,
          fontSize: 9,
          margin: 5,
          formatter: (value, index) => {
            if (index % 4 === 0 && value) {
              const match = value.match(/(\d+)时(\d+)分/);
              if (match) {
                const hour = match[1];
                const minute = match[2];
                return hour + ':' + minute;
              }
              return value.length > 6 ? value.substring(0, 5) : value;
            }
            return '';
          }
        },
        axisTick: { alignWithLabel: true, length: 3 },
        axisLine: { lineStyle: { color: '#ddd' } }
      },
      yAxis: {
        type: 'value',
        name: '湿度(%)',
        nameLocation: 'middle',
        nameGap: 35,
        nameTextStyle: { fontSize: 11 },
        min: 0,
        max: 100,
        axisLabel: { fontSize: 10 },
        splitLine: { lineStyle: { type: 'dashed', color: '#eee' } }
      },
      series: [{
        name: 'DHT11湿度',
        type: 'line',
        data: this.data.humidityData.map(item => item.value),
        smooth: true,
        symbol: 'circle',
        symbolSize: 4,
        lineStyle: { width: 2, color: '#EE6666' },
        itemStyle: { color: '#EE6666', borderWidth: 1, borderColor: '#fff' },
        markLine: this.getThresholdLines('humi')
      }]
    };
    
    chart.setOption(option, true);
    
    setTimeout(() => {
      chart.resize();
    }, 50);
  },

  // 获取阈值线配置
  getThresholdLines(type) {
    const lines = [];
    const threshold = this.data.threshold;
    
    if (type === 'temp') {
      if (threshold.tempMin !== null) {
        lines.push({
          name: '最低阈值',
          yAxis: threshold.tempMin,
          lineStyle: {
            color: '#ff0000',
            type: 'dashed',
            width: 2
          },
          label: {
            formatter: '最低: ' + threshold.tempMin + '℃',
            position: 'insideStartTop'
          }
        });
      }
      if (threshold.tempMax !== null) {
        lines.push({
          name: '最高阈值',
          yAxis: threshold.tempMax,
          lineStyle: {
            color: '#ff0000',
            type: 'dashed',
            width: 2
          },
          label: {
            formatter: '最高: ' + threshold.tempMax + '℃',
            position: 'insideStartTop'
          }
        });
      }
    } else {
      if (threshold.humiMin !== null) {
        lines.push({
          name: '最低阈值',
          yAxis: threshold.humiMin,
          lineStyle: {
            color: '#ff0000',
            type: 'dashed',
            width: 2
          },
          label: {
            formatter: '最低: ' + threshold.humiMin + '%',
            position: 'insideStartTop'
          }
        });
      }
      if (threshold.humiMax !== null) {
        lines.push({
          name: '最高阈值',
          yAxis: threshold.humiMax,
          lineStyle: {
            color: '#ff0000',
            type: 'dashed',
            width: 2
          },
          label: {
            formatter: '最高: ' + threshold.humiMax + '%',
            position: 'insideStartTop'
          }
        });
      }
    }
    
    return lines.length ? { data: lines, silent: true } : null;
  },

  handleDateChange(e) {
    const selectedDate = e.detail.value;
    const year = selectedDate.split('-')[0];
    const month = selectedDate.split('-')[1];
    const day = selectedDate.split('-')[2];
    const formattedDate = `${year}年${month}月${day}日`;
    
    this.setData({ 
      selectedDate: formattedDate 
    }, () => {
      this.fetchData(formattedDate);
    });
  },

  toggleLoading() {
    const newLoadingState = !this.data.isLoading;
    this.setData({ isLoading: newLoadingState });
    
    if (newLoadingState) {
      this.refreshTimer = setInterval(() => {
        const formattedDate = this.data.selectedDate;
        this.fetchData(formattedDate);
      }, 5000);
      wx.showToast({
        title: '开始自动刷新',
        icon: 'success'
      });
    } else {
      if (this.refreshTimer) {
        clearInterval(this.refreshTimer);
        this.refreshTimer = null;
      }
      wx.showToast({
        title: '停止刷新',
        icon: 'none'
      });
    }
  },

  setThreshold() {
    this.updateCharts();
    wx.showToast({
      title: '阈值已应用',
      icon: 'success',
      duration: 1500
    });
  },

  updateThreshold(e) {
    const field = e.currentTarget.dataset.field;
    const value = e.detail.value;
    let numValue = value === '' ? null : parseFloat(value);
    
    if (numValue !== null && isNaN(numValue)) {
      numValue = null;
    }
    
    this.setData({
      [`threshold.${field}`]: numValue
    });
  }
});