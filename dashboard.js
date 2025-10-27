// dashboard.js - Dashboard Logic Module

const dashboard = {
    allCollections: {},

    
    // Load main dashboard
    async loadDashboard() {
        try {
            console.log('📊 Loading dashboard...');
            
            // Get all collections and count items
            const collections = await api.fetchAllCollections();
            this.allCollections = collections;
            
            const totalCollections = Object.keys(collections).length;
            let totalItems = 0;
            
            Object.values(collections).forEach(items => {
                totalItems += Object.keys(items).length;
            });
            
            // Update dashboard stats
            document.getElementById('totalUsers').textContent = '1';
            document.getElementById('totalCollections').textContent = totalCollections;
            document.getElementById('totalItems').textContent = totalItems;
            
            // Display recent data
            this.displayRecentData(collections);
            
        } catch (error) {
            console.error('Error loading dashboard:', error);
        }
    },
    
    // Display recent data with DELETE buttons
    displayRecentData(collections) {
        const container = document.getElementById('recentDataTable');
        
        const allItems = [];
        
        Object.entries(collections).forEach(([collectionName, items]) => {
            Object.entries(items).forEach(([key, value]) => {
                allItems.push({
                    collection: collectionName,
                    key: key,
                    data: value
                });
            });
        });
        
        if (allItems.length === 0) {
            container.innerHTML = '<div class="empty-state"><h3>No data yet</h3><p>Create data in the Data Manager</p></div>';
            return;
        }
        
        // Show only the 10 most recent items
        const recentItems = allItems.slice(0, 10);
        
        container.innerHTML = `
            <table class="data-table">
                <thead>
                    <tr>
                        <th>Collection</th>
                        <th>Key</th>
                        <th>Owner</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody>
                    ${recentItems.map(item => `
                        <tr>
                            <td><span class="badge badge-primary">${item.collection}</span></td>
                            <td>${item.key}</td>
                            <td>${api.currentUsername}</td>
                            <td>
                                <button class="btn-delete" onclick="api.deleteItem('${item.collection}', '${item.key}')">
                                    Delete
                                </button>
                            </td>
                        </tr>
                    `).join('')}
                </tbody>
            </table>
        `;
    },
    

    
    // Load Collections View
    async loadCollectionsView() {
        const container = document.getElementById('collectionsTable');
        container.innerHTML = '<div class="loading">Loading...</div>';
        
        const collections = await api.fetchAllCollections();
        
        if (Object.keys(collections).length === 0) {
            container.innerHTML = '<div class="empty-state"><h3>No collections</h3><p>Create data to see collections</p></div>';
            return;
        }
        
        container.innerHTML = `
            <table class="data-table">
                <thead>
                    <tr>
                        <th>Name</th>
                        <th>Items</th>
                    </tr>
                </thead>
                <tbody>
                    ${Object.entries(collections).map(([name, items]) => `
                        <tr>
                            <td><span class="badge badge-success">${name}</span></td>
                            <td>${Object.keys(items).length}</td>
                        </tr>
                    `).join('')}
                </tbody>
            </table>
        `;
    }
};