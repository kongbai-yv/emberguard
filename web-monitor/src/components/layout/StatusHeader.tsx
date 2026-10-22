import { cn } from '@/lib/utils'
import { Wifi, WifiOff, Activity, Clock, Server } from 'lucide-react'

interface StatusHeaderProps {
  mqttConnected: boolean
  deviceOnline: boolean
  connectionStatus: string
  lastDataTime: Date | null
  dataTimeout: boolean
  onReconnect: () => void
  className?: string
}

export function StatusHeader({
  mqttConnected,
  deviceOnline,
  connectionStatus: _connectionStatus,
  lastDataTime,
  dataTimeout,
  onReconnect,
  className,
}: StatusHeaderProps) {
  return (
    <header className={cn('flex items-center justify-between', className)}>
      {/* 左侧标题 */}
      <div>
        <h1 className="text-xl font-light tracking-tight text-white">
          <span className="text-indigo-400">豫安</span>
          <span className="text-slate-500 mx-2">·</span>
          <span className="text-slate-300">危化品安全监控</span>
        </h1>
        <p className="text-xs text-slate-500 mt-0.5 flex items-center gap-2">
          <Server className="w-3 h-3" />
          MingKai Cloud · 实时数据采集与分析
        </p>
      </div>

      {/* 右侧状态 */}
      <div className="flex items-center gap-3">
        {/* 连接状态 */}
        <div
          className={cn(
            'flex items-center gap-2 px-3 py-1.5 rounded-lg border text-xs',
            mqttConnected
              ? 'bg-emerald-500/5 border-emerald-500/20 text-emerald-400'
              : 'bg-red-500/5 border-red-500/20 text-red-400'
          )}
        >
          {mqttConnected ? (
            <>
              <Wifi className="w-3.5 h-3.5" />
              <span>MQTT已连接</span>
            </>
          ) : (
            <>
              <WifiOff className="w-3.5 h-3.5" />
              <span>MQTT断开</span>
            </>
          )}
        </div>

        {/* 设备状态 */}
        {mqttConnected && (
          <div
            className={cn(
              'flex items-center gap-2 px-3 py-1.5 rounded-lg border text-xs',
              deviceOnline
                ? 'bg-emerald-500/5 border-emerald-500/20 text-emerald-400'
                : dataTimeout
                  ? 'bg-amber-500/5 border-amber-500/20 text-amber-400'
                  : 'bg-slate-800 border-slate-700 text-slate-400'
            )}
          >
            <div
              className={cn(
                'w-1.5 h-1.5 rounded-full',
                deviceOnline
                  ? 'bg-emerald-400 animate-pulse'
                  : dataTimeout
                    ? 'bg-amber-400'
                    : 'bg-slate-500'
              )}
            />
            <span>
              {deviceOnline ? '设备在线' : dataTimeout ? '设备离线' : '等待数据'}
            </span>
          </div>
        )}

        {/* 时间 */}
        {lastDataTime && (
          <div className="flex items-center gap-1.5 text-xs text-slate-500">
            <Clock className="w-3 h-3" />
            <span>{lastDataTime.toLocaleTimeString()}</span>
          </div>
        )}

        {/* 重连按钮 */}
        {!mqttConnected && (
          <button
            onClick={onReconnect}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-indigo-500/10 border border-indigo-500/20 text-indigo-400 text-xs hover:bg-indigo-500/20 transition-colors"
          >
            <Activity className="w-3 h-3" />
            重新连接
          </button>
        )}
      </div>
    </header>
  )
}
