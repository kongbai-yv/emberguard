import { useMemo } from 'react'
import { cn } from '@/lib/utils'
import { Activity, AlertTriangle, CheckCircle, TrendingUp, TrendingDown, Minus } from 'lucide-react'

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

interface SmartSummaryProps {
  sensorData: SensorData | null
  tempHistory: HistoryPoint[]
  humiHistory: HistoryPoint[]
  mq4History: HistoryPoint[]
  className?: string
}

type Trend = 'rising' | 'falling' | 'stable'

interface SummaryInsight {
  icon: 'success' | 'warning' | 'danger' | 'trend-up' | 'trend-down' | 'stable'
  title: string
  description: string
  detail?: string
}

function getTrend(history: HistoryPoint[]): Trend {
  if (history.length < 3) return 'stable'
  const recent = history.slice(-5)
  const first = recent[0].value
  const last = recent[recent.length - 1].value
  const diff = last - first
  const threshold = first * 0.05 // 5%变化阈值

  if (diff > threshold) return 'rising'
  if (diff < -threshold) return 'falling'
  return 'stable'
}

function getTrendIcon(trend: Trend) {
  switch (trend) {
    case 'rising':
      return TrendingUp
    case 'falling':
      return TrendingDown
    default:
      return Minus
  }
}

function generateInsights(
  data: SensorData,
  tempHistory: HistoryPoint[],
  humiHistory: HistoryPoint[],
  mq4History: HistoryPoint[]
): SummaryInsight[] {
  const insights: SummaryInsight[] = []
  const hasAlarm = Object.values(data.alert).some((v) => v)

  // 主状态
  if (hasAlarm) {
    const alarms = []
    if (data.alert.fire) alarms.push('火焰')
    if (data.alert.gas) alarms.push('燃气泄漏')
    if (data.alert.temp_high) alarms.push('温度过高')
    if (data.alert.humi_high) alarms.push('湿度过高')
    if (data.alert.tc_high) alarms.push('PT100超温')

    insights.push({
      icon: 'danger',
      title: '报警触发',
      description: `检测到${alarms.join('、')}报警，需要立即处理`,
      detail: '请检查柜内化学品状态，必要时启动应急预案',
    })
  } else {
    insights.push({
      icon: 'success',
      title: '系统正常运行',
      description: '所有传感器指标均在安全范围内',
    })
  }

  // 温度趋势分析
  const tempTrend = getTrend(tempHistory)
  if (tempTrend === 'rising' && data.temp > 30) {
    insights.push({
      icon: 'trend-up',
      title: '温度上升趋势',
      description: `当前温度 ${data.temp.toFixed(1)}℃，近5分钟持续上升`,
      detail: '建议检查通风系统或降低柜内热源',
    })
  } else if (tempTrend === 'falling') {
    insights.push({
      icon: 'trend-down',
      title: '温度回落',
      description: `温度已降至 ${data.temp.toFixed(1)}℃，趋于稳定`,
    })
  }

  // 湿度关联分析
  const humiTrend = getTrend(humiHistory)
  if (humiTrend === 'rising' && data.humi > 55) {
    insights.push({
      icon: 'warning',
      title: '湿度偏高',
      description: `当前湿度 ${data.humi.toFixed(1)}%，可能影响化学品稳定性`,
      detail: '高湿度环境可能加速某些化学品的分解',
    })
  }

  // 燃气浓度分析
  const mq4Trend = getTrend(mq4History)
  if (mq4Trend === 'rising') {
    insights.push({
      icon: 'warning',
      title: '燃气浓度上升',
      description: `MQ-4读数 ${data.mq4.toFixed(0)}ppm，呈上升趋势`,
      detail: '建议检查是否有泄漏源，确保通风良好',
    })
  }

  // 风暴预警
  if (data.storm_warning) {
    insights.push({
      icon: 'warning',
      title: '恶劣天气预警',
      description: '气压骤降，可能有暴风雨来临',
      detail: '请确保柜门关闭，做好防护措施',
    })
  }

  return insights
}

