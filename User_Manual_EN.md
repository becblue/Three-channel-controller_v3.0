# Three-Channel High-Voltage Switch Box Control System - User Manual

## System Overview

This system is a high-security level three-channel high-voltage switch box controller based on the STM32F103RCT6 microcontroller, featuring comprehensive safety monitoring, status feedback, temperature protection, and human-machine interface capabilities. The system adopts a dual-redundancy design to ensure reliable operation in high-voltage environments.

### Core Features
- **Three-Channel Interlocking Control**: Ensures only one channel operates at a time, preventing conflicts
- **Dual-Redundancy Protection**: Each channel equipped with dual relays and status feedback
- **15 Types of Exception Monitoring**: Real-time detection of various abnormal states with graded alarms
- **Intelligent Temperature Management**: Three-way NTC temperature monitoring with automatic fan control
- **Human-Machine Interface**: OLED display + key operation + serial port debugging
- **Data Recording Function**: Flash storage for system operation logs with historical query capability

---

## Hardware Interface Description

### Input Signals
| Signal Name | Pin | Function Description |
|-------------|-----|---------------------|
| K1_EN | PB9 | Channel 1 enable control (active low) |
| K2_EN | PB8 | Channel 2 enable control (active low) |
| K3_EN | PA15 | Channel 3 enable control (active low) |
| DC_CTRL | PB5 | External 24V power monitoring (high level abnormal) |

### Status Feedback
| Signal Name | Pin | Function Description |
|-------------|-----|---------------------|
| K1_1_STA | PC4 | Channel 1 first relay status |
| K1_2_STA | PB1 | Channel 1 second relay status |
| K2_1_STA | PC5 | Channel 2 first relay status |
| K2_2_STA | PB10 | Channel 2 second relay status |
| K3_1_STA | PB0 | Channel 3 first relay status |
| K3_2_STA | PB11 | Channel 3 second relay status |
| SW1_STA | PA8 | Channel 1 contactor status feedback |
| SW2_STA | PC9 | Channel 2 contactor status feedback |
| SW3_STA | PC8 | Channel 3 contactor status feedback |

### Output Control
| Signal Name | Pin | Function Description |
|-------------|-----|---------------------|
| ALARM | PB4 | Exception alarm output (active low) |
| BEEP | PB3 | Buzzer control (active low) |
| FAN_PWM | PA6 | Fan PWM control (20KHz, adjustable duty cycle) |

### User Interface
| Interface Name | Pin | Function Description |
|----------------|-----|---------------------|
| KEY1 | PC13 | Function key 1 (long press to view logs) |
| KEY2 | PC14 | Function key 2 (long press to clear data) |
| OLED Display | PB6/PB7 | I2C interface OLED display |
| Debug Serial | PC10/PC11 | USART3 debug output (115200 baud rate) |

### Temperature Monitoring
| Sensor | Pin | Function Description |
|--------|-----|---------------------|
| NTC_1 | PA0 | Channel 1 temperature monitoring (ADC sampling) |
| NTC_2 | PA1 | Channel 2 temperature monitoring (ADC sampling) |
| NTC_3 | PA2 | Channel 3 temperature monitoring (ADC sampling) |
| FAN_SEN | PC12 | Fan speed detection |

---

## System Startup Process

### 1. Power-On Startup
The system starts in the following sequence after power-on:

**Phase 1: LOGO Display (2 seconds)**
- OLED display shows company LOGO
- System completes basic hardware initialization
- Serial port outputs startup information

**Phase 2: System Self-Test (3 seconds)**
- OLED displays self-test progress bar
- Executes four-step self-test process:
  1. Expected state identification
  2. Relay status check and correction
  3. Contactor status check
  4. Temperature safety detection

**Phase 3: Normal Operation**
- After self-test passes, enters normal working mode
- Starts responding to external control signals
- Activates various monitoring functions

### 2. Self-Test Items Detailed

**Step 1: Expected State Identification**
- Determines current expected state based on K1_EN, K2_EN, K3_EN signal combinations
- Only allows single channel enable or all channels off

