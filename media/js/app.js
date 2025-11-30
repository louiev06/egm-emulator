// EGM Emulator GUI Application
// Communication with C++ backend via REST API

class EGMEmulator {
    constructor() {
        this.apiBase = 'http://localhost:8080/api';
        this.wsUrl = 'ws://localhost:8080/ws';
        this.ws = null;

        // State
        this.credits = 0;
        this.winAmount = 0;
        this.currentDenom = 0.01;
        this.gameName = 'Zeus Lightning';
        this.isPlaying = false;
        this.ipAddress = 'Loading...';
        this.lastDenomChange = 0;

        // Available denominations - will be loaded from backend
        this.denoms = [];
        this.denomIndex = 0;

        // SAS Exception codes - will be populated from API
        this.exceptions = [];

        // Initialize
        this.initElements();
        this.attachEventListeners();
        this.getIPAddress();
        this.loadDenominations();
        this.loadExceptions();
        // Note: WebSocket not implemented in C++ backend, using REST polling only
        // this.connectWebSocket();
        this.updateDisplay();

        // Poll status every 500ms
        setInterval(() => this.pollStatus(), 500);

        // Set initial status
        this.setStatus('Ready');
    }

    initElements() {
        this.elements = {
            credits: document.getElementById('credits'),
            winAmount: document.getElementById('winAmount'),
            denomValue: document.getElementById('denomValue'),
            gameName: document.getElementById('gameName'),
            ipAddress: document.getElementById('ipAddress'),
            statusBar: document.getElementById('statusBar'),
            btnPlay: document.getElementById('btnPlay'),
            btnCashout: document.getElementById('btnCashout'),
            btnDenomUp: document.getElementById('btnDenomUp'),
            btnDenomDown: document.getElementById('btnDenomDown'),
            btnException: document.getElementById('btnException'),
            btnBillInsert: document.getElementById('btnBillInsert'),
            btnReboot: document.getElementById('btnReboot'),
            exceptionModal: document.getElementById('exceptionModal'),
            billModal: document.getElementById('billModal'),
            rebootModal: document.getElementById('rebootModal'),
            exceptionGrid: document.getElementById('exceptionGrid'),
            closeExceptionModal: document.getElementById('closeExceptionModal'),
            closeBillModal: document.getElementById('closeBillModal'),
            closeRebootModal: document.getElementById('closeRebootModal'),
            customBillAmount: document.getElementById('customBillAmount'),
            btnCustomBill: document.getElementById('btnCustomBill'),
            btnCancelReboot: document.getElementById('btnCancelReboot'),
            btnConfirmReboot: document.getElementById('btnConfirmReboot')
        };
    }

    attachEventListeners() {
        this.elements.btnPlay.addEventListener('click', () => this.play());
        this.elements.btnCashout.addEventListener('click', () => this.cashout());
        this.elements.btnDenomUp.addEventListener('click', () => this.changeDenom(1));
        this.elements.btnDenomDown.addEventListener('click', () => this.changeDenom(-1));
        this.elements.btnException.addEventListener('click', () => this.showExceptionModal());
        this.elements.btnBillInsert.addEventListener('click', () => this.showBillModal());
        this.elements.btnReboot.addEventListener('click', () => this.showRebootModal());

        // Modal close buttons
        this.elements.closeExceptionModal.addEventListener('click', () => this.hideExceptionModal());
        this.elements.closeBillModal.addEventListener('click', () => this.hideBillModal());
        this.elements.closeRebootModal.addEventListener('click', () => this.hideRebootModal());
        this.elements.btnCancelReboot.addEventListener('click', () => this.hideRebootModal());
        this.elements.btnConfirmReboot.addEventListener('click', () => this.confirmReboot());

        // Custom bill insert
        this.elements.btnCustomBill.addEventListener('click', () => this.insertCustomBill());

        // Keyboard support for testing
        document.addEventListener('keydown', (e) => {
            switch(e.key) {
                case ' ': this.play(); break;
                case 'c': this.cashout(); break;
                case 'ArrowUp': this.changeDenom(1); break;
                case 'ArrowDown': this.changeDenom(-1); break;
                case 'e': this.showExceptionModal(); break;
                case 'b': this.showBillModal(); break;
                case 'Escape':
                    this.hideExceptionModal();
                    this.hideBillModal();
                    break;
            }
        });
    }

