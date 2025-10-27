/**
 * AlphaBase Client Library v1.0
 * Universal backend client for web applications
 * 
 * Usage:
 *   const ab = new AlphaBase('http://your-server:8000');
 *   await ab.login('username', 'password');
 *   await ab.set('collection', 'key', { data });
 */

class AlphaBase {
    constructor(baseURL) {
        this.baseURL = baseURL || 'http://localhost:8000';
        this.authToken = null;
        this.currentUser = null;
        this.ws = null;
        this.wsCallbacks = [];
    }

    // ========================================================================
    // AUTHENTICATION
    // ========================================================================

    /**
     * Register a new user
     * @param {string} username 
     * @param {string} email 
     * @param {string} password 
     * @returns {Promise<Object>}
     */
    async register(username, email, password) {
        try {
            const response = await fetch(`${this.baseURL}/auth/register`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, email, password })
            });

            if (!response.ok) {
                throw new Error('Registration failed');
            }

            const data = await response.json();
            this.authToken = data.access_token;
            this.currentUser = username;

            return { success: true, token: data.access_token };
        } catch (error) {
            console.error('AlphaBase Register Error:', error);
            return { success: false, error: error.message };
        }
    }

    /**
     * Login to AlphaBase
     * @param {string} username 
     * @param {string} password 
     * @returns {Promise<Object>}
     */
    async login(username, password) {
        try {
            const response = await fetch(`${this.baseURL}/auth/login`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, password })
            });

            if (!response.ok) {
                throw new Error('Login failed');
            }

            const data = await response.json();
            this.authToken = data.access_token;
            this.currentUser = username;

            return { success: true, token: data.access_token };
        } catch (error) {
            console.error('AlphaBase Login Error:', error);
            return { success: false, error: error.message };
        }
    }

    /**
     * Get current user info
     * @returns {Promise<Object>}
     */
    async getCurrentUser() {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const response = await fetch(`${this.baseURL}/auth/me`, {
                headers: { 'Authorization': `Bearer ${this.authToken}` }
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase Get User Error:', error);
            throw error;
        }
    }

    /**
     * Logout
     */
    logout() {
        this.authToken = null;
        this.currentUser = null;
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
    }

    // ========================================================================
    // DATA OPERATIONS (CRUD)
    // ========================================================================

    /**
     * Set/Update data
     * @param {string} collection 
     * @param {string} key 
     * @param {Object} value 
     * @returns {Promise<Object>}
     */
    async set(collection, key, value) {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const response = await fetch(`${this.baseURL}/data/set`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${this.authToken}`
                },
                body: JSON.stringify({ collection, key, value })
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase Set Error:', error);
            throw error;
        }
    }

    /**
     * Get data
     * @param {string} collection 
     * @param {string} key 
     * @returns {Promise<Object>}
     */
    async get(collection, key) {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const response = await fetch(`${this.baseURL}/data/get/${collection}/${key}`, {
                headers: { 'Authorization': `Bearer ${this.authToken}` }
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase Get Error:', error);
            throw error;
        }
    }

    /**
     * List all data in a collection
     * @param {string} collection 
     * @returns {Promise<Object>}
     */
    async list(collection) {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const response = await fetch(`${this.baseURL}/data/list/${collection}`, {
                headers: { 'Authorization': `Bearer ${this.authToken}` }
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase List Error:', error);
            throw error;
        }
    }

    /**
     * Query data with filters
     * @param {string} collection 
     * @param {Object} options - { where: 'field==value', orderBy: 'field', limit: 10 }
     * @returns {Promise<Object>}
     */
    async query(collection, options = {}) {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const params = new URLSearchParams();
            if (options.where) params.append('where', options.where);
            if (options.orderBy) params.append('orderBy', options.orderBy);
            if (options.limit) params.append('limit', options.limit);

            const response = await fetch(`${this.baseURL}/data/query/${collection}?${params}`, {
                headers: { 'Authorization': `Bearer ${this.authToken}` }
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase Query Error:', error);
            throw error;
        }
    }

    /**
     * Delete data
     * @param {string} collection 
     * @param {string} key 
     * @returns {Promise<Object>}
     */
    async delete(collection, key) {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const response = await fetch(`${this.baseURL}/data/delete/${collection}/${key}`, {
                method: 'DELETE',
                headers: { 'Authorization': `Bearer ${this.authToken}` }
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase Delete Error:', error);
            throw error;
        }
    }

    // ========================================================================
    // REAL-TIME (WebSocket)
    // ========================================================================

    /**
     * Connect to real-time updates
     * @param {Function} callback - Called when data updates
     */
    connectRealtime(callback) {
        const wsURL = this.baseURL.replace('http', 'ws') + '/ws';

        this.ws = new WebSocket(wsURL);

        this.ws.onopen = () => {
            console.log('✅ AlphaBase Real-time Connected');
        };

        this.ws.onmessage = (event) => {
            try {
                const message = JSON.parse(event.data);
                
                // Call all registered callbacks
                this.wsCallbacks.forEach(cb => cb(message));
                
                // Call the provided callback
                if (callback) callback(message);
            } catch (error) {
                console.error('WebSocket message error:', error);
            }
        };

        this.ws.onerror = (error) => {
            console.error('❌ AlphaBase WebSocket Error:', error);
        };

        this.ws.onclose = () => {
            console.log('❌ AlphaBase Real-time Disconnected');
        };
    }

    /**
     * Add a callback for real-time updates
     * @param {Function} callback 
     */
    onUpdate(callback) {
        this.wsCallbacks.push(callback);
    }

    /**
     * Disconnect from real-time
     */
    disconnectRealtime() {
        if (this.ws) {
            this.ws.close();
            this.ws = null;
            this.wsCallbacks = [];
        }
    }

    // ========================================================================
    // FILE STORAGE
    // ========================================================================

    /**
     * Upload a file
     * @param {File} file 
     * @param {boolean} isPublic 
     * @returns {Promise<Object>}
     */
    async uploadFile(file, isPublic = false) {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const formData = new FormData();
            formData.append('file', file);
            formData.append('is_public', isPublic.toString());

            const response = await fetch(`${this.baseURL}/storage/upload`, {
                method: 'POST',
                headers: { 'Authorization': `Bearer ${this.authToken}` },
                body: formData
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase Upload Error:', error);
            throw error;
        }
    }

    /**
     * Get file download URL
     * @param {string} fileId 
     * @returns {string}
     */
    getFileURL(fileId) {
        return `${this.baseURL}/storage/download/${fileId}`;
    }

    /**
     * List user's files
     * @returns {Promise<Object>}
     */
    async listFiles() {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const response = await fetch(`${this.baseURL}/storage/files`, {
                headers: { 'Authorization': `Bearer ${this.authToken}` }
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase List Files Error:', error);
            throw error;
        }
    }

    /**
     * Delete a file
     * @param {string} fileId 
     * @returns {Promise<Object>}
     */
    async deleteFile(fileId) {
        if (!this.authToken) {
            throw new Error('Not authenticated. Call login() first.');
        }

        try {
            const response = await fetch(`${this.baseURL}/storage/delete/${fileId}`, {
                method: 'DELETE',
                headers: { 'Authorization': `Bearer ${this.authToken}` }
            });

            return await response.json();
        } catch (error) {
            console.error('AlphaBase Delete File Error:', error);
            throw error;
        }
    }

    // ========================================================================
    // UTILITIES
    // ========================================================================

    /**
     * Check if user is authenticated
     * @returns {boolean}
     */
    isAuthenticated() {
        return this.authToken !== null;
    }

    /**
     * Get auth token
     * @returns {string|null}
     */
    getToken() {
        return this.authToken;
    }

    /**
     * Set auth token manually
     * @param {string} token 
     */
    setToken(token) {
        this.authToken = token;
    }
}

// Export for use in Node.js or as module
if (typeof module !== 'undefined' && module.exports) {
    module.exports = AlphaBase;
}