**Step 2: Relay Status Check and Correction**
- Checks if all relay states match expectations
- Automatically corrects relay states when errors are found
- Generates exception alarms if correction fails

**Step 3: Contactor Status Check**
- Checks if all contactor states match expectations
- Generates alarms when errors are found (cannot actively correct)

**Step 4: Temperature Safety Detection**
- Checks if three-way NTC temperatures are within safe range (<60°„C)
- Triggers protection mechanism when temperature is abnormal

---

## Channel Control Operation

### Channel Opening Operation

**Channel 1 Opening**
1. Ensure K2_EN, K3_EN are at high level (other channels closed)
2. Pull K1_EN signal low and maintain
3. System automatically detects after 50ms debounce and executes opening
4. Simultaneously drives K1_1 and K1_2 relays (500ms pulse)
5. Check status feedback after 500ms delay
6. OLED displays Channel 1 open status upon success

**Channel 2 Opening**
1. Ensure K1_EN, K3_EN are at high level
2. Pull K2_EN signal low and maintain
3. System executes same opening process
4. Display Channel 2 open status upon success

**Channel 3 Opening**
1. Ensure K1_EN, K2_EN are at high level
2. Pull K3_EN signal low and maintain
3. System executes same opening process
4. Display Channel 3 open status upon success

### Channel Closing Operation

**Channel Closing**
1. Pull corresponding channel EN signal from low to high level
2. System detects rising edge and executes closing
3. Drive corresponding OFF relay (500ms pulse)
4. Check status feedback after 500ms delay
5. OLED displays channel closed status upon success

### Safety Interlock Mechanism

**Interlock Protection**
- System strictly enforces three-channel interlocking, allowing only one channel to work at a time
- Immediately generates Type A exception when multiple channels are detected to be enabled simultaneously
- Must ensure other channels are completely closed before any channel switching

**Status Verification**
- Check relay and contactor status feedback after each operation
- Generate corresponding exception alarms when status doesn't match expectations
- Dual relays must work synchronously to ensure reliability

---

## Exception Monitoring and Alarms

### Exception Type Description

**Type A Exception: Enable Signal Conflict**
- Multiple EN signals detected simultaneously at low level
- Alarm method: 1-second interval pulse buzzer
- Clear condition: Ensure only one path or all at high level

**Type B~J Exceptions: Status Feedback Abnormal**
- B: K1_1 relay status abnormal
- C: K2_1 relay status abnormal
- D: K3_1 relay status abnormal
- E: K1_2 relay status abnormal
- F: K2_2 relay status abnormal
- G: K3_2 relay status abnormal
- H: SW1 contactor status abnormal
- I: SW2 contactor status abnormal
- J: SW3 contactor status abnormal
- Alarm method: 50ms interval pulse buzzer
- Clear condition: Status conforms to truth table requirements

**Type K~M Exceptions: Temperature Abnormal**
- K: NTC_1 temperature °›60°„C
- L: NTC_2 temperature °›60°„C
- M: NTC_3 temperature °›60°„C
- Alarm method: Continuous low level buzzer
- Clear condition: Temperature drops below 58°„C (2°„C hysteresis)

**Type N Exception: Self-Test Failure**
- Exceptions found during system self-test process
- Alarm method: 1-second interval pulse buzzer
- Clear condition: Restart system

**Type O Exception: External Power Abnormal**
- DC_CTRL detects 24V power interruption
- Alarm method: 1-second interval pulse buzzer
- Clear condition: Power returns to normal

### Alarm Output Description

**ALARM Pin Output**
- Immediately outputs low level when any exception occurs
- Returns to high level after all exceptions are cleared
- Can be used for external alarm indication

**Buzzer Alarm**
- Different exception types have different alarm rhythms
- Temperature exceptions: Continuous beep (highest priority)
- Status exceptions: Fast pulse (50ms interval)
- Conflict exceptions: Slow pulse (1-second interval)

**OLED Display Alarm**
- Top area scrolls to display all current exceptions
- Automatically scrolls when too many exceptions
- Real-time update of exception status

