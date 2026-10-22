import * as THREE from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js'

export interface SensorData {
  mq4: number
  flame: number
  flame_mv: number
  temp: number
  humi: number
  tc: number
  pressure: number
  altitude: number
  storm_warning: boolean
  alert: {
    fire: boolean
    gas: boolean
    temp_high: boolean
    humi_high: boolean
    tc_high: boolean
  }
}

interface DataPanel {
  name: string
  label: string
  unit: string
  value: number
  maxValue: number
  threshold: number
  alarm: boolean
  canvas: HTMLCanvasElement
  texture: THREE.CanvasTexture
  mesh: THREE.Mesh
  color: string
}

interface TopScreen {
  label: string
  unit: string
  value: number
  maxValue: number
  alarm: boolean
  canvas: HTMLCanvasElement
  texture: THREE.CanvasTexture
  mesh: THREE.Mesh
}

export class CabinetScene {
  private container: HTMLElement
  private scene: THREE.Scene
  private camera: THREE.PerspectiveCamera
  private renderer: THREE.WebGLRenderer
  private controls: OrbitControls
  private clock: THREE.Clock
  private animationId: number | null = null

  private cabinet!: THREE.Group
  private doorScreens: DataPanel[] = []
  private topScreens: TopScreen[] = []
  private warningLights: THREE.PointLight[] = []
  private warningBulbs: THREE.Mesh[] = []
  private doorLeft!: THREE.Group
  private doorRight!: THREE.Group
  private doorOpen = 0
  private doorTarget = 0
  private innerGlow!: THREE.Mesh
  private ventParticles!: THREE.Points

  private currentData: SensorData | null = null
  private hasAlarm = false
  private smoothedValues: Map<string, number> = new Map()

