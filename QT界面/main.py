from PyQt5.QtWidgets import QWidget, QApplication, QMessageBox, QFileDialog
from PyQt5.QtCore import QTimer, QDate
import pymysql
import sys
import warnings
import csv
from datetime import datetime
from data import Ui_Form

warnings.filterwarnings("ignore", category=DeprecationWarning)


class Demo(QWidget, Ui_Form):
    def __init__(self):
        super(Demo, self).__init__()
        self.setupUi(self)

        # 初始化变量
        self.current_data = []
        self.dates = []
        self.ds18b20_temps = []
        self.dht11_temps = []
        self.dht11_humis = []

        # 添加变量记录当前显示日期
        self.current_date = QDate.currentDate().toString("yyyy年MM月dd日")

        # 初始化绘图
        self.init_plots()

        # 连接按钮信号
        self.ExcelButton.clicked.connect(self.export_to_excel)

        # 连接日期编辑框信号 - 修正：使用dateEdit而不是timeLineEdit
        # 设置初始日期为今天
        self.dateEdit.setDate(QDate.currentDate())

        # 连接日期变化信号
        self.dateEdit.dateChanged.connect(self.on_date_changed)

        # 设置定时器自动更新
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_data_and_plots)
        self.timer.start(5000)  # 每5秒更新一次

        # 初始加载今天的数据
        self.update_data_and_plots()

    def init_plots(self):
        """初始化绘图区域"""
        import pyqtgraph as pg

        # 第一个绘图区域 - 温度
        self.pw_1 = pg.PlotWidget()
        self.pw_1.addLegend(size=(100, 50), offset=(0, 0))
        self.pw_1.showGrid(x=True, y=True, alpha=0.5)
        self.pw_1.setLabel('left', '温度 (°C)')
        self.pw_1.setLabel('bottom', '时间')
        self.pw_1.setTitle('温度传感器数据')

        # DS18B20温度曲线
        self.curve_ds18b20 = self.pw_1.plot(
            pen='r',
            name='DS18B20温度',
            symbol='t',
            symbolSize=4,
            symbolPen=(0, 0, 0),
            symbolBrush=(255, 0, 0)
        )

        # DHT11温度曲线
        self.curve_dht11_temp = self.pw_1.plot(
            pen='g',
            name='DHT11温度',
            symbol='o',
            symbolSize=4,
            symbolPen=(0, 0, 0),
            symbolBrush=(0, 255, 0)
        )

        # 添加图例
        self.verticalLayout_1.addWidget(self.pw_1)

        # 第二个绘图区域 - 湿度
        self.pw_2 = pg.PlotWidget()
        self.pw_2.addLegend(size=(100, 50), offset=(0, 0))
        self.pw_2.showGrid(x=True, y=True, alpha=0.5)
        self.pw_2.setLabel('left', '湿度 (%)')
        self.pw_2.setLabel('bottom', '时间')
        self.pw_2.setTitle('DHT11湿度数据')

        # DHT11湿度曲线
        self.curve_dht11_humi = self.pw_2.plot(
            pen='b',
            name='DHT11湿度',
            symbol='s',
            symbolSize=4,
            symbolPen=(0, 0, 0),
            symbolBrush=(0, 0, 255)
        )

        # 添加图例
        self.verticalLayout_2.addWidget(self.pw_2)

    def query_database(self, target_date=None):
        """查询数据库数据"""
        if target_date is None:
            # 默认查询今天的数据
            target_date = self.current_date

        try:
            conn = pymysql.connect(
                host='127.0.0.1',
                user='root',
                password='yiyi1123',
                database='sensor_data',
                charset='utf8mb4',
                cursorclass=pymysql.cursors.DictCursor
            )

            cursor = conn.cursor()

            query = """
            SELECT * FROM f1app_test1
            WHERE time_day = %s
            ORDER BY time
            """

            cursor.execute(query, (target_date,))

            results = cursor.fetchall()

            data_list = []
            for row in results:
                data_list.append(row)

            cursor.close()
            conn.close()

            return data_list

        except pymysql.Error as e:
            print(f"数据库错误: {e}")
            return []
        except Exception as e:
            print(f"其他错误: {e}")
            return []

    def process_data(self, data_list):
        """处理查询到的数据"""
        self.current_data = data_list
        self.dates = []
        self.ds18b20_temps = []
        self.dht11_temps = []
        self.dht11_humis = []

        if not data_list:
            return

        for item in data_list:
            # 获取时间
            time_str = item.get('time', '')
            self.dates.append(time_str)

            # 获取DS18B20温度
            ds18b20_temp = item.get('ds18b20_temp', 'NULL')
            if ds18b20_temp != 'NULL':
                try:
                    self.ds18b20_temps.append(float(ds18b20_temp))
                except ValueError:
                    self.ds18b20_temps.append(0)
            else:
                self.ds18b20_temps.append(0)

            # 获取DHT11温度
            dht11_temp = item.get('dht11_temp', 'NULL')
            if dht11_temp != 'NULL':
                try:
                    self.dht11_temps.append(float(dht11_temp))
                except ValueError:
                    self.dht11_temps.append(0)
            else:
                self.dht11_temps.append(0)

            # 获取DHT11湿度
            dht11_humi = item.get('dht11_humi', 'NULL')
            if dht11_humi != 'NULL':
                try:
                    self.dht11_humis.append(float(dht11_humi))
                except ValueError:
                    self.dht11_humis.append(0)
            else:
                self.dht11_humis.append(0)

        print(f"处理了 {len(data_list)} 条数据，日期: {self.current_date}")

        # 更新文本框显示最新数据（如果有数据）
        if data_list:
            latest_data = data_list[-1]  # 获取最新一条数据
            self.update_text_boxes(latest_data)

    def update_text_boxes(self, data):
        """更新文本框显示最新数据"""
        try:
            # PIN_ID
            self.lineEdit_1.setText(str(data.get('pin_id', '')))
            # ID
            self.lineEdit_2.setText(str(data.get('node_id', '')))
            # DS18B20温度
            ds18b20_temp = data.get('ds18b20_temp', 'NULL')
            self.lineEdit_3.setText(str(ds18b20_temp) if ds18b20_temp != 'NULL' else '无数据')
            # DHT11温度
            dht11_temp = data.get('dht11_temp', 'NULL')
            self.lineEdit_4.setText(str(dht11_temp) if dht11_temp != 'NULL' else '无数据')
            # DHT11湿度
            dht11_humi = data.get('dht11_humi', 'NULL')
            self.lineEdit_5.setText(str(dht11_humi) if dht11_humi != 'NULL' else '无数据')
            # 时间
            time_str = data.get('time', '')
            self.lineEdit_6.setText(str(time_str))
        except Exception as e:
            print(f"更新文本框时出错: {e}")

    def update_plots(self):
        """更新绘图"""
        if not self.dates:
            # 没有数据时清除曲线
            self.curve_ds18b20.setData([])
            self.curve_dht11_temp.setData([])
            self.curve_dht11_humi.setData([])

            # 设置空图表的标题
            self.pw_1.setTitle(f'温度传感器数据 - {self.current_date} (无数据)')
            self.pw_2.setTitle(f'DHT11湿度数据 - {self.current_date} (无数据)')

            # 清空文本框
            self.clear_text_boxes()
            return

        # 创建X轴索引
        x = list(range(len(self.dates)))

        # 更新温度图
        self.curve_ds18b20.setData(x, self.ds18b20_temps)
        self.curve_dht11_temp.setData(x, self.dht11_temps)

        # 更新湿度图
        self.curve_dht11_humi.setData(x, self.dht11_humis)

        # 设置图表标题
        self.pw_1.setTitle(f'温度传感器数据 - {self.current_date}')
        self.pw_2.setTitle(f'DHT11湿度数据 - {self.current_date}')

        # 设置X轴标签
        if len(self.dates) > 0:
            # 每隔几个点显示一个时间标签
            tick_positions = []
            tick_labels = []
            step = max(1, len(self.dates) // 10)
            for i in range(0, len(self.dates), step):
                tick_positions.append(i)
                tick_labels.append(self.dates[i])

            # 设置X轴刻度
            if tick_positions:
                ticks = [(pos, label) for pos, label in zip(tick_positions, tick_labels)]
                self.pw_1.getAxis('bottom').setTicks([ticks])
                self.pw_2.getAxis('bottom').setTicks([ticks])

            # 自动调整Y轴范围
            if self.ds18b20_temps or self.dht11_temps:
                all_temps = self.ds18b20_temps + self.dht11_temps
                min_temp = min(all_temps) if all_temps else 0
                max_temp = max(all_temps) if all_temps else 50
                self.pw_1.setYRange(max(-5, min_temp - 2), max_temp + 2)

            if self.dht11_humis:
                min_humi = min(self.dht11_humis)
                max_humi = max(self.dht11_humis)
                self.pw_2.setYRange(max(0, min_humi - 5), min(100, max_humi + 5))

    def clear_text_boxes(self):
        """清空文本框"""
        self.lineEdit_1.setText("")
        self.lineEdit_2.setText("")
        self.lineEdit_3.setText("")
        self.lineEdit_4.setText("")
        self.lineEdit_5.setText("")
        self.lineEdit_6.setText("")

    def update_data_and_plots(self):
        """更新数据和绘图"""
        # 默认加载当前日期数据
        data = self.query_database(self.current_date)

        if data:
            self.process_data(data)
            self.update_plots()
            print(f"更新了 {len(data)} 条数据，日期: {self.current_date}，时间: {datetime.now().strftime('%H:%M:%S')}")
        else:
            print(f"没有查询到数据，日期: {self.current_date}")
            self.update_plots()  # 更新图表以显示无数据状态

    def on_date_changed(self, date):
        """处理日期变化"""
        try:
            # 获取新日期并转换为字符串格式
            self.current_date = date.toString("yyyy年MM月dd日")

            print(f"日期已更改为: {self.current_date}")

            # 重新加载数据
            data = self.query_database(self.current_date)

            if data:
                self.process_data(data)
                self.update_plots()
                print(f"加载了 {len(data)} 条数据，日期: {self.current_date}")
            else:
                print(f"没有查询到数据，日期: {self.current_date}")
                self.update_plots()  # 更新图表以显示无数据状态

        except Exception as e:
            print(f"日期更改错误: {e}")

    def export_to_excel(self):
        """导出数据到Excel（使用csv格式，兼容性更好）"""
        if not self.current_data:
            QMessageBox.warning(self, "警告", "没有数据可以导出")
            return

        try:
            # 使用当前显示日期作为默认文件名
            default_filename = f"传感器数据_{self.current_date}"

            # 让用户选择保存位置和文件名
            filename, _ = QFileDialog.getSaveFileName(
                self,
                "保存数据",
                default_filename,
                "CSV文件 (*.csv);;Excel文件 (*.xlsx);;所有文件 (*.*)"
            )

            if not filename:
                return  # 用户取消了保存

            # 根据文件扩展名决定保存格式
            if filename.endswith('.csv'):
                self.save_as_csv(filename)
            elif filename.endswith('.xlsx'):
                self.save_as_excel(filename)
            else:
                # 默认保存为CSV
                if not filename.endswith('.csv'):
                    filename += '.csv'
                self.save_as_csv(filename)

        except Exception as e:
            QMessageBox.critical(self, "错误", f"导出数据时出错: {str(e)}")

    def save_as_csv(self, filename):
        """保存为CSV格式"""
        try:
            with open(filename, 'w', newline='', encoding='utf-8-sig') as csvfile:
                if self.current_data:
                    # 获取所有字段名
                    fieldnames = self.current_data[0].keys()
                    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

                    # 写入表头
                    writer.writeheader()

                    # 写入数据
                    writer.writerows(self.current_data)

                QMessageBox.information(self, "成功", f"数据已成功导出到:\n{filename}")
                print(f"数据已导出到: {filename}")
        except Exception as e:
            raise Exception(f"保存CSV文件时出错: {str(e)}")

    def save_as_excel(self, filename):
        """保存为Excel格式（使用openpyxl）"""
        try:
            # 尝试导入openpyxl
            try:
                from openpyxl import Workbook
            except ImportError:
                # 如果openpyxl未安装，提示用户
                reply = QMessageBox.question(
                    self,
                    "缺少依赖",
                    "需要安装openpyxl库来导出Excel文件。是否现在安装？",
                    QMessageBox.Yes | QMessageBox.No
                )

                if reply == QMessageBox.Yes:
                    import subprocess
                    import sys
                    try:
                        subprocess.check_call([sys.executable, "-m", "pip", "install", "openpyxl"])
                        from openpyxl import Workbook
                        QMessageBox.information(self, "成功", "openpyxl安装成功，请重新尝试导出。")
                    except:
                        QMessageBox.warning(self, "错误", "安装openpyxl失败，请手动安装或选择CSV格式。")
                        return
                else:
                    return

            # 创建Workbook
            wb = Workbook()
            ws = wb.active
            ws.title = "传感器数据"

            # 写入表头
            if self.current_data:
                headers = list(self.current_data[0].keys())
                for col, header in enumerate(headers, start=1):
                    ws.cell(row=1, column=col, value=header)

                # 写入数据
                for row_idx, row_data in enumerate(self.current_data, start=2):
                    for col_idx, header in enumerate(headers, start=1):
                        value = row_data.get(header, '')
                        ws.cell(row=row_idx, column=col_idx, value=value)

            # 保存文件
            wb.save(filename)
            QMessageBox.information(self, "成功", f"数据已成功导出到:\n{filename}")
            print(f"数据已导出到: {filename}")

        except Exception as e:
            raise Exception(f"保存Excel文件时出错: {str(e)}")


if __name__ == '__main__':
    app = QApplication(sys.argv)
    myWin = Demo()
    myWin.show()
    sys.exit(app.exec_())