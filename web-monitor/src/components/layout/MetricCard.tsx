import { cn } from '@/lib/utils'

interface MetricCardProps {
  label: string
  value: number | string
  unit?: string
  icon: React.ReactNode
  color: string
  alarm?: boolean
  trend?: 'up' | 'down' | 'stable'
  className?: string
}

export function MetricCard({
  label,
  value,
  unit,
  icon,
  color,
  alarm = false,
  trend,
  className,
}: MetricCardProps) {
  const displayValue = typeof value === 'number' ? value.toFixed(1) : value

  return (
    <div
      className={cn(
        'relative p-4 rounded-xl border transition-all duration-500 group',
        alarm
          ? 'bg-red-500/5 border-red-500/30 shadow-[0_0_20px_rgba(239,68,68,0.1)]'
          : 'bg-slate-900/50 border-slate-800 hover:border-slate-700',
        className
      )}
    >
      {/* 背景光晕 */}
      <div
        className={cn(
          'absolute inset-0 rounded-xl opacity-0 group-hover:opacity-100 transition-opacity duration-500',
          alarm ? 'bg-red-500/5' : ''
        )}
        style={{
          background: alarm
            ? undefined
            : `radial-gradient(circle at 50% 50%, ${color}10, transparent 70%)`,
        }}
      />

      <div className="relative flex items-start justify-between">
        <div className="space-y-2">
          <div className="flex items-center gap-2">
            <div
              className="w-6 h-6 rounded-md flex items-center justify-center"
              style={{ backgroundColor: `${color}20` }}
            >
              <div style={{ color }} className="w-3.5 h-3.5">
                {icon}
              </div>
            </div>
            <span className="text-xs font-medium text-slate-400 uppercase tracking-wider">
              {label}
            </span>
          </div>

          <div className="flex items-baseline gap-1.5">
            <span
              className={cn(
                'text-2xl font-light tracking-tight',
                alarm ? 'text-red-400 animate-pulse' : 'text-white'
              )}
            >
              {displayValue}
            </span>
            {unit && (
              <span className="text-sm text-slate-500 font-light">{unit}</span>
            )}
          </div>
        </div>

        {/* 趋势指示 */}
        {trend && (
          <div
            className={cn(
              'w-5 h-5 rounded-full flex items-center justify-center',
              trend === 'up'
                ? 'bg-orange-400/20 text-orange-400'
                : trend === 'down'
                  ? 'bg-blue-400/20 text-blue-400'
                  : 'bg-slate-700/50 text-slate-500'
            )}
          >
            {trend === 'up' ? (
              <svg className="w-3 h-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 15l7-7 7 7" />
              </svg>
            ) : trend === 'down' ? (
              <svg className="w-3 h-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" />
              </svg>
            ) : (
              <svg className="w-3 h-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 12h14" />
              </svg>
            )}
          </div>
        )}
      </div>

      {/* 报警指示条 */}
      {alarm && (
        <div className="absolute bottom-0 left-4 right-4 h-0.5 bg-gradient-to-r from-transparent via-red-500 to-transparent animate-pulse" />
      )}
    </div>
  )
}
