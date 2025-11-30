# SAS Long Poll Implementation TODO

This document tracks the implementation status of all SAS long poll commands found in the reference implementation.

## Legend
- ✅ **DONE** - Fully implemented and tested
- 🚧 **IN PROGRESS** - Currently being implemented
- ⏳ **TODO** - Not yet started
- ⚠️ **COMPLEX** - Requires significant implementation effort
- 🔄 **PARTIAL** - Partially implemented, needs completion

---

## Implementation Status by Category

### Already Implemented
- ✅ 0x11 - Send Total Coin In (MeterCommands)
- ✅ 0x12 - Send Total Coin Out (MeterCommands)
- ✅ 0x13 - Send Total Drop (MeterCommands)
- ✅ 0x14 - Send Total Jackpot (MeterCommands)
- ✅ 0x15 - Send Games Played (MeterCommands)
- ✅ 0x16 - Send Games Won (MeterCommands)
- ✅ 0x17 - Send Games Lost (MeterCommands)
- ✅ 0x19 - Send Total Coin In and Associated Meters (MeterCommands)
- ✅ 0x1B - Send Date/Time (DateTimeCommands)
- ✅ 0x20 - Send Total Bills (MeterCommands)
- ✅ 0x54 - Send Machine ID and Serial Number (ConfigCommands)
- ✅ 0x01 - Enable Game (EnableCommands)
- ✅ 0x02 - Disable Game (EnableCommands)
- ✅ 0x03 - Enable Bill Acceptor (EnableCommands)
- ✅ 0x04 - Disable Bill Acceptor (EnableCommands)

---

## Commands to Implement (Priority Order)

### High Priority - Basic Meters (Required for most SAS hosts)

#### ⏳ 0x10 - Send Cancelled Credits Meter
- **File**: `src/polls/LongPoll10Message.cpp`
- **Response**: Address, 0x10, Cancelled Credits (4 BCD), CRC (8 bytes total)
- **Meter**: gCC (meterdef: 58) - Cancel Credit
- **Implementation**: Simple 4-byte BCD meter response
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x1A - Send Current Credits
- **File**: `src/polls/LongPoll1AMessage.cpp`
- **Response**: Address, 0x1A, Current Credits (4 BCD), CRC (8 bytes total)
- **Meter**: mCrd (metersdef: 25) - Credit Meter
- **Implementation**: Simple 4-byte BCD meter response
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x1C - Send Gaming Machine Meters 1-8
- **File**: `src/polls/LongPoll1CMessage.cpp`
- **Response**: Address, 0x1C, Coin In (4), Coin Out (4), Total Drop (4), Jackpot (4), Games Played (4), Games Won (4), Slot Door (4), Power Reset (4), CRC (36 bytes total)
- **Meters**: gCI(53), gCO(54), mTotalDrop(87), gJP(60), gGS(55), gGW(56), mActualSlotDr(67)
- **Implementation**: Multi-meter response with 8 meters
- **Handler File**: MeterCommands.cpp
- **Notes**: Power reset meter may need special handling

#### ⏳ 0x2A - Send True Coin In
- **File**: `src/polls/LongPoll2AMessage.cpp`
- **Response**: Address, 0x2A, True Coin In (4 BCD), CRC (8 bytes total)
- **Meter**: mTCi (metersdef: 26)
- **Implementation**: Simple 4-byte BCD meter response
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x2B - Send True Coin Out
- **File**: `src/polls/LongPoll2BMessage.cpp`
- **Response**: Address, 0x2B, True Coin Out (4 BCD), CRC (8 bytes total)
- **Meter**: mTCo (metersdef: 27)
- **Implementation**: Simple 4-byte BCD meter response
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x46 - Send Bills Accepted Credits
- **File**: `src/polls/LongPoll46Message.cpp`
- **Response**: Address, 0x46, Bills Accepted (4 BCD), CRC (8 bytes total)
- **Meter**: mBD (metersdef: 28) - Bill Drop Credits
- **Implementation**: Simple 4-byte BCD meter response
- **Handler File**: MeterCommands.cpp

---

### High Priority - Bill Meters

