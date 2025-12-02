# This file is executed on every boot (including wake-boot from deepsleep)
#import esp
#esp.osdebug(None)
#import webrepl
#webrepl.start()

# Auto-start main application from src folder
import sys
sys.path.append('/src')

try:
    import main
except Exception as e:
    print("Failed to start main.py:", e)
    import sys
    sys.print_exception(e)
