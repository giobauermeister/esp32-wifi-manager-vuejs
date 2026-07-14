import { createApp } from 'petite-vue'
import './style.css'

const MOCK_MODE = import.meta.env.DEV && import.meta.env.VITE_USE_MOCK !== 'false'
const DEVICE_HOST = import.meta.env.VITE_DEVICE_HOST || (location.hostname === 'localhost' ? '192.168.4.1' : location.hostname)

function wifiManager() {
  return {
    socket: null,
    reconnectTimer: null,
    heartbeatTimer: null,
    lastPongTime: 0,
    isStaticIp: false,
    isDeviceConnected: false,
    isScanning: false,
    isTaskRunning: false,
    showNetworks: false,
    showPassword: false,
    ssid: '',
    password: '',
    staticIp: '',
    netmask: '',
    gateway: '',
    ssidList: [],
    macAddress: '',
    dialog: false,
    dialogMessage: '',
    pendingAction: '',

    mounted() {
      if (MOCK_MODE) {
        this.loadDeviceInfo({
          macAddress: '30:AE:A4:C1:FB:14', ssid: 'GiosWifi', password: 'password', isStaticIp: true,
          staticIp: '192.168.0.35', netmask: '255.255.255.0', gateway: '192.168.0.1'
        })
        return
      }

      this.connectWebSocket()
      this.heartbeatTimer = window.setInterval(() => {
        if (this.socket?.readyState === WebSocket.OPEN && !this.isTaskRunning) this.socket.send('ping')
        if (!this.isTaskRunning && Date.now() - this.lastPongTime > 5000) this.isDeviceConnected = false
      }, 2000)
    },

    connectWebSocket() {
      if (this.socket && [WebSocket.OPEN, WebSocket.CONNECTING].includes(this.socket.readyState)) return

      this.socket = new WebSocket(`ws://${DEVICE_HOST}:9090`, 'arduino')
      this.socket.addEventListener('open', () => this.socket.send('ping'))
      this.socket.addEventListener('message', (event) => this.handleMessage(event.data))
      this.socket.addEventListener('close', () => {
        this.isDeviceConnected = false
        window.clearTimeout(this.reconnectTimer)
        this.reconnectTimer = window.setTimeout(() => this.connectWebSocket(), 2000)
      })
      this.socket.addEventListener('error', () => this.socket.close())
    },

    handleMessage(payload) {
      let data
      try {
        data = JSON.parse(payload)
      } catch {
        this.showStatus('The device returned an invalid response.')
        return
      }

      this.lastPongTime = Date.now()
      this.isDeviceConnected = true

      if (data.deviceInfo) this.loadDeviceInfo(data.deviceInfo)
      if (data.ssidArray) {
        this.ssidList = [...new Set(data.ssidArray)]
        this.isScanning = false
        this.showNetworks = true
      }
      if (data.saveOk) this.finishTask('Save OK')
      if (data.eraseOk) {
        this.resetConfiguration()
        this.finishTask('Erase OK')
      }
      if (data.startDeviceOk) this.finishTask('Device started!')
      if (data.testWifiStatus) this.finishWifiTest(data.testWifiStatus)
    },

    loadDeviceInfo(info) {
      this.macAddress = info.macAddress || ''
      this.ssid = info.ssid || ''
      this.password = info.password || ''
      this.isStaticIp = Boolean(info.isStaticIp)
      this.staticIp = info.staticIp || ''
      this.netmask = info.netmask || ''
      this.gateway = info.gateway || ''
      this.lastPongTime = Date.now()
      this.isDeviceConnected = true
    },

    send(payload) {
      if (MOCK_MODE) return true
      if (this.socket?.readyState !== WebSocket.OPEN) {
        this.showStatus('Device not connected')
        return false
      }
      this.socket.send(typeof payload === 'string' ? payload : JSON.stringify(payload))
      return true
    },

    toggleNetworks() {
      if (this.showNetworks) {
        this.showNetworks = false
        return
      }
      if (!this.isDeviceConnected) return this.showStatus('Device not connected')
      this.showNetworks = true
      this.isScanning = true
      this.ssidList = []
      if (MOCK_MODE) {
        window.setTimeout(() => {
          this.ssidList = ['Home WiFi', 'Office Network', 'ESP32 Test', 'IoT Router', 'Factory WiFi']
          this.isScanning = false
        }, 3000)
      } else {
        this.send('scanNetworks')
      }
    },

    selectNetwork(network) {
      this.ssid = network
      this.showNetworks = false
    },

    confirmAction(action) {
      const messages = {
        save: 'Are you sure you want to save WiFi configuration?',
        erase: 'Are you sure you want to erase current WiFi configuration?',
        test: 'This will test the WiFi connection.',
        start: 'Are you sure you want to start your device?'
      }
      this.pendingAction = action
      this.dialogMessage = messages[action]
      this.dialog = true
    },

    runPendingAction() {
      const action = this.pendingAction
      this.pendingAction = ''
      if (!this.isDeviceConnected) return this.showStatus('Device not connected')
      if (action === 'save' && !this.isConfigurationValid()) {
        return this.showStatus('SSID should not be empty. If IP is static, fill all fields.')
      }

      const taskMessages = { save: 'Saving to device…', erase: 'Erasing current WiFi data…', test: 'Testing WiFi connection…', start: 'Starting device…' }
      this.dialogMessage = taskMessages[action]
      this.isTaskRunning = true

      if (MOCK_MODE) {
        window.setTimeout(() => {
          if (action === 'erase') this.resetConfiguration()
          this.finishTask({ save: 'Save OK', erase: 'Erase OK', test: 'WiFi OK! [WL_CONNECTED]', start: 'Device started!' }[action])
        }, 700)
        return
      }

      const messages = {
        save: { deviceConfiguration: this.configuration() },
        erase: 'eraseConfig',
        test: 'testWifi',
        start: { startDevice: this.configuration() }
      }
      if (!this.send(messages[action])) this.isTaskRunning = false
    },

    configuration() {
      return {
        ssid: this.ssid,
        password: this.password,
        isStaticIp: this.isStaticIp,
        staticIp: this.staticIp,
        netmask: this.netmask,
        gateway: this.gateway
      }
    },

    isConfigurationValid() {
      return this.ssid.trim() !== '' && (!this.isStaticIp || [this.staticIp, this.netmask, this.gateway].every(value => value.trim() !== ''))
    },

    finishWifiTest(status) {
      const messages = {
        WL_CONNECTED: 'WiFi OK! [WL_CONNECTED]',
        blankdevice: 'Connection failed. Device not configured',
        WL_CONNECT_FAILED: 'Connection failed [WL_CONNECT_FAILED]',
        WL_NO_SSID_AVAIL: 'Could not find WiFi [WL_NO_SSID_AVAIL]',
        error: 'Connection failed.'
      }
      this.finishTask(messages[status] || `Connection status: ${status}`)
    },

    finishTask(message) {
      this.isTaskRunning = false
      this.dialogMessage = message
      this.dialog = true
    },

    showStatus(message) {
      this.pendingAction = ''
      this.isTaskRunning = false
      this.dialogMessage = message
      this.dialog = true
    },

    closeDialog() {
      this.dialog = false
      this.pendingAction = ''
    },

    resetConfiguration() {
      this.ssid = ''
      this.password = ''
      this.isStaticIp = false
      this.staticIp = ''
      this.netmask = ''
      this.gateway = ''
    }
  }
}

createApp(wifiManager()).mount('#app')
