import { useMemo, useState } from 'react'
import { cn } from '@/lib/utils'
import { Clock, AlertTriangle, ChevronDown, ChevronUp, Flame, Thermometer, Droplets, Wind, Gauge } from 'lucide-react'

interface SensorData {
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

interface HistoryPoint {
  time: string
  value: number
}

interface AlarmEvent {
  id: string
  time: string
  type: 'fire' | 'gas' | 'temp_high' | 'humi_high' | 'tc_high'
  value: number
  threshold: number
  causeChain?: string[]
}

interface AlarmTraceProps {
  sensorData: SensorData | null
  tempHistory: HistoryPoint[]
  humiHistory: HistoryPoint[]
  mq4History: HistoryPoint[]
  tcHistory: HistoryPoint[]
  className?: string
}

const alarmConfig = {
  fire: {
    label: '火焰报警',
    icon: Flame,
    color: 'text-red-400',
    bgColor: 'bg-red-400/10',
    borderColor: 'border-red-400/30',
    threshold: 1,
    unit: '',
    getValue: (data: SensorData) => data.flame,
    getCauseChain: (_data: SensorData, temp: HistoryPoint[], mq4: HistoryPoint[]) => {
      const causes = []
      if (temp.length > 0 && temp[temp.length - 1].value > 30) {
        causes.push('柜内温度偏高，可能加速挥发')
      }
      if (mq4.length > 0 && mq4[mq4.length - 1].value > 200) {
        causes.push('燃气浓度升高，存在泄漏风险')
      }
      causes.push('检查是否有明火或高温热源')
      return causes
    },
  },
  gas: {
    label: '燃气泄漏',
    icon: Wind,
    color: 'text-orange-400',
    bgColor: 'bg-orange-400/10',
    borderColor: 'border-orange-400/30',
    threshold: 400,
    unit: 'ppm',
    getValue: (data: SensorData) => data.mq4,
    getCauseChain: (_data: SensorData, temp: HistoryPoint[]) => {
      const causes = []
      if (temp.length > 0 && temp[temp.length - 1].value > 35) {
        causes.push('高温可能加速化学品挥发')
      }
      causes.push('检查容器是否密封完好')
      causes.push('检查通风系统是否正常工作')
      return causes
    },
  },
  temp_high: {
    label: '温度过高',
    icon: Thermometer,
    color: 'text-yellow-400',
    bgColor: 'bg-yellow-400/10',
    borderColor: 'border-yellow-400/30',
    threshold: 35,
    unit: '℃',
    getValue: (data: SensorData) => data.temp,
    getCauseChain: () => [
      '检查环境温度是否异常',
      '检查柜内是否有放热反应',
      '检查通风散热系统',
    ],
  },
  humi_high: {
    label: '湿度过高',
    icon: Droplets,
    color: 'text-blue-400',
    bgColor: 'bg-blue-400/10',
    borderColor: 'border-blue-400/30',
    threshold: 60,
    unit: '%',
    getValue: (data: SensorData) => data.humi,
    getCauseChain: () => [
      '检查环境湿度变化',
      '检查柜门密封性',
      '检查除湿系统是否正常',
    ],
  },
  tc_high: {
    label: 'PT100超温',
    icon: Gauge,
    color: 'text-pink-400',
    bgColor: 'bg-pink-400/10',
    borderColor: 'border-pink-400/30',
    threshold: 100,
    unit: '℃',
    getValue: (data: SensorData) => data.tc,
    getCauseChain: () => [
      '检查监测点温度是否异常',
      '检查是否有局部过热',
      '建议降低环境温度',
    ],
  },
}

function findAlarmEvents(
  data: SensorData,
  tempHistory: HistoryPoint[],
  _humiHistory: HistoryPoint[],
  mq4History: HistoryPoint[],
  _tcHistory: HistoryPoint[]
): AlarmEvent[] {
  const events: AlarmEvent[] = []
  const now = new Date()

  Object.entries(alarmConfig).forEach(([type, config]) => {
    if (data.alert[type as keyof typeof data.alert]) {
      const causeChain = config.getCauseChain(data, tempHistory, mq4History)

      events.push({
        id: `${type}-${now.getTime()}`,
        time: now.toLocaleTimeString(),
        type: type as AlarmEvent['type'],
        value: config.getValue(data),
        threshold: config.threshold,
        causeChain,
      })
    }
  })

  return events
}

export function AlarmTrace({
  sensorData,
  tempHistory,
  humiHistory,
  mq4History,
  tcHistory,
  className,
}: AlarmTraceProps) {
  const [expandedEvent, setExpandedEvent] = useState<string | null>(null)

  const alarmEvents = useMemo(() => {
    if (!sensorData) return []
    return findAlarmEvents(sensorData, tempHistory, humiHistory, mq4History, tcHistory)
  }, [sensorData, tempHistory, humiHistory, mq4History, tcHistory])

  if (!sensorData) {
    return null
  }

  const hasAlarms = alarmEvents.length > 0

  return (
    <div className={cn('space-y-3', className)}>
      <div className="flex items-center justify-between px-1">
        <div className="flex items-center gap-2">
          <Clock className="w-4 h-4 text-slate-400" />
          <h3 className="text-sm font-medium text-slate-400">
            {hasAlarms ? '异常溯源' : '事件记录'}
          </h3>
        </div>
        {hasAlarms && (
          <span className="text-xs px-2 py-0.5 rounded-full bg-red-400/20 text-red-400">
            {alarmEvents.length} 个报警
          </span>
        )}
      </div>

      {hasAlarms ? (
        <div className="space-y-2">
          {alarmEvents.map((event) => {
            const config = alarmConfig[event.type]
            const Icon = config.icon
            const isExpanded = expandedEvent === event.id

            return (
              <div
                key={event.id}
                className={cn(
                  'rounded-xl border overflow-hidden transition-all duration-300',
                  config.bgColor,
                  config.borderColor
                )}
              >
                <button
                  onClick={() => setExpandedEvent(isExpanded ? null : event.id)}
                  className="w-full p-4 flex items-start gap-3 text-left"
                >
                  <Icon className={cn('w-5 h-5 flex-shrink-0 mt-0.5', config.color)} />
                  <div className="flex-1 min-w-0">
                    <div className="flex items-center justify-between">
                      <h4 className={cn('font-medium text-sm', config.color)}>
                        {config.label}
                      </h4>
                      <span className="text-xs text-slate-500">{event.time}</span>
                    </div>
                    <p className="text-sm text-slate-300 mt-1">
                      当前值{' '}
                      <span className={cn('font-mono font-medium', config.color)}>
                        {event.value.toFixed(1)}
                        {config.unit}
                      </span>{' '}
                      超过阈值{' '}
                      <span className="font-mono text-slate-400">
                        {event.threshold}
                        {config.unit}
                      </span>
                    </p>
                  </div>
                  {isExpanded ? (
                    <ChevronUp className="w-4 h-4 text-slate-500 flex-shrink-0" />
                  ) : (
                    <ChevronDown className="w-4 h-4 text-slate-500 flex-shrink-0" />
                  )}
                </button>

                {isExpanded && event.causeChain && (
                  <div className="px-4 pb-4 pt-0">
                    <div className="pl-8 border-l-2 border-slate-700 space-y-3">
                      <div className="text-xs text-slate-500 font-medium uppercase tracking-wider">
                        可能原因分析
                      </div>
                      {event.causeChain.map((cause, index) => (
                        <div key={index} className="flex items-start gap-3">
                          <div className="w-5 h-5 rounded-full bg-slate-800 flex items-center justify-center flex-shrink-0 mt-0.5">
                            <span className="text-xs text-slate-400">{index + 1}</span>
                          </div>
                          <p className="text-sm text-slate-300">{cause}</p>
                        </div>
                      ))}
                    </div>

                    <button className="mt-4 w-full py-2 px-3 rounded-lg bg-slate-800/50 text-sm text-slate-400 hover:text-slate-300 hover:bg-slate-800 transition-colors">
                      查看完整事件日志
                    </button>
                  </div>
                )}
              </div>
            )
          })}
        </div>
      ) : (
        <div className="p-4 rounded-xl bg-slate-800/30 border border-slate-700/50">
          <div className="flex items-center gap-3 text-slate-500">
            <div className="w-8 h-8 rounded-full bg-emerald-400/10 flex items-center justify-center">
              <AlertTriangle className="w-4 h-4 text-emerald-400" />
            </div>
            <div>
              <p className="text-sm text-slate-300">系统运行正常</p>
              <p className="text-xs text-slate-500">暂无异常事件</p>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}
