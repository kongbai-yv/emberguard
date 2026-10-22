<template>
	<view class="container">
		<!-- 顶部导航栏 -->
		<view class="header">
			<view class="header-content">
				<text class="app-title">豫安实时监测系统</text>
				<text class="app-subtitle">EMBERGUARD IoT Platform</text>
			</view>
			<view class="status-indicator" :class="connectionStatus === '已连接' ? 'connected' : 'disconnected'">
				<view class="status-dot"></view>
				<text class="status-text">{{connectionStatus}}</text>
			</view>
		</view>

		<!-- 预警信息条 -->
		<view class="alert-banner" v-if="hasAlerts">
			<view class="alert-item" v-for="(alert, index) in activeAlerts" :key="index">
				<text class="alert-label">{{alert.label}}</text>
			</view>
		</view>

		<!-- 加载提示 -->
		<view class="loading-box" v-if="loading">
			<text class="loading-text">正在连接设备...</text>
		</view>

		<!-- 主要内容区 -->
		<view class="main-content" v-if="!loading && !error">
			<!-- 关键指标卡片 -->
			<view class="metrics-grid">
				<!-- 燃气浓度 -->
				<view class="metric-card">
					<view class="metric-header">
						<text class="metric-title">燃气浓度</text>
						<text class="metric-unit">ppm</text>
					</view>
					<view class="metric-value" :class="{'value-warning': sensorData.mq4 > thresholds.mq4_high}">
						{{sensorData.mq4.toFixed(1)}}
					</view>
					<view class="metric-footer">
						<text class="threshold-label">阈值: {{thresholds.mq4_high}}</text>
					</view>
				</view>

				<!-- 环境温度 -->
				<view class="metric-card">
					<view class="metric-header">
						<text class="metric-title">环境温度</text>
						<text class="metric-unit">°C</text>
					</view>
					<view class="metric-value" :class="{'value-warning': sensorData.temp > thresholds.temp_high}">
						{{sensorData.temp.toFixed(1)}}
					</view>
					<view class="metric-footer">
						<text class="threshold-label">阈值: {{thresholds.temp_high}}</text>
					</view>
				</view>

				<!-- 环境湿度 -->
				<view class="metric-card">
					<view class="metric-header">
						<text class="metric-title">环境湿度</text>
						<text class="metric-unit">%</text>
					</view>
					<view class="metric-value" :class="{'value-warning': sensorData.humi > thresholds.humi_high}">
						{{sensorData.humi.toFixed(1)}}
					</view>
					<view class="metric-footer">
						<text class="threshold-label">阈值: {{thresholds.humi_high}}</text>
					</view>
				</view>

				<!-- PT100温度 -->
				<view class="metric-card">
					<view class="metric-header">
						<text class="metric-title">PT100温度</text>
						<text class="metric-unit">°C</text>
					</view>
					<view class="metric-value" :class="{'value-warning': sensorData.tc > thresholds.tc_high}">
						{{sensorData.tc.toFixed(1)}}
					</view>
					<view class="metric-footer">
						<text class="threshold-label">阈值: {{thresholds.tc_high}}</text>
					</view>
				</view>
			</view>

			<!-- 次要数据列表 -->
			<view class="secondary-data">
				<view class="data-row">
					<view class="data-item">
						<text class="data-label">火焰检测</text>
						<text class="data-value" :class="sensorData.flame === 1 ? 'status-danger' : 'status-normal'">
							{{sensorData.flame === 1 ? '检测到' : '正常'}}
						</text>
					</view>
					<view class="data-item">
						<text class="data-label">火焰电压</text>
						<text class="data-value">{{sensorData.flame_mv}} mV</text>
					</view>
				</view>
				<view class="data-row">
					<view class="data-item">
						<text class="data-label">大气压</text>
						<text class="data-value">{{sensorData.pressure.toFixed(2)}} hPa</text>
					</view>
					<view class="data-item">
						<text class="data-label">海拔高度</text>
						<text class="data-value">{{sensorData.altitude.toFixed(2)}} m</text>
					</view>
				</view>
				<view class="data-row">
					<view class="data-item">
						<text class="data-label">风暴预警</text>
						<text class="data-value" :class="sensorData.storm_warning ? 'status-danger' : 'status-normal'">
							{{sensorData.storm_warning ? '有预警' : '无预警'}}
						</text>
					</view>
					<view class="data-item">
						<text class="data-label">更新时间</text>
						<text class="data-value time-value">{{lastUpdateTime || '--:--:--'}}</text>
					</view>
				</view>
			</view>

			<!-- 操作按钮 -->
			<view class="action-buttons">
				<view class="action-btn primary" @click="showHistory">
					<text class="btn-text">历史数据</text>
				</view>
				<view class="action-btn secondary" @click="showThresholdModal">
					<text class="btn-text">阈值设置</text>
				</view>
			</view>
		</view>

		<!-- 历史数据弹窗 -->
		<view class="modal" v-if="showHistoryModal">
			<view class="modal-content history-modal">
				<view class="modal-header">
					<text class="modal-title">历史数据</text>
					<text class="modal-close" @click="closeHistoryModal">×</text>
				</view>
				<view class="modal-body history-body">
					<scroll-view class="history-list" scroll-y="true">
						<view class="history-item" v-for="(item, index) in historyData" :key="index">
							<view class="history-time">{{item.time}}</view>
							<view class="history-data">
								<text class="data-item">燃气: {{item.mq4}}ppm</text>
								<text class="data-item">温度: {{item.temp}}°C</text>
								<text class="data-item">湿度: {{item.humi}}%</text>
								<text class="data-item">气压: {{item.pressure}}hPa</text>
							</view>
						</view>
						<view class="empty-tip" v-if="historyData.length === 0">
							<text>暂无历史数据</text>
						</view>
					</scroll-view>
				</view>
			</view>
		</view>

		<!-- 阈值设置弹窗 -->
		<view class="modal" v-if="showModal">
			<view class="modal-content">
				<view class="modal-header">
					<text class="modal-title">阈值设置</text>
					<text class="modal-close" @click="closeModal">×</text>
				</view>
				<view class="modal-body">
					<view class="input-group">
						<text class="input-label">温度阈值 (°C)</text>
						<input class="input-field" type="digit" v-model="thresholds.temp_high" placeholder="请输入温度阈值" />
					</view>
					<view class="input-group">
						<text class="input-label">湿度阈值 (%)</text>
						<input class="input-field" type="digit" v-model="thresholds.humi_high" placeholder="请输入湿度阈值" />
					</view>
					<view class="input-group">
						<text class="input-label">燃气阈值 (ppm)</text>
						<input class="input-field" type="digit" v-model="thresholds.mq4_high" placeholder="请输入燃气阈值" />
					</view>
					<view class="input-group">
						<text class="input-label">PT100阈值 (°C)</text>
						<input class="input-field" type="digit" v-model="thresholds.tc_high" placeholder="请输入PT100阈值" />
					</view>
				</view>
				<view class="modal-footer">
					<view class="modal-btn cancel-btn" @click="closeModal">取消</view>
					<view class="modal-btn confirm-btn" @click="submitThreshold">确认下发</view>
				</view>
			</view>
		</view>
	</view>
