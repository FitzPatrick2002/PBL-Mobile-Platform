"""
This module contains TheSetuper class responsible for:
Reading the config file and providing setup information for the wifi connection between PC app and the platform.
"""
import csv
import os

class TheSetuper:
    """Reads the following input data necessary to setup the connection between PC app and the platform:
        wifi-ssid
        wifi-password
        pc-server-name
        platform-server-name
        platform-server-comm-endp
        platform-server-scan-ednp
        pc_server_post_endp
    """
    def __init__(self, input_path: str):
        """Initiates the input path and immediately calls the method to read the data from it.

        Keyword arguments:
            input_path -- the input path to the file containing the internet configuration data.
        """
        base_dir = os.path.dirname(os.path.abspath(__file__))
        self._input_path = os.path.join(base_dir, input_path)
        self._read_setup()

    def _read_setup(self):
        """Reads and assigns the setup information."""
        with open(self._input_path, 'r') as file:
            csv_reader = csv.reader(file)

            read_values = []

            row: list[str]
            for row in csv_reader:
                read_values.append(row[1])

            self.__wifi_ssid = read_values[0]
            self.__wifi_password = read_values[1]
            self.__pc_server_name = read_values[2]
            self.__platform_server_name = read_values[3]
            self.__platform_server_comm_endp = read_values[4]
            self.__platform_server_scan_endp = read_values[5]
            self.__pc_server_post_endp = read_values[6]

            # print(self.__wifi_ssid)
            # print(self.__wifi_password)
            # print(self.__pc_server_name)
            # print(self.__platform_server_name)
            # print(self.__platform_server_comm_endp)
            # print(self.__platform_server_scan_endp)
            # print(self.__pc_server_post_endp)