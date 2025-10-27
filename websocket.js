// websocket.js - Real-time WebSocket Communication Module

const wsManager = {
    ws: null,
    reconnectAttempts: 0,
    maxReconnectAttempts: 5,
    reconnectDelay: 3000,

    // Connect to WebSocket
    connect() {
        const wsURL = 'ws://localhost:8000/ws';

        console.log('🔌 Connecting to WebSocket...');
        this.ws = new WebSocket(wsURL);

        this.ws.onopen = () => {
            console.log('✅ WebSocket connected - Real-time updates enabled');
            this.reconnectAttempts = 0;
            this.updateRealtimeStatus(true);
            this.showAlert('Real-time Connected', 'Connected to live updates', 'success');
        };

        this.ws.onmessage = (event) => {
            try {
                const message = JSON.parse(event.data);
                console.log('📨 Real-time update received:', message);

                // Handle different message types
                this.handleMessage(message);

            } catch (error) {
                console.error('WebSocket message parse error:', error);
            }
        };

        this.ws.onerror = (error) => {
            console.error('❌ WebSocket error:', error);
            this.updateRealtimeStatus(false);
        };

        this.ws.onclose = (event) => {
            console.log(`❌ WebSocket disconnected: ${event.code} - ${event.reason}`);
            this.updateRealtimeStatus(false);

            // Attempt to reconnect
            if (this.reconnectAttempts < this.maxReconnectAttempts) {
                this.reconnectAttempts++;
                console.log(`🔄 Reconnecting... (Attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
                setTimeout(() => this.connect(), this.reconnectDelay);
            } else {
                this.showAlert('Connection Lost', 'Real-time updates disconnected. Refresh the page to reconnect.', 'error');
            }
        };
    },

    // Handle incoming messages
    handleMessage(message) {
        const { action, collection, key, source } = message;

        // Show notification for updates
        if (action === 'update') {
            this.showAlert(
                'Data Updated',
                `${collection}/${key} was ${source === 'mqtt' ? 'updated via MQTT' : 'updated'}`,
                'info',
                3000
            );
        } else if (action === 'delete') {
            this.showAlert(
                'Data Deleted',
                `${collection}/${key} was deleted`,
                'warning',
                3000
            );
        }

        // Auto-refresh active views
        if (app.currentView === 'dashboard') {
            console.log('🔄 Auto-refreshing dashboard...');
            dashboard.loadDashboard();
        }

        if (app.currentView === 'analytics') {
            console.log('🔄 Auto-refreshing analytics...');
            charts.loadAnalytics();
        }


        if (app.currentView === 'collections') {
            console.log('🔄 Auto-refreshing collections...');
            dashboard.loadCollectionsView();
        }


    },

    // Update real-time badge status
    updateRealtimeStatus(connected) {
        const badge = document.querySelector('.realtime-badge');
        const dot = document.querySelector('.realtime-dot');
        const text = document.getElementById('realtimeText');

        if (connected) {
            badge.style.background = '#e6f4ea';
            badge.style.color = '#137333';
            dot.style.background = '#34a853';
            dot.style.animation = 'pulse 2s infinite';
            text.textContent = 'Real-time';
        } else {
            badge.style.background = '#fce8e6';
            badge.style.color = '#c5221f';
            dot.style.background = '#ea4335';
            dot.style.animation = 'none';
            text.textContent = 'Disconnected';
        }
    },

    // Show alert notification
    showAlert(title, message, type = 'info', duration = 5000) {
        const container = document.getElementById('alertContainer');

        const alert = document.createElement('div');
        alert.className = `alert ${type}`;
        alert.innerHTML = `
            <div class="alert-header">
                <span class="alert-title">${title}</span>
                <button class="alert-close" onclick="this.parentElement.parentElement.remove()">×</button>
            </div>
            <div class="alert-message">${message}</div>
        `;

        container.appendChild(alert);

        // Auto-remove after duration
        setTimeout(() => {
            if (alert.parentElement) {
                alert.style.animation = 'slideIn 0.3s ease-out reverse';
                setTimeout(() => alert.remove(), 300);
            }
        }, duration);
    },

    // Disconnect
    disconnect() {
        if (this.ws) {
            this.ws.close();
            this.ws = null;
            this.updateRealtimeStatus(false);
        }
    }
};