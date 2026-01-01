import os.path

import open3d as o3d
import numpy as np
import random
import multiprocessing
from pathlib import Path

from certifi import contents

from utils.DataTypes import LidarScan, OdometryData
from utils.FileManagement import PCDSaver

# Messages format
# From flask server to o3d app:
'''
// Data of the new lidar scan
{
    type: "lidar"
    payload: LidarScan
}

// Stores info about the odometry which needs to be appended
{
    type: "odometry"
    payload: OdometryData
}

// Stores the name of session to which we need to switch
{
    type: "session"
    name: str
}
'''

# From o3d app to flask server:
'''

// Specifies the names of existing sessions during application startup
{
    type : "sessions-config",
    names : list[str]
}
'''

class VisualizationApp:
    '''
    Visualizes point clouds and other geometries.
    Provides a running open3d application, which communicates with other processes via a Queue.
    Should be run on a different thread.
    Accepts LidarScan & OdometryData objects from the queue and displays them in the scene.

    Menu: In progress...

    :param _id: ID of a geometry in the scene.
                Each geometry has a name (point clouds names start with 'pcd_')
                which ends with a unique ID.
                Adding a new geometry increments this index
    :param _queue: Queue from the Multiprocessing module.
                   It is used to accept data from other processes.
    '''
    # Constants which index the menu states
    MENU_RANDOM = 1
    MENU_QUIT = 2

    def __init__(self, queue : multiprocessing.Queue, o3d_to_f : multiprocessing.Queue):
        '''
        Adds necessary features to the gui.Application instance.
        Initializes the menu, menu callbacks and callbacks which handle communication
        with other processes.
        :param queue: Queue used to communicate with different processes.
                      Accepts LidarScan & OdometryData objects, other are discarded.
        '''
        # _id stores the id of added geometry
        self._id = 0

        # Setup the queue, which communicates with the server
        self._queue = queue

        # Setup the queue which sends data from o3d app to flask server
        self._o3d_to_f = o3d_to_f

        # Creates the window and adds the scene widget (basically are where we can draw stuff)
        # Widget stores the real 3D scene
        self.window = o3d.visualization.gui.Application.instance.create_window("Add spheres example", 1024, 768)
        self.scene = o3d.visualization.gui.SceneWidget()

        # Set the scene properties,
        # The renderer specifies which window will render it?
        self.scene.scene = o3d.visualization.rendering.Open3DScene(self.window.renderer)
        self.scene.scene.set_background([1,1,1,1])
        self.scene.scene.scene.set_sun_light(
            [-1,-1,-1], # direction
            [1, 1, 1],  # color
            1000000     #intensity
        )

        # Setup the camera (camera is bound with the SceneWidget I guess?)
        bbox = o3d.geometry.AxisAlignedBoundingBox([-10,-10,-10],
                                                   [10,10,10])
        self.scene.setup_camera(60, bbox, [0,0,0])

        # Add the scene widget as child of the main window
        self.window.add_child(self.scene)

        # If there is no menubar, add it
        # Menubar seems to be bound to the application instance rather to a widget
        if o3d.visualization.gui.Application.instance.menubar is None:
            debug_menu = o3d.visualization.gui.Menu()
            debug_menu.add_item("Add Radom PCD", VisualizationApp.MENU_RANDOM)
            debug_menu.add_item("Quit", VisualizationApp.MENU_QUIT)

            # Add the instance of menu to the application
            menu = o3d.visualization.gui.Menu()
            menu.add_menu("Debug", debug_menu)
            o3d.visualization.gui.Application.instance.menubar = menu

        # Callbacks for the menubar
        self.window.set_on_menu_item_activated(VisualizationApp.MENU_RANDOM,
                                               self._on_menu_random)
        self.window.set_on_menu_item_activated(VisualizationApp.MENU_QUIT,
                                               self._on_menu_quit)

        # Setup callback for the tick event (run every frame)
        self.window.set_on_tick_event(self._monitor_server)

        # Initiate the file saving system
        self._sessions_path = os.path.dirname(Path(__file__).resolve()) / Path("sessions")
        self._pcd_saver = PCDSaver(self._sessions_path)

        # Send info about existing sessions to flask
        # Flask will propagate this info to the qt app which needs it
        existing_sessions = self._pcd_saver.get_sessions()
        print(f"Sending exisitng sessions: {existing_sessions}")
        self._o3d_to_f.put({
            "type" : "sessions-config",
            "names" : existing_sessions
        })

    # ------------ MENU CALLBACKS ------------ #

    def _on_menu_random(self):
        '''
        Callback for menu, which adds random point cloud to the scene.
        :return:
        '''
        # Adds some randomly generated points in the space
        self._id += 1

        # Generate point cloud within values [-10, 10]
        pts = 20.0 * np.random.rand(300, 3).astype("float32") - 10.0
        new_pcd = o3d.geometry.PointCloud()
        new_pcd.points = o3d.utility.Vector3dVector(pts)

        # Generate material
        mat = o3d.visualization.rendering.MaterialRecord()
        mat.base_color = [
            random.random(),
            random.random(),
            random.random(),
            1.0
        ]
        mat.point_size = 5.0

        # Add the new geometry to the scene
        self.scene.scene.add_geometry("pcd" + str(self._id), new_pcd, mat)

    def _on_menu_quit(self):
        '''
        Menu callback which stops the application.
        :return:
        '''
        o3d.visualization.gui.Application.instance.quit()

    # ------------ PCD ADDING ------------ #

    def _interpret_message(self, message : dict):
        '''
        Interprets message received from the server.
        '''

        match message["type"]:
            case "lidar":
                # Process new pcd, add it to the scene and save it in the session files

                # Process the payload and turn it into a pcd
                new_pcd = self._lidar_to_pcd(message["payload"])

                # Add the pcd to the scene and save it in sessions files
                self._add_pcd(new_pcd)
                self._pcd_saver.add_record(np.asarray(new_pcd.points))

                print("Visualization received lidar data")

            case "odometry":
                print("Visualization received odometry data")

            case "session":
                # Clear the scene and change currently selected session folder
                # Load the selected session data into the secene

                print(f"o3d: Reloading session with message: {message}")
                self._reload_session(message["name"])
                print(f"Session has been switched")
                contents = self._pcd_saver.get_dirs()
                print(f"Session structure: {contents}")

            case _:
                print(f"Visualization app received message of unknown type: {message}")

    def _reload_session(self, name : str):
        '''
        Clears the scene and resets the self._id.
        Creates new session or selects an exisitng one.
        Loads the 'combined' file of the selected session into the scene with _id = 0.
        :param name: Name of the session
        '''
        # Switch to different session
        self._pcd_saver.start_session(name)
        print(f"o3d: Session started: {name}")

        # Clear the scene
        self.scene.scene.clear_geometry()
        print(f"o3d: Geometry cleared")

        # Clear the geometry _id
        self._id = 0
        print(f"o3d: _id reset to: {self._id}")

        # Download data from the current session 'combined' file and convert it to PointCloud
        combined_pcd = o3d.geometry.PointCloud()
        print("o3d: _reload_session(): placeholder for combined pcd created")
        combined_arr = self._pcd_saver.get_combined()
        print(f"o3d: _reload_session(): Combined read from the file: {combined_arr}")
        if combined_arr.size > 0:
            print(f"o3d: _reload_session(): Adding points to pcd ")
            combined_pcd.points = o3d.utility.Vector3dVector(self._pcd_saver.get_combined())
        else:
            print(f"o3d: _reload_session(): combined file is empty, scene is loaded as empty after session switch")

        print("o3d: Reloading done, resetting the pcd.")
        # Add the pcd to the scene
        self._add_pcd(combined_pcd)

    def _monitor_server(self) -> bool:
        '''
        Monitors the state of the queue.
        If new data is received, process it.
        If its is an instance of LidarData or OdometryData, accept it and display on the scene.
        Other data is discarded.
        :return: True - when data from the queue has been accepted.
                        Scene is then redrawn with new content.
                 False - otherwise.
        '''
        try:
            # Get data if there is any in the queue
            if not self._queue.empty():
                # If there is no data in the queue, don't wait for it
                print("o3d: trying to get message from queue")
                new_message = self._queue.get_nowait() # .get(False)
                print(f"o3d: message intercepted: {new_message}")

                self._interpret_message(new_message)

                # Check the type of received data
               # if isinstance(new_data, LidarScan):
                    # Append the new array to the current scene
                #    self._add_pcd(self._lidar_to_pcd(new_data))

                    # Save data into the current session
                #    self._pcd_saver.add_record(new_data.payload)
                #elif isinstance(new_data, OdometryData):
                #    pass
                #else:
                #    print("Application received from server object which was neither LidarScan nor OdometryData.")

            '''
            # new_data is of type LidarScan
            lidar_data = new_data.payload
            np_data = np.asarray(lidar_data, dtype="float32").reshape((-1, 3))

            # Create pcd out of these points
            new_pcd = o3d.geometry.PointCloud()
            new_pcd.points = o3d.utility.Vector3dVector(np_data)

            # Add the pcd to the scene
            self._add_pcd(new_pcd)
            '''
        except Exception as e:
            print(f"Something went wrong in _monitor_server(): {e}")
            return False

        return True

    def _add_pcd(self, pcd : o3d.geometry.PointCloud):
        '''
        Adds a new pcd to the scene.
        :param pcd: Processed pcd, correctly oriented and translated by the self._lidar_to_pcd()
        :return:
        '''
        print(f"o3d: _add_pcd(): Starting function")
        self._id += 1
        mat = o3d.visualization.rendering.MaterialRecord()
        mat.base_color = [
            random.random(),
            random.random(),
            random.random(),
            1.0
        ]
        mat.point_size = 5.0
        print(f"o3d: _add_pcd(): material created, adding pcd...")
        self.scene.scene.add_geometry("pcd" + str(self._id), pcd, mat)
        print(f"o3d: _add_pcd(): pcd successfully added")

    # ------------ PCD OPERATIONS ------------ #

    def _lidar_to_pcd(self, lidar_data : LidarScan) -> o3d.geometry.PointCloud:
        '''
        Accepts a LidarData object and turns it into a PointCloud.
        :param lidar_data: Lidar data to be processed.
        :return: Lidar data as a PointCloud in cartesian coordinate frame.
        '''
        # 1. Include bearing in the phi angle
        # 2. Convert spherical to cartesian coordinates
        # 3. Account for the current platform position
        # 4. Convert to PCD

        # 2. Convert from spherical to cartesian
        polar_data = np.asarray(lidar_data.payload).reshape((-1, 3))

        r = polar_data[ : , 0] / 100 # Convert from mm to m
        phi = polar_data[ : , 1] * 3.14 / 180.0
        theta = polar_data[ : , 2] * 3.14 / 180.0

        x = r * np.cos(phi) * np.sin(theta) # r * cos(phi) * sin(theta)
        y = r * np.sin(phi) * np.sin(theta) # r * sin(phi) * sin(theta)
        z = r * np.cos(theta) # r * cos(theta)

        print(f"x: {x}")
        print(f"y: {y}")
        print(f"z: {z}")

        # 3. Account for scan position
        x = lidar_data.position[0] + x
        y = lidar_data.position[1] + y
        points = np.stack([x, y, z], axis=1) # Convert into a list of points [[x0, y0, z0], ...]

        # 4. Convert to PCD
        new_pcd = o3d.geometry.PointCloud()
        new_pcd.points = o3d.utility.Vector3dVector(points)

        return new_pcd

'''
def main():
    # Init the application
    o3d.visualization.gui.Application.instance.initialize()
    # Constructor of SpheresApp adds stuff to the application
    VisualizationApp()
    # Run the o3d app
    o3d.visualization.gui.Application.instance.run()

if __name__ == "__main__":
    main()
'''