---

## Temperature Monitoring and Fan Control

### Temperature Monitoring Function

**Monitoring Range**
- Three-way NTC temperature sensors (10K resistance, B value 3435)
- Monitoring range: -40°„C~+125°„C
- Accuracy: °¿1°„C
- Sampling frequency: Updated every 100ms

**Temperature Status Classification**
- Normal temperature: <35°„C (green indication)
- High temperature state: 35°„C~59°„C (yellow indication)
- Abnormal overheating: °›60°„C (red indication, triggers alarm)

### Fan Control Function

**Automatic Adjustment**
- Temperature <35°„C: Fan runs at 50% PWM
- Temperature °›35°„C: Fan runs at 95% PWM
- Temperature °›60°„C: Fan 95% PWM + temperature exception alarm

**Speed Monitoring**
- Real-time detection of fan speed (RPM)
- Update speed display every second
- Prompt to check fan when speed is abnormal

**Temperature Hysteresis**
- Set 2°„C temperature hysteresis to avoid frequent switching
- Temperature must be 2°„C below threshold to recover when dropping

---

## OLED Display Interface

### Display Layout

**Three-Zone Display**
- Top area: Exception information display (6°¡8 font)
- Middle area: Three-channel status display (8°¡16 font)
- Bottom area: Temperature and fan information (6°¡8 font)

### Display Content

**Top Exception Zone**
- Display all currently active exceptions
- Scroll display when too many exceptions
- Format: "A Exception: Enable Conflict"

**Middle Status Zone**
- Channel1: OFF/ON
- Channel2: OFF/ON
- Channel3: OFF/ON
- Real-time display of each channel working status

**Bottom Information Zone**
- Temperature display: T1:25°„C T2:26°„C T3:24°„C
- Fan status: Fan:1200RPM 50%PWM
- Real-time update of temperature and fan data

### Display Modes

**Startup Mode**
- LOGO display (2 seconds)
- Self-test progress bar display (3 seconds)

**Normal Working Mode**
- Three-zone real-time status display
- Automatic refresh, avoid flickering

**Exception Alarm Mode**
- Highlight exception information display
- Status information continues normal display

---

## Key Operation Functions

### KEY1 Functions (PC13)

**Short Press (<3 seconds)**
- No function (avoid misoperation)

**Long Press 3-5 seconds**
- Output all system operation logs (detailed format)
- Serial port output, can be used for problem diagnosis

**Long Press 5-8 seconds**
- Batch output system logs (one every 10ms)
- Display progress percentage
- Suitable for viewing large amounts of logs

**Long Press °›8 seconds**
- View Flash storage status
- Display storage capacity, health, etc.

### KEY2 Functions (PC14)

**Short Press (<3 seconds)**
- No function (avoid misoperation)

**Long Press 3-8 seconds**
- Clear system operation logs
- Reset exception statistics
- Output "Cleared" confirmation message

**Long Press 8-10 seconds**
- Execute quick storage test
- Verify Flash read/write function

**Long Press 10-15 seconds**
- Execute complete storage test
- Include erase/write performance test

**Long Press °›15 seconds**
- Complete format storage (dangerous operation)
- Clear all historical data

### Combination Keys

**KEY1+KEY2 Long Press °›3 seconds**
- Execute Flash data backup and cleanup
- Retain important logs, clear redundant data

---

## Data Recording and Query

### Log Recording Function

**Recording Content**
- System startup/shutdown records
- Exception events and handling processes
- Channel switching operation records
- Temperature exceeded limit events
- System protection action records

**Storage Characteristics**
- Uses 128Mbit Flash storage (W25Q128)
- Supports approximately 240,000 log records
- Circular overwrite storage, retains latest data
- Data not lost during power outage

**Log Format**
- Timestamp + event category + detailed description
- Supports simple format, detailed format, CSV format output
- Convenient for data analysis and fault diagnosis

### Log Query Methods

**Serial Port Query**
- Connect debug serial port (115200 baud rate)
- Long press KEY1 to view all logs
- Supports real-time log output