#### ⏳ 0x1E - Send Bill Meters (All Denominations)
- **File**: `src/polls/LongPoll1EMessage.cpp`
- **Response**: Address, 0x1E, $1(4), $5(4), $10(4), $20(4), $50(4), $100(4), CRC (28 bytes total)
- **Meters**: mD1(12), mD5(14), mD10(15), mD20(16), mD50(17), mD100(18)
- **Implementation**: Multi-meter response with 6 bill meters
- **Handler File**: MeterCommands.cpp (new function)

#### ⏳ 0x31 - Send $1 Bills Meter
- **File**: `src/polls/LongPoll31Message.cpp`
- **Response**: Address, 0x31, $1 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD1 (metersdef: 12)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x32 - Send $2 Bills Meter
- **File**: `src/polls/LongPoll32Message.cpp`
- **Response**: Address, 0x32, $2 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD2 (metersdef: 13)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x33 - Send $5 Bills Meter
- **File**: `src/polls/LongPoll33Message.cpp`
- **Response**: Address, 0x33, $5 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD5 (metersdef: 14)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x34 - Send $10 Bills Meter
- **File**: `src/polls/LongPoll34Message.cpp`
- **Response**: Address, 0x34, $10 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD10 (metersdef: 15)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x35 - Send $20 Bills Meter
- **File**: `src/polls/LongPoll35Message.cpp`
- **Response**: Address, 0x35, $20 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD20 (metersdef: 16)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x36 - Send $50 Bills Meter
- **File**: `src/polls/LongPoll36Message.cpp`
- **Response**: Address, 0x36, $50 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD50 (metersdef: 17)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x37 - Send $100 Bills Meter
- **File**: `src/polls/LongPoll37Message.cpp`
- **Response**: Address, 0x37, $100 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD100 (metersdef: 18)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x38 - Send $500 Bills Meter
- **File**: `src/polls/LongPoll38Message.cpp`
- **Response**: Address, 0x38, $500 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD500 (metersdef: 69)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x39 - Send $1000 Bills Meter
- **File**: `src/polls/LongPoll39Message.cpp`
- **Response**: Address, 0x39, $1000 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD1000 (metersdef: 70)
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x3A - Send $200 Bills Meter
- **File**: `src/polls/LongPoll3AMessage.cpp`
- **Response**: Address, 0x3A, $200 Bills (4 BCD), CRC (8 bytes total)
- **Meter**: mD200 (metersdef: 68)
- **Handler File**: MeterCommands.cpp

---

### High Priority - Game Configuration

#### ⏳ 0x1F - Send Game Configuration
- **File**: `src/polls/LongPoll1FMessage.cpp`
- **Response**: Address, 0x1F, Game ID (2 ASCII), Additional ID (3 ASCII), Denomination, Max Bet, Progressive Group, Game Options (2), Pay Table (6 ASCII), Base Percent (4 ASCII), CRC
- **Implementation**: Configuration data response
- **Handler File**: ConfigCommands.cpp (new function)
- **Notes**: Need to read game configuration from Machine

#### ⏳ 0x51 - Send Number of Games Implemented
- **File**: `src/polls/LongPoll51Message.cpp`
- **Response**: Address, 0x51, Number of Games (2 BCD), CRC (6 bytes total)
- **Implementation**: Return count of enabled games
- **Handler File**: ConfigCommands.cpp

#### ⏳ 0x55 - Send Selected Game Number
- **File**: `src/polls/LongPoll55Message.cpp`
- **Response**: Address, 0x55, Selected Game Number (2 BCD), CRC (6 bytes total)
- **Implementation**: Return currently selected game
- **Handler File**: ConfigCommands.cpp

---

### High Priority - AFT Meters

#### ⏳ 0x1D - Send AFT Registration Meters
- **File**: `src/polls/LongPoll1DMessage.cpp`
- **Response**: Address, 0x1D, Promo Credit In (4), Non-Cash Credit In (4), Transferred Credits (4), Cashable Credits (4), CRC (20 bytes total)
- **Meters**: mPbAR2G(72), mPbAN2G(73), mPbAC2H(74), mPbAC2G(71)
- **Implementation**: Multi-meter AFT response
- **Handler File**: AFTCommands.cpp (new function)