</template>

<script>
	import sensorService from '../../utils/sensor.js'

	export default {
		data() {
			return {
				sensorData: {
					mq4: 0,
					flame: 0,
					flame_mv: 0,
					temp: 0,
					humi: 0,
					tc: 0,
					pressure: 0,
					altitude: 0,
					storm_warning: false,
					alert: {
						fire: false,
						gas: false,
						temp_high: false,
						humi_high: false,
						tc_high: false
					}
				},
				lastUpdateTime: '',
				connectionStatus: '未连接',
				showModal: false,
				showHistoryModal: false,
				thresholds: {
					temp_high: 35,
					humi_high: 60,
					mq4_high: 400,
					tc_high: 100
				},
				updateTimer: null,
				loading: true,
				error: '',
				historyData: []
			}
		},
		computed: {
			hasAlerts() {
				const alert = this.sensorData.alert
				return alert.fire || alert.gas || alert.temp_high || alert.humi_high || alert.tc_high
			},
			activeAlerts() {
				const alerts = []
				if (this.sensorData.alert.fire) alerts.push({ label: '火灾报警', type: 'fire' })
				if (this.sensorData.alert.gas) alerts.push({ label: '燃气报警', type: 'gas' })
				if (this.sensorData.alert.temp_high) alerts.push({ label: '温度过高', type: 'temp' })
				if (this.sensorData.alert.humi_high) alerts.push({ label: '湿度过高', type: 'humi' })
				if (this.sensorData.alert.tc_high) alerts.push({ label: 'PT100过高', type: 'tc' })
				return alerts
			}
		},
		async onLoad() {
			await this.initConnection()
			this.startDataUpdate()
		},
		onUnload() {
			this.stopDataUpdate()
			sensorService.disconnect()
		},
		methods: {
			async initConnection() {
				this.loading = true
				this.error = ''
				
				try {
					const success = await sensorService.init()
					
					if (success) {
						this.connectionStatus = '已连接'
						this.loading = false
						uni.showToast({ title: '连接成功', icon: 'success', duration: 2000 })
					} else {
						this.connectionStatus = '连接失败'
						this.loading = false
						this.error = 'MQTT连接失败，请检查网络'
					}
				} catch (error) {
					this.connectionStatus = '连接异常'
					this.loading = false
					this.error = error.message || '连接失败，请检查网络后重试'
					console.error('连接失败:', error)
					
					uni.showModal({
						title: '连接失败',
						content: this.error,
						showCancel: true,
						confirmText: '重试',
						cancelText: '关闭',
						success: (res) => {
							if (res.confirm) {
								this.initConnection()
							}
						}
					})
				}
			},

			startDataUpdate() {
				this.updateTimer = setInterval(() => {
					const data = sensorService.getSensorData()
					this.sensorData = data
					this.lastUpdateTime = data.lastUpdateTime
					
					// 保存历史数据（最近20条）
					if (data.lastUpdateTime) {
						this.historyData.unshift({
							time: data.lastUpdateTime,
							mq4: data.mq4,
							temp: data.temp,
							humi: data.humi,
							pressure: data.pressure
						})
						if (this.historyData.length > 20) {
							this.historyData.pop()
						}
					}
				}, 1000)
			},

			stopDataUpdate() {
				if (this.updateTimer) {
					clearInterval(this.updateTimer)
					this.updateTimer = null
				}
			},

			showThresholdModal() {
				this.showModal = true
			},

			closeModal() {
				this.showModal = false
			},

			showHistory() {
				this.showHistoryModal = true
			},

			closeHistoryModal() {
				this.showHistoryModal = false
			},

			async submitThreshold() {
				try {
					uni.showLoading({ title: '下发中...' })
					
					const thresholdData = {}
					if (this.thresholds.temp_high !== '' && this.thresholds.temp_high !== null) {
						thresholdData.temp_high = Number(this.thresholds.temp_high)
					}
					if (this.thresholds.humi_high !== '' && this.thresholds.humi_high !== null) {
						thresholdData.humi_high = Number(this.thresholds.humi_high)
					}
					if (this.thresholds.mq4_high !== '' && this.thresholds.mq4_high !== null) {
						thresholdData.mq4_high = Number(this.thresholds.mq4_high)
					}
					if (this.thresholds.tc_high !== '' && this.thresholds.tc_high !== null) {
						thresholdData.tc_high = Number(this.thresholds.tc_high)
					}
					
					console.log('下发阈值数据:', thresholdData)
					
					const success = await sensorService.setThreshold(thresholdData)
					uni.hideLoading()
					
					if (success) {
						uni.showToast({ title: '阈值下发成功', icon: 'success' })
						this.closeModal()
					} else {
						uni.showToast({ title: '阈值下发失败', icon: 'error' })
					}
				} catch (error) {
					uni.hideLoading()
					uni.showToast({ title: '下发异常', icon: 'error' })
					console.error('下发异常:', error)
				}
			},

			retryConnect() {
				this.initConnection()
			}
		}
	}
