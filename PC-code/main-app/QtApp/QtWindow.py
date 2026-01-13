# This Python file uses the following encoding: utf-8

"""
This module contains the main menu window in QT, communication methods and platform steering.
"""
import math
import sys
import multiprocessing

from PySide6 import QtGui
from PySide6.QtGui import QColor
from PySide6.QtWidgets import QApplication, QMainWindow
from PySide6.QtCore import Slot, QTimer
from QtApp.UI.UiMainWindow import Ui_MainWindow


"""Communication protocol with flask server
    1. Packetes are in json format

    2. Packets received from the server:

    // Packet is sent periodically as an update of the telemetry
    {
        // Specifies the type of data sent as the payload
        type: 'telemetry',
        
        // Position data & heading
        x: float,    
        y: float,
        angle: float,
        
        // Distance sensors status
        collision: bool
    }
    
    // Packet sent on demand.
    // Server sends it to qt app, to inform about a successfully ended scan
    {
        type: 'scan-status',
        
        success: bool
    }
    
    // Packet sent by the server as a response to request to session creation
    {
        type: 'session-creation'
        
        success: bool
    }
    
    // Packet sent by the server as a response to request to session switch
    {
        type: 'session-switch'
        
        success: bool
    }
    
    // Packet sent by the server which tells qt app which sessions exist
    {
        type: "sessions-config",
        names: list[str] // Names of existing sessions
    }


    3. Packets sent to the server:


    // New session creation / start
    {
        type: 'new-session'
        
        // Name of the new session
        name: str
    }
    
    // Session switch / change
    {
        type: 'session-switch'
        
        // Name of the currently used session
        name: str
    }
    
    // Steering data
    {
        type: 'steering'
        
        // Specifies which way the platform should move
        // 'forward', 'backward', 'left', 'right'
        direction: str 
    }
    
    // Scan request
    {
        type: 'scan'
        
        // Details of the method of scanning (how many rotations & how many points to skip)
        rotations: int
        every_nth: int
    }
    
    // Calibration request
    {
        type: 'calibration'
    }

    // Standby request
    {
        type: 'standby'
    }
"""

