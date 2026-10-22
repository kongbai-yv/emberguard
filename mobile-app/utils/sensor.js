// 传感器数据服务
import mqttClient from './mqtt.js'

class SensorService {
  constructor() {
    this.sensorData = {
      mq4: 0,
      flame: 0,
      flame_mv: 0,
      temp: 0,
      humi: 0,
      tc: 0,
      pressure: 0,
      altitude: 0,
      storm_warning: false,
      alert: {
        fire: false,
        gas: false,
        temp_high: false,
        humi_high: false,
        tc_high: false
      }
    }
    this.lastUpdateTime = ''
    this.isConnecting = false
  }

  // 初始化连接
  async init() {
    if (this.isConnecting) return
    
    try {
      this.isConnecting = true
      
      // 检查网络状态
      const networkType = await this.checkNetwork()
      console.log('网络类型:', networkType)
      
      if (networkType === 'none') {
        throw new Error('无网络连接')
      }
      
      await mqttClient.connect()
      console.log('MQTT连接成功')
      
      // 订阅数据主题
      await mqttClient.subscribe('sensor/data', (data) => {
        this.updateSensorData(data)
      })
      
      this.isConnecting = false
      return true
    } catch (error) {
      console.error('MQTT初始化失败:', error)
      this.isConnecting = false
      throw error
    }
  }

  // 检查网络状态
  checkNetwork() {
    return new Promise((resolve) => {
      uni.getNetworkType({
        success: (res) => {
          resolve(res.networkType)
        },
        fail: () => {
          resolve('unknown')
        }
      })
    })
  }

  // 更新传感器数据
  updateSensorData(data) {
    if (data && typeof data === 'object') {
      this.sensorData = { ...this.sensorData, ...data }
      this.lastUpdateTime = this.formatTime(new Date())
      console.log('传感器数据已更新:', this.sensorData)
    }
  }

  // 获取传感器数据
  getSensorData() {
    return {
      ...this.sensorData,
      lastUpdateTime: this.lastUpdateTime
    }
  }

  // 下发阈值
  async setThreshold(thresholdData) {
    try {
      await mqttClient.publish('threshold/set', thresholdData)
      console.log('阈值下发成功:', thresholdData)
      return true
    } catch (error) {
      console.error('阈值下发失败:', error)
      return false
    }
  }

  // 格式化时间
  formatTime(date) {
    const h = date.getHours().toString().padStart(2, '0')
    const m = date.getMinutes().toString().padStart(2, '0')
    const s = date.getSeconds().toString().padStart(2, '0')
    return `${h}:${m}:${s}`
  }

  // 断开连接
  disconnect() {
    mqttClient.disconnect()
  }
}

export default new SensorService()
