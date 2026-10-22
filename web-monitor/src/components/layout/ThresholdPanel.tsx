import { useState, useCallback } from 'react'
import { cn } from '@/lib/utils'
import { Settings, Save, Check } from 'lucide-react'

interface ThresholdConfig {
  temp_high: number
  humi_high: number
  mq4_high: number
  tc_high: number
}

interface ThresholdPanelProps {
  thresholds: ThresholdConfig
  isConnected: boolean
  onThresholdChange: (field: string, value: number) => void
  onSaveAll: () => void
  className?: string
}

const thresholdFields = [
  { key: 'temp_high', label: '温度上限', unit: '℃', min: 20, max: 60, step: 0.5 },
  { key: 'humi_high', label: '湿度上限', unit: '%', min: 30, max: 90, step: 1 },
  { key: 'mq4_high', label: '燃气浓度上限', unit: 'ppm', min: 100, max: 1000, step: 50 },
  { key: 'tc_high', label: 'PT100温度上限', unit: '℃', min: 50, max: 200, step: 5 },
]

export function ThresholdPanel({
  thresholds,
  isConnected,
  onThresholdChange,
  onSaveAll,
  className,
}: ThresholdPanelProps) {
  const [localValues, setLocalValues] = useState<ThresholdConfig>(thresholds)
  const [isSaved, setIsSaved] = useState(false)

  const handleChange = useCallback(
    (key: string, value: string) => {
      const numValue = parseFloat(value)
      if (!isNaN(numValue)) {
        setLocalValues((prev) => ({ ...prev, [key]: numValue }))
      }
    },
    []
  )

  const handleBlur = useCallback(
    (key: string) => {
      const value = localValues[key as keyof ThresholdConfig]
      if (value !== thresholds[key as keyof ThresholdConfig]) {
        onThresholdChange(key, value)
      }
    },
    [localValues, thresholds, onThresholdChange]
  )

  const handleSaveAll = useCallback(() => {
    onSaveAll()
    setIsSaved(true)
    setTimeout(() => setIsSaved(false), 2000)
  }, [onSaveAll])

  return (
    <div className={cn('space-y-4', className)}>
      <div className="flex items-center justify-between px-1">
        <div className="flex items-center gap-2">
          <Settings className="w-4 h-4 text-slate-400" />
          <h3 className="text-sm font-medium text-slate-400">阈值配置</h3>
        </div>
        <button
          onClick={handleSaveAll}
          disabled={!isConnected}
          className={cn(
            'flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs transition-all',
            isConnected
              ? isSaved
                ? 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30'
                : 'bg-indigo-500/10 text-indigo-400 border border-indigo-500/20 hover:bg-indigo-500/20'
              : 'bg-slate-800 text-slate-500 border border-slate-700 cursor-not-allowed'
          )}
        >
          {isSaved ? (
            <>
              <Check className="w-3 h-3" />
              已保存
            </>
          ) : (
            <>
              <Save className="w-3 h-3" />
              保存全部
            </>
          )}
        </button>
      </div>

      <div className="grid grid-cols-2 gap-3">
        {thresholdFields.map(({ key, label, unit, min, max, step }) => (
          <div
            key={key}
            className="p-3 rounded-xl bg-slate-900/50 border border-slate-800 space-y-2"
          >
            <label className="text-xs text-slate-500">{label}</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                min={min}
                max={max}
                step={step}
                value={localValues[key as keyof ThresholdConfig]}
                onChange={(e) => handleChange(key, e.target.value)}
                onBlur={() => handleBlur(key)}
                className="flex-1 bg-slate-800/50 border border-slate-700 rounded-lg px-3 py-2 text-sm text-white font-mono focus:outline-none focus:border-indigo-500 transition-colors"
              />
              <span className="text-xs text-slate-500 w-8">{unit}</span>
            </div>
          </div>
        ))}
      </div>
    </div>
  )
}
