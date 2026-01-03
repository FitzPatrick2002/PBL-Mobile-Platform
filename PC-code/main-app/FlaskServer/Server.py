import multiprocessing
import threading
from pathlib import Path
import requests

from flask import Flask, request
from utils.DataTypes import LidarScan, OdometryData


class FlaskServer:
    '''
        Class runs the server logic.
        :param HOST: Server IP address.
        :param PORT: Port on which server will be run
        :param open3d_queue: Mutliprocessing queue, used for communication with the
               open3d app, which is running in different process.
        :param qt_app_queue: Mutliprocessing queue, used for communication with the
        Qt app, which is running in separate process.
    '''

    # ------------------ Constructor ------------------ #

    def __init__(self, open3d_queue : multiprocessing.Queue,
                 o3d_to_f: multiprocessing.Queue,
                 from_qt_app_queue : multiprocessing.Queue,
                 to_qt_app_queue: multiprocessing.Queue,
                 host : str = "192.168.21.17", port : int = 9000):
        self.HOST = host
        self.PORT = port

        # Create the app object which manages the Flask server
        self.app = Flask(__name__)
        self.app.config["MAX_CONTENT_LENGTH"] = 16 * 1024 * 1024

        # Setup queues, for interprocess communication
        self.f_to_qt = to_qt_app_queue
        self.qt_to_f = from_qt_app_queue
        self.f_to_o3d = open3d_queue
        self.o3d_to_f = o3d_to_f

        # Setup routes for the flask server
        self.setup_routes()

    def setup_routes(self):
        '''
        Establishes routes, to which external devices can make http requests.
        Serviced requests are: POST. (and that's it)
        '''
        @self.app.route('/receive_post', methods=["POST"])
        def receive_data():
            # Check the request method
            print(f"Here: {request}")

            data = request.get_json(silent=True, force=True)
            print(f"Flask: Received raw data: {request.get_data()}")
            if data is None:
                print("Flask: receive_data(): Received json was missing or was empty")
                return "JSON missing or empty"

            print(request.get_json())
            print("YaY")
            if request.method == "POST":
                # Print the received data
                #data = request.get_json()
                print(f"Received data: {data}")

                # Service the message from the mobile platform
                self.handle_platform_messages(data)

                '''
                data_type = data["data-type"]
                # Check the data type
                if data_type == "lidar":
                    # Copy the data
                    bearing = float(data["bearing"])
                    position = data["position"]
                    payload = data["payload"]

                    # Print what has been received
                    print("Received data:")
                    print(f"Bearing: {bearing}")
                    print(f"Position: {position}")
                    print(f"Payload: {payload}")

                    # Create the new LidarScan object
                    lidar_scan = LidarScan(bearing, position, payload)

                    # Send the scan data into the o3d app
                    self.f_to_o3d.put(lidar_scan)
                '''

                return "POST accepted"

            print("Post is not really working")
            return "This wasn't POST"

    # ------------------ Initialization ------------------ #

    def start_server(self):
        '''
        Use this method to start the server.
        You can start it in a separate process if you wish.
        Loop which handles reading messages from qt app runs on a thread separate from flask.
        '''

        # Run the polling function in separate thread
        qt_poll_thread = threading.Thread(target=self.poll_qt_queue)
        qt_poll_thread.daemon = True

        o3d_poll_thread = threading.Thread(target=self.poll_o3d_queue)
        o3d_poll_thread.daemon = True

        o3d_poll_thread.start()
        qt_poll_thread.start()

        self.app.run(host=self.HOST, port=self.PORT, use_reloader=False)

        o3d_poll_thread.join()
        qt_poll_thread.join()

    # ------------------ Communication With o3d App ------------------ #

    def handle_platform_messages(self, data):
        '''
        Handles actions based on messages sent from the mobile platform.
        :param data: JSON sent by the platform and received via POST.
        '''
        message_type = data["data-type"]

        match message_type:
            # Propagate the lidar data to visualization app
            case "lidar":
                bearing = float(data["bearing"])
                position = data["position"]
                payload = data["payload"]

                # Print what has been received
                print("Received data:")
                print(f"Bearing: {bearing}")
                print(f"Position: {position}")
                print(f"Payload: {payload}")

                # Create the new LidarScan object
                lidar_scan = LidarScan(bearing, position, payload)

                # Send the scan data into the o3d app
                self.f_to_o3d.put({
                    "type" : "lidar",
                    "payload" : lidar_scan
                })

            # Propagate the odometry data further to the
            case "odometry":
                print(f"Flask handle_platform_messages(): Received odometry data from platform:")
                print(data)
                # Create the OdometryData object
                odometry_data = OdometryData(data["payload"])

                self.f_to_o3d.put({
                    "type": "odometry",
                    "payload": odometry_data
                })

            case _:
                print(f"Flask handle_platform_messages(): Unknown type of message received in flask server from mobile platform")

    # ------------------ Communication With Qt App ------------------ #

    def poll_qt_queue(self):
        '''
        Polls qt_to_f queue in search for messages from the qt app
        '''
        while True:
            try:
                # If queue is empty for longer than specified timeout, exception will be thrown
                new_qt_message = self.qt_to_f.get(timeout=0.1)

                # If new message has been received, handle it
                self.handle_qt_message(new_qt_message)

                print(f"Flask received message: {new_qt_message}")
            except Exception:
                pass

    def poll_o3d_queue(self):
        '''
        Polls qt_to_f queue in search for messages from the qt app
        '''
        while True:
            try:
                # If queue is empty for longer than specified timeout, exception will be thrown
                new_o3d_message = self.o3d_to_f.get(timeout=0.1)

                # If new message has been received, handle it
                self.handle_o3d_message(new_o3d_message)

                print(f"Flask received message: {new_o3d_message}")
            except Exception:
                pass

    def handle_o3d_message(self, message : dict):
        message_type = message["type"]

        match message_type:
            case "sessions-config":
                # Configuration info about the existing sessions
                # Send it to qt app so that it can configure its combobox for session selection
                print("Received in flask:")
                print(message["names"])
                self.f_to_qt.put({
                    "type": "sessions-config",
                    "names": message["names"]
                })

    def handle_qt_message(self, message : dict):
        '''
        Handles interpretation of messages from the qt app.
        :param message: The message.
        '''
        message_type = message["type"]

        match message_type:
            case "session-creation":
                # When a new session request is received, propagate it further to the o3d app

                # If the name has been specified, request creaton of a new session
                if message["name"]:
                    self.f_to_o3d.put({
                        "type" : "session",
                        "name" : message["name"]
                    })

                # For now immediately send session switch confirmation to the qt app
                self.f_to_qt.put({
                    "type": "session-creation",
                    "success": bool(message["name"])
                })

            case "session-switch":
                # When a session switch order is received, propagate it further to the o3d app

                # If name of the session has been specified, switch sessions
                if message["name"]:
                    self.f_to_o3d.put({
                        "type" : "session",
                        "name" : message["name"]
                    })

                # For now immediately confirm the session switch to qt app
                self.f_to_qt.put({
                    "type": "session-switch",
                    "success": bool(message["name"])
                })

            case "steering":
                # Make a POST request to the mobile platform for steering
                print(f"Flask: Sending a steering request")
                r = requests.post("http://192.168.21.30/command", data={
                    "type" : "steering",
                    "direction" : message["direction"]
                })
                print(f"Request: {r}")

            case "scan":
                # Send a scan request to the platform
                r = requests.post("http://192.168.21.30/lidar", data={
                    "type" : "lidar",
                    "rotations" : message["rotations"],
                    "every_nth" : message["every_nth"]
                })

            case _:
                print(f"Unknown type of message received in flask server from qt app: {message_type}")