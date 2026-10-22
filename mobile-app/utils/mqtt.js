// MQTT协议常量
const MQTT_TYPE_CONNECT = 0x10
const MQTT_TYPE_CONNACK = 0x20
const MQTT_TYPE_PUBLISH = 0x30
const MQTT_TYPE_PUBACK = 0x40
const MQTT_TYPE_SUBSCRIBE = 0x82
const MQTT_TYPE_SUBACK = 0x90
const MQTT_TYPE_PINGREQ = 0xC0
const MQTT_TYPE_PINGRESP = 0xD0
const MQTT_TYPE_DISCONNECT = 0xE0

const QOS_AT_MOST_ONCE = 0
const QOS_AT_LEAST_ONCE = 1
const QOS_EXACTLY_ONCE = 2

// MQTT连接工具类
const MQTT_CONFIG = {
  host: 'YOUR_BROKER_HOST',
  port: 8084,
  protocol: 'wss',
  path: '/mqtt',
  username: 'YOUR_MQTT_USER',
  password: 'YOUR_MQTT_PASSWORD',
  pubTopic: 'sensor/data',
  subTopic: 'threshold/set'
}

let socketTask = null
let isConnected = false
let messageCallback = null
let reconnectTimer = null
let pingTimer = null
let packetId = 1
let connectResolve = null
let connectReject = null