    async getIPAddress() {
        try {
            const response = await fetch(`${this.apiBase}/ip`);
            if (response.ok) {
                const data = await response.json();
                this.ipAddress = data.ip || 'Unknown';
            } else {
                // Fallback to showing hostname
                this.ipAddress = window.location.hostname || '127.0.0.1';
            }
        } catch (error) {
            console.error('Failed to get IP:', error);
            this.ipAddress = '127.0.0.1';
        }
        this.updateDisplay();
    }

    async loadDenominations() {
        try {
            const response = await fetch(`${this.apiBase}/denoms`);
            if (response.ok) {
                const data = await response.json();
                this.denoms = data.denoms || [0.01];
                this.denoms.sort((a, b) => a - b);  // Sort ascending
                // Set current denom index based on current denom
                this.denomIndex = this.denoms.findIndex(d => Math.abs(d - this.currentDenom) < 0.001) || 0;
            } else {
                // Fallback to common denominations
                this.denoms = [0.01, 0.05, 0.25, 1.00];
            }
        } catch (error) {
            console.error('Failed to load denominations:', error);
            this.denoms = [0.01, 0.05, 0.25, 1.00];
        }
        this.updateDisplay();
    }

    connectWebSocket() {
        try {
            this.ws = new WebSocket(this.wsUrl);

            this.ws.onopen = () => {
                console.log('WebSocket connected');
                this.setStatus('Connected');
            };

            this.ws.onmessage = (event) => {
                const data = JSON.parse(event.data);
                this.handleWebSocketMessage(data);
            };

            this.ws.onerror = (error) => {
                console.error('WebSocket error:', error);
                this.setStatus('Connection error');
            };

            this.ws.onclose = () => {
                console.log('WebSocket disconnected, reconnecting...');
                this.setStatus('Reconnecting...');
                setTimeout(() => this.connectWebSocket(), 2000);
            };
        } catch (error) {
            console.error('WebSocket connection failed:', error);
            this.setStatus('Offline - using polling');
        }
    }

    handleWebSocketMessage(data) {
        if (data.credits !== undefined) this.credits = data.credits;
        if (data.winAmount !== undefined) {
            const oldWin = this.winAmount;
            this.winAmount = data.winAmount;
            // Show win animation
            if (this.winAmount > oldWin && this.winAmount > 0) {
                this.showWinAnimation();
            }
        }
        if (data.denom !== undefined) this.currentDenom = data.denom;
        if (data.gameName !== undefined) this.gameName = data.gameName;
        if (data.status !== undefined) this.setStatus(data.status);
        if (data.isPlaying !== undefined) this.isPlaying = data.isPlaying;

        this.updateDisplay();
    }

    async pollStatus() {
        try {
            const response = await fetch(`${this.apiBase}/status`);
            if (response.ok) {
                const data = await response.json();
                // Update state from polling data (but don't update status to avoid flicker)
                if (data.credits !== undefined) this.credits = data.credits;
                // Don't overwrite denom if we just changed it (wait 1 second for backend to update)
                const timeSinceLastDenomChange = Date.now() - this.lastDenomChange;
                if (data.denom !== undefined && timeSinceLastDenomChange > 1000) {
                    this.currentDenom = data.denom;
                }
                if (data.gameName !== undefined) this.gameName = data.gameName;
                // Don't update status from polling - only from user actions
                this.updateDisplay();
            }
        } catch (error) {
            // Silently fail - we're polling frequently
            console.debug('Poll failed:', error);
        }
    }