  constructor(container: HTMLElement) {
    this.container = container
    this.scene = new THREE.Scene()
    this.camera = new THREE.PerspectiveCamera(40, container.clientWidth / container.clientHeight, 0.1, 100)
    this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true })
    this.controls = new OrbitControls(this.camera, this.renderer.domElement)
    this.clock = new THREE.Clock()
    this.init()
  }

  private init() {
    this.setupScene()
    this.setupLights()
    this.createFloor()
    this.createCabinet()
    this.createTopScreens()
    this.createVentEffect()
    this.setupEventListeners()
    this.animate()
  }

  /* ===================== 场景 ===================== */

  private setupScene() {
    this.scene.background = new THREE.Color(0x0a1628)
    this.scene.fog = new THREE.FogExp2(0x0a1628, 0.008)
    this.renderer.setSize(this.container.clientWidth, this.container.clientHeight)
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping
    this.renderer.toneMappingExposure = 1.6
    this.renderer.shadowMap.enabled = true
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap
    this.container.appendChild(this.renderer.domElement)

    // 天空穹顶
    const skyGeo = new THREE.SphereGeometry(40, 32, 32)
    const skyCanvas = document.createElement('canvas')
    skyCanvas.width = 2; skyCanvas.height = 256
    const skyCtx = skyCanvas.getContext('2d')!
    const grad = skyCtx.createLinearGradient(0, 0, 0, 256)
    grad.addColorStop(0, '#0a1e3a')    // 天顶 深蓝
    grad.addColorStop(0.3, '#1a3a5c')  // 上空
    grad.addColorStop(0.55, '#2a5580') // 中天
    grad.addColorStop(0.75, '#4a7aaa') // 下天
    grad.addColorStop(0.9, '#6a9ac0')  // 地平线
    grad.addColorStop(1, '#3a6080')    // 地面
    skyCtx.fillStyle = grad
    skyCtx.fillRect(0, 0, 2, 256)
    const skyTex = new THREE.CanvasTexture(skyCanvas)
    const skyMat = new THREE.MeshBasicMaterial({ map: skyTex, side: THREE.BackSide, depthWrite: false })
    const sky = new THREE.Mesh(skyGeo, skyMat)
    this.scene.add(sky)

    this.camera.position.set(4, 5.5, 5)
    this.camera.lookAt(0, 1.4, 0)

    this.controls.enableDamping = true
    this.controls.dampingFactor = 0.06
    this.controls.minDistance = 3
    this.controls.maxDistance = 10
    this.controls.maxPolarAngle = Math.PI * 0.55
    this.controls.minPolarAngle = Math.PI * 0.1
    this.controls.enablePan = false
    this.controls.autoRotate = false
    this.controls.minAzimuthAngle = -Math.PI * 0.75
    this.controls.maxAzimuthAngle = Math.PI * 0.75
    this.controls.target.set(0, 2.0, 0)
  }

  private setupLights() {
    this.scene.add(new THREE.AmbientLight(0x445577, 1.2))

    const main = new THREE.DirectionalLight(0xfff5e8, 1.8)
    main.position.set(3, 8, 4)
    main.castShadow = true
    main.shadow.mapSize.set(1024, 1024)
    this.scene.add(main)

    const front = new THREE.PointLight(0xddeeff, 1.0, 15)
    front.position.set(0, 4, 6)
    this.scene.add(front)

    const left = new THREE.PointLight(0x8899bb, 0.6, 12)
    left.position.set(-4, 3, 2)
    this.scene.add(left)

    const right = new THREE.PointLight(0x8899bb, 0.6, 12)
    right.position.set(4, 3, 2)
    this.scene.add(right)

    const bottom = new THREE.PointLight(0x445566, 0.4, 10)
    bottom.position.set(0, 0.5, 3)
    this.scene.add(bottom)

    const spot = new THREE.SpotLight(0xfff0d0, 1.2, 12, Math.PI / 5, 0.5)
    spot.position.set(0, 7, 1)
    spot.target.position.set(0, 2, 0)
    this.scene.add(spot)
    this.scene.add(spot.target)

    const backFill = new THREE.PointLight(0xaabbcc, 0.9, 14)
    backFill.position.set(0, 3, -4)
    this.scene.add(backFill)

    const underGlow = new THREE.PointLight(0x6688aa, 0.5, 8)
    underGlow.position.set(0, -0.3, 1.5)
    this.scene.add(underGlow)

    const hemi = new THREE.HemisphereLight(0x8899bb, 0x334455, 0.7)
    this.scene.add(hemi)

    const topSpot = new THREE.SpotLight(0xffffff, 1.5, 8, Math.PI / 4, 0.6)
    topSpot.position.set(0, 3.0, 0)
    topSpot.target.position.set(0, 0.5, 0.5)
    this.scene.add(topSpot)
    this.scene.add(topSpot.target)
  }

  private createFloor() {
    const floor = new THREE.Mesh(
      new THREE.PlaneGeometry(16, 16),
      new THREE.MeshStandardMaterial({ color: 0x0e1220, metalness: 0.15, roughness: 0.92 })
    )
    floor.rotation.x = -Math.PI / 2
    floor.receiveShadow = true
    this.scene.add(floor)

    const grid = new THREE.GridHelper(16, 32, 0x162040, 0x0e1830)
    grid.position.y = 0.001
    this.scene.add(grid)

    // 黄色警示区
    const stripeMat = new THREE.MeshBasicMaterial({ color: 0xffaa00, transparent: true, opacity: 0.3 })
    for (let i = 0; i < 4; i++) {
      const s = new THREE.Mesh(new THREE.PlaneGeometry(2.8, 0.06), stripeMat)
      s.rotation.x = -Math.PI / 2
      s.position.set(0, 0.002, -1.4 + i * 0.12)
      this.scene.add(s)
    }
  }

  /* ===================== 柜子 ===================== */

  private createCabinet() {
    this.cabinet = new THREE.Group()

    const bodyMat = new THREE.MeshStandardMaterial({ color: 0xd47a3a, metalness: 0.72, roughness: 0.3 })
    const darkMat = new THREE.MeshStandardMaterial({ color: 0x222838, metalness: 0.5, roughness: 0.5 })

    // 后板
    const back = new THREE.Mesh(new THREE.BoxGeometry(2.0, 2.6, 0.06), bodyMat)
    back.position.set(0, 1.5, -0.53)
    this.cabinet.add(back)

    // 顶/底板
    const topBot = new THREE.BoxGeometry(2.0, 0.06, 1.0)
    const top = new THREE.Mesh(topBot, bodyMat); top.position.set(0, 2.8, -0.03)
    this.cabinet.add(top)
    const bot = new THREE.Mesh(topBot, bodyMat); bot.position.set(0, 0.2, -0.03)
    this.cabinet.add(bot)

    // 侧板
    const sideGeo = new THREE.BoxGeometry(0.06, 2.6, 1.0)
    const ls = new THREE.Mesh(sideGeo, bodyMat); ls.position.set(-1.0, 1.5, -0.03)
    const rs = new THREE.Mesh(sideGeo, bodyMat); rs.position.set(1.0, 1.5, -0.03)
    this.cabinet.add(ls, rs)

    // 内部隔板
    const shelfGeo = new THREE.BoxGeometry(1.88, 0.03, 0.9)
    for (let i = 1; i <= 3; i++) {
      const sh = new THREE.Mesh(shelfGeo, darkMat)
      sh.position.set(0, 0.2 + i * 0.6, -0.03)
      this.cabinet.add(sh)
    }

    // 内部辉光
    const glowMat = new THREE.MeshBasicMaterial({ color: 0x334466, transparent: true, opacity: 0.15 })
    this.innerGlow = new THREE.Mesh(new THREE.BoxGeometry(1.86, 2.4, 0.01), glowMat)
    this.innerGlow.position.set(0, 1.5, -0.48)
    this.cabinet.add(this.innerGlow)

    // 门
    this.createDoors()

    // 底座
    const footMat = new THREE.MeshStandardMaterial({ color: 0x282e3a, metalness: 0.6, roughness: 0.4 })
    const footGeo = new THREE.BoxGeometry(0.15, 0.15, 0.15)
    for (const p of [{ x: -0.85, z: -0.35 }, { x: 0.85, z: -0.35 }, { x: -0.85, z: 0.35 }, { x: 0.85, z: 0.35 }]) {
      const f = new THREE.Mesh(footGeo, footMat); f.position.set(p.x, 0.075, p.z)
      this.cabinet.add(f)
    }

    // 报警灯
    this.createWarningBeacons()

    // 侧面通风槽
    this.createVentSlots()

    // 警示标识
    this.createWarningLabels()

    // 柜门数据屏
    this.createDoorScreens()

    this.scene.add(this.cabinet)
  }

  private createDoors() {
    const doorMat = new THREE.MeshStandardMaterial({ color: 0xd47a3a, metalness: 0.75, roughness: 0.28 })
    const glassMat = new THREE.MeshStandardMaterial({ color: 0x1a2840, metalness: 0.9, roughness: 0.1, transparent: true, opacity: 0.28 })
    const handleMat = new THREE.MeshStandardMaterial({ color: 0x8890a8, metalness: 0.9, roughness: 0.15 })

    this.doorLeft = new THREE.Group()
    const ld = new THREE.Mesh(new THREE.BoxGeometry(0.97, 2.5, 0.04), doorMat); ld.position.x = -0.485; ld.receiveShadow = false
    this.doorLeft.add(ld)
    const lg = new THREE.Mesh(new THREE.BoxGeometry(0.7, 1.6, 0.01), glassMat); lg.position.set(-0.485, 0.15, 0.025); lg.receiveShadow = false
    this.doorLeft.add(lg)
    const lh = new THREE.Mesh(new THREE.CylinderGeometry(0.015, 0.015, 0.3, 8), handleMat)
    lh.position.set(-0.08, 0, 0.04); lh.rotation.x = Math.PI / 2
    this.doorLeft.add(lh)
    this.doorLeft.position.set(-1.0, 1.45, 0.46)
    this.cabinet.add(this.doorLeft)

    this.doorRight = new THREE.Group()
    const rd = new THREE.Mesh(new THREE.BoxGeometry(0.97, 2.5, 0.04), doorMat); rd.position.x = 0.485; rd.receiveShadow = false
    this.doorRight.add(rd)
    const rg = new THREE.Mesh(new THREE.BoxGeometry(0.7, 1.6, 0.01), glassMat); rg.position.set(0.485, 0.15, 0.025); rg.receiveShadow = false
    this.doorRight.add(rg)
    const rh = new THREE.Mesh(new THREE.CylinderGeometry(0.015, 0.015, 0.3, 8), handleMat)
    rh.position.set(0.08, 0, 0.04); rh.rotation.x = Math.PI / 2
    this.doorRight.add(rh)
    this.doorRight.position.set(1.0, 1.45, 0.46)
    this.cabinet.add(this.doorRight)

    // 中缝条
    const cf = new THREE.Mesh(new THREE.BoxGeometry(0.03, 2.5, 0.06),
      new THREE.MeshStandardMaterial({ color: 0x505868, metalness: 0.8, roughness: 0.25 }))
    cf.position.set(0, 1.45, 0.48)
    this.cabinet.add(cf)
  }

  private createWarningBeacons() {
    for (const pos of [{ x: -0.6, z: 0.0 }, { x: 0.6, z: 0.0 }]) {
      const base = new THREE.Mesh(
        new THREE.CylinderGeometry(0.08, 0.1, 0.06, 16),
        new THREE.MeshStandardMaterial({ color: 0x333333, metalness: 0.8, roughness: 0.3 })
      )
      base.position.set(pos.x, 3.15, pos.z)
      this.cabinet.add(base)

      const bulb = new THREE.Mesh(
        new THREE.SphereGeometry(0.08, 16, 16),
        new THREE.MeshBasicMaterial({ color: 0x22cc66, transparent: true, opacity: 0.6 })
      )
      bulb.position.set(pos.x, 3.22, pos.z)
      this.cabinet.add(bulb)
      this.warningBulbs.push(bulb)

      const light = new THREE.PointLight(0x22cc66, 0.8, 3)
      light.position.set(pos.x, 3.22, pos.z)
      this.cabinet.add(light)
      this.warningLights.push(light)
    }
  }

  private createVentSlots() {
    const slotMat = new THREE.MeshStandardMaterial({ color: 0x404858, metalness: 0.6, roughness: 0.5 })
    const darkMat = new THREE.MeshStandardMaterial({ color: 0x0c0e14, metalness: 0.3, roughness: 0.8 })
    for (const side of [-1, 1]) {
      for (let i = 0; i < 8; i++) {
        const slot = new THREE.Mesh(new THREE.BoxGeometry(0.02, 0.03, 0.4), slotMat)
        slot.position.set(side * 1.03, 0.6 + i * 0.06, 0)
        this.cabinet.add(slot)
        const dark = new THREE.Mesh(new THREE.BoxGeometry(0.01, 0.015, 0.38), darkMat)
        dark.position.set(side * 1.04, 0.6 + i * 0.06, 0)
        this.cabinet.add(dark)
      }
    }
  }

  private createWarningLabels() {
    const c = document.createElement('canvas'); c.width = 128; c.height = 128
    const ctx = c.getContext('2d')!
    ctx.fillStyle = '#fff'
    ctx.beginPath(); ctx.moveTo(64, 15); ctx.lineTo(115, 105); ctx.lineTo(13, 105); ctx.closePath(); ctx.fill()
    ctx.fillStyle = '#cc0000'
    ctx.beginPath(); ctx.moveTo(64, 25); ctx.lineTo(108, 100); ctx.lineTo(20, 100); ctx.closePath(); ctx.fill()
    ctx.fillStyle = '#fff'; ctx.font = 'bold 48px sans-serif'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle'
    ctx.fillText('!', 64, 70)
    const tex = new THREE.CanvasTexture(c)
    const mat = new THREE.MeshBasicMaterial({ map: tex, transparent: true })
    const geo = new THREE.PlaneGeometry(0.2, 0.2)
    for (const x of [-0.75, 0.75]) {
      const m = new THREE.Mesh(geo, mat.clone())
      m.position.set(x, 2.2, 0.49)
      this.cabinet.add(m)
    }

    // "危险化学品储存柜" 标牌
    const sc = document.createElement('canvas'); sc.width = 768; sc.height = 96
    const sctx = sc.getContext('2d')!
    sctx.fillStyle = '#0d1520'; sctx.fillRect(0, 0, 768, 96)
    sctx.strokeStyle = '#ffcc00'; sctx.lineWidth = 4; sctx.strokeRect(4, 4, 760, 88)
    sctx.shadowColor = '#ffaa00'; sctx.shadowBlur = 12
    sctx.fillStyle = '#ffee44'; sctx.font = 'bold 40px "Microsoft YaHei",sans-serif'
    sctx.textAlign = 'center'; sctx.textBaseline = 'middle'
    sctx.fillText('危险化学品储存柜', 384, 48)
    sctx.shadowBlur = 0
    const stex = new THREE.CanvasTexture(sc)
    const sign = new THREE.Mesh(
      new THREE.PlaneGeometry(1.5, 0.19),
      new THREE.MeshBasicMaterial({ map: stex, transparent: true })
    )
    sign.position.set(0, 2.78, 0.48)
    this.cabinet.add(sign)
  }

  /* ===================== 顶部数据屏 ===================== */

  private createTopScreens() {
    const cfgs = [
      { label: '温度', unit: '℃', maxValue: 50, x: -0.38 },
      { label: '湿度', unit: '%', maxValue: 100, x: 0 },
      { label: '燃气', unit: 'ppm', maxValue: 800, x: 0.38 },
    ]
    // 顶部控制面板外壳
    const panelMat = new THREE.MeshStandardMaterial({ color: 0x1a2030, metalness: 0.6, roughness: 0.4 })
    const panel = new THREE.Mesh(new THREE.BoxGeometry(1.85, 0.22, 0.5), panelMat)
    panel.position.set(0, 2.98, 0)
    this.cabinet.add(panel)
    // 黄色边框
    const borderMat = new THREE.MeshStandardMaterial({ color: 0xffaa00, emissive: 0xffaa00, emissiveIntensity: 0.15, metalness: 0.5, roughness: 0.4 })
    for (const y of [3.1, 2.86]) {
      const b = new THREE.Mesh(new THREE.BoxGeometry(1.89, 0.008, 0.54), borderMat)
      b.position.set(0, y, 0); this.cabinet.add(b)
    }

    cfgs.forEach((cfg) => {
      const canvas = document.createElement('canvas')
      canvas.width = 180; canvas.height = 90
      const texture = new THREE.CanvasTexture(canvas)
      texture.minFilter = THREE.LinearFilter

      const mesh = new THREE.Mesh(
        new THREE.PlaneGeometry(0.32, 0.16),
        new THREE.MeshBasicMaterial({ map: texture, transparent: true })
      )
      mesh.position.set(cfg.x, 2.98, 0.26)
      mesh.rotation.x = -Math.PI * 0.12
      this.cabinet.add(mesh)

      this.topScreens.push({ ...cfg, value: 0, alarm: false, canvas, texture, mesh })
      this.smoothedValues.set(cfg.label, 0)
      this.drawTopScreen(this.topScreens[this.topScreens.length - 1])
    })
  }

  private drawTopScreen(s: TopScreen) {
    const ctx = s.canvas.getContext('2d')!
    const w = s.canvas.width, h = s.canvas.height

    ctx.fillStyle = '#080e18'; ctx.fillRect(0, 0, w, h)
    ctx.strokeStyle = s.alarm ? '#ff3333' : '#1a3050'; ctx.lineWidth = 2; ctx.strokeRect(1, 1, w - 2, h - 2)

    ctx.fillStyle = '#5588aa'; ctx.font = '14px "Microsoft YaHei",sans-serif'; ctx.textAlign = 'left'
    ctx.fillText(s.label, 8, 20)

    const smoothed = this.smoothedValues.get(s.label) || 0
    ctx.fillStyle = s.alarm ? '#ff4444' : '#00ddff'; ctx.font = 'bold 32px "Consolas",monospace'; ctx.textAlign = 'center'
    ctx.fillText(smoothed.toFixed(1), w / 2, 55)
    ctx.fillStyle = '#557788'; ctx.font = '12px sans-serif'; ctx.textAlign = 'right'
    ctx.fillText(s.unit, w - 8, 55)

    const ratio = Math.min(smoothed / s.maxValue, 1)
    ctx.fillStyle = '#121822'; ctx.fillRect(8, h - 12, w - 16, 5)
    ctx.fillStyle = s.alarm ? '#ff3333' : ratio > 0.7 ? '#ffaa00' : '#00cc66'
    ctx.fillRect(8, h - 12, (w - 16) * ratio, 5)

    s.texture.needsUpdate = true
  }

  /* ===================== 柜门数据屏 ===================== */

  private createDoorScreens() {
    const cfgs = [
      { name: 'temp', label: '温度', unit: '℃', maxValue: 50, threshold: 35, color: '#ff4444' },
      { name: 'humi', label: '湿度', unit: '%', maxValue: 100, threshold: 60, color: '#4488ff' },
      { name: 'mq4', label: '燃气', unit: 'ppm', maxValue: 800, threshold: 400, color: '#ff8800' },
      { name: 'tc', label: 'PT100', unit: '℃', maxValue: 150, threshold: 100, color: '#ff44aa' },
      { name: 'pressure', label: '气压', unit: 'hPa', maxValue: 1100, threshold: 1050, color: '#aa66ff' },
      { name: 'flame_mv', label: '火焰', unit: 'mV', maxValue: 4096, threshold: 3000, color: '#ffcc00' },
    ]

    // 左门3个：温度、湿度、燃气；右门3个：PT100、气压、火焰
    const positions = [
      { door: this.doorLeft,  x: -0.48, y: -0.60 },
      { door: this.doorLeft,  x: -0.48, y:  0.00 },
      { door: this.doorLeft,  x: -0.48, y:  0.60 },
      { door: this.doorRight, x:  0.48, y: -0.60 },
      { door: this.doorRight, x:  0.48, y:  0.00 },
      { door: this.doorRight, x:  0.48, y:  0.60 },
    ]

    cfgs.forEach((cfg, i) => {
      const pos = positions[i]

      const canvas = document.createElement('canvas')
      canvas.width = 240; canvas.height = 140
      const texture = new THREE.CanvasTexture(canvas)
      texture.minFilter = THREE.LinearFilter

      // 发光背板
      const glowMat = new THREE.MeshBasicMaterial({ color: new THREE.Color(cfg.color), transparent: true, opacity: 0.25, depthWrite: false })
      const glow = new THREE.Mesh(new THREE.PlaneGeometry(0.84, 0.50), glowMat)
      glow.position.set(pos.x, pos.y, 0.13)
      pos.door.add(glow)

      const mesh = new THREE.Mesh(
        new THREE.PlaneGeometry(0.78, 0.45),
        new THREE.MeshBasicMaterial({ map: texture, transparent: true, depthWrite: false })
      )
      mesh.position.set(pos.x, pos.y, 0.14)
      pos.door.add(mesh)

      this.smoothedValues.set(cfg.name, 0)
      this.doorScreens.push({ ...cfg, value: 0, alarm: false, canvas, texture, mesh })
      this.drawSidePanel(this.doorScreens[i])
    })
  }

  private drawSidePanel(p: DataPanel) {
    const ctx = p.canvas.getContext('2d')!
    const w = p.canvas.width, h = p.canvas.height
    const smoothed = this.smoothedValues.get(p.name) || 0

    ctx.clearRect(0, 0, w, h)

    // 背景
    ctx.fillStyle = p.alarm ? 'rgba(50,8,8,0.93)' : 'rgba(6,14,28,0.93)'
    this.roundRect(ctx, 0, 0, w, h, 10); ctx.fill()

    // 边框
    ctx.strokeStyle = p.alarm ? '#ff3333' : p.color; ctx.lineWidth = 2
    this.roundRect(ctx, 1, 1, w - 2, h - 2, 9); ctx.stroke()

    // 顶部色条
    ctx.fillStyle = p.alarm ? '#ff3333' : p.color; ctx.globalAlpha = 0.55
    this.roundRect(ctx, 0, 0, w, 28, [10, 10, 0, 0]); ctx.fill()
    ctx.globalAlpha = 1

    // 标签 + 单位
    ctx.fillStyle = '#fff'; ctx.font = 'bold 15px "Microsoft YaHei",sans-serif'; ctx.textAlign = 'left'; ctx.textBaseline = 'middle'
    ctx.fillText(p.label, 10, 14)
    ctx.fillStyle = 'rgba(255,255,255,0.55)'; ctx.font = '11px sans-serif'; ctx.textAlign = 'right'
    ctx.fillText(p.unit, w - 10, 14)

    // 数值
    ctx.fillStyle = p.alarm ? '#ff5555' : '#fff'; ctx.font = 'bold 40px "Consolas",monospace'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle'
    ctx.fillText(smoothed.toFixed(1), w / 2, h * 0.47)

    // 进度条
    const bx = 14, by = h - 26, bw = w - 28, bh = 7
    const ratio = Math.min(smoothed / p.maxValue, 1)
    ctx.fillStyle = 'rgba(255,255,255,0.07)'; this.roundRect(ctx, bx, by, bw, bh, 3.5); ctx.fill()
    const barColor = p.alarm ? '#ff3333' : ratio > 0.7 ? '#ffaa00' : p.color
    ctx.fillStyle = barColor
    if (bw * ratio > 7) { this.roundRect(ctx, bx, by, bw * ratio, bh, 3.5); ctx.fill() }

    // 阈值刻度
    const tx = bx + bw * Math.min(p.threshold / p.maxValue, 1)
    if (tx > bx && tx < bx + bw) {
      ctx.fillStyle = 'rgba(255,255,255,0.45)'; ctx.fillRect(tx - 0.5, by - 2, 1.5, bh + 4)
    }

    // 状态文字
    ctx.fillStyle = p.alarm ? '#ff6666' : 'rgba(255,255,255,0.3)'; ctx.font = '10px "Microsoft YaHei",sans-serif'; ctx.textAlign = 'center'
    ctx.fillText(p.alarm ? '!! 超过阈值 !!' : `阈值 ${p.threshold}${p.unit}`, w / 2, h - 7)

    p.texture.needsUpdate = true
  }

  private roundRect(ctx: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, r: number | number[]) {
    const rd = typeof r === 'number' ? [r, r, r, r] : r
    ctx.beginPath()
    ctx.moveTo(x + rd[0], y); ctx.lineTo(x + w - rd[1], y); ctx.quadraticCurveTo(x + w, y, x + w, y + rd[1])
    ctx.lineTo(x + w, y + h - rd[2]); ctx.quadraticCurveTo(x + w, y + h, x + w - rd[2], y + h)
    ctx.lineTo(x + rd[3], y + h); ctx.quadraticCurveTo(x, y + h, x, y + h - rd[3])
    ctx.lineTo(x, y + rd[0]); ctx.quadraticCurveTo(x, y, x + rd[0], y); ctx.closePath()
  }

  /* ===================== 粒子 ===================== */

  private createVentEffect() {
    const count = 60
    const positions = new Float32Array(count * 3)
    for (let i = 0; i < count; i++) {
      positions[i * 3] = (Math.random() - 0.5) * 3 + 1
      positions[i * 3 + 1] = Math.random() * 4
      positions[i * 3 + 2] = (Math.random() - 0.5) * 2
    }
    const geo = new THREE.BufferGeometry()
    geo.setAttribute('position', new THREE.BufferAttribute(positions, 3))
    this.ventParticles = new THREE.Points(geo, new THREE.PointsMaterial({
      color: 0x4488cc, size: 0.02, transparent: true, opacity: 0.4,
      blending: THREE.AdditiveBlending, depthWrite: false
    }))
    this.scene.add(this.ventParticles)
  }

  /* ===================== 事件 & 动画 ===================== */

  private setupEventListeners() {
    window.addEventListener('resize', this.onResize.bind(this))
  }

  private onResize() {
    const w = this.container.clientWidth, h = this.container.clientHeight
    this.camera.aspect = w / h; this.camera.updateProjectionMatrix()
    this.renderer.setSize(w, h)
  }

  private animate() {
    this.animationId = requestAnimationFrame(this.animate.bind(this))
    const time = this.clock.getElapsedTime()

    this.updateTopScreens(time)
    this.updateDoorScreens(time)
    this.updateWarningLights(time)
    this.updateDoors()
    this.updateGlow()
    this.updateParticles(time)

    this.controls.update()
    this.renderer.render(this.scene, this.camera)
  }

  private updateTopScreens(_t: number) {
    this.topScreens.forEach((s) => {
      const key = s.label
      const sm = this.smoothedValues.get(key) || 0
      this.smoothedValues.set(key, sm + (s.value - sm) * 0.1)
      this.drawTopScreen(s)
    })
  }

  private updateDoorScreens(_time: number) {
    this.doorScreens.forEach((p) => {
      const key = p.name
      const sm = this.smoothedValues.get(key) || 0
      this.smoothedValues.set(key, sm + (p.value - sm) * 0.08)
      this.drawSidePanel(p)
    })
  }

  private updateWarningLights(time: number) {
    const blink = Math.sin(time * 6) > 0
    this.warningLights.forEach((l, i) => {
      if (this.hasAlarm) {
        const phase = i * Math.PI
        l.intensity = (Math.sin(time * 8 + phase) * 0.5 + 0.5) * 2
        const m = this.warningBulbs[i].material as THREE.MeshBasicMaterial
        m.opacity = 0.4 + l.intensity * 0.35
        m.color.setHex(blink ? 0xff2222 : 0xffaa00)
        l.color.setHex(blink ? 0xff2222 : 0xffaa00)
      } else {
        const pulse = Math.sin(time * 2 + i) * 0.15 + 0.85
        l.intensity = 0.8 * pulse
        l.color.setHex(0x22cc66)
        const m = this.warningBulbs[i].material as THREE.MeshBasicMaterial
        m.opacity = 0.5 * pulse; m.color.setHex(0x22cc66)
      }
    })
  }

  private updateDoors() {
    this.doorTarget = this.hasAlarm ? 0.35 : 0
    this.doorOpen += (this.doorTarget - this.doorOpen) * 0.03
    this.doorLeft.rotation.y = -this.doorOpen
    this.doorRight.rotation.y = this.doorOpen
  }

  private updateGlow() {
    const m = this.innerGlow.material as THREE.MeshBasicMaterial
    if (this.hasAlarm) {
      m.color.setHex(0x662222); m.opacity = 0.12 + Math.sin(Date.now() * 0.004) * 0.06
    } else {
      m.color.setHex(0x334466); m.opacity = 0.15
    }
  }

  private updateParticles(time: number) {
    if (!this.ventParticles) return
    this.ventParticles.rotation.y = time * 0.04
    const pos = this.ventParticles.geometry.attributes.position.array as Float32Array
    for (let i = 0; i < pos.length; i += 3) {
      pos[i + 1] += 0.003 + Math.sin(time + i) * 0.001
      if (pos[i + 1] > 4) pos[i + 1] = 0
    }
    this.ventParticles.geometry.attributes.position.needsUpdate = true
  }

  /* ===================== 公开接口 ===================== */

  updateData(data: SensorData) {
    this.currentData = data
    this.hasAlarm = Object.values(data.alert).some((v) => v)

    // 顶部屏幕
    if (this.topScreens[0]) { this.topScreens[0].value = data.temp; this.topScreens[0].alarm = data.alert.temp_high }
    if (this.topScreens[1]) { this.topScreens[1].value = data.humi; this.topScreens[1].alarm = data.alert.humi_high }
    if (this.topScreens[2]) { this.topScreens[2].value = data.mq4; this.topScreens[2].alarm = data.alert.gas }

    // 柜门数据屏
    const valMap: Record<string, number> = { temp: data.temp, humi: data.humi, mq4: data.mq4, tc: data.tc, pressure: data.pressure, flame_mv: data.flame_mv }
    const almMap: Record<string, boolean> = { temp: data.alert.temp_high, humi: data.alert.humi_high, mq4: data.alert.gas, tc: data.alert.tc_high, pressure: false, flame_mv: data.alert.fire }
    this.doorScreens.forEach((p) => { p.value = valMap[p.name] ?? 0; p.alarm = almMap[p.name] ?? false })
  }

  dispose() {
    if (this.animationId) cancelAnimationFrame(this.animationId)
    window.removeEventListener('resize', this.onResize.bind(this))
    this.topScreens.forEach((s) => s.texture.dispose())
    this.doorScreens.forEach((p) => p.texture.dispose())
    this.renderer.dispose()
    if (this.renderer.domElement.parentNode) this.renderer.domElement.parentNode.removeChild(this.renderer.domElement)
  }
}