#### ⏳ 0x27 - Send Non-Cashable Electronic Promotion Credits
- **File**: `src/polls/LongPoll27Message.cpp`
- **Response**: Address, 0x27, Promo Credits (4 BCD), CRC (8 bytes total)
- **Meter**: mNcepCredits (metersdef: 90)
- **Implementation**: Simple 4-byte BCD meter response
- **Handler File**: AFTCommands.cpp

---

### Medium Priority - Multi-Game Support

#### ⏳ 0x52 - Send Selected Game Meters
- **File**: `src/polls/LongPoll52Message.cpp`
- **Send**: Address, 0x52, Game Number (2 BCD), CRC (6 bytes)
- **Response**: Address, 0x52, Game Number (2), Coin In (4), Coin Out (4), Jackpot (4), Games Played (4), CRC (20 bytes total)
- **Meters**: For game 0: gCI(53), gCO(54), gJP(60), gGS(55). For sub-games: use subgameMeters enum
- **Implementation**: Game-specific meter response
- **Handler File**: MeterCommands.cpp (new function)

#### ⏳ 0x53 - Send Game N Configuration
- **File**: `src/polls/LongPoll53Message.cpp`
- **Send**: Address, 0x53, Game Number (2 BCD), CRC (6 bytes)
- **Response**: Address, 0x53, Game Number (2), Game ID (2), Additional ID (3), Denomination, Max Bet, Progressive Group, Game Options (2), Pay Table (6), Base Percent (4), CRC
- **Implementation**: Game-specific configuration
- **Handler File**: ConfigCommands.cpp

#### ⏳ 0x56 - Send Enabled Game Numbers
- **File**: `src/polls/LongPoll56Message.cpp`
- **Response**: Address, 0x56, Length, Number of Enabled Games, Game Numbers (2 BCD each), CRC
- **Implementation**: Variable response with list of enabled games
- **Handler File**: ConfigCommands.cpp
- **Notes**: Variable length response

#### ⏳ 0xB1 - Send Current Denomination
- **File**: `src/polls/LongPollB1Message.cpp`
- **Response**: Address, 0xB1, Current Denomination, CRC (5 bytes total)
- **Implementation**: Return active denomination code
- **Handler File**: ConfigCommands.cpp

#### ⏳ 0xB2 - Send Enabled Denominations
- **File**: `src/polls/LongPollB2Message.cpp`
- **Response**: Address, 0xB2, Length, Number of Denoms, Denom Codes (1 byte each), CRC
- **Implementation**: Variable response with list of denominations
- **Handler File**: ConfigCommands.cpp
- **Notes**: Variable length response

#### ⏳ 0xB5 - Send Extended Game Configuration
- **File**: `src/polls/LongPollB5Message.cpp`
- **Send**: Address, 0xB5, Game Number (2 BCD), CRC (6 bytes)
- **Response**: Address, 0xB5, Length, Game Number (2), Max Bet (var BCD), Progressive Group, Progressive Level IDs (4), Game Name Length, Game Name (var), Paytable Name Length, Paytable Name (var), CRC
- **Implementation**: Extended game configuration with names
- **Handler File**: ConfigCommands.cpp
- **Notes**: Variable length response

---

### Medium Priority - TITO (Ticket In/Ticket Out)

#### ⚠️ 0x70 - Send Ticket Information
- **File**: `src/polls/LongPoll70Message.cpp`
- **Response**: Address, 0x70, Length, Ticket Status, Amount (5 BCD), Parsing Code, Validation Number (9), CRC
- **Implementation**: Variable response for pending ticket
- **Handler File**: TITOCommands.cpp (new function)
- **Notes**: Variable length, requires ticket buffer management