**Category Query**
- Safety-critical logs: Exception events and system protection
- System status logs: Startup/shutdown and status changes
- Monitoring data logs: Temperature and performance data

---

## System Maintenance Functions

### Health Monitoring

**System Self-Test**
- Execute complete self-test at each startup
- Check all key functional modules
- Timely alarm when problems are found

**Storage Health**
- Monitor Flash erase/write cycles
- Calculate storage health percentage
- Early warning of storage aging

**Temperature Monitoring**
- Continuously monitor internal device temperature
- Automatically adjust cooling fan
- Over-temperature protection mechanism

### Fault Diagnosis

**Exception Codes**
- 15 exception types with clear coding
- Detailed exception description information
- Historical exception record query

**Status Indication**
- OLED real-time status display
- LED indicator status
- Serial port detailed debug information

**Remote Diagnosis**
- Serial port outputs detailed operation status
- Supports remote monitoring and diagnosis
- Convenient for technical support analysis

---

## Safety Precautions

### Operational Safety

**High Voltage Safety**
- This device is used in high-voltage environments, ensure power off before operation
- Strictly prohibit live operation of relay connections
- Regularly check contactor status

**System Protection**
- Do not force operation in abnormal states
- Pay attention to temperature monitoring, avoid overheating
- Regularly check fan operation status

**Data Safety**
- Backup log data before important operations
- Avoid frequent clearing of log records
- Regularly export important operation data

### Maintenance Recommendations

**Regular Inspection**
- Check system self-test results once a month
- Check if relays and contactors operate normally
- Clean internal dust, maintain good heat dissipation

**Preventive Maintenance**
- Regularly replace aging relays
- Check if connection lines are secure
- Update system software versions

**Emergency Handling**
- Prioritize personnel safety during exceptions
- Record exception phenomena and operation processes
- Contact technical support personnel promptly

---

## Technical Parameters

### Basic Parameters
- Main controller: STM32F103RCT6
- Operating voltage: DC 24V
- Power consumption: <10W
- Operating temperature: -10°„C~+60°„C
- Storage temperature: -40°„C~+85°„C
- Relative humidity: 5%~95% (non-condensing)

### Relay Parameters
- Contact capacity: AC 250V/10A
- Action time: <500ms
- Release time: <500ms
- Mechanical life: >10^7 times
- Electrical life: >10^5 times

### Temperature Monitoring Parameters
- Sensor type: NTC 10K B3435
- Temperature range: -40°„C~+125°„C
- Temperature accuracy: °¿1°„C
- Response time: <30s
- Over-temperature protection: 60°„C

### Display Parameters
- Display: 2.42-inch OLED
- Resolution: 128°¡64 pixels
- Control chip: SSD1309
- Interface type: I2C
- Display color: White

---

## Troubleshooting

### Common Faults

**System Cannot Start**
- Check if power connection is normal
- Check if main controller is damaged
- View serial port startup information

**Channel Cannot Switch**
- Check EN signal connection
- Confirm relay status feedback
- Check for exception alarms

**Temperature Monitoring Abnormal**
- Check NTC sensor connection
- Confirm ADC sampling is normal
- Calibrate temperature sensor

**OLED Display Abnormal**
- Check I2C connection lines
- Confirm display power supply is normal
- Reinitialize display module

### Error Codes

**System Error Codes**
- A001: Enable signal conflict
- B001~J001: Status feedback abnormal
- K001~M001: Temperature exceeded limit
- N001: Self-test failure
- O001: External power abnormal

**Solutions**
- Find specific cause according to error code
- Refer to exception monitoring chapter for handling
- Contact technical support if necessary

---

## Contact Support

For technical issues or technical support needs, please contact through the following methods:

**Technical Support**
- Serial debug information export
- System log data backup
- Detailed fault phenomenon description

**Maintenance Services**
- Regular inspection services
- Preventive maintenance
- System upgrade services

---

*This manual applies to the Three-Channel High-Voltage Switch Box Control System. Please refer to the latest version for updates.*