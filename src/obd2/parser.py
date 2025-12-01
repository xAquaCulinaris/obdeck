"""
OBD2 Response Parser
Parses ELM327 responses and converts to vehicle data

Features:
- PID response parsing
- Data conversion formulas
- Error handling
- DTC decoding
"""


class OBD2Parser:
    """
    Parser for OBD2 responses from ELM327

    Handles Mode 01 (current data) responses
    """

    @staticmethod
    def parse_response(response, pid):
        """
        Parse ELM327 response and convert to value

        Args:
            response: Raw response string from ELM327
            pid: PID code (e.g., '05' for coolant temp)

        Returns:
            Parsed value (float/int) or None on error
        """
        if not response or 'NO DATA' in response:
            return None

        # Remove spaces, carriage returns, '>' prompt
        data = response.replace(' ', '').replace('>', '').replace('\r', '').replace('\n', '')

        # Response format: 41[PID][DATA]
        # Mode 41 = response to Mode 01 query
        if not data.startswith('41'):
            return None

        try:
            # Extract data bytes after mode and PID (skip first 4 chars: "41XX")
            hex_data = data[4:]

            # Apply formula based on PID
            if pid == '05':  # Coolant temp: A - 40
                return OBD2Parser._parse_single_byte(hex_data, lambda a: a - 40)

            elif pid == '0C':  # RPM: (A*256 + B) / 4
                return OBD2Parser._parse_two_bytes(hex_data, lambda a, b: (a * 256 + b) / 4)

            elif pid == '0D':  # Speed: A
                return OBD2Parser._parse_single_byte(hex_data, lambda a: a)

            elif pid == '0F':  # Intake temp: A - 40
                return OBD2Parser._parse_single_byte(hex_data, lambda a: a - 40)

            elif pid == '11':  # Throttle: (A * 100) / 255
                return OBD2Parser._parse_single_byte(hex_data, lambda a: (a * 100) / 255)

            elif pid == '42':  # Battery voltage: (A*256 + B) / 1000
                return OBD2Parser._parse_two_bytes(hex_data, lambda a, b: (a * 256 + b) / 1000)

            elif pid == '10':  # MAF: (A*256 + B) / 100
                return OBD2Parser._parse_two_bytes(hex_data, lambda a, b: (a * 256 + b) / 100)

            elif pid == '0A':  # Fuel pressure: A * 3
                return OBD2Parser._parse_single_byte(hex_data, lambda a: a * 3)

            elif pid == '04':  # Engine load: (A * 100) / 255
                return OBD2Parser._parse_single_byte(hex_data, lambda a: (a * 100) / 255)

            elif pid == '1F':  # Runtime: A*256 + B
                return OBD2Parser._parse_two_bytes(hex_data, lambda a, b: a * 256 + b)

        except (ValueError, IndexError) as e:
            print(f"Parse error for PID {pid}: {e}")
            return None

        return None


    @staticmethod
    def _parse_single_byte(hex_data, formula):
        """
        Parse single-byte response

        Args:
            hex_data: Hex string (e.g., '5A')
            formula: Lambda function to apply (e.g., lambda a: a - 40)

        Returns:
            Calculated value
        """
        a = int(hex_data[0:2], 16)
        return formula(a)


    @staticmethod
    def _parse_two_bytes(hex_data, formula):
        """
        Parse two-byte response

        Args:
            hex_data: Hex string (e.g., '0FA0')
            formula: Lambda function to apply (e.g., lambda a, b: (a*256 + b)/4)

        Returns:
            Calculated value
        """
        a = int(hex_data[0:2], 16)
        b = int(hex_data[2:4], 16)
        return formula(a, b)


    @staticmethod
    def is_valid_response(response):
        """
        Check if response is valid OBD2 data

        Args:
            response: Response string from ELM327

        Returns:
            bool: True if valid response
        """
        if not response:
            return False

        # Remove whitespace
        data = response.strip().upper()

        # Check for error responses
        error_keywords = ['NO DATA', 'ERROR', 'UNABLE', 'BUS INIT', '?']
        for keyword in error_keywords:
            if keyword in data:
                return False

        # Check for valid Mode 01 response (starts with 41)
        clean_data = data.replace(' ', '')
        if clean_data.startswith('41'):
            return True

        return False


    @staticmethod
    def parse_dtc_response(response):
        """
        Parse DTC response from Mode 03/07/0A

        Args:
            response: Raw DTC response

        Returns:
            list: List of DTC codes (e.g., ['P0133', 'P0420'])
        """
        from obd2.commands import DTCCommands

        if not response or 'NO DATA' in response:
            return []

        # Remove spaces and format
        data = response.replace(' ', '').upper()

        dtcs = []

        # Response format: 43[count][DTC1][DTC2]... (for Mode 03)
        # Skip mode response byte and count
        i = 4  # Start after "43XX"

        while i + 4 <= len(data):
            byte1 = int(data[i:i+2], 16)
            byte2 = int(data[i+2:i+4], 16)

            # Skip if both bytes are 0x00
            if byte1 == 0 and byte2 == 0:
                break

            dtc = DTCCommands.decode_dtc(byte1, byte2)
            dtcs.append(dtc)
            i += 4

        return dtcs