    async play() {
        if (this.isPlaying) return;
        if (this.credits < this.currentDenom) {
            this.setStatus('Insufficient credits');
            return;
        }

        this.isPlaying = true;
        this.updateDisplay();
        this.setStatus('Spinning...');

        try {
            const response = await fetch(`${this.apiBase}/play`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ denom: this.currentDenom })
            });

            if (response.ok) {
                const data = await response.json();
                this.credits = data.credits;
                this.winAmount = data.winAmount;

                if (data.winAmount > 0) {
                    this.setStatus(`WIN: $${data.winAmount.toFixed(2)}!`);
                    this.showWinAnimation();
                } else {
                    this.setStatus('No win');
                }
            } else {
                this.setStatus('Play failed');
            }
        } catch (error) {
            console.error('Play error:', error);
            this.setStatus('Communication error');
        }

        this.isPlaying = false;
        this.updateDisplay();
    }

    async cashout() {
        if (this.credits <= 0) {
            this.setStatus('No credits to cash out');
            return;
        }

        const cashoutAmount = this.credits;
        this.setStatus(`Cashing out $${cashoutAmount.toFixed(2)}...`);

        try {
            const response = await fetch(`${this.apiBase}/cashout`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' }
            });

            if (response.ok) {
                const data = await response.json();
                this.credits = 0;
                this.winAmount = 0;
                this.setStatus(`Ticket printed: $${cashoutAmount.toFixed(2)}`);
            } else {
                this.setStatus('Cashout failed');
            }
        } catch (error) {
            console.error('Cashout error:', error);
            this.setStatus('Communication error');
        }

        this.updateDisplay();
    }

    async changeDenom(direction) {
        this.denomIndex += direction;
        if (this.denomIndex < 0) this.denomIndex = 0;
        if (this.denomIndex >= this.denoms.length) this.denomIndex = this.denoms.length - 1;

        this.currentDenom = this.denoms[this.denomIndex];
        this.lastDenomChange = Date.now();

        try {
            await fetch(`${this.apiBase}/denom`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ denom: this.currentDenom })
            });
        } catch (error) {
            console.error('Denom change error:', error);
        }

        this.setStatus(`Denomination: $${this.currentDenom.toFixed(2)}`);
        this.updateDisplay();
    }

    async loadExceptions() {
        try {
            const response = await fetch(`${this.apiBase}/exceptions`);
            if (response.ok) {
                const data = await response.json();
                this.exceptions = data.exceptions || [];
                this.populateExceptionGrid();
            } else {
                // Fallback to common SAS exceptions
                this.exceptions = this.getDefaultExceptions();
                this.populateExceptionGrid();
            }
        } catch (error) {
            console.error('Failed to load exceptions:', error);
            this.exceptions = this.getDefaultExceptions();
            this.populateExceptionGrid();
        }
    }

    getDefaultExceptions() {
        // Common SAS exception codes
        return [
            { code: 0x10, name: 'Slot Door Open' },
            { code: 0x11, name: 'Drop Door Open' },
            { code: 0x12, name: 'Card Cage Open' },
            { code: 0x13, name: 'Cashbox Door Open' },
            { code: 0x14, name: 'Cashbox Removed' },
            { code: 0x15, name: 'Belly Door Open' },
            { code: 0x16, name: 'Diverter Malfunction' },
            { code: 0x17, name: 'Bill Acceptor Failure' },
            { code: 0x18, name: 'Bill Acceptor Full' },
            { code: 0x19, name: 'Printer Failure' },
            { code: 0x1A, name: 'Printer Paper Out' },
            { code: 0x1B, name: 'Reel 1 Tilt' },
            { code: 0x1C, name: 'Reel 2 Tilt' },
            { code: 0x1D, name: 'Reel 3 Tilt' },
            { code: 0x1E, name: 'Reverse Coin-In' },
            { code: 0x1F, name: 'Coin-In Jam' },
            { code: 0x20, name: 'RAM Error' },
            { code: 0x21, name: 'Low Battery' },
            { code: 0x40, name: 'Handpay Pending' },
            { code: 0x51, name: 'Game Tilt' },
            { code: 0x52, name: 'Power Off/On' }
        ];
    }

    populateExceptionGrid() {
        this.elements.exceptionGrid.innerHTML = '';
        this.exceptions.forEach(exc => {
            const btn = document.createElement('button');
            btn.className = 'exception-btn';
            btn.innerHTML = `<span class="exception-code">0x${exc.code.toString(16).toUpperCase().padStart(2, '0')}</span><span class="exception-name">${exc.name}</span>`;
            btn.addEventListener('click', () => this.triggerException(exc.code));
            this.elements.exceptionGrid.appendChild(btn);
        });
    }

    showExceptionModal() {
        this.elements.exceptionModal.classList.add('active');
    }

    hideExceptionModal() {
        this.elements.exceptionModal.classList.remove('active');
    }

    showBillModal() {
        this.elements.billModal.classList.add('active');
        // Setup bill button listeners
        document.querySelectorAll('.bill-btn').forEach(btn => {
            btn.onclick = () => {
                const amount = parseFloat(btn.dataset.amount);
                this.insertBill(amount);
            };
        });
    }

    hideBillModal() {
        this.elements.billModal.classList.remove('active');
    }

    showRebootModal() {
        this.elements.rebootModal.classList.add('active');
    }

    hideRebootModal() {
        this.elements.rebootModal.classList.remove('active');
    }

    async confirmReboot() {
        this.hideRebootModal();
        this.setStatus('Saving meters and rebooting...');

        try {
            const response = await fetch(`${this.apiBase}/reboot`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });

            if (response.ok) {
                const data = await response.json();
                this.setStatus(data.message || 'Rebooting...');
                // Disable all buttons during reboot
                document.querySelectorAll('.btn').forEach(btn => btn.disabled = true);
            } else {
                this.setStatus('Reboot failed');
            }
        } catch (error) {
            console.error('Reboot error:', error);
            this.setStatus('Communication error');
        }
    }

    async triggerException(exceptionCode) {
        this.hideExceptionModal();
        const excName = this.exceptions.find(e => e.code === exceptionCode)?.name || `0x${exceptionCode.toString(16)}`;
        this.setStatus(`Exception: ${excName}`);

        try {
            await fetch(`${this.apiBase}/exception`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code: exceptionCode })
            });
        } catch (error) {
            console.error('Exception trigger error:', error);
            this.setStatus('Communication error');
        }
    }

    async insertBill(amount) {
        this.hideBillModal();
        this.setStatus(`Inserting $${amount.toFixed(2)}...`);

        try {
            const response = await fetch(`${this.apiBase}/billinsert`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ amount: amount })
            });

            if (response.ok) {
                const data = await response.json();
                this.credits = data.credits;
                this.setStatus(`Inserted $${amount.toFixed(2)}`);
            }
        } catch (error) {
            console.error('Bill insert error:', error);
            this.setStatus('Communication error');
        }

        this.updateDisplay();
    }

    async insertCustomBill() {
        const amount = parseFloat(this.elements.customBillAmount.value);
        if (isNaN(amount) || amount <= 0) {
            this.setStatus('Invalid amount');
            return;
        }

        this.elements.customBillAmount.value = '';
        await this.insertBill(amount);
    }

    updateDisplay() {
        this.elements.credits.textContent = this.credits.toFixed(2);
        this.elements.winAmount.textContent = this.winAmount.toFixed(2);
        this.elements.denomValue.textContent = `$${this.currentDenom.toFixed(2)}`;
        this.elements.gameName.textContent = this.gameName;
        this.elements.ipAddress.textContent = this.ipAddress;

        // Disable play button if no credits or already playing
        this.elements.btnPlay.disabled = this.isPlaying || this.credits < this.currentDenom;
    }

    setStatus(message) {
        this.elements.statusBar.textContent = message;
        console.log('Status:', message);
    }

    showWinAnimation() {
        this.elements.winAmount.classList.add('win-animation', 'flash');
        setTimeout(() => {
            this.elements.winAmount.classList.remove('win-animation', 'flash');
        }, 1500);
    }
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.egm = new EGMEmulator();
});
