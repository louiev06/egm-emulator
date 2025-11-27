#!/bin/bash
# Fix all port-> calls in Machine.cpp to use SASCommPort casting

FILE="src/simulator/Machine.cpp"

# Create backup
cp "$FILE" "${FILE}.backup"

# Fix port->stop()
sed -i 's/for (auto\& port : ports_) {$/&\n        auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);\n        if (sasPort) {/g' "$FILE"
sed -i '/for (auto\& port : ports_) {$/,/^    }$/ {
    s/        port->stop();/            sasPort->stop();\n        }/
    s/        port->doorOpen();/            sasPort->doorOpen();\n        }/
    s/        port->doorClose();/            sasPort->doorClose();\n        }/
    s/        port->lightOn();/            sasPort->lightOn();\n        }/
    s/        port->lightOff();/            sasPort->lightOff();\n        }/
    s/        port->hopperLow();/            sasPort->hopperLow();\n        }/
    s/        port->handpayPending/            sasPort->handpayPending/
    s/        port->cashoutButton();/            sasPort->cashoutButton();\n        }/
    s/        port->ramClear();/            sasPort->ramClear();\n        }/
    s/        port->optionsChanged();/            sasPort->optionsChanged();\n        }/
    s/        port->resetOldestHandpay();/            sasPort->resetOldestHandpay();\n        }/
}' "$FILE"

# Fix isConnected and setConnected which are different patterns
sed -i 's/        if (!port->isConnected()) {/        auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);\n        if (sasPort \&\& !sasPort->isConnected()) {/g' "$FILE"
sed -i 's/        port->setConnected(connected);/        auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);\n        if (sasPort) {\n            sasPort->setConnected(connected);\n        }/g' "$FILE"

echo "Fixed all port-> calls in Machine.cpp"
echo "Backup saved to ${FILE}.backup"