#### ⚠️ 0x71 - Send Ticket Validation Data / Status Query
- **File**: `src/polls/LongPoll71Message.cpp`
- **Send Query**: Address, 0x71, 0x01, 0xFF, CRC
- **Send Validation**: Address, 0x71, Length, Transfer Code, Amount (5), Parsing Code, Validation Number (9), Expiration (4), Pool ID (2), CRC
- **Response**: Address, 0x71, Length, Ticket Status, Amount (5), Parsing Code, Validation Number (9), CRC
- **Implementation**: Complex ticket validation handler
- **Handler File**: TITOCommands.cpp (update existing)
- **Notes**: Variable length, requires ticket state machine

#### ⏳ 0x7B - Gaming Machine ID and Information
- **File**: `src/polls/LongPoll7BMessage.cpp`
- **Send**: Address, 0x7B, Length (0x0A), Control Mask (2), Status (2), Cashable Expiration (2), Restricted Expiration (2), CRC
- **Response**: Address, 0x7B, Length, Asset Number (4), Status (2), Cashable Expiration (2), Restricted Expiration (2), CRC
- **Implementation**: Machine configuration handler
- **Handler File**: ConfigCommands.cpp
- **Notes**: Control/status flags for ticket printer capabilities

#### ⏳ 0x7D - Extended Ticket Transfer
- **File**: `src/polls/LongPoll7DMessage.cpp`
- **Send**: Address, 0x7D, Length, Host ID (2), Expiration, Location Length, Location (var), Address1 Length, Address1 (var), Address2 Length, Address2 (var), CRC
- **Response**: ACK (5 bytes)
- **Implementation**: Extended ticket info handler
- **Handler File**: TITOCommands.cpp
- **Notes**: Variable length send with string data

---

### Medium Priority - AFT (Account Funds Transfer)

#### ⚠️ 0x72 - AFT Transfer Funds / Status Query
- **File**: `src/polls/LongPoll72Message.cpp`
- **Send Query**: Address, 0x72, 0x02, 0xFF, 0x00, CRC
- **Send Transfer**: Address, 0x72, Length, Transfer Code, Transaction Index, Transfer Type, Cashable Amount (5), Restricted Amount (5), Non-restricted Amount (5), Transfer Flags, Asset Number (4), Registration Key (20), Transaction ID Length, Transaction ID (var), Expiration (4), Pool ID (2), Receipt Length, Receipt Type, Receipt Data Length, Receipt Data (var), CRC
- **Response**: Address, 0x72, Length, Transaction Buffer Position, Transfer Status, Receipt Status, Transfer Type, Cashable (5), Restricted (5), Non-restricted (5), Transfer Flags, Asset Number (4), Transaction ID Length, Transaction ID, Date (4), Time (3), Expiration (4), Pool ID (2), [Cumulative meters], CRC
- **Implementation**: Complex AFT protocol handler
- **Handler File**: AFTCommands.cpp (update existing)
- **Notes**: VERY complex, variable length, state machine required

#### ⚠️ 0x74 - AFT Request Lock / Game Lock and Status Request
- **File**: `src/polls/LongPoll74Message.cpp`
- **Send**: Address, 0x74, Lock Code, Transfer Condition, Lock Timeout (2 BCD), CRC (8 bytes)
- **Response**: Address, 0x74, Length, Asset Number (4), Game Lock Status, Available Transfers, Host Cashout Status, AFT Status, Max Buffer Index, Current Cashable (5), Current Restricted (5), Current Non-restricted (5), EGM Transfer Limit (5), Restricted Expiration (4), Pool ID (2), CRC
- **Implementation**: AFT lock management
- **Handler File**: AFTCommands.cpp (update existing)
- **Notes**: Variable length, game lock state management

#### ⏳ 0x6A - Send EFT Capabilities
- **File**: `src/polls/LongPoll6AMessage.cpp`
- **Response**: Address, 0x6A, [EFT capability data], Flag Byte, CRC (8 bytes total)
- **Implementation**: Return EFT capability flags
- **Handler File**: AFTCommands.cpp
- **Notes**: Flag byte bit 0 = EFT Download, bit 1 = EFT Upload

---

### Medium Priority - Progressive

