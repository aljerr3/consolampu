#ifndef GAME_H
#define GAME_H

const char GAME_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Laberinto MPU</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            color: #fff;
            overflow: hidden;
        }
        h1 { font-size: 1.5em; color: #00d4ff; text-shadow: 0 0 10px rgba(0,212,255,0.5); }
        #info { display: flex; gap: 20px; margin: 10px 0; font-size: 0.9em; }
        .info-item {
            background: rgba(255,255,255,0.1);
            padding: 8px 16px;
            border-radius: 20px;
            border: 1px solid rgba(255,255,255,0.2);
        }
        .info-item span { color: #00ff88; font-weight: bold; }
        #gameContainer {
            position: relative;
            border: 3px solid #00d4ff;
            border-radius: 10px;
            box-shadow: 0 0 30px rgba(0,212,255,0.3);
        }
        #gameCanvas { display: block; background: #0a0a1a; }
        #status {
            position: absolute;
            top: 50%; left: 50%;
            transform: translate(-50%, -50%);
            background: rgba(0,0,0,0.9);
            padding: 30px 50px;
            border-radius: 15px;
            text-align: center;
            display: none;
            border: 2px solid #00d4ff;
        }
        #status h2 { color: #00ff88; margin-bottom: 15px; }
        #status p { color: #aaa; }
        #connectionStatus {
            position: fixed; top: 10px; right: 10px;
            padding: 8px 16px; border-radius: 20px; font-size: 0.8em; font-weight: bold;
        }
        .connected { background: rgba(0,255,136,0.2); color: #00ff88; border: 1px solid #00ff88; }
        .disconnected { background: rgba(255,68,68,0.2); color: #ff4444; border: 1px solid #ff4444; }
        #debug {
            position: fixed; bottom: 10px; left: 10px;
            background: rgba(0,0,0,0.7); padding: 10px; border-radius: 8px;
            font-size: 0.7em; font-family: monospace; color: #888;
        }
        .timer-warning { color: #ff4444 !important; animation: blink 0.5s infinite; }
        @keyframes blink { 50% { opacity: 0.3; } }
        #credits {
            position: fixed; bottom: 10px; right: 10px;
            font-size: 0.75em; color: #556; font-family: 'Segoe UI', sans-serif;
        }
    </style>
</head>
<body>
    <div id="connectionStatus" class="disconnected">Desconectado</div>
    <div id="header">
        <h1>Laberinto MPU</h1>
        <div id="info">
            <div class="info-item" id="timerItem">Tiempo: <span id="timerDisplay">3:00</span></div>
            <div class="info-item">Nivel: <span id="levelDisplay">1</span>/10</div>
            <div class="info-item">Intentos: <span id="attempts">0</span></div>
        </div>
    </div>
    <div id="gameContainer">
        <canvas id="gameCanvas"></canvas>
        <div id="status">
            <h2 id="statusTitle"></h2>
            <p id="statusMessage"></p>
        </div>
    </div>
    <div id="debug">Pitch: <span id="pitchVal">0</span> | Roll: <span id="rollVal">0</span></div>
    <div id="credits">Creado por Alejandro Rodr&iacute;guez - ARMakerLab</div>

    <script>
        const canvas = document.getElementById('gameCanvas');
        const ctx = canvas.getContext('2d');
        const size = Math.min(window.innerWidth - 40, window.innerHeight - 200, 500);
        canvas.width = size;
        canvas.height = size;
        const CELL = size / 20;

        let currentLevel = 1, attempts = 0, gameRunning = false;
        let gameStarted = false;
        let remainingSeconds = 180;

        const ball = {
            x: 0, y: 0, vx: 0, vy: 0, radius: CELL * 0.4,
            reset(sx, sy) { this.x = sx; this.y = sy; this.vx = 0; this.vy = 0; }
        };

        let sensorData = { pitch: 0, roll: 0 };
        const sensitivity = 0.08, friction = 0.94, maxSpeed = CELL * 0.3;

        const levels = [
            // Nivel 1 - Tutorial
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:6,y:0,w:1,h:14},{x:13,y:6,w:1,h:14}], holes: [{x:3,y:10}], start: {x:2,y:2}, goal: {x:17,y:17} },
            // Nivel 2
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:4,y:0,w:1,h:12},{x:9,y:8,w:1,h:12},{x:14,y:0,w:1,h:12}], holes: [{x:2,y:8},{x:7,y:14},{x:12,y:8},{x:17,y:14}], start: {x:2,y:2}, goal: {x:17,y:17} },
            // Nivel 3
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:3,y:4,w:14,h:1},{x:3,y:10,w:14,h:1},{x:3,y:15,w:14,h:1},{x:3,y:4,w:1,h:6},{x:16,y:10,w:1,h:6}], holes: [{x:8,y:2},{x:8,y:7},{x:8,y:13},{x:8,y:17}], start: {x:2,y:2}, goal: {x:17,y:17} },
            // Nivel 4
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:4,y:0,w:1,h:8},{x:4,y:12,w:1,h:8},{x:8,y:4,w:1,h:12},{x:12,y:0,w:1,h:8},{x:12,y:12,w:1,h:8},{x:16,y:4,w:1,h:12}], holes: [{x:2,y:10},{x:6,y:5},{x:6,y:14},{x:10,y:10},{x:14,y:5},{x:14,y:14}], start: {x:2,y:2}, goal: {x:17,y:17} },
            // Nivel 5
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:3,y:0,w:1,h:7},{x:3,y:10,w:1,h:4},{x:6,y:4,w:1,h:8},{x:6,y:15,w:1,h:5},{x:9,y:0,w:1,h:5},{x:9,y:8,w:1,h:7},{x:12,y:3,w:1,h:5},{x:12,y:11,w:1,h:9},{x:15,y:0,w:1,h:8},{x:15,y:11,w:1,h:5}], holes: [{x:1.5,y:8},{x:4.5,y:3},{x:4.5,y:13},{x:7.5,y:7},{x:7.5,y:16},{x:10.5,y:6},{x:10.5,y:16},{x:13.5,y:9},{x:17,y:5},{x:17,y:15}], start: {x:1.5,y:1.5}, goal: {x:17,y:17} },
            // Nivel 6
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:4,y:0,w:1,h:6},{x:4,y:9,w:1,h:5},{x:4,y:17,w:1,h:3},{x:7,y:3,w:1,h:5},{x:7,y:11,w:1,h:6},{x:10,y:0,w:1,h:4},{x:10,y:7,w:1,h:6},{x:10,y:16,w:1,h:4},{x:13,y:2,w:1,h:6},{x:13,y:11,w:1,h:5},{x:16,y:0,w:1,h:7},{x:16,y:10,w:1,h:6}], holes: [{x:2,y:7},{x:2,y:15},{x:5.5,y:2},{x:5.5,y:8},{x:5.5,y:17},{x:8.5,y:5},{x:8.5,y:14},{x:11.5,y:6},{x:11.5,y:17},{x:14.5,y:8},{x:14.5,y:16},{x:17.5,y:8}], start: {x:1.5,y:1.5}, goal: {x:18,y:18} },
            // Nivel 7
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:3,y:0,w:1,h:5},{x:3,y:8,w:1,h:4},{x:3,y:15,w:1,h:5},{x:6,y:3,w:1,h:5},{x:6,y:11,w:1,h:6},{x:9,y:0,w:1,h:4},{x:9,y:7,w:1,h:5},{x:9,y:15,w:1,h:5},{x:12,y:2,w:1,h:5},{x:12,y:10,w:1,h:5},{x:12,y:18,w:1,h:2},{x:15,y:0,w:1,h:6},{x:15,y:9,w:1,h:5},{x:15,y:17,w:1,h:3},{x:3,y:8,w:4,h:1},{x:9,y:12,w:4,h:1}], holes: [{x:1.5,y:6},{x:1.5,y:13},{x:4.5,y:2},{x:4.5,y:9},{x:4.5,y:17},{x:7.5,y:5},{x:7.5,y:13},{x:10.5,y:5},{x:10.5,y:13},{x:10.5,y:17},{x:13.5,y:7},{x:13.5,y:16},{x:16.5,y:7},{x:17,y:15}], start: {x:1.5,y:1.5}, goal: {x:18,y:18} },
            // Nivel 8
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:4,y:4,w:12,h:1},{x:4,y:8,w:12,h:1},{x:4,y:12,w:12,h:1},{x:4,y:15,w:12,h:1},{x:4,y:4,w:1,h:4},{x:15,y:8,w:1,h:4},{x:4,y:12,w:1,h:4}], holes: [{x:8,y:2},{x:8,y:6},{x:8,y:10},{x:8,y:14},{x:8,y:17},{x:12,y:6},{x:12,y:14}], start: {x:2,y:2}, goal: {x:17,y:17} },
            // Nivel 9
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:3,y:0,w:1,h:6},{x:3,y:9,w:1,h:5},{x:3,y:17,w:1,h:3},{x:7,y:3,w:1,h:6},{x:7,y:12,w:1,h:5},{x:11,y:0,w:1,h:5},{x:11,y:8,w:1,h:6},{x:11,y:17,w:1,h:3},{x:15,y:3,w:1,h:5},{x:15,y:11,w:1,h:6}], holes: [{x:1.5,y:7},{x:5,y:11},{x:5,y:16},{x:9,y:6},{x:9,y:15},{x:13,y:7},{x:13,y:17},{x:17,y:9}], start: {x:1.5,y:1.5}, goal: {x:17,y:17} },
            // Nivel 10 - Final
            { walls: [{x:0,y:0,w:20,h:1},{x:0,y:19,w:20,h:1},{x:0,y:0,w:1,h:20},{x:19,y:0,w:1,h:20},{x:3,y:3,w:14,h:1},{x:3,y:7,w:14,h:1},{x:3,y:11,w:14,h:1},{x:3,y:15,w:14,h:1},{x:3,y:3,w:1,h:4},{x:16,y:7,w:1,h:4},{x:3,y:11,w:1,h:4},{x:16,y:15,w:1,h:4}], holes: [{x:6,y:1.5},{x:12,y:1.5},{x:6,y:5},{x:12,y:5},{x:6,y:9},{x:12,y:9},{x:6,y:13},{x:12,y:13},{x:6,y:17},{x:12,y:17}], start: {x:1.5,y:1.5}, goal: {x:17,y:17} }
        ];

        let currentLevelData = levels[0];
        let ws;

        function connect() {
            ws = new WebSocket('ws://' + location.hostname + ':81');
            ws.onopen = () => {
                document.getElementById('connectionStatus').className = 'connected';
                document.getElementById('connectionStatus').textContent = 'Conectado';
            };
            ws.onmessage = (e) => {
                try {
                    const d = JSON.parse(e.data);
                    // Comandos del servidor
                    if (d.cmd === 'start') {
                        remainingSeconds = d.duration || 180;
                        gameStarted = true;
                        gameRunning = true;
                        currentLevel = 1;
                        attempts = 0;
                        document.getElementById('levelDisplay').textContent = '1';
                        document.getElementById('attempts').textContent = '0';
                        loadLevel(1);
                        hideMsg();
                        updateTimerDisplay();
                    } else if (d.cmd === 'timeout') {
                        gameRunning = false;
                        gameStarted = false;
                        remainingSeconds = 0;
                        updateTimerDisplay();
                        showMsg('Tiempo Agotado!', 'No completaste los 10 niveles. El juego se reiniciara...');
                    } else if (d.cmd === 'win') {
                        gameRunning = false;
                        gameStarted = false;
                    }
                    // Timer del servidor
                    if (d.timer !== undefined) {
                        remainingSeconds = d.timer;
                        updateTimerDisplay();
                    }
                    // Datos del sensor
                    if (d.pitch !== undefined) {
                        sensorData.pitch = d.pitch;
                        sensorData.roll = d.roll || 0;
                        document.getElementById('pitchVal').textContent = sensorData.pitch.toFixed(1);
                        document.getElementById('rollVal').textContent = sensorData.roll.toFixed(1);
                    }
                } catch(err) {}
            };
            ws.onclose = () => {
                document.getElementById('connectionStatus').className = 'disconnected';
                document.getElementById('connectionStatus').textContent = 'Desconectado';
                setTimeout(connect, 2000);
            };
        }

        function circleRect(cx, cy, r, rx, ry, rw, rh) {
            const closestX = Math.max(rx, Math.min(cx, rx + rw));
            const closestY = Math.max(ry, Math.min(cy, ry + rh));
            return ((cx - closestX) ** 2 + (cy - closestY) ** 2) < r * r;
        }

        function checkWalls(nx, ny) {
            for (let w of currentLevelData.walls) {
                if (circleRect(nx, ny, ball.radius, w.x * CELL, w.y * CELL, w.w * CELL, w.h * CELL)) return true;
            }
            return false;
        }

        function checkHoles() {
            for (let h of currentLevelData.holes) {
                const hx = (h.x + 0.5) * CELL, hy = (h.y + 0.5) * CELL;
                if (Math.hypot(ball.x - hx, ball.y - hy) < CELL * 0.45) return true;
            }
            return false;
        }

        function checkGoal() {
            const gx = (currentLevelData.goal.x + 0.5) * CELL;
            const gy = (currentLevelData.goal.y + 0.5) * CELL;
            return Math.hypot(ball.x - gx, ball.y - gy) < CELL * 0.6 - ball.radius;
        }

        function update() {
            if (!gameRunning) return;
            ball.vx += sensorData.roll * sensitivity;
            ball.vy += sensorData.pitch * sensitivity;
            const spd = Math.hypot(ball.vx, ball.vy);
            if (spd > maxSpeed) { ball.vx = ball.vx / spd * maxSpeed; ball.vy = ball.vy / spd * maxSpeed; }
            ball.vx *= friction; ball.vy *= friction;
            let nx = ball.x + ball.vx, ny = ball.y + ball.vy;
            if (checkWalls(nx, ball.y)) { ball.vx *= -0.3; nx = ball.x; }
            if (checkWalls(ball.x, ny)) { ball.vy *= -0.3; ny = ball.y; }
            ball.x = nx; ball.y = ny;
            if (checkHoles()) fallIn();
            if (checkGoal()) win();
        }

        function fallIn() {
            gameRunning = false;
            attempts++;
            document.getElementById('attempts').textContent = attempts;
            showMsg('Caiste!', 'Reintentando...');
            setTimeout(() => { if (!gameStarted) return; hideMsg(); resetLevel(); gameRunning = true; }, 1000);
        }

        function win() {
            gameRunning = false;
            if (currentLevel < 10) {
                showMsg('Nivel ' + currentLevel + ' Completado!', 'Siguiente nivel...');
                setTimeout(() => { if (!gameStarted) return; hideMsg(); currentLevel++; document.getElementById('levelDisplay').textContent = currentLevel; loadLevel(currentLevel); gameRunning = true; }, 1500);
            } else {
                gameStarted = false;
                showMsg('Felicidades!', 'Completaste los 10 niveles con ' + attempts + ' intentos en ' + formatTime(180 - remainingSeconds) + '!');
                if (ws && ws.readyState === WebSocket.OPEN) ws.send('win');
            }
        }

        function updateTimerDisplay() {
            const el = document.getElementById('timerDisplay');
            const mins = Math.floor(remainingSeconds / 60);
            const secs = remainingSeconds % 60;
            el.textContent = mins + ':' + (secs < 10 ? '0' : '') + secs;
            const item = document.getElementById('timerItem');
            if (remainingSeconds <= 30) {
                item.classList.add('timer-warning');
            } else {
                item.classList.remove('timer-warning');
            }
        }

        function formatTime(totalSec) {
            const m = Math.floor(totalSec / 60);
            const s = totalSec % 60;
            return m + ':' + (s < 10 ? '0' : '') + s;
        }

        function showMsg(t, m) {
            document.getElementById('statusTitle').textContent = t;
            document.getElementById('statusMessage').textContent = m;
            document.getElementById('status').style.display = 'block';
        }
        function hideMsg() { document.getElementById('status').style.display = 'none'; }

        function loadLevel(n) { currentLevelData = levels[n - 1]; resetLevel(); }
        function resetLevel() { ball.reset((currentLevelData.start.x + 0.5) * CELL, (currentLevelData.start.y + 0.5) * CELL); }

        function draw() {
            ctx.fillStyle = '#0a0a1a'; ctx.fillRect(0, 0, canvas.width, canvas.height);
            // Paredes
            ctx.fillStyle = '#3d5a80'; ctx.strokeStyle = '#5d8ab4'; ctx.lineWidth = 2;
            for (let w of currentLevelData.walls) { ctx.fillRect(w.x * CELL, w.y * CELL, w.w * CELL, w.h * CELL); ctx.strokeRect(w.x * CELL, w.y * CELL, w.w * CELL, w.h * CELL); }
            // Hoyos
            for (let h of currentLevelData.holes) {
                const hx = (h.x + 0.5) * CELL, hy = (h.y + 0.5) * CELL, hr = CELL * 0.45;
                const g = ctx.createRadialGradient(hx, hy, 0, hx, hy, hr);
                g.addColorStop(0, '#000'); g.addColorStop(1, '#330000');
                ctx.beginPath(); ctx.arc(hx, hy, hr, 0, Math.PI * 2); ctx.fillStyle = g; ctx.fill();
                ctx.strokeStyle = '#660000'; ctx.stroke();
            }
            // Meta
            const gx = (currentLevelData.goal.x + 0.5) * CELL, gy = (currentLevelData.goal.y + 0.5) * CELL, gr = CELL * 0.6;
            const pulse = Math.sin(Date.now() / 200) * 0.2 + 0.8;
            const gg = ctx.createRadialGradient(gx, gy, 0, gx, gy, gr);
            gg.addColorStop(0, `rgba(0,255,136,${pulse})`); gg.addColorStop(1, 'rgba(0,150,75,0.3)');
            ctx.beginPath(); ctx.arc(gx, gy, gr, 0, Math.PI * 2); ctx.fillStyle = gg; ctx.fill();
            ctx.strokeStyle = '#00ff88'; ctx.lineWidth = 2; ctx.stroke();
            // Pelota
            const bg = ctx.createRadialGradient(ball.x - ball.radius * 0.3, ball.y - ball.radius * 0.3, 0, ball.x, ball.y, ball.radius);
            bg.addColorStop(0, '#ff6b6b'); bg.addColorStop(1, '#cc4444');
            ctx.beginPath(); ctx.arc(ball.x, ball.y, ball.radius, 0, Math.PI * 2); ctx.fillStyle = bg; ctx.fill();
            ctx.strokeStyle = '#ff8888'; ctx.lineWidth = 2; ctx.stroke();
        }

        function loop() { update(); draw(); requestAnimationFrame(loop); }

        loadLevel(1);
        showMsg('Esperando...', 'Conectando con ConsolaMPU...');
        connect();
        loop();
    </script>
</body>
</html>
)rawliteral";

#endif
