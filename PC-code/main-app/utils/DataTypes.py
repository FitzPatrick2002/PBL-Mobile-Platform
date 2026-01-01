from typing import List

class LidarScan:
    '''
    Stores data received from a single LiDAR scan.

    '''
    def __init__(self, bearing : float, position : List[float], payload : List[float]):
        self.bearing = bearing   # Bearing of the scan
        self.position = position # Position when the scan was being made
        self.payload = payload   # Main payload of the scan in a serialized form (r, phi, theta, ...). Flat array.

class OdometryData:
    '''
    Stores data received from odometry measurements.
    '''
    def __init__(self, payload : List[float]):
        self.payload = payload
