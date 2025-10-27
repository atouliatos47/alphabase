// charts.js - Data Visualization Module

// charts.js - Generic Data Visualization Module

const charts = {
    activityChart: null,

    // Initialize analytics view
    async loadAnalytics() {
        console.log('📊 Loading analytics...');

        try {
            // Fetch all collections
            const collections = await api.fetchAllCollections();

            if (Object.keys(collections).length === 0) {
                this.showEmptyAnalytics();
                return;
            }

            // Process data for charts
            const chartData = this.processCollectionsForChart(collections);
            const stats = this.calculateStats(collections);

            // Create chart
            this.createCollectionChart(chartData);

            // Update statistics
            this.updateStatistics(stats);

        } catch (error) {
            console.error('Analytics error:', error);
        }
    },

    // Process collections for chart visualization
    processCollectionsForChart(collections) {
        const labels = [];
        const data = [];
        const colors = [
            'rgba(25, 103, 210, 0.8)',   // Blue
            'rgba(52, 168, 83, 0.8)',    // Green
            'rgba(251, 188, 4, 0.8)',    // Yellow
            'rgba(234, 67, 53, 0.8)',    // Red
            'rgba(142, 68, 173, 0.8)',   // Purple
            'rgba(52, 152, 219, 0.8)'    // Light Blue
        ];

        Object.entries(collections).forEach(([name, items]) => {
            labels.push(name);
            data.push(Object.keys(items).length);
        });

        return { labels, data, colors };
    },

    // Calculate general statistics
    calculateStats(collections) {
        const stats = {
            totalCollections: Object.keys(collections).length,
            totalItems: 0,
            largestCollection: { name: '', count: 0 },
            newestCollection: ''
        };

        Object.entries(collections).forEach(([name, items]) => {
            const itemCount = Object.keys(items).length;
            stats.totalItems += itemCount;

            if (itemCount > stats.largestCollection.count) {
                stats.largestCollection = { name, count: itemCount };
            }
        });

        return stats;
    },

    // Create collection distribution chart
    createCollectionChart(chartData) {
        const ctx = document.getElementById('activityChart');

        // Destroy existing chart if it exists
        if (this.activityChart) {
            this.activityChart.destroy();
        }

        this.activityChart = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: chartData.labels,
                datasets: [{
                    label: 'Data Items',
                    data: chartData.data,
                    backgroundColor: chartData.colors,
                    borderColor: chartData.colors.map(color => color.replace('0.8', '1')),
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    title: {
                        display: true,
                        text: 'Data Distribution by Collection',
                        font: {
                            size: 16,
                            weight: 'bold'
                        }
                    },
                    legend: {
                        display: false
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        ticks: {
                            stepSize: 1
                        },
                        title: {
                            display: true,
                            text: 'Number of Items'
                        }
                    },
                    x: {
                        title: {
                            display: true,
                            text: 'Collections'
                        }
                    }
                }
            }
        });
    },

    // Update statistics cards
    updateStatistics(stats) {
        document.getElementById('press1Runtime').textContent = stats.totalCollections;
        document.getElementById('press2Runtime').textContent = stats.totalItems;
        document.getElementById('press3Runtime').textContent = stats.largestCollection.name || 'N/A';
        document.getElementById('totalCycles').textContent = stats.largestCollection.count;
    },

    // Show empty state
    showEmptyAnalytics() {
        const chartContainer = document.getElementById('activityChart').parentElement;
        chartContainer.innerHTML = `
            <div class="empty-state">
                <h3>No Data Yet</h3>
                <p>Add data to see analytics and visualizations</p>
            </div>
        `;

        // Reset statistics
        document.getElementById('press1Runtime').textContent = '0';
        document.getElementById('press2Runtime').textContent = '0';
        document.getElementById('press3Runtime').textContent = 'N/A';
        document.getElementById('totalCycles').textContent = '0';
    }
};