#### ⏳ 0x80 - Send Progressive Amount Broadcast
- **File**: `src/polls/LongPoll80Message.cpp`
- **Send**: Address, 0x80, Group, Level, Amount (5 BCD), CRC
- **Response**: ACK (if not broadcast address 0x00)
- **Implementation**: Receive progressive broadcast
- **Handler File**: ProgressiveCommands.cpp (update existing)
- **Notes**: Can be broadcast (address 0x00)

#### ⏳ 0x84 - Send Current Progressive Win Amount
- **File**: `src/polls/LongPoll84Message.cpp`
- **Response**: Address, 0x84, Progressive Group, Progressive Level, Amount (5 BCD), CRC (11 bytes total)
- **Implementation**: Return pending progressive win
- **Handler File**: ProgressiveCommands.cpp (update existing)

#### ⏳ 0x85 - Send SAP Win Amount
- **File**: `src/polls/LongPoll85Message.cpp`
- **Response**: Address, 0x85, Progressive Group, Progressive Level, Amount (5 BCD), CRC (11 bytes total)
- **Implementation**: Return SAP win information
- **Handler File**: ProgressiveCommands.cpp (update existing)

#### ⏳ 0x86 - Send Multiple Progressive Amount Broadcast
- **File**: `src/polls/LongPoll86Message.cpp`
- **Send**: Address, 0x86, Group, [Level, Amount (5)] repeated, CRC
- **Response**: ACK (if not broadcast)
- **Implementation**: Receive multiple progressive amounts
- **Handler File**: ProgressiveCommands.cpp (update existing)
- **Notes**: Variable length, broadcast capable

#### ⏳ 0x8A - Send Legacy Bonus Award
- **File**: `src/polls/LongPoll8AMessage.cpp`
- **Send**: Address, 0x8A, Amount (4 BCD), Tax Status, CRC
- **Response**: ACK
- **Implementation**: Award bonus to credit meter
- **Handler File**: ProgressiveCommands.cpp (update existing)
- **Notes**: Tax Status: 0=Deductible, 1=Non-deductible, 2=Wager match

---

### Medium Priority - Advanced Meters

#### ⏳ 0x2D - Send Selected Game Number and Handpay Cancelled Credits
- **File**: `src/polls/LongPoll2DMessage.cpp`
- **Send**: Address, 0x2D, Game Number (2), CRC (6 bytes)
- **Response**: Address, 0x2D, Game Number (2), Handpay Cancelled Credits (4 BCD), CRC (10 bytes total)
- **Meter**: mHCC (metersdef: 66)
- **Implementation**: Game-specific handpay cancelled meter
- **Handler File**: MeterCommands.cpp

#### ⚠️ 0x2F - Send Selected Meters for Game N
- **File**: `src/polls/LongPoll2FMessage.cpp`
- **Send**: Address, 0x2F, Length, Game Number (2 BCD), Meter Codes (var), CRC
- **Response**: Address, 0x2F, Length, Game Number (2 BCD), Meter Data (var), CRC
- **Implementation**: Variable meter selection handler
- **Handler File**: MeterCommands.cpp (new function)
- **Notes**: Variable length, max 10 meters, IGT S2000 special handling

#### ⚠️ 0x6F/0xAF - Send Multiple Selected Meters for Game N
- **File**: `src/polls/LongPoll6FMessage.cpp`
- **Send**: Address, 0x6F (or 0xAF), Length, Game Number (2 BCD), Meter Codes (2 bytes each), CRC
- **Response**: Address, 0x6F (or 0xAF), Length, Game Number (2), [Meter Code (2), Meter Size (1), Meter Value (var)] repeated, CRC
- **Implementation**: Enhanced variable meter selection
- **Handler File**: MeterCommands.cpp (new function)
- **Notes**: Variable length, max 12 meters, swaps between 0x6F and 0xAF

#### ⏳ 0x28 - Send Last Five EFT Transaction Statuses
- **File**: `src/polls/LongPoll28Message.cpp`
- **Response**: Address, 0x28, [Transaction data for last 5], CRC (49 bytes total)
- **Implementation**: Return last 5 EFT transaction statuses
- **Handler File**: AFTCommands.cpp
- **Notes**: Fixed 49-byte response

