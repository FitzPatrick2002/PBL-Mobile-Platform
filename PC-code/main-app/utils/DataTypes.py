"""
This module provides x
It contains two classes for data storage.
"""
from typing import List

class LidarScan:
    """Stores data received from a single LiDAR scan."""
    def __init__(self, bearing : float, position : List[float], payload : List[float]):
        """LidarScan initialisation.

        Keyword arguments:
            bearing -- Bearing of the scan.
            position -- Position when the scan was being made.
            payload -- Main payload of the scan in a serialized form (r, phi, theta, ...). Flat array.
        """
        self.bearing = bearing
        self.position = position
        self.payload = payload

class OdometryData:
    """Stores data received from odometry measurements."""
    def __init__(self, payload : List[float]):
        """OdometryData initialisation.

        Keyword arguments:
            payload --
        """
        self.payload = payload
