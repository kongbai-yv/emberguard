import { useEffect, useRef } from 'react'
import { CabinetScene, type SensorData } from './CabinetScene'
import { cn } from '@/lib/utils'

interface CabinetDisplayProps {
  sensorData: SensorData | null
  className?: string
}

export function CabinetDisplay({ sensorData, className }: CabinetDisplayProps) {
  const containerRef = useRef<HTMLDivElement>(null)
  const sceneRef = useRef<CabinetScene | null>(null)
  const sensorRef = useRef<SensorData | null>(null)

  sensorRef.current = sensorData

  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    let disposed = false

    const initScene = () => {
      if (disposed) return
      if (el.clientWidth < 10 || el.clientHeight < 10) {
        // 容器还没渲染出尺寸，等下一帧再试
        requestAnimationFrame(initScene)
        return
      }
      const scene = new CabinetScene(el)
      sceneRef.current = scene
      // 把已有的数据立即灌进去
      if (sensorRef.current) {
        scene.updateData(sensorRef.current)
      }
    }

    initScene()

    return () => {
      disposed = true
      if (sceneRef.current) {
        sceneRef.current.dispose()
        sceneRef.current = null
      }
    }
  }, [])

  useEffect(() => {
    if (sceneRef.current && sensorData) {
      sceneRef.current.updateData(sensorData)
    }
  }, [sensorData])

  return (
    <div
      ref={containerRef}
      className={cn('w-full h-full rounded-xl overflow-hidden', className)}
      style={{ minHeight: '400px' }}
    />
  )
}
