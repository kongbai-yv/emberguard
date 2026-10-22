import { useState } from 'react'
import { cn } from '@/lib/utils'

interface ThresholdInputProps {
  label: string
  field: string
  value: number
  unit: string
  onChange: (field: string, value: number) => void
}

export function ThresholdInput({ label, field, value, unit, onChange }: ThresholdInputProps) {
  const [inputValue, setInputValue] = useState(value.toString())
  const [_isEditing, setIsEditing] = useState(false)

  const handleBlur = () => {
    setIsEditing(false)
    const numValue = parseFloat(inputValue)
    if (!isNaN(numValue) && numValue !== value) {
      onChange(field, numValue)
    } else {
      setInputValue(value.toString())
    }
  }

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter') {
      handleBlur()
    }
  }

  return (
    <div className="space-y-2">
      <label className="text-sm text-muted-foreground block">{label}</label>
      <div className="relative">
        <input
          type="number"
          step="0.1"
          value={inputValue}
          onChange={(e) => setInputValue(e.target.value)}
          onFocus={() => setIsEditing(true)}
          onBlur={handleBlur}
          onKeyDown={handleKeyDown}
          className={cn(
            'w-full bg-input border border-border rounded-md px-3 py-2 pr-12',
            'text-foreground text-lg font-mono',
            'focus:outline-none focus:border-primary focus:ring-1 focus:ring-primary',
            'transition-colors duration-200'
          )}
        />
        <span className="absolute right-3 top-1/2 -translate-y-1/2 text-sm text-muted-foreground">
          {unit}
        </span>
      </div>
    </div>
  )
}
