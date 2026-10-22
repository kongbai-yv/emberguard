<script>
	export default {
		onLaunch: function() {
			console.log('App Launch')
		},
		onShow: function() {
			console.log('App Show')
		},
		onHide: function() {
			console.log('App Hide')
		}
	}
</script>

<style>
	/* 每个页面公共css */
	page {
		background-color: #F0F8FF;
		font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
	}
	
	/* 全局滚动条样式 */
	::-webkit-scrollbar {
		width: 0;
		height: 0;
		color: transparent;
	}
	
	/* 全局按钮样式重置 */
	button {
		margin: 0;
		padding: 0;
		border: none;
		background: none;
		line-height: normal;
	}
	
	button::after {
		border: none;
	}
	
	/* 全局输入框样式 */
	input {
		font-family: inherit;
	}
</style>