#### ⏳ 0x50 - Send Total Validations/Value
- **File**: `src/polls/LongPoll50Message.cpp`
- **Send**: Address, 0x50, Validation Type, CRC (5 bytes)
- **Response**: Address, 0x50, Validation Type, Total Validations (4 BCD), Cumulative Amount (5 BCD), CRC
- **Implementation**: Validation statistics by type
- **Handler File**: TITOCommands.cpp
- **Notes**: Validation types: 0x00=Cashable, 0x80=Cashable Redeemed, etc.

---

### Low Priority - Special Features

#### ⏳ 0x0E - Enable/Disable Real Time Event Reporting
- **File**: `src/polls/LongPoll0EMessage.cpp`
- **Send**: Address, 0x0E, Enable Flag, CRC
- **Response**: ACK
- **Implementation**: Enable/disable real-time events
- **Handler File**: ExceptionCommands.cpp (update existing)

#### ⏳ 0x1E - Send Handpay Information (DUPLICATE - check if same as 0x1B)
- **File**: `src/polls/LongPoll1EMessage.cpp`
- **Response**: Address, 0x1E, Progressive Group, Progressive Level, Amount (5 BCD), Partial Amount Paid (2 BCD), Reset ID, CRC (24 bytes)
- **Implementation**: Handpay information
- **Handler File**: TITOCommands.cpp
- **Notes**: Check if duplicate of 0x1B

#### ⏳ 0x2E - Send Game Delay
- **File**: `src/polls/LongPoll2EMessage.cpp`
- **Send**: Address, 0x2E, Game Delay (2), CRC
- **Response**: ACK
- **Implementation**: Set game delay parameter
- **Handler File**: ConfigCommands.cpp

#### ⏳ 0x3D - Send Current Hopper Level
- **File**: `src/polls/LongPoll3DMessage.cpp`
- **Response**: Address, 0x3D, Validation Number (4), Amount (5 BCD), CRC (13 bytes total)
- **Implementation**: Hopper level with validation
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x4C - Request System Validation
- **File**: `src/polls/LongPoll4CMessage.cpp`
- **Send**: Address, 0x4C, Machine Validation ID (var), Validation Sequence Number (var), CRC
- **Response**: Address, 0x4C, Machine Validation ID, Validation Sequence Number, CRC
- **Implementation**: System validation handshake
- **Handler File**: TITOCommands.cpp
- **Notes**: Variable length

#### ⏳ 0x4D - Send Enhanced Validation Information
- **File**: `src/polls/LongPoll4DMessage.cpp`
- **Send**: Address, 0x4D, Peek Flag (0xFF=peek, 0x00=remove), CRC (5 bytes)
- **Response**: Address, 0x4D, Validation Type Code, Index, Date (4), Time (3), Validation Number (8), Amount (5), Ticket Number (2), Validation System ID, Expiration (4), Pool ID (2), CRC (35 bytes total)
- **Implementation**: Enhanced validation/ticket data
- **Handler File**: TITOCommands.cpp
- **Notes**: Peek flag controls buffer removal

#### ⏳ 0x7F - Send Gaming Machine Date and Time
- **File**: `src/polls/LongPoll7FMessage.cpp`
- **Send**: Address, 0x7F, Date (4 BCD MMDDYYYY), Time (3 BCD HHMMSS), CRC
- **Response**: ACK
- **Implementation**: Set EGM date/time
- **Handler File**: DateTimeCommands.cpp (update existing)

#### ⏳ 0x83 - Send Last Accepted Bill Information
- **File**: `src/polls/LongPoll83Message.cpp`
- **Send**: Address, 0x83, Game Number (2), CRC (6 bytes)
- **Response**: Address, 0x83, Game Number (2), Amount (4 BCD), CRC (10 bytes total)
- **Implementation**: Last bill accepted for game N
- **Handler File**: MeterCommands.cpp

#### ⏳ 0x8E - Send Poker Hand Information
- **File**: `src/polls/LongPoll8EMessage.cpp`
- **Response**: Address, 0x8E, Hand Type, Card Data (5 cards), CRC (10 bytes total)
- **Implementation**: Poker hand information
- **Handler File**: New file - PokerCommands.cpp
- **Notes**: Game-specific

