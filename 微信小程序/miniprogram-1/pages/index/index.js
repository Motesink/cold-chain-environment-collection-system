// index.js
const defaultAvatarUrl = 'https://mmbiz.qpic.cn/mmbiz/icTdbqWNOwNRna42FI242Lcia07jQodd2FJGIYQfG0LAJGFxM4FbnQP6yfMxBgJ0F3YRqJCJ1aPAK2dQagdusBZg/0'

// Page({
//   data: {
//     user:{}
//     },
//   handleLoadData(){
//     //发送请求之前loading效果
//     wx.showLoading({
//       title: '加载中~~',
//       mask:true   //加载透明蒙层，设置后，加载框后的按钮都不可点击
//     })

//     wx.request({
//       url:'http://192.168.224.1:8080/get_test1_data/',
//       method:'GET',
//       data:{},
//       header:{},
//       success:(res)=>{
//         //请求成功
//         console.log(res.data)
//         this.setData({
//           user:res.data
//         })
//       },
//       fail:(error)=>{
//         console.log(error)
//       },
//       complete:(res)=>{
//         //关闭加载
//         wx.hideLoading()
//       }
//     })
//   }
// })

Page({
  data: {
    latestData: null,
    isLoading: false,
    timer: null
  },

  onLoad() {
    this.loadData();
  },

  loadData() {
    const that = this;
    wx.request({
      url: 'http://192.168.198.1:8080/get_test1_data/',
      success(res) {
        if (res.data.latest_data) {
          that.setData({
            latestData: {
              ...res.data.latest_data,
              formattedTime: `${res.data.latest_data.time_day} ${res.data.latest_data.time}`
            }
          });
        }
      }
    });
  },

  toggleLoading() {
    if (this.data.isLoading) {
      clearInterval(this.data.timer);
      this.setData({ isLoading: false });
    } else {
      this.setData({
        isLoading: true,
        timer: setInterval(() => this.loadData(), 3000)
      });
    }
  },

  onUnload() {
    clearInterval(this.data.timer);
  }
});
