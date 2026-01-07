from FlaskServer.Server import FlaskServer
from QtApp.QtWindow import MainQtWindow
from open3dApp.Visualization import VisualizationApp

from PySide6.QtWidgets import QApplication

import open3d as o3d

import sys
import threading
import multiprocessing
import time

def start_flask(f_to_o3d, o3d_to_f, f_to_qt, qt_to_f):
    flask_server = FlaskServer(open3d_queue=f_to_o3d,
                               o3d_to_f=o3d_to_f,
                               from_qt_app_queue=qt_to_f,
                               to_qt_app_queue=f_to_qt)

    flask_server.start_server()

'''
def start_flask(f_to_o3d, f_to_qt, qt_to_f):
    flask_server = FlaskServer(open3d_queue=f_to_o3d,
                               from_qt_app_queue=qt_to_f,
                               to_qt_app_queue=f_to_qt)

    flask_thread = threading.Thread(target=flask_server.start_server, name="server-thread")
    flask_thread.daemon = True
    flask_thread.start()

    x = 0
    y = 0

    f_col = False
    b_col = True

    timepoint = time.time()
    itr = 0
    while itr < 20:
        if time.time() - timepoint > 3:
            f_to_qt.put({
                "type" : "telemetry",

                "x" : x,
                "y" : y,
                "angle" : 0.5,

                "front" : f_col,
                "back" : b_col
            })

            itr += 1
            x += 1
            y -= 1

            f_col = not f_col
            b_col = not b_col

            timepoint = time.time()
            print("Sending")

    print("Terminating flask?")
'''

def start_open3d(f_to_o3d, o3d_to_f):
    o3d.visualization.gui.Application.instance.initialize()
    VisualizationApp(f_to_o3d, o3d_to_f)
    o3d.visualization.gui.Application.instance.run()

def start_qt(f_to_qt, qt_to_f):
    qt_app = QApplication(sys.argv)

    qt_window = MainQtWindow(from_server=f_to_qt, to_server=qt_to_f)
    qt_window.show()

    sys.exit(qt_app.exec())

if __name__ == "__main__":
    # Queue used to send data from server to open3d app
    f_to_o3d = multiprocessing.Queue()
    # Queue used to send data from o3d app to server
    o3d_to_f = multiprocessing.Queue()
    # Queue used to send data from server to qt app
    f_to_qt = multiprocessing.Queue()
    # Queue used to send data from qt app to server
    qt_to_f = multiprocessing.Queue()

    # Start the flask server, open3d app and qt app as separate processes
    flask_process = multiprocessing.Process(target=start_flask, name="flask-server", args=(f_to_o3d, o3d_to_f, f_to_qt, qt_to_f))
    #flask_process.start()

    open3d_process = multiprocessing.Process(target=start_open3d, name="open3d-app", args=(f_to_o3d, o3d_to_f))
    #open3d_process.start()

    qt_process = multiprocessing.Process(target=start_qt, name="qt-app", args=(f_to_qt, qt_to_f))
    #qt_process.start()

    processes = [flask_process, open3d_process, qt_process]

    try:
        # Start all the processes
        for p in processes:
            p.start()

        # Keep the main process alive as long as the other processes are alive
        while any([p.is_alive() for p in processes]):
            time.sleep(0.5)

    except KeyboardInterrupt:
        # When user interrupted, kill processes and join them
        print(f"Main process terminated, shutting down other processes:")

    finally:
        for p in processes:
            if p.is_alive():
                print(f"Killing: {p.name}")
                p.terminate()
                p.join()


    # Close the queues after processes terminated
    f_to_o3d.close()
    f_to_qt.close()
    qt_to_f.close()