#### ⏳ 0x94 - Remote Handpay Reset
- **File**: `src/polls/LongPoll94Message.cpp`
- **Send**: Address, 0x94, CRC (4 bytes)
- **Response**: Address, 0x94, ACK Code, CRC (5 bytes total)
- **Implementation**: Remote handpay reset handler
- **Handler File**: TITOCommands.cpp

#### ⏳ 0x9A - Send Physical Reel Stop Information for Game N
- **File**: `src/polls/LongPoll9AMessage.cpp`
- **Send**: Address, 0x9A, Game Number (2), CRC (6 bytes)
- **Response**: Address, 0x9A, Game Number (2), Deductible Bonus (4), Non-Deductible Bonus (4), Wager Match Bonus (4), CRC (18 bytes total)
- **Implementation**: Bonus meter information
- **Handler File**: MeterCommands.cpp

#### ⏳ 0xA0 - Send Enabled Features for Game N
- **File**: `src/polls/LongPollA0Message.cpp`
- **Send**: Address, 0xA0, Game Number (2 BCD), CRC (6 bytes)
- **Response**: Address, 0xA0, Game Number (2 BCD), Feature Flags (3+ bytes), CRC (12 bytes total)
- **Implementation**: Return enabled SAS features
- **Handler File**: ConfigCommands.cpp
- **Notes**: Feature flags indicate AFT, tournaments, validation extensions, etc.

#### ⏳ 0xA8 - Enable/Disable Jackpot Handpay Reset Method
- **File**: `src/polls/LongPollA8Message.cpp`
- **Send**: Address, 0xA8, Reset Method, CRC (5 bytes)
- **Response**: Address, 0xA8, ACK Code, CRC (5 bytes total)
- **Implementation**: Set handpay reset method
- **Handler File**: ConfigCommands.cpp
- **Notes**: Reset Method: 0=Standard handpay, 1=Credit reset

#### ⏳ 0xB6 - Enable/Disable Game Auto Rebet
- **File**: `src/polls/LongPollB6Message.cpp`
- **Send**: Address, 0xB6, 0x01, Status, CRC
- **Response**: Address, 0xB6, Length (0x01), Status, CRC
- **Implementation**: Auto-rebet control
- **Handler File**: ConfigCommands.cpp

#### ⏳ 0xB7 - Send Gaming Machine Settings
- **File**: `src/polls/LongPollB7Message.cpp`
- **Send**: Address, 0xB7, Length, Asset Number (4), Floor Location Length, Floor Location (var), CRC
- **Response**: Address, 0xB7, Length, Control Flag, Asset Number (4), Floor Location Length, Floor Location (var), CRC
- **Implementation**: Machine settings (asset, location)
- **Handler File**: ConfigCommands.cpp
- **Notes**: Variable length

---

### Low Priority - Multi-level Commands

#### ⏳ 0x09 / 0xB0 0x09 - Enable/Disable Game (Multi-level)
- **File**: `src/polls/LongPoll09Message.cpp`
- **Send**: Address, 0xB0, 0x05, 0x04, 0x09, Game Number (2 BCD), Enable Flag, CRC
- **Response**: ACK (1 byte)
- **Implementation**: Multi-level game enable/disable
- **Handler File**: EnableCommands.cpp (update existing)

#### ⏳ 0xB0 0x56 - Send Enabled Game Numbers at Denomination
- **File**: `src/polls/LongPollB056Message.cpp`
- **Send**: Address, 0xB0, 0x02, Denomination, 0x56, CRC (7 bytes)
- **Response**: Address, 0xB0, 0x56, Length, Number of Enabled Games, Game Numbers (2 BCD each), CRC
- **Implementation**: Games at specific denomination
- **Handler File**: ConfigCommands.cpp
- **Notes**: Variable length

