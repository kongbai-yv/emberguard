import { cn } from '@/lib/utils'

interface SensorCardProps {
  title: string
  value: number | string
  unit?: string
  icon?: React.ReactNode
  alarm?: boolean
  className?: string
}

export function SensorCard({ title, value, unit, icon, alarm, className }: SensorCardProps) {
  return (
    <div
      className={cn(
        'bg-card border border-border rounded-lg p-4 transition-all duration-300',
        alarm && 'border-destructive shadow-[0_0_20px_hsl(var(--destructive)_/_0.3)]',
        className
      )}
    >
      <div className="flex items-start justify-between mb-3">
        <h3 className="text-sm text-muted-foreground font-medium">{title}</h3>
        {icon && (
          <div className="text-primary flex-shrink-0" style={{ filter: 'drop-shadow(0 0 4px hsl(var(--primary-glow) / 0.6))' }}>
            {icon}
          </div>
        )}
      </div>
      <div className="flex items-baseline gap-2">
        <span
          className={cn(
            'text-3xl font-bold tracking-tight',
            alarm && 'text-destructive animate-pulse'
          )}
        >
          {value}
        </span>
        {unit && <span className="text-sm text-muted-foreground">{unit}</span>}
      </div>
    </div>
  )
}
