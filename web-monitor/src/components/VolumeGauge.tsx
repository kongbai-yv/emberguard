import { cn } from '@/lib/utils'

interface VolumeGaugeProps {
  percentage: number
  label?: string
}

export function VolumeGauge({ percentage, label = '剩余容积' }: VolumeGaugeProps) {
  const clampedPercentage = Math.max(0, Math.min(100, percentage))
  const isEmpty = clampedPercentage <= 10
  const isFull = clampedPercentage >= 90
  const isLow = clampedPercentage <= 30

  let barColor = 'bg-primary'
  if (isEmpty) barColor = 'bg-destructive'
  else if (isLow) barColor = 'bg-warning'
  else if (isFull) barColor = 'bg-success'

  return (
    <div className="bg-card border border-border rounded-lg p-6">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-sm text-muted-foreground font-medium">{label}</h3>
        <span className="text-2xl font-bold">{clampedPercentage.toFixed(1)}%</span>
      </div>

      <div className="relative h-8 bg-secondary rounded-full overflow-hidden border border-border">
        <div
          className={cn(
            'absolute left-0 top-0 h-full transition-all duration-500 ease-out',
            barColor
          )}
          style={{ width: `${clampedPercentage}%` }}
        />

        <div className="absolute inset-0 flex items-center justify-center">
          <span className="text-xs font-semibold text-foreground drop-shadow-md">
            {clampedPercentage > 15 ? `${clampedPercentage.toFixed(0)}%` : ''}
          </span>
        </div>
      </div>

      <div className="flex justify-between mt-2 text-xs text-muted-foreground">
        <span>空</span>
        <span>满</span>
      </div>

      {isEmpty && (
        <div className="mt-3 px-3 py-2 bg-destructive/10 border border-destructive rounded text-xs text-destructive text-center">
          容器已空
        </div>
      )}
      {isFull && (
        <div className="mt-3 px-3 py-2 bg-success/10 border border-success rounded text-xs text-success text-center">
          容器已满
        </div>
      )}
    </div>
  )
}
