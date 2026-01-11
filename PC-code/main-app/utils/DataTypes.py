"""
This module provides classes which store data received from the mobile platform.
It contains two classes for data storage: LidarScan & OdometryData .
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
            payload -- Odometry readings in format [x0, y0, x1, y1, ...] where x0, y0  are oldest. Flat array.
        """
        self.payload = payload
