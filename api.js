// api.js - AlphaBase API Communication Module

const api = {
    baseURL: 'http://localhost:8000',
    authToken: null,
    currentUsername: null,

    // Login to AlphaBase
    async login(username, password) {
        try {
            const response = await fetch(`${this.baseURL}/auth/login`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ username, password })
            });

            if (!response.ok) {
                throw new Error('Invalid credentials');
            }

            const data = await response.json();
            this.authToken = data.access_token;
            this.currentUsername = username;

            return { success: true };
        } catch (error) {
            console.error('Login error:', error);
            return { success: false, error: error.message };
        }
    },

    // Fetch all accessible collections (auto-discovery)
    async fetchAllCollections() {
        const collections = {};

        try {
            // Get list of all collections from server
            const response = await fetch(`${this.baseURL}/data/collections`, {
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            // Option 2: Try common collection names + any we find
            const knownCollections = [
                'sensors', 'devices', 'my_collection', 'presses', 'todos',
                'users', 'products', 'orders', 'customers', 'inventory',
                'tasks', 'notes', 'messages', 'notifications', 'settings',
                'logs', 'events', 'analytics', 'files', 'uploads',
                // ESP32 related collections
                'esp32', 'esp32_sensors', 'esp32_data', 'esp32_logs',
                'iot_devices', 'iot_sensors', 'microcontroller', 'arduino',
                'sensor_data', 'device_data', 'telemetry', 'iot_telemetry'
            ];

            let availableCollections = [];

            if (response.ok) {
                const result = await response.json();
                if (result.success && result.collections.length > 0) {
                    availableCollections = result.collections;
                }
            }

            // Combine server collections with known collections
            const allCollectionsToTry = [...new Set([...availableCollections, ...knownCollections])];

            // Fetch data from each collection
            for (const collectionName of allCollectionsToTry) {
                try {
                    const dataResponse = await fetch(`${this.baseURL}/data/list/${collectionName}`, {
                        headers: {
                            'Authorization': `Bearer ${this.authToken}`
                        }
                    });

                    if (dataResponse.ok) {
                        const dataResult = await dataResponse.json();

                        if (dataResult.success && Object.keys(dataResult.items).length > 0) {
                            collections[collectionName] = dataResult.items;
                        }
                    }
                } catch (error) {
                    console.log(`⏭️ Skipping ${collectionName}: ${error.message}`);
                }
            }
        } catch (error) {
            console.error('Error fetching collections:', error);
        }

        return collections;
    },

    // Get specific data
    async getData(collection, key) {
        try {
            const response = await fetch(`${this.baseURL}/data/get/${collection}/${key}`, {
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            const result = await response.json();
            return result;
        } catch (error) {
            console.error('Get data error:', error);
            return { success: false, error: error.message };
        }
    },

    // Save data
    async saveData() {
        const collection = document.getElementById('collection').value;
        const key = document.getElementById('key').value;
        const dataText = document.getElementById('dataValue').value;

        if (!collection || !key || !dataText) {
            this.showDataStatus('Please fill in all fields', 'error');
            return;
        }

        try {
            const value = JSON.parse(dataText);

            const response = await fetch(`${this.baseURL}/data/set`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${this.authToken}`
                },
                body: JSON.stringify({
                    collection: collection,
                    key: key,
                    value: value
                })
            });

            const result = await response.json();
            this.showDataResult(result);
            this.showDataStatus('Data saved successfully', 'success');

        } catch (error) {
            this.showDataStatus('Error: ' + error.message, 'error');
        }
    },

    // Load data
    async loadData() {
        const collection = document.getElementById('collection').value;
        const key = document.getElementById('key').value;

        if (!collection || !key) {
            this.showDataStatus('Please enter collection and key', 'error');
            return;
        }

        try {
            const response = await fetch(`${this.baseURL}/data/get/${collection}/${key}`, {
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            const result = await response.json();

            if (result.success) {
                this.showDataResult(result);
                document.getElementById('dataValue').value = JSON.stringify(result.data, null, 2);
                this.showDataStatus('Data loaded successfully', 'success');
            } else {
                this.showDataStatus('Data not found', 'error');
            }
        } catch (error) {
            this.showDataStatus('Error: ' + error.message, 'error');
        }
    },

    // Delete item
    async deleteItem(collection, key) {
        if (!confirm(`Delete ${key} from ${collection}?`)) {
            return;
        }

        try {
            const response = await fetch(`${this.baseURL}/data/delete/${collection}/${key}`, {
                method: 'DELETE',
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            const result = await response.json();

            if (result.success) {
                alert(`✅ Deleted ${key} from ${collection}`, 'success');
                console.log('Data deleted - dashboard will auto-refresh via WebSocket');
            } else {
                alert('Failed to delete data', 'error');
            }
        } catch (error) {
            alert('Error: ' + error.message, 'error');
        }
    },

    // Fetch press events
    async fetchPressEvents() {
        try {
            const response = await fetch(`${this.baseURL}/data/list/presses`, {
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            if (!response.ok) return [];

            const result = await response.json();

            if (result.success) {
                // Convert to array and sort by timestamp
                const events = Object.entries(result.items).map(([key, value]) => ({
                    key: key,
                    ...value
                }));

                events.sort((a, b) => b.timestamp - a.timestamp);
                return events;
            }

            return [];
        } catch (error) {
            console.error('Fetch press events error:', error);
            return [];
        }
    },

    // Get system status
    async getSystemStatus() {
        try {
            const response = await fetch(`${this.baseURL}/system/status`, {
                headers: {
                    'Authorization': `Bearer ${this.authToken}`
                }
            });

            const result = await response.json();
            return result;
        } catch (error) {
            console.error('System status error:', error);
            return null;
        }
    },

    // UI Helper Functions
    showDataStatus(message, type) {
        const statusDiv = document.getElementById('dataStatus');
        statusDiv.textContent = message;
        statusDiv.className = `status ${type}`;
        setTimeout(() => {
            statusDiv.style.display = 'none';
        }, 3000);
    },

    showDataResult(data) {
        const resultDiv = document.getElementById('dataResult');
        const responseDiv = document.getElementById('dataResponse');
        responseDiv.textContent = JSON.stringify(data, null, 2);
        resultDiv.style.display = 'block';
    }
};