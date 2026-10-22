import mqtt, { MqttClient } from 'mqtt'

export interface SensorData {
  mq4: number
  flame: number
  flame_mv: number
  temp: number
  humi: number
  tc: number
  pressure: number
  altitude: number
  storm_warning: boolean
  alert: {
    fire: boolean
    gas: boolean
    temp_high: boolean
    humi_high: boolean
    tc_high: boolean
  }
}

export interface ThresholdConfig {
  temp_high: number
  humi_high: number
  mq4_high: number
  tc_high: number
}

type MessageHandler = (topic: string, payload: Uint8Array) => void
type StatusChangeHandler = (connected: boolean) => void

class MQTTService {
  private client: MqttClient | null = null
  private handlers: Map<string, MessageHandler[]> = new Map()
  private isConnected: boolean = false
  private statusHandlers: StatusChangeHandler[] = []

  onStatusChange(handler: StatusChangeHandler): () => void {
    this.statusHandlers.push(handler)
    return () => {
      const index = this.statusHandlers.indexOf(handler)
      if (index > -1) {
        this.statusHandlers.splice(index, 1)
      }
    }
  }

  private updateStatus(connected: boolean) {
    this.isConnected = connected
    this.statusHandlers.forEach(handler => handler(connected))
  }

  connect(brokerUrl: string, username: string, password: string): Promise<void> {
    return new Promise((resolve, reject) => {
      try {
        this.client = mqtt.connect(brokerUrl, {
          username,
          password,
          reconnectPeriod: 5000,
          connectTimeout: 30000,
          clean: true,
          clientId: `web_monitor_${Math.random().toString(16).substr(2, 8)}`,
          protocolVersion: 4,
          keepalive: 60,
          resubscribe: true,
        })

        this.client.on('connect', () => {
          console.log('[MQTT] Connected to broker successfully')
          this.updateStatus(true)
          resolve()
        })

        this.client.on('message', (topic, payload) => {
          console.log(`[MQTT] Message received on ${topic}:`, new TextDecoder().decode(payload))
          const handlers = this.handlers.get(topic)
          if (handlers) {
            handlers.forEach(handler => handler(topic, payload))
          }
        })

        this.client.on('error', (err) => {
          console.error('[MQTT] Connection error:', err.message)
          this.updateStatus(false)
        })

        this.client.on('close', () => {
          console.log('[MQTT] Connection closed')
          this.updateStatus(false)
        })

        this.client.on('reconnect', () => {
          console.log('[MQTT] Attempting to reconnect...')
          this.updateStatus(false)
        })

        this.client.on('offline', () => {
          console.log('[MQTT] Client is offline')
          this.updateStatus(false)
        })

        this.client.on('end', () => {
          console.log('[MQTT] Connection ended')
          this.updateStatus(false)
        })
      } catch (error) {
        console.error('[MQTT] Connection failed:', error)
        this.updateStatus(false)
        reject(error)
      }
    })
  }

  subscribe(topic: string, qos: 0 | 1 | 2 = 1): void {
    if (!this.client || !this.isConnected) {
      throw new Error('MQTT client not connected')
    }
    this.client.subscribe(topic, { qos }, (err) => {
      if (err) {
        console.error(`[MQTT] Failed to subscribe to ${topic}:`, err)
      } else {
        console.log(`[MQTT] Subscribed to ${topic}`)
      }
    })
  }

  publish(topic: string, message: object, qos: 0 | 1 | 2 = 1): void {
    if (!this.client || !this.isConnected) {
      throw new Error('MQTT client not connected')
    }
    const payload = JSON.stringify(message)
    this.client.publish(topic, payload, { qos }, (err) => {
      if (err) {
        console.error('[MQTT] Publish error:', err)
      } else {
        console.log(`[MQTT] Published to ${topic}:`, payload)
      }
    })
  }

  onMessage(topic: string, handler: MessageHandler): () => void {
    if (!this.handlers.has(topic)) {
      this.handlers.set(topic, [])
    }
    this.handlers.get(topic)!.push(handler)
    
    // Return unsubscribe function
    return () => {
      const handlers = this.handlers.get(topic)
      if (handlers) {
        const index = handlers.indexOf(handler)
        if (index > -1) {
          handlers.splice(index, 1)
        }
      }
    }
  }

  getConnectionStatus(): boolean {
    return this.isConnected
  }

  disconnect(): void {
    if (this.client) {
      this.client.end(true, () => {
        console.log('[MQTT] Client disconnected')
      })
      this.client = null
      this.updateStatus(false)
      this.handlers.clear()
    }
  }
}

export const mqttService = new MQTTService()