class MainQtWindow(QMainWindow):
    """This class manages the communication between the flask server and qt app.

    Important parameter:
        server_queue: Multiprocessing queue, used to communicate with the flask server.
    """

    def __init__(self, from_server: multiprocessing.Queue,
                 to_server: multiprocessing.Queue, parent=None):
        """Initialisation of the window.

        Keyword arguments:
            from_server -- Queue used to store communication messages from the server.
            to_server -- Queue used to store communication messages to the server from the qt app.
        """
        super(MainQtWindow, self).__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)


        """Timer is used to periodically read the data sent from the server to qt app."""
        self.timer = QTimer()
        self.timer.setSingleShot(False)
        """Read the queue every 100 ms."""
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.read_flask_queue)
        self.timer.start()

        """Queues are used for communication with the server.
            We need bidirectional communication so there are 2 queues,
            one for sending to server and one for receiving data from server.
        """
        self.f_to_qt = from_server
        self.qt_to_f = to_server

        """------------ Movement Slots ------------"""

        """Slots connections for forward motion."""
        self.ui.forward_ctrl_btn.pressed.connect(self.onForwardClicked)
        self.ui.forward_ctrl_btn.released.connect(self.onForwardReleased)

        """Slots for backward motion."""
        self.ui.backward_ctrl_btn.pressed.connect(self.onBackwardClicked)
        self.ui.backward_ctrl_btn.released.connect(self.onBackwardReleased)

        """Slots for turning left."""
        self.ui.left_ctrl_button.pressed.connect(self.onLeftClicked)
        self.ui.left_ctrl_button.released.connect(self.onLeftReleased)

        """Slots for turning right."""
        self.ui.right_ctrl_btn.pressed.connect(self.onRightClicked)
        self.ui.right_ctrl_btn.released.connect(self.onRightReleased)

        """------------ Session Management Slots ------------"""

        """Slot for starting a new session."""
        self.ui.new_session_btn.clicked.connect(self.onNewSessionClicked)

        """Slot for selecting a different session."""
        self.ui.session_select.currentTextChanged.connect(self.onSessionSelected)

        """------------ Measurements Slots ------------"""

        self.ui.scan_btn.clicked.connect(self.onMeasurementClicked)

        """------------ LCD Number Display ------------"""
        
        """Make it display 3-digit numbers with accuracy 10^-2"""
        self.ui.angle_display.setDigitCount(7)
        self.ui.angle_display.setSmallDecimalPoint(True)

    """------------ Session Management Slots ------------"""

    @Slot()
    def onNewSessionClicked(self):
        """Requests creation of a new session by the server (and switch to it)."""

        """Check the name of the new session."""
        new_name = self.ui.new_session_name_edit.text()

        """Send the request to the server."""
        self.qt_to_f.put({
            "type" : "session-creation",
            "name" : new_name
        })

    @Slot(str)
    def onSessionSelected(self, new_text):
        """Sends a message to the server, requesting a session change.

        Keyword arguments:
            new_text -- The new session name selected from the list of available sessions.
        """

        """Request switch to a different session."""
        self.qt_to_f.put({
            "type": "session-switch",
            "name": new_text
        })

    """------------ Measurements Slots ------------"""

    @Slot()
    def onMeasurementClicked(self):
        """Sends a request for a scan to the server.
            Blocks the scan button until completion packet arrives from the server.
        """

        """Check the scan configuration."""
        rot = int(self.ui.scans_number_selector.currentText())
        nth = int(self.ui.every_nth_selector.currentText())

        """Send info to the server."""
        self.qt_to_f.put({
            "type": "scan",

            "rotations": rot,
            "every_nth": nth
        })

        """Disable the scan button for the time duration of the scan."""
        self.ui.scan_btn.setEnabled(False)
        print("Measurement requested")

    """------------ Movement Slots ------------"""

    @Slot()
    def onForwardClicked(self):
        """Callback method for when the forward motion button is pressed."""
        print("Going forward")
        self.qt_to_f.put({
            "type" : "steering",
            "direction" : "forward"
        })

    @Slot()
    def onForwardReleased(self):
        """Callback method for when the forward motion button is released."""
        print("Stopping forward motion")
        self.qt_to_f.put({
            "type": "steering",
            "direction": "none"
        })

    @Slot()
    def onBackwardClicked(self):
        """Callback method for when the backward motion button is pressed."""
        print("Going backwards")
        self.qt_to_f.put({
            "type": "steering",
            "direction": "backward"
        })

    @Slot()
    def onBackwardReleased(self):
        """Callback method for when the backward motion button is released."""
        print("Stopping backward motion")
        self.qt_to_f.put({
            "type": "steering",
            "direction": "none"
        })

    @Slot()
    def onLeftClicked(self):
        """Callback method for when the left motion button is pressed."""
        print("Going left")
        self.qt_to_f.put({
            "type": "steering",
            "direction": "left"
        })

    @Slot()
    def onLeftReleased(self):
        """Callback method for when the left motion button is released."""
        print("Stopping left motion")
        self.qt_to_f.put({
            "type": "steering",
            "direction": "none"
        })

    @Slot()
    def onRightClicked(self):
        """Callback method for when the right motion button is pressed."""
        print("Going right")
        self.qt_to_f.put({
            "type": "steering",
            "direction": "right"
        })

    @Slot()
    def onRightReleased(self):
        """Callback method for when the right motion button is released."""
        print("Stopping right motion")
        self.qt_to_f.put({
            "type": "steering",
            "direction": "none"
        })

    """------------ Reading from Flask ------------"""

    @Slot()
    def read_flask_queue(self):
        """Reads data from the flask queue and sets the UI elements."""

        """While there is some info from the server, poll it."""
        while not self.f_to_qt.empty():
            packet = self.f_to_qt.get_nowait()
            tt = packet["type"]
            print(f"Qt: Reading server queue: {tt}")

            match packet["type"]:
                case "telemetry":
                    """Display the telemetry data in Qt app."""
                    print(f"Qt: Qt received telemetry update: {packet}")
                    print(f"Qt: Telemetry angle: {str(packet['angle'])}")

                    """Display position in LCDs."""
                    self.ui.x_pos_display.display(str(packet["x"]))
                    self.ui.y_pos_display.display(str(packet["y"]))

                    """Transform angle to degrees [0 ; 360]."""
                    angle = float(packet["angle"]) * 180.0 / math.pi
                    angle = (angle + 360) % 360

                    angle = round(angle, 2)

                    self.ui.angle_display.display(str(angle))

                    """Update status of collision bars."""

                    """If collision status is true, set element to red as there is possibility of collision,
                        If not, reset to green (yes THAT green).
                    """

                    if packet["collision"]:
                        self.ui.collision_frame.setStyleSheet("background-color:rgb(255, 0, 0)")
                    else:
                        self.ui.collision_frame.setStyleSheet("background-color:rgb(0, 255, 0)")

                case "scan-status":
                    """Do some stuff which shows that scan is going on or something,
                        Unblock the button for scanning or something.
                    """

                    self.ui.scan_btn.setEnabled(True)
                    print(f"Qt: Scanning done, unblocking the button")

                case "session-switch":
                    """Server response to the session switch request.
                        If session switch was successful, then do nothing.
                    """
                    if packet["success"]:
                        print("Session change was successful in qt app")
                    else:
                        print("Session switch was successful as seen by the qt app")

                case "session-creation":
                    """Server response to the session creation request.
                        If sessions has been created successfully, then do nothing.
                    """
                    if packet["success"]:
                        """If creation of new session has been successful, add the option in the combobox
                            and switch to the new session?
                        """
                        print("Session creation was successful as seen by qt app")
                        self.ui.session_select.addItem(self.ui.new_session_name_edit.text())

                    else:
                        print("Session creation was NOT successful as seen by qt app")

                case "sessions-config":
                    """Info about the existing sessions
                        Configure the combobox so that user can choose from among existing sessions.
                    """

                    """Clear the combobox."""
                    #self.ui.session_select.clear()

                    """Fill it with possible names for sessions."""
                    for name in packet["names"]:
                        self.ui.session_select.addItem(name)

                case _:

                    print(f"Qt: Unknown packet received from server by Qt app: {packet}")

    """------------ Running The QT Window ------------"""

    """Unused so far."""
    def run_qt_window(self):
        """Runs the main Qt application window.
        Handles the cleanup after closing the qt window.
        """
        qt_window = MainQtWindow()
        qt_app = QApplication(sys.argv)

        qt_window.show()
        sys.exit(qt_app.exec())

'''
if __name__ == "__main__":
    app = QApplication(sys.argv)

    window = MainQtWindow()
    window.show()

    sys.exit(app.exec())
'''