const iconMap = {
  success: CheckCircle,
  warning: AlertTriangle,
  danger: AlertTriangle,
  'trend-up': TrendingUp,
  'trend-down': TrendingDown,
  stable: Minus,
}

const colorMap = {
  success: 'text-emerald-400 bg-emerald-400/10 border-emerald-400/20',
  warning: 'text-amber-400 bg-amber-400/10 border-amber-400/20',
  danger: 'text-red-400 bg-red-400/10 border-red-400/20',
  'trend-up': 'text-orange-400 bg-orange-400/10 border-orange-400/20',
  'trend-down': 'text-blue-400 bg-blue-400/10 border-blue-400/20',
  stable: 'text-slate-400 bg-slate-400/10 border-slate-400/20',
}

export function SmartSummary({
  sensorData,
  tempHistory,
  humiHistory,
  mq4History,
  className,
}: SmartSummaryProps) {
  const insights = useMemo(() => {
    if (!sensorData) return []
    return generateInsights(sensorData, tempHistory, humiHistory, mq4History)
  }, [sensorData, tempHistory, humiHistory, mq4History])

  if (!sensorData) {
    return (
      <div className={cn('p-6 rounded-xl bg-slate-900/50 border border-slate-800', className)}>
        <div className="flex items-center gap-2 text-slate-500">
          <Activity className="w-4 h-4" />
          <span className="text-sm">等待数据接入...</span>
        </div>
      </div>
    )
  }

  return (
    <div className={cn('space-y-3', className)}>
      <div className="flex items-center gap-2 px-1">
        <Activity className="w-4 h-4 text-indigo-400" />
        <h3 className="text-sm font-medium text-slate-400">智能分析</h3>
      </div>

      {insights.map((insight, index) => {
        const Icon = iconMap[insight.icon]
        const colors = colorMap[insight.icon]

        return (
          <div
            key={index}
            className={cn(
              'p-4 rounded-xl border transition-all duration-500',
              colors,
              index === 0 && 'ring-1 ring-opacity-50'
            )}
          >
            <div className="flex items-start gap-3">
              <Icon className="w-5 h-5 flex-shrink-0 mt-0.5" />
              <div className="flex-1 min-w-0">
                <h4 className="font-medium text-sm">{insight.title}</h4>
                <p className="text-sm opacity-80 mt-1">{insight.description}</p>
                {insight.detail && (
                  <p className="text-xs opacity-60 mt-2 pl-0 border-l-2 border-current ml-0">
                    {insight.detail}
                  </p>
                )}
              </div>
            </div>
          </div>
        )
      })}

      {/* 趋势指示器 */}
      <div className="grid grid-cols-3 gap-2 pt-2">
        {[
          { label: '温度', history: tempHistory, value: sensorData.temp, unit: '℃' },
          { label: '湿度', history: humiHistory, value: sensorData.humi, unit: '%' },
          { label: '燃气', history: mq4History, value: sensorData.mq4, unit: 'ppm' },
        ].map(({ label, history, value, unit }) => {
          const trend = getTrend(history)
          const TrendIcon = getTrendIcon(trend)
          const trendColor =
            trend === 'rising'
              ? 'text-orange-400'
              : trend === 'falling'
                ? 'text-blue-400'
                : 'text-slate-400'

          return (
            <div
              key={label}
              className="p-2 rounded-lg bg-slate-800/50 border border-slate-700/50"
            >
              <div className="text-xs text-slate-500 mb-1">{label}</div>
              <div className="flex items-center justify-between">
                <span className="text-sm font-medium text-slate-200">
                  {value.toFixed(1)}
                  <span className="text-xs text-slate-500 ml-0.5">{unit}</span>
                </span>
                <TrendIcon className={cn('w-3 h-3', trendColor)} />
              </div>
            </div>
          )
        })}
      </div>
    </div>
  )
}
