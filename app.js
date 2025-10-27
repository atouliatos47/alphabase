// app.js - Main Application Controller

const app = {
    currentView: 'dashboard',

    // Initialize application
    init() {
        console.log('🚀 Initializing AlphaBase Console v4.0...');

        // Setup login form
        document.getElementById('loginForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleLogin();
        });

        // Check if already logged in (for development)
        // In production, you'd check for stored token
        console.log('✅ Application initialized');
    },

    // Handle login
    async handleLogin() {
        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;

        try {
            const result = await api.login(username, password);

            if (result.success) {
                // Hide login, show console
                document.getElementById('loginScreen').style.display = 'none';
                document.getElementById('console').style.display = 'block';
                document.getElementById('currentUser').textContent = username;

                // Connect to WebSocket for real-time updates
                wsManager.connect();

                // Load initial dashboard
                await dashboard.loadDashboard();

                // Show welcome message
                wsManager.showAlert(
                    'Welcome!',
                    `Signed in as ${username}`,
                    'success',
                    3000
                );

            } else {
                this.showLoginStatus('Sign in failed. Please check your credentials.', 'error');
            }
        } catch (error) {
            this.showLoginStatus('Sign in failed. ' + error.message, 'error');
        }
    },

    // Logout
    logout() {
        // Disconnect WebSocket
        wsManager.disconnect();

        // Reset API
        api.authToken = null;
        api.currentUsername = null;

        // Show login screen
        document.getElementById('loginScreen').style.display = 'flex';
        document.getElementById('console').style.display = 'none';
        document.getElementById('loginForm').reset();

        console.log('👋 Logged out');
    },

    // Switch between views
    switchView(viewName) {
        console.log(`📄 Switching to view: ${viewName}`);

        // Update navigation
        document.querySelectorAll('.nav-item').forEach(item => item.classList.remove('active'));
        event.target.classList.add('active');

        // Update views
        document.querySelectorAll('.view').forEach(view => view.classList.remove('active'));

        // Store current view
        this.currentView = viewName;

        // Show selected view and load its data
        // Show selected view and load its data
        if (viewName === 'dashboard') {
            document.getElementById('dashboardView').classList.add('active');
            dashboard.loadDashboard();
        } else if (viewName === 'analytics') {
            document.getElementById('analyticsView').classList.add('active');
            charts.loadAnalytics();
        } else if (viewName === 'data') {
            document.getElementById('dataView').classList.add('active');
        } else if (viewName === 'collections') {
            document.getElementById('collectionsView').classList.add('active');
            dashboard.loadCollectionsView();
        }
    },

    // Show login status message
    showLoginStatus(message, type) {
        const statusDiv = document.getElementById('loginStatus');
        statusDiv.textContent = message;
        statusDiv.className = `status ${type}`;
        setTimeout(() => {
            statusDiv.style.display = 'none';
        }, 3000);
    }
};

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    app.init();
});