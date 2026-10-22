import { AlertTriangle, CheckCircle } from 'lucide-react'
import { cn } from '@/lib/utils'

interface AlarmIndicatorProps {
  label: string
  isActive: boolean
}

export function AlarmIndicator({ label, isActive }: AlarmIndicatorProps) {
  return (
    <div
      className={cn(
        'flex items-center justify-between px-4 py-3 rounded-md border transition-all duration-300',
        isActive
          ? 'bg-destructive/10 border-destructive alarm-active'
          : 'bg-secondary border-border'
      )}
    >
      <div className="flex items-center gap-3">
        {isActive ? (
          <AlertTriangle className="w-5 h-5 text-destructive flex-shrink-0" style={{ filter: 'drop-shadow(0 0 4px hsl(var(--destructive) / 0.6))' }} />
        ) : (
          <CheckCircle className="w-5 h-5 text-success flex-shrink-0" style={{ filter: 'drop-shadow(0 0 4px hsl(var(--success) / 0.6))' }} />
        )}
        <span className={cn('font-medium', isActive ? 'text-destructive' : 'text-foreground')}>
          {label}
        </span>
      </div>
      <span
        className={cn(
          'text-xs font-semibold px-2 py-1 rounded',
          isActive
            ? 'bg-destructive text-destructive-foreground'
            : 'bg-success/20 text-success'
        )}
      >
        {isActive ? '报警' : '正常'}
      </span>
    </div>
  )
}
