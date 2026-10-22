import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts'

interface DataPoint {
  time: string
  value: number
}

interface TrendChartProps {
  title: string
  data: DataPoint[]
  color: string
  unit: string
}

export function TrendChart({ title, data, color, unit }: TrendChartProps) {
  return (
    <div className="bg-card border border-border rounded-lg p-4" style={{ minHeight: '280px' }}>
      <h3 className="text-sm text-muted-foreground mb-4">{title}</h3>
      <div style={{ width: '100%', height: '200px' }}>
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={data} margin={{ top: 5, right: 20, left: 0, bottom: 5 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="hsl(var(--border))" opacity={0.3} />
            <XAxis
              dataKey="time"
              stroke="hsl(var(--muted-foreground))"
              fontSize={11}
              tickMargin={8}
              tick={{ fill: 'hsl(var(--muted-foreground))' }}
            />
            <YAxis
              stroke="hsl(var(--muted-foreground))"
              fontSize={11}
              tickMargin={8}
              tick={{ fill: 'hsl(var(--muted-foreground))' }}
              label={{ 
                value: unit, 
                angle: -90, 
                position: 'insideLeft', 
                fill: 'hsl(var(--muted-foreground))',
                style: { textAnchor: 'middle' }
              }}
            />
            <Tooltip
              contentStyle={{
                backgroundColor: 'hsl(var(--card))',
                border: '1px solid hsl(var(--border))',
                borderRadius: '6px',
                color: 'hsl(var(--foreground))',
                boxShadow: '0 4px 6px -1px rgba(0, 0, 0, 0.3)',
              }}
              labelStyle={{ color: 'hsl(var(--muted-foreground))' }}
              itemStyle={{ color: `hsl(${color})` }}
            />
            <Line
              type="monotone"
              dataKey="value"
              stroke={`hsl(${color})`}
              strokeWidth={2.5}
              dot={{ r: 3, fill: `hsl(${color})`, strokeWidth: 1, stroke: 'hsl(var(--card))' }}
              activeDot={{ r: 5, fill: `hsl(${color})`, strokeWidth: 2, stroke: 'hsl(var(--card))' }}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
      {data.length === 0 && (
        <div className="text-center text-muted-foreground text-sm mt-4">
          等待数据...
        </div>
      )}
    </div>
  )
}