export default {
  // 连接MQTT
  connect() {
    return new Promise((resolve, reject) => {
      if (isConnected) {
        resolve()
        return
      }

      try {
        // 保存resolve和reject供handleConnack使用
        connectResolve = resolve
        connectReject = reject
        
        const url = `${MQTT_CONFIG.protocol}://${MQTT_CONFIG.host}:${MQTT_CONFIG.port}${MQTT_CONFIG.path}`
        console.log('正在连接MQTT:', url)

        socketTask = uni.connectSocket({
          url: url,
          header: {
            'Sec-WebSocket-Protocol': 'mqtt'
          },
          success: () => {
            console.log('WebSocket连接请求已发送')
          },
          fail: (err) => {
            console.error('WebSocket连接失败:', err)
            connectReject = null
            reject(err)
          }
        })

        // 监听WebSocket打开
        uni.onSocketOpen(() => {
          console.log('WebSocket已打开，发送MQTT连接包')
          this.sendConnectPacket()
        })

        // 监听WebSocket消息
        uni.onSocketMessage((res) => {
          console.log('收到WebSocket消息，类型:', typeof res.data)
          console.log('消息长度:', res.data.byteLength || res.data.length)
          this.handleMessage(res.data)
        })

        // 监听WebSocket错误
        uni.onSocketError((error) => {
          console.error('WebSocket错误:', error)
          isConnected = false
          if (connectReject) {
            connectReject(error)
            connectReject = null
          }
          this.scheduleReconnect()
        })

        // 监听WebSocket关闭
        uni.onSocketClose(() => {
          console.log('WebSocket已关闭')
          isConnected = false
          this.clearPingTimer()
          this.scheduleReconnect()
        })

        // 设置连接超时
        setTimeout(() => {
          if (!isConnected) {
            if (connectReject) {
              connectReject(new Error('连接超时'))
              connectReject = null
            }
          }
        }, 10000)

      } catch (error) {
        console.error('MQTT连接异常:', error)
        if (connectReject) {
          connectReject(error)
          connectReject = null
        }
      }
    })
  },

  // 发送MQTT连接包
  sendConnectPacket() {
    const clientId = 'EMBERGUARD_App_' + Math.random().toString(16).substr(2, 8)
    const protocolName = 'MQTT'
    const protocolLevel = 4
    const connectFlags = 0xC2 // Clean session + Username + Password
    const keepAlive = 60

    let buffer = []
    
    // 构建Payload
    const clientIdBytes = this.encodeString(clientId)
    const willTopic = []
    const willMessage = []
    const usernameBytes = this.encodeString(MQTT_CONFIG.username)
    const passwordBytes = this.encodeString(MQTT_CONFIG.password)
    
    const payload = [
      ...clientIdBytes,
      ...willTopic,
      ...willMessage,
      ...usernameBytes,
      ...passwordBytes
    ]
    
    // 构建Variable Header
    const variableHeader = [
      ...this.encodeString(protocolName),
      protocolLevel,
      connectFlags,
      keepAlive >> 8, 
      keepAlive & 0xFF
    ]
    
    const remainingLength = variableHeader.length + payload.length
    
    // Fixed header
    buffer.push(0x10) // CONNECT packet type
    buffer.push(...this.encodeRemainingLength(remainingLength))
    buffer.push(...variableHeader)
    buffer.push(...payload)
    
    this.sendBinary(buffer)
    console.log('MQTT连接包已发送，clientId:', clientId)
  },

  // 处理接收到的消息
  handleMessage(data) {
    try {
      // 将ArrayBuffer或字符串转为字节数组
      let bytes
      if (typeof data === 'string') {
        console.log('收到字符串消息')
        bytes = this.stringToBytes(data)
      } else if (data instanceof ArrayBuffer) {
        console.log('收到ArrayBuffer消息')
        bytes = Array.from(new Uint8Array(data))
      } else {
        console.error('未知的消息类型:', typeof data)
        return
      }

      if (bytes.length === 0) {
        console.log('空消息，忽略')
        return
      }

      console.log('消息字节:', bytes.slice(0, 20)) // 打印前20个字节用于调试

      const packetType = (bytes[0] & 0xF0) >> 4
      console.log('MQTT包类型:', packetType)

      switch (packetType) {
        case 0x02: // CONNACK
          this.handleConnack(bytes)
          break
        case 0x03: // PUBLISH
          this.handlePublish(bytes)
          break
        case 0x09: // SUBACK
          console.log('订阅确认')
          break
        case 0x0D: // PINGRESP
          console.log('收到PING响应')
          break
        default:
          console.log('未知包类型:', packetType, '完整数据:', bytes)
      }
    } catch (error) {
      console.error('处理消息失败:', error)
    }
  },

  // 处理CONNACK
  handleConnack(bytes) {
    const returnCode = bytes[3]
    if (returnCode === 0) {
      console.log('MQTT连接成功')
      isConnected = true
      this.startPingTimer()
      
      // 触发连接成功回调
      if (connectResolve) {
        connectResolve()
        connectResolve = null
      }
    } else {
      console.error('MQTT连接失败，返回码:', returnCode)
      isConnected = false
      if (connectReject) {
        connectReject(new Error('连接失败，返回码: ' + returnCode))
        connectReject = null
      }
    }
  },

  // 处理PUBLISH消息
  handlePublish(bytes) {
    console.log('开始处理PUBLISH消息，字节长度:', bytes.length)
    console.log('前50个字节:', bytes.slice(0, 50))
    
    const flags = bytes[0] & 0x0F
    const qos = (flags >> 1) & 0x03
    const retain = flags & 0x01
    
    console.log('QoS:', qos, 'flags:', flags, 'retain:', retain)
    
    // 解析Remaining Length
    let offset = 1
    let multiplier = 1
    let remainingLength = 0
    let digit
    
    do {
      digit = bytes[offset]
      remainingLength += (digit & 0x7F) * multiplier
      multiplier *= 128
      offset++
    } while ((digit & 0x80) !== 0)
    
    console.log('remainingLength:', remainingLength, 'offset after length:', offset)
    
    // Topic name (2字节长度 + topic内容)
    const topicLength = (bytes[offset] << 8) | bytes[offset + 1]
    offset += 2
    
    console.log('Topic长度:', topicLength, 'offset before topic:', offset - 2)
    
    const topicBytes = bytes.slice(offset, offset + topicLength)
    const topic = this.bytesToString(topicBytes)
    offset += topicLength
    
    console.log('Topic:', topic)
    
    // Packet ID (if QoS > 0)
    let packetId = 0
    if (qos > 0) {
      packetId = (bytes[offset] << 8) | bytes[offset + 1]
      offset += 2
      console.log('QoS > 0, Packet ID:', packetId)
    }
    
    // Payload
    const payloadLength = bytes.length - offset
    console.log('Payload长度:', payloadLength, 'offset at payload:', offset)
    
    const payloadBytes = bytes.slice(offset)
    const message = this.bytesToString(payloadBytes)
    
    console.log('收到消息 - Topic:', topic)
    console.log('收到消息 - Payload:', message)
    console.log('messageCallback是否存在:', !!messageCallback)
    
    try {
      const data = JSON.parse(message)
      console.log('解析后的数据:', data)
      if (messageCallback) {
        messageCallback(data)
        console.log('回调已触发')
      } else {
        console.log('警告: messageCallback为空')
      }
    } catch (error) {
      console.error('消息解析失败:', error)
      console.error('原始消息:', message)
      console.error('消息长度:', message.length)
      if (messageCallback) {
        messageCallback(message)
      }
    }
  },

  // 订阅主题
  subscribe(topic, callback) {
    return new Promise((resolve, reject) => {
      if (!isConnected) {
        reject(new Error('MQTT未连接'))
        return
      }

      // 保存消息回调
      messageCallback = callback
      const id = packetId++
      
      // 保存订阅的resolve/reject
      const subResolve = resolve
      const subReject = reject
      
      let buffer = [0x82] // SUBSCRIBE
      
      const variableHeader = [id >> 8, id & 0xFF]
      const payload = [
        ...this.encodeString(topic),
        0x01 // QoS 1
      ]
      
      const remainingLength = variableHeader.length + payload.length
      buffer.push(...this.encodeRemainingLength(remainingLength))
      buffer.push(...variableHeader)
      buffer.push(...payload)
      
      this.sendBinary(buffer)
      console.log('订阅主题:', topic, 'packetId:', id)
      
      // 等待SUBACK（简化处理，实际应该用packetId匹配）
      setTimeout(() => {
        console.log('订阅成功:', topic)
        subResolve()
      }, 500)
    })
  },

  // 发布消息
  publish(topic, message) {
    return new Promise((resolve, reject) => {
      if (!isConnected) {
        reject(new Error('MQTT未连接'))
        return
      }

      const payload = typeof message === 'object' ? JSON.stringify(message) : message
      const id = packetId++
      
      console.log('准备发布消息 - Topic:', topic)
      console.log('准备发布消息 - Payload:', payload)
      console.log('准备发布消息 - PacketID:', id)
      
      // MQTT PUBLISH包: Fixed header (1 byte) + Remaining Length + Variable Header + Payload
      // Fixed header: 0x30 | (QoS << 1) | DUP | RETAIN
      // QoS 1 = 0x32
      let buffer = [0x32] // PUBLISH with QoS 1, no DUP, no RETAIN
      
      // Variable Header: Topic Length (2 bytes) + Topic + Packet ID (2 bytes)
      const topicBytes = this.stringToBytes(topic)
      const topicLength = topicBytes.length
      const topicLengthBytes = [topicLength >> 8, topicLength & 0xFF]
      const packetIdBytes = [id >> 8, id & 0xFF]
      
      const variableHeader = [
        ...topicLengthBytes,
        ...topicBytes,
        ...packetIdBytes
      ]
      
      // Payload
      const payloadBytes = this.stringToBytes(payload)
      
      // Remaining Length = Variable Header + Payload
      const remainingLength = variableHeader.length + payloadBytes.length
      buffer.push(...this.encodeRemainingLength(remainingLength))
      buffer.push(...variableHeader)
      buffer.push(...payloadBytes)
      
      console.log('发布包字节:', buffer)
      console.log('发布包长度:', buffer.length)
      
      this.sendBinary(buffer)
      console.log('发布消息已发送')
      
      // QoS 1需要等待PUBACK（包类型4），但我们简化处理直接resolve
      // 实际应用中应该监听PUBACK
      setTimeout(() => {
        resolve()
      }, 500)
    })
  },

  // 发送PING请求
  sendPing() {
    if (!isConnected) return
    
    const buffer = [0xC0, 0x00]
    this.sendBinary(buffer)
    console.log('发送PING请求')
  },

  // 启动PING定时器
  startPingTimer() {
    this.clearPingTimer()
    pingTimer = setInterval(() => {
      this.sendPing()
    }, 30000) // 每30秒发送一次
  },

  // 清除PING定时器
  clearPingTimer() {
    if (pingTimer) {
      clearInterval(pingTimer)
      pingTimer = null
    }
  },

  // 安排重连
  scheduleReconnect(reject) {
    if (reject) {
      reject(new Error('连接失败'))
    }
    
    if (reconnectTimer) {
      clearTimeout(reconnectTimer)
    }
    
    reconnectTimer = setTimeout(() => {
      console.log('尝试重新连接...')
      this.connect().catch(() => {})
    }, 5000)
  },

  // 断开连接
  disconnect() {
    if (isConnected && socketTask) {
      const buffer = [0xE0, 0x00]
      this.sendBinary(buffer)
    }
    
    if (socketTask) {
      uni.closeSocket()
      socketTask = null
    }
    
    isConnected = false
    this.clearPingTimer()
    
    if (reconnectTimer) {
      clearTimeout(reconnectTimer)
      reconnectTimer = null
    }
    
    console.log('MQTT已断开')
  },

  // 获取连接状态
  getConnectionStatus() {
    return isConnected
  },

  // ========== 辅助方法 ==========

  // 编码字符串（MQTT格式）
  encodeString(str) {
    const bytes = this.stringToBytes(str)
    return [bytes.length >> 8, bytes.length & 0xFF, ...bytes]
  },

  // 编码剩余长度
  encodeRemainingLength(length) {
    const bytes = []
    do {
      let digit = length % 128
      length = Math.floor(length / 128)
      if (length > 0) {
        digit |= 0x80
      }
      bytes.push(digit)
    } while (length > 0)
    return bytes
  },

  // 解码剩余长度
  decodeRemainingLength(bytes, offset) {
    let multiplier = 1
    let value = 0
    let digit
    
    do {
      digit = bytes[offset++]
      value += (digit & 0x7F) * multiplier
      multiplier *= 128
    } while ((digit & 0x80) !== 0)
    
    return value
  },

  // 字符串转字节数组
  stringToBytes(str) {
    const bytes = []
    for (let i = 0; i < str.length; i++) {
      const code = str.charCodeAt(i)
      if (code < 0x80) {
        bytes.push(code)
      } else if (code < 0x800) {
        bytes.push(0xC0 | (code >> 6))
        bytes.push(0x80 | (code & 0x3F))
      } else {
        bytes.push(0xE0 | (code >> 12))
        bytes.push(0x80 | ((code >> 6) & 0x3F))
        bytes.push(0x80 | (code & 0x3F))
      }
    }
    return bytes
  },

  // 字节数组转字符串
  bytesToString(bytes) {
    try {
      // 使用TextDecoder来正确解码UTF-8
      const uint8Array = new Uint8Array(bytes)
      const decoder = new TextDecoder('utf-8')
      return decoder.decode(uint8Array)
    } catch (error) {
      // 降级方案：使用传统方法
      let str = ''
      for (let i = 0; i < bytes.length; i++) {
        str += String.fromCharCode(bytes[i])
      }
      return decodeURIComponent(escape(str))
    }
  },

  // 发送二进制数据
  sendBinary(buffer) {
    if (!socketTask) {
      console.error('Socket未初始化')
      return
    }

    const arrayBuffer = new ArrayBuffer(buffer.length)
    const uint8Array = new Uint8Array(arrayBuffer)
    
    for (let i = 0; i < buffer.length; i++) {
      uint8Array[i] = buffer[i]
    }

    uni.sendSocketMessage({
      data: arrayBuffer,
      success: () => {
        console.log('消息发送成功')
      },
      fail: (error) => {
        console.error('消息发送失败:', error)
      }
    })
  }
}