#### ⏳ 0xB0 0xB5 - Send Extended Game N Configuration with Denomination
- **File**: `src/polls/LongPollB0B5Message.cpp`
- **Send**: Address, 0xB0, 0x04, Denomination, 0xB5, Game Number (2 BCD), CRC (9 bytes)
- **Response**: Address, 0xB0, 0xB5, Length, Game Number (2), Max Bet (var), Progressive Group, Progressive Level IDs (4), Game Name Length, Game Name (var), Paytable Name Length, Paytable Name (var), CRC
- **Implementation**: Extended game config with denomination
- **Handler File**: ConfigCommands.cpp
- **Notes**: Variable length

---

### Special/Unclear Commands

#### ⏳ 0x01 - Send Command to Gaming Machine (Basic)
- **File**: `src/polls/LongPoll01Message.cpp`
- **Response**: ACK (implied)
- **Implementation**: Basic command (unused/placeholder)
- **Handler File**: May not need implementation

#### ⏳ 0x02 - Send Command to Gaming Machine (Basic)
- **File**: `src/polls/LongPoll02Message.cpp`
- **Response**: ACK (implied)
- **Implementation**: Basic command (unused/placeholder)
- **Handler File**: May not need implementation

#### ⏳ 0x0D - TITO Transaction Request
- **File**: `src/polls/LongPollDMessage.cpp`
- **Send**: Address, Poll Value, Transaction Number, ACK, Transfer Amount (4 BCD), CRC
- **Response**: Address, Poll Value, Transaction Number, ACK, Status, Transfer Amount (4 BCD), CRC
- **Implementation**: TITO transaction handler
- **Handler File**: TITOCommands.cpp
- **Notes**: Poll value is variable

#### ⏳ 0xU (Variable) - TITO Transaction Request
- **File**: `src/polls/LongPollUMessage.cpp`
- **Send**: Address, Poll Value, Transaction Number, ACK, CRC
- **Response**: Address, Poll Value, Transaction Number, ACK, Status, Transfer Amount (5 BCD), CRC
- **Implementation**: Generic TITO transaction
- **Handler File**: TITOCommands.cpp
- **Notes**: Variable poll value, no retry increment

---

## Implementation Plan

### Phase 1: Basic Meters (Week 1)
1. Implement all single 4-byte BCD meter responses (0x10, 0x1A, 0x2A, 0x2B, 0x46, 0x27)
2. Implement bill denomination meters (0x31-0x3A)
3. Implement multi-meter response 0x1C
4. Implement bill meters response 0x1E

### Phase 2: Game Configuration (Week 2)
1. Implement game configuration commands (0x1F, 0x51, 0x55, 0x56)
2. Implement denomination commands (0xB1, 0xB2)
3. Implement selected game meters (0x52, 0x53)

### Phase 3: AFT/TITO Basic (Week 3)
1. Implement AFT meters (0x1D)
2. Implement TITO basic commands (0x70, 0x7B, 0x7D)
3. Implement validation commands (0x4C, 0x50)

### Phase 4: Advanced Features (Week 4)
1. Implement progressive commands (0x80, 0x84, 0x85, 0x86, 0x8A)
2. Implement variable meter selection (0x2F, 0x6F)
3. Implement extended game configuration (0xB5, 0xB0 0xB5)

### Phase 5: Complex AFT/TITO (Week 5)
1. Implement AFT transfer (0x72)
2. Implement AFT lock management (0x74)
3. Implement ticket validation (0x71)
4. Implement TITO transactions (0x0D, 0xU)

### Phase 6: Special Features (Week 6)
1. Implement remaining special commands
2. Implement multi-level commands
3. Testing and validation

---

## Notes

- **Meters**: Reference `metersdef.h` for meter enum values
- **BCD Encoding**: Use `BCD::encode()` and `BCD::toBCD()` from `sas/BCD.h`
- **CRC**: Use `CRC16::calculate()` from `sas/CRC16.h`
- **Message Format**: Use `sas::Message` struct from `sas/SASCommands.h`
- **Routing**: Add cases to `SASCommPort::handleLongPoll()` in `src/sas/SASCommPort.cpp`
- **.bak Files**: Rename `.cpp` and `.h` files in `src/polls/` to `.bak` after implementing each command

---

## Progress Tracking

**Total Commands**: 67
**Implemented**: 15 (22%)
**Remaining**: 52 (78%)

Last Updated: 2025-01-28
