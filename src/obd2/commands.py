"""
OBD2 PID Commands and Definitions
Mode 01: Current vehicle data PIDs

Reference: SAE J1979 (OBD-II PIDs)
Vehicle: 2010 Opel Corsa D
"""


class OBD2Commands:
    """
    OBD2 PID definitions and command formatting

    Mode 01: Current Data (real-time)
    Mode 03: Stored DTCs
    Mode 04: Clear DTCs
    Mode 07: Pending DTCs
    """

    # Mode 01 PIDs (Current Data)
    PIDS = {
        # Engine Parameters
        'COOLANT_TEMP': '05',       # Engine coolant temperature
        'RPM': '0C',                 # Engine RPM
        'SPEED': '0D',               # Vehicle speed
        'INTAKE_TEMP': '0F',         # Intake air temperature
        'THROTTLE': '11',            # Throttle position

        # Fuel System
        'MAF': '10',                 # Mass air flow rate
        'FUEL_PRESSURE': '0A',       # Fuel pressure

        # Electrical
        'BATTERY_VOLTAGE': '42',     # Control module voltage

        # Status
        'ENGINE_LOAD': '04',         # Calculated engine load
        'RUNTIME': '1F',             # Engine run time
    }

    # PID Metadata (name, bytes, formula description)
    PID_INFO = {
        '05': ('Coolant Temp', 1, 'A - 40', '°C'),
        '0C': ('Engine RPM', 2, '(A*256 + B) / 4', 'RPM'),
        '0D': ('Vehicle Speed', 1, 'A', 'km/h'),
        '0F': ('Intake Temp', 1, 'A - 40', '°C'),
        '11': ('Throttle', 1, '(A * 100) / 255', '%'),
        '10': ('MAF', 2, '(A*256 + B) / 100', 'g/s'),
        '0A': ('Fuel Pressure', 1, 'A * 3', 'kPa'),
        '42': ('Battery Voltage', 2, '(A*256 + B) / 1000', 'V'),
        '04': ('Engine Load', 1, '(A * 100) / 255', '%'),
        '1F': ('Runtime', 2, 'A*256 + B', 's'),
    }

    @staticmethod
    def get_pid_command(pid_name):
        """
        Get OBD2 command for a PID name

        Args:
            pid_name: PID name from PIDS dict (e.g., 'RPM')

        Returns:
            str: Mode 01 command (e.g., '010C')
        """
        if pid_name in OBD2Commands.PIDS:
            pid = OBD2Commands.PIDS[pid_name]
            return f'01{pid}'
        return None


    @staticmethod
    def get_pid_info(pid_hex):
        """
        Get metadata for a PID

        Args:
            pid_hex: PID code in hex (e.g., '0C')

        Returns:
            tuple: (name, bytes, formula, unit) or None
        """
        return OBD2Commands.PID_INFO.get(pid_hex.upper())


    @staticmethod
    def get_supported_pids():
        """
        Get list of supported PID names for Opel Corsa D

        Returns:
            list: PID names that are known to work
        """
        return [
            'COOLANT_TEMP',
            'RPM',
            'SPEED',
            'INTAKE_TEMP',
            'THROTTLE',
            'BATTERY_VOLTAGE'  # May not be available on all ECUs
        ]


class DTCCommands:
    """
    Diagnostic Trouble Code (DTC) commands
    """

    # DTC Modes
    READ_STORED = '03'      # Read stored DTCs
    CLEAR_CODES = '04'      # Clear DTCs and MIL
    READ_PENDING = '07'     # Read pending DTCs
    READ_PERMANENT = '0A'   # Read permanent DTCs

    @staticmethod
    def decode_dtc(byte1, byte2):
        """
        Decode 2-byte DTC code to string format

        Args:
            byte1: First byte (hex)
            byte2: Second byte (hex)

        Returns:
            str: DTC code (e.g., 'P0133')
        """
        # First 2 bits determine prefix
        prefixes = {0: 'P', 1: 'C', 2: 'B', 3: 'U'}

        first_digit = (byte1 >> 6) & 0x03
        prefix = prefixes[first_digit]

        # Remaining digits
        second_digit = (byte1 >> 4) & 0x03
        third_digit = byte1 & 0x0F
        fourth_digit = (byte2 >> 4) & 0x0F
        fifth_digit = byte2 & 0x0F

        return f"{prefix}{second_digit}{third_digit:X}{fourth_digit:X}{fifth_digit:X}"
