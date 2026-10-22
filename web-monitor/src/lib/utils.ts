import { clsx, type ClassValue } from "clsx"

export function cn(...inputs: ClassValue[]) {
  return clsx(inputs)
}

export function formatNumber(value: number, decimals: number = 1): string {
  return value.toFixed(decimals)
}

export function getAlarmColor(isActive: boolean): string {
  return isActive ? 'text-destructive' : 'text-success'
}

export function getStatusText(status: boolean): string {
  return status ? '报警' : '正常'
}
