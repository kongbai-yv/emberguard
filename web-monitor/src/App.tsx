import { useState, useEffect, useCallback } from 'react'
import { mqttService, type SensorData, type ThresholdConfig } from '@/lib/mqtt'
import { CabinetDisplay } from '@/components/data3d/CabinetDisplay'
import { SmartSummary } from '@/components/insights/SmartSummary'
import { AlarmTrace } from '@/components/insights/AlarmTrace'
import { StatusHeader } from '@/components/layout/StatusHeader'
import { MetricCard } from '@/components/layout/MetricCard'
import { ThresholdPanel } from '@/components/layout/ThresholdPanel'
import { Thermometer, Droplets, Wind, Flame, Gauge, Mountain } from 'lucide-react'

const MQTT_BROKER = 'wss://YOUR_BROKER_HOST:8084/mqtt'
const MQTT_USERNAME = 'ESP32'
const MQTT_PASSWORD = 'YOUR_PASSWORD'
const DATA_TOPIC = 'sensor/data'
const THRESHOLD_TOPIC = 'threshold/set'
const DATA_TIMEOUT_MS = 30000

interface HistoryPoint {
  time: string
  value: number
}

export default function App() {
  const [mqttConnected, setMqttConnected] = useState(false)
  const [hasData, setHasData] = useState(false)
  const [sensorData, setSensorData] = useState<SensorData | null>(null)
  const [connectionStatus, setConnectionStatus] = useState('正在连接...')
  const [lastDataTime, setLastDataTime] = useState<Date | null>(null)
  const [deviceOnline, setDeviceOnline] = useState(false)
  const [dataTimeout, setDataTimeout] = useState(false)

  const [thresholds, setThresholds] = useState<ThresholdConfig>({
    temp_high: 35.0,
    humi_high: 60.0,
    mq4_high: 400.0,
    tc_high: 100.0,
  })

  const [tempHistory, setTempHistory] = useState<HistoryPoint[]>([])
  const [humiHistory, setHumiHistory] = useState<HistoryPoint[]>([])
  const [mq4History, setMq4History] = useState<HistoryPoint[]>([])
  const [tcHistory, setTcHistory] = useState<HistoryPoint[]>([])
  const [_pressureHistory, setPressureHistory] = useState<HistoryPoint[]>([])

  const isConnected = mqttConnected && hasData

  // 设备离线检测
  useEffect(() => {
    if (!lastDataTime) {
      setDeviceOnline(false)
      return
    }

    const checkTimeout = () => {
      const now = new Date()
      const timeSinceLastData = now.getTime() - lastDataTime.getTime()
      const isTimedOut = timeSinceLastData > DATA_TIMEOUT_MS

      setDataTimeout(isTimedOut)
      setDeviceOnline(!isTimedOut)

      if (isTimedOut && hasData) {
        setConnectionStatus('设备离线 - 未收到数据')
      }
    }

    checkTimeout()
    const interval = setInterval(checkTimeout, 5000)
    return () => clearInterval(interval)
  }, [lastDataTime, hasData])

  // MQTT连接
  const connectMQTT = useCallback(async () => {
    try {
      setConnectionStatus('正在连接到 EMQX Cloud...')
      setHasData(false)
      setLastDataTime(null)

      await mqttService.connect(MQTT_BROKER, MQTT_USERNAME, MQTT_PASSWORD)
      setMqttConnected(true)
      setConnectionStatus('MQTT已连接，等待传感器数据...')

      mqttService.onStatusChange((connected) => {
        setMqttConnected(connected)
        if (!connected) {
          setHasData(false)
          setConnectionStatus('MQTT连接已断开')
        }
      })

      mqttService.subscribe(DATA_TOPIC, 1)
      mqttService.subscribe(THRESHOLD_TOPIC, 1)

      mqttService.onMessage(DATA_TOPIC, (_topic, payload) => {
        try {
          const text = new TextDecoder().decode(payload)
          const data: SensorData = JSON.parse(text)
          setSensorData(data)

          if (!hasData) {
            setHasData(true)
            setDeviceOnline(true)
            setDataTimeout(false)
            setConnectionStatus('数据接收正常')
          }

          const now = new Date()
          setLastDataTime(now)
          setDeviceOnline(true)
          setDataTimeout(false)

          const timeStr = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`

          setTempHistory((prev) => [...prev, { time: timeStr, value: data.temp }].slice(-20))
          setHumiHistory((prev) => [...prev, { time: timeStr, value: data.humi }].slice(-20))
          setMq4History((prev) => [...prev, { time: timeStr, value: data.mq4 }].slice(-20))
          setTcHistory((prev) => [...prev, { time: timeStr, value: data.tc }].slice(-20))
          setPressureHistory((prev) => [...prev, { time: timeStr, value: data.pressure }].slice(-20))
        } catch (error) {
          console.error('[Parse Error]:', error)
        }
      })
    } catch (error) {
      console.error('[Connection Error]:', error)
      setConnectionStatus(`连接失败: ${error instanceof Error ? error.message : '未知错误'}`)
      setMqttConnected(false)
      setHasData(false)
    }
  }, [])

  const handleReconnect = () => {
    setMqttConnected(false)
    setHasData(false)
    setConnectionStatus('正在重新连接...')
    mqttService.disconnect()
    setTimeout(() => connectMQTT(), 500)
  }

  useEffect(() => {
    connectMQTT()
    return () => mqttService.disconnect()
  }, [connectMQTT])

  const handleThresholdChange = useCallback(
    (field: string, value: number) => {
      const newThresholds = { ...thresholds, [field]: value }
      setThresholds(newThresholds)

      if (mqttService.getConnectionStatus()) {
        mqttService.publish(THRESHOLD_TOPIC, { [field]: value }, 1)
        setConnectionStatus(`已发送阈值更新: ${field}=${value}`)
      }
    },
    [thresholds]
  )

  const publishAllThresholds = () => {
    if (mqttService.getConnectionStatus()) {
      mqttService.publish(THRESHOLD_TOPIC, thresholds, 1)
      setConnectionStatus('已发送全部阈值设置')
    }
  }

  // 计算趋势
  const getTrend = (history: HistoryPoint[]): 'up' | 'down' | 'stable' => {
    if (history.length < 3) return 'stable'
    const recent = history.slice(-5)
    const first = recent[0].value
    const last = recent[recent.length - 1].value
    const diff = last - first
    const threshold = first * 0.03
    if (diff > threshold) return 'up'
    if (diff < -threshold) return 'down'
    return 'stable'
  }

  return (
    <div className="min-h-screen bg-[#08080f] text-white">
      {/* 背景装饰 */}
      <div className="fixed inset-0 pointer-events-none">
        <div className="absolute top-0 left-1/4 w-96 h-96 bg-indigo-500/5 rounded-full blur-3xl" />
        <div className="absolute bottom-0 right-1/4 w-96 h-96 bg-purple-500/5 rounded-full blur-3xl" />
      </div>

      <div className="relative max-w-[1800px] mx-auto px-6 py-4">
        {/* 头部状态栏 */}
        <StatusHeader
          mqttConnected={mqttConnected}
          deviceOnline={deviceOnline}
          connectionStatus={connectionStatus}
          lastDataTime={lastDataTime}
          dataTimeout={dataTimeout}
          onReconnect={handleReconnect}
          className="mb-6"
        />

        {/* 主内容区 */}
        <div className="grid grid-cols-12 gap-6">
          {/* 左侧面板 - 指标卡片 */}
          <div className="col-span-3 space-y-4">
            <MetricCard
              label="环境温度"
              value={sensorData?.temp ?? 0}
              unit="℃"
              icon={<Thermometer className="w-full h-full" />}
              color="#ef4444"
              alarm={sensorData?.alert.temp_high}
              trend={getTrend(tempHistory)}
            />
            <MetricCard
              label="环境湿度"
              value={sensorData?.humi ?? 0}
              unit="%"
              icon={<Droplets className="w-full h-full" />}
              color="#3b82f6"
              alarm={sensorData?.alert.humi_high}
              trend={getTrend(humiHistory)}
            />
            <MetricCard
              label="燃气浓度"
              value={sensorData?.mq4 ?? 0}
              unit="ppm"
              icon={<Wind className="w-full h-full" />}
              color="#f59e0b"
              alarm={sensorData?.alert.gas}
              trend={getTrend(mq4History)}
            />
            <MetricCard
              label="PT100温度"
              value={sensorData?.tc ?? 0}
              unit="℃"
              icon={<Gauge className="w-full h-full" />}
              color="#ec4899"
              alarm={sensorData?.alert.tc_high}
              trend={getTrend(tcHistory)}
            />
            <MetricCard
              label="大气压力"
              value={sensorData?.pressure ?? 0}
              unit="hPa"
              icon={<Mountain className="w-full h-full" />}
              color="#8b5cf6"
              alarm={false}
            />
            <MetricCard
              label="火焰检测"
              value={sensorData?.flame === 1 ? '检测到' : '正常'}
              icon={<Flame className="w-full h-full" />}
              color="#f97316"
              alarm={sensorData?.alert.fire}
            />
          </div>

          {/* 中央 - 危化品储存柜 */}
          <div className="col-span-6">
            <div className="rounded-2xl overflow-hidden border border-slate-800/50 bg-slate-900/30 backdrop-blur-sm">
              <CabinetDisplay sensorData={sensorData} className="h-[600px]" />
            </div>

            {/* 底部数据条 */}
            <div className="mt-4 grid grid-cols-6 gap-2">
              {[
                { label: '火焰电压', value: sensorData?.flame_mv ?? 0, unit: 'mV', color: '#f97316' },
                { label: '海拔', value: sensorData?.altitude ?? 0, unit: 'm', color: '#06b6d4' },
                { label: '风暴预警', value: sensorData?.storm_warning ? '是' : '否', color: '#ef4444' },
                { label: 'MQTT', value: mqttConnected ? '已连接' : '断开', color: mqttConnected ? '#10b981' : '#ef4444' },
                { label: '设备', value: deviceOnline ? '在线' : '离线', color: deviceOnline ? '#10b981' : '#f59e0b' },
                { label: '数据点', value: tempHistory.length, unit: '个', color: '#6366f1' },
              ].map((item) => (
                <div
                  key={item.label}
                  className="p-2 rounded-lg bg-slate-900/50 border border-slate-800 text-center"
                >
                  <div className="text-[10px] text-slate-500">{item.label}</div>
                  <div className="text-sm font-medium" style={{ color: item.color }}>
                    {typeof item.value === 'number' ? item.value.toFixed(1) : item.value}
                    {item.unit && <span className="text-[10px] text-slate-500 ml-0.5">{item.unit}</span>}
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* 右侧面板 - 智能分析 */}
          <div className="col-span-3 space-y-6">
            <SmartSummary
              sensorData={sensorData}
              tempHistory={tempHistory}
              humiHistory={humiHistory}
              mq4History={mq4History}
            />

            <AlarmTrace
              sensorData={sensorData}
              tempHistory={tempHistory}
              humiHistory={humiHistory}
              mq4History={mq4History}
              tcHistory={tcHistory}
            />

            <ThresholdPanel
              thresholds={thresholds}
              isConnected={isConnected}
              onThresholdChange={handleThresholdChange}
              onSaveAll={publishAllThresholds}
            />
          </div>
        </div>

        {/* 底部状态栏 */}
        <footer className="mt-6 pt-4 border-t border-slate-800/50 flex items-center justify-between text-xs text-slate-600">
          <span>EMQX Cloud · MQTT Monitor</span>
          <span>{connectionStatus}</span>
        </footer>
      </div>
    </div>
  )
}