</script>

<style scoped>
	.container {
		background: linear-gradient(135deg, #0f172a 0%, #1e293b 50%, #0f172a 100%);
		min-height: 100vh;
		padding: 0;
		box-sizing: border-box;
	}

	/* 顶部导航栏 */
	.header {
		background: rgba(15, 23, 42, 0.95);
		border-bottom: 2rpx solid rgba(59, 130, 246, 0.3);
		padding: 30rpx 32rpx;
		display: flex;
		flex-direction: row;
		justify-content: space-between;
		align-items: center;
		backdrop-filter: blur(10px);
	}

	.header-content {
		flex: 1;
	}

	.app-title {
		font-size: 36rpx;
		font-weight: 700;
		color: #ffffff;
		display: block;
		letter-spacing: 2rpx;
	}

	.app-subtitle {
		font-size: 22rpx;
		color: rgba(148, 163, 184, 0.9);
		margin-top: 6rpx;
		display: block;
		letter-spacing: 1rpx;
		text-transform: uppercase;
	}

	.status-indicator {
		display: flex;
		flex-direction: row;
		align-items: center;
		gap: 10rpx;
		padding: 10rpx 20rpx;
		border-radius: 24rpx;
		background: rgba(30, 41, 59, 0.8);
		border: 1rpx solid rgba(59, 130, 246, 0.3);
	}

	.status-indicator.connected {
		border-color: rgba(34, 197, 94, 0.5);
		background: rgba(34, 197, 94, 0.1);
	}

	.status-indicator.disconnected {
		border-color: rgba(239, 68, 68, 0.5);
		background: rgba(239, 68, 68, 0.1);
	}

	.status-dot {
		width: 10rpx;
		height: 10rpx;
		border-radius: 50%;
		background: #94a3b8;
	}

	.status-indicator.connected .status-dot {
		background: #22c55e;
		box-shadow: 0 0 12rpx rgba(34, 197, 94, 0.6);
		animation: pulse 2s infinite;
	}

	.status-indicator.disconnected .status-dot {
		background: #ef4444;
	}

	@keyframes pulse {
		0%, 100% { opacity: 1; }
		50% { opacity: 0.5; }
	}

	.status-text {
		font-size: 24rpx;
		color: #e2e8f0;
		font-weight: 500;
	}

	/* 预警信息条 */
	.alert-banner {
		background: linear-gradient(90deg, rgba(239, 68, 68, 0.15) 0%, rgba(239, 68, 68, 0.05) 100%);
		border-left: 4rpx solid #ef4444;
		padding: 16rpx 32rpx;
		display: flex;
		flex-direction: row;
		gap: 16rpx;
		animation: slideIn 0.3s ease-out;
	}

	@keyframes slideIn {
		from { transform: translateX(-100%); opacity: 0; }
		to { transform: translateX(0); opacity: 1; }
	}

	.alert-item {
		padding: 8rpx 16rpx;
		background: rgba(239, 68, 68, 0.2);
		border-radius: 8rpx;
		border: 1rpx solid rgba(239, 68, 68, 0.4);
	}

	.alert-label {
		font-size: 24rpx;
		color: #fca5a5;
		font-weight: 600;
	}

	/* 加载提示 */
	.loading-box {
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
		min-height: 60vh;
		padding: 100rpx;
	}

	.loading-text {
		color: #94a3b8;
		font-size: 28rpx;
		text-align: center;
	}

	/* 主要内容区 */
	.main-content {
		padding: 24rpx 32rpx;
	}

	/* 关键指标网格 */
	.metrics-grid {
		display: grid;
		grid-template-columns: repeat(2, 1fr);
		gap: 20rpx;
		margin-bottom: 24rpx;
	}

	.metric-card {
		background: rgba(30, 41, 59, 0.6);
		border: 1rpx solid rgba(59, 130, 246, 0.2);
		border-radius: 16rpx;
		padding: 28rpx;
		backdrop-filter: blur(10px);
		transition: all 0.3s ease;
	}

	.metric-card:active {
		transform: scale(0.98);
		border-color: rgba(59, 130, 246, 0.5);
	}

	.metric-header {
		display: flex;
		flex-direction: row;
		justify-content: space-between;
		align-items: center;
		margin-bottom: 16rpx;
	}

	.metric-title {
		font-size: 26rpx;
		color: #94a3b8;
		font-weight: 500;
	}

	.metric-unit {
		font-size: 22rpx;
		color: #64748b;
		font-weight: 400;
	}

	.metric-value {
		font-size: 52rpx;
		font-weight: 700;
		color: #ffffff;
		margin-bottom: 12rpx;
		font-family: 'Courier New', monospace;
		letter-spacing: -1rpx;
	}

	.metric-value.value-warning {
		color: #fbbf24;
		text-shadow: 0 0 20rpx rgba(251, 191, 36, 0.4);
	}

	.metric-footer {
		padding-top: 12rpx;
		border-top: 1rpx solid rgba(59, 130, 246, 0.1);
	}

	.threshold-label {
		font-size: 22rpx;
		color: #64748b;
	}

	/* 次要数据列表 */
	.secondary-data {
		background: rgba(30, 41, 59, 0.6);
		border: 1rpx solid rgba(59, 130, 246, 0.2);
		border-radius: 16rpx;
		padding: 24rpx;
		margin-bottom: 24rpx;
		backdrop-filter: blur(10px);
	}

	.data-row {
		display: flex;
		flex-direction: row;
		justify-content: space-between;
		padding: 20rpx 0;
		border-bottom: 1rpx solid rgba(59, 130, 246, 0.1);
	}

	.data-row:last-child {
		border-bottom: none;
	}

	.data-item {
		flex: 1;
		display: flex;
		flex-direction: column;
		gap: 8rpx;
	}

	.data-label {
		font-size: 24rpx;
		color: #64748b;
		font-weight: 400;
	}

	.data-value {
		font-size: 28rpx;
		color: #e2e8f0;
		font-weight: 600;
		font-family: 'Courier New', monospace;
	}

	.data-value.time-value {
		font-size: 22rpx;
		color: #94a3b8;
	}

	.status-normal {
		color: #22c55e !important;
	}

	.status-danger {
		color: #ef4444 !important;
	}

	/* 操作按钮 */
	.action-buttons {
		display: flex;
		flex-direction: row;
		gap: 16rpx;
	}

	.action-btn {
		flex: 1;
		padding: 28rpx;
		border-radius: 12rpx;
		text-align: center;
		transition: all 0.3s ease;
	}

	.action-btn:active {
		transform: scale(0.96);
	}

	.action-btn.primary {
		background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
		border: 1rpx solid rgba(59, 130, 246, 0.5);
		box-shadow: 0 4rpx 16rpx rgba(59, 130, 246, 0.3);
	}

	.action-btn.secondary {
		background: rgba(30, 41, 59, 0.8);
		border: 1rpx solid rgba(59, 130, 246, 0.3);
	}

	.btn-text {
		font-size: 28rpx;
		color: #ffffff;
		font-weight: 600;
		letter-spacing: 1rpx;
	}

	.action-btn.secondary .btn-text {
		color: #94a3b8;
	}

	/* 弹窗样式 */
	.modal {
		position: fixed;
		top: 0;
		left: 0;
		width: 100%;
		height: 100%;
		background: rgba(0, 0, 0, 0.75);
		display: flex;
		flex-direction: row;
		align-items: center;
		justify-content: center;
		z-index: 999;
		backdrop-filter: blur(4px);
	}

	.modal-content {
		background: #1e293b;
		border: 1rpx solid rgba(59, 130, 246, 0.3);
		border-radius: 20rpx;
		width: 640rpx;
		max-height: 80vh;
		overflow: hidden;
		box-shadow: 0 20rpx 60rpx rgba(0, 0, 0, 0.5);
	}

	.history-modal {
		width: 680rpx;
	}

	.modal-header {
		display: flex;
		flex-direction: row;
		justify-content: space-between;
		align-items: center;
		padding: 32rpx;
		border-bottom: 1rpx solid rgba(59, 130, 246, 0.2);
		background: rgba(15, 23, 42, 0.5);
	}

	.modal-title {
		font-size: 32rpx;
		font-weight: 700;
		color: #ffffff;
		letter-spacing: 1rpx;
	}

	.modal-close {
		font-size: 48rpx;
		color: #64748b;
		line-height: 1;
		padding: 0 8rpx;
	}

	.modal-body {
		padding: 32rpx;
	}

	.history-body {
		padding: 0;
		max-height: 600rpx;
	}

	.history-list {
		max-height: 600rpx;
	}

	.history-item {
		padding: 24rpx 32rpx;
		border-bottom: 1rpx solid rgba(59, 130, 246, 0.1);
		background: rgba(15, 23, 42, 0.3);
	}

	.history-item:nth-child(even) {
		background: rgba(30, 41, 59, 0.3);
	}

	.history-time {
		font-size: 22rpx;
		color: #64748b;
		margin-bottom: 12rpx;
		font-family: 'Courier New', monospace;
	}

	.history-data {
		display: flex;
		flex-direction: row;
		flex-wrap: wrap;
		gap: 20rpx;
	}

	.data-item {
		font-size: 24rpx;
		color: #cbd5e1;
		font-family: 'Courier New', monospace;
	}

	.empty-tip {
		padding: 80rpx;
		text-align: center;
		color: #64748b;
		font-size: 26rpx;
	}

	.input-group {
		margin-bottom: 28rpx;
	}

	.input-label {
		display: block;
		font-size: 26rpx;
		color: #94a3b8;
		margin-bottom: 12rpx;
		font-weight: 500;
	}

	.input-field {
		width: 100%;
		height: 88rpx;
		border: 1rpx solid rgba(59, 130, 246, 0.3);
		border-radius: 12rpx;
		padding: 0 24rpx;
		font-size: 28rpx;
		box-sizing: border-box;
		background: rgba(15, 23, 42, 0.6);
		color: #e2e8f0;
	}

	.input-field:focus {
		border-color: #3b82f6;
		background: rgba(15, 23, 42, 0.8);
	}

	.modal-footer {
		display: flex;
		flex-direction: row;
		gap: 16rpx;
		padding: 24rpx 32rpx 32rpx 32rpx;
		border-top: 1rpx solid rgba(59, 130, 246, 0.2);
		background: rgba(15, 23, 42, 0.5);
	}

	.modal-btn {
		flex: 1;
		border-radius: 12rpx;
		padding: 28rpx;
		font-size: 28rpx;
		text-align: center;
		font-weight: 600;
		transition: all 0.3s ease;
	}

	.cancel-btn {
		background: rgba(51, 65, 85, 0.8);
		color: #94a3b8;
		border: 1rpx solid rgba(71, 85, 105, 0.5);
	}

	.confirm-btn {
		background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
		color: #ffffff;
		border: 1rpx solid rgba(59, 130, 246, 0.5);
		box-shadow: 0 4rpx 16rpx rgba(59, 130, 246, 0.3);
	}
</style>
