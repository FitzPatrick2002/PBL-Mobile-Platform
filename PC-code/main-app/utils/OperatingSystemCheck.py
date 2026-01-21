"""
This module contains three constant values regarding the user's operating system.
"""
import platform

"""Creates the constants for Operating System name and versions."""
OS_SYSTEM, OS_RELEASE, OS_VERSION = platform.system_alias(system=platform.system(), release=platform.release(), version=platform.version())