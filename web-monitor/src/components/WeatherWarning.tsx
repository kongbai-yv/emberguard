import { Cloud, CloudRain, TrendingUp, TrendingDown } from 'lucide-react'
import { cn } from '@/lib/utils'

interface WeatherWarningProps {
  stormWarning: boolean
  pressure: number
  altitude: number
}

export function WeatherWarning({ stormWarning, pressure, altitude }: WeatherWarningProps) {
  const isStormWarning = stormWarning
  
  return (
    <div className={cn(
      'bg-card border rounded-lg p-6 transition-all duration-300',
      isStormWarning 
        ? 'border-destructive shadow-[0_0_20px_hsl(var(--destructive)_/_0.3)]' 
        : 'border-border'
    )}>
      {/* 标题栏 */}
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-sm text-muted-foreground font-medium">天气预警</h3>
        {isStormWarning ? (
          <CloudRain className="w-5 h-5 text-destructive flex-shrink-0" style={{ filter: 'drop-shadow(0 0 4px hsl(var(--destructive) / 0.6))' }} />
        ) : (
          <Cloud className="w-5 h-5 text-success flex-shrink-0" style={{ filter: 'drop-shadow(0 0 4px hsl(var(--success) / 0.6))' }} />
        )}
      </div>

      {/* 预警状态 */}
      <div className={cn(
        'flex items-center gap-3 px-4 py-3 rounded-md mb-4',
        isStormWarning ? 'bg-destructive/10' : 'bg-success/10'
      )}>
        {isStormWarning ? (
          <>
            <TrendingDown className="w-5 h-5 text-destructive" />
            <div>
              <div className="text-destructive font-semibold">风暴预警</div>
              <div className="text-xs text-destructive/80">气压骤降，可能有恶劣天气</div>
            </div>
          </>
        ) : (
          <>
            <TrendingUp className="w-5 h-5 text-success" />
            <div>
              <div className="text-success font-semibold">天气正常</div>
              <div className="text-xs text-success/80">气压稳定</div>
            </div>
          </>
        )}
      </div>

      {/* 气压显示 */}
      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <span className="text-sm text-muted-foreground">大气压</span>
          <div className="flex items-baseline gap-2">
            <span className="text-2xl font-bold text-foreground">{pressure.toFixed(2)}</span>
            <span className="text-sm text-muted-foreground">hPa</span>
          </div>
        </div>

        {/* 气压进度条 */}
        <div className="relative h-3 bg-secondary rounded-full overflow-hidden">
          <div
            className={cn(
              'absolute left-0 top-0 h-full transition-all duration-500 ease-out',
              isStormWarning ? 'bg-destructive' : 'bg-primary'
            )}
            style={{ width: `${Math.min(100, Math.max(0, (pressure - 950) / 100 * 100))}%` }}
          />
        </div>

        {/* 海拔显示 */}
        <div className="flex items-center justify-between pt-2 border-t border-border">
          <span className="text-sm text-muted-foreground">估算海拔</span>
          <div className="flex items-baseline gap-2">
            <span className="text-xl font-bold text-foreground">{altitude.toFixed(1)}</span>
            <span className="text-sm text-muted-foreground">m</span>
          </div>
        </div>
      </div>

      {/* 预警说明 */}
      {isStormWarning && (
        <div className="mt-4 px-3 py-2 bg-destructive/10 border border-destructive rounded text-xs text-destructive">
          <strong>注意：</strong>5分钟内气压下降 ≥ 1.0 hPa，请做好防风暴准备
        </div>
      )}
    </div>
  )
}
