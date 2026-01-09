import os.path

import open3d as o3d
import numpy as np
import random
import multiprocessing
from pathlib import Path
import matplotlib.pyplot as plt

from utils.DataTypes import LidarScan, OdometryData
from utils.FileManagement import PCDSaver
import utils.OperatingSystemCheck

from open3dApp.downsampling.linux.Downsample_module import DownsampleModule
from open3dApp.downsampling.windows.Downsample import DownsampleWindows
from open3dApp.downsampling.MenuDialog import MenuDialog

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
    DOWNSAMPLE = 1
    MENU_RANDOM = 2
    MENU_COORD_FRAME = 3
    MENU_REF_PLANE = 4
    MENU_HEIGHT_MAP = 5
    MENU_QUIT = 6

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

        self._downsample_dialog = None

        # Setup the queue which sends data from o3d app to flask server
        self._o3d_to_f = o3d_to_f

        # Creates the window and adds the scene widget (basically are where we can draw stuff)
        # Widget stores the real 3D scene
        # Window title based on the Operating system
        window_title = "Scan Visualization on " + utils.OperatingSystemCheck.OS_SYSTEM
        self.window = o3d.visualization.gui.Application.instance.create_window(window_title, 1024, 768)

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
            options_menu = o3d.visualization.gui.Menu()
            options_menu.add_item("Downsample", VisualizationApp.DOWNSAMPLE)
            options_menu.add_item("Coordinate frame", VisualizationApp.MENU_COORD_FRAME)
            options_menu.add_item("Ground plane & sky", VisualizationApp.MENU_REF_PLANE)
            options_menu.add_item("Height Map", VisualizationApp.MENU_HEIGHT_MAP)
            options_menu.add_item("Quit", VisualizationApp.MENU_QUIT)

            debug_menu = o3d.visualization.gui.Menu()
            debug_menu.add_item("Add Random PCD", VisualizationApp.MENU_RANDOM)

            # Add the instance of menu to the application
            menu = o3d.visualization.gui.Menu()
            menu.add_menu("Options", options_menu)
            menu.add_menu("Debug", debug_menu)
            o3d.visualization.gui.Application.instance.menubar = menu

        # Callbacks for the menubar
        self.window.set_on_menu_item_activated(VisualizationApp.DOWNSAMPLE,
                                               self._downsample_window)
        self.window.set_on_menu_item_activated(VisualizationApp.MENU_RANDOM,
                                               self._on_menu_random)
        self.window.set_on_menu_item_activated(VisualizationApp.MENU_COORD_FRAME,
                                               self._show_coordinate_frame)
        self.window.set_on_menu_item_activated(VisualizationApp.MENU_REF_PLANE,
                                               self._toggle_reference_plane)
        self.window.set_on_menu_item_activated(VisualizationApp.MENU_HEIGHT_MAP,
                                               self._toggle_height_map)
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

        # By default disable the ground plan
        self._show_ground = False

        # By default don't show the reference frame arrows
        self._ref_frame_visible = False

        # List of geometries stores all the names of added geometries
        self._scene_pcds = dict()

        # By default disable the height map
        self._height_map_on = False

    # ------------ MENU CALLBACKS ------------ #

    def _downsample(self, mode, percent, lines):
        # prepare a
        print(f"Downsampling {mode}  {percent}%")

        o3d.visualization.gui.Application.instance.post_to_main_thread(
            self.window,
            lambda: self.window.close_dialog() ) # Closing in the main thread

        if((mode in (1,2) & lines >= 1) | mode == 0):
            if (utils.OperatingSystemCheck.OS_SYSTEM == 'Linux'):
                Linux_downsample = DownsampleModule()
                Linux_downsample.downsample(mode, percent, lines, "","")
            else:
                Windows_downsample = DownsampleWindows()
                Windows_downsample.downsample(mode, percent, lines, "", "")

    def _downsample_window(self):
        # run a seperate window collecting all necessary args
        o3d.visualization.gui.Application.instance.post_to_main_thread(
            self.window,
            lambda: MenuDialog(parent_window=self.window, callback=self._downsample)
        )


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

    def _show_coordinate_frame(self):
        '''
        Adds the arrows for axes x, y, z.
        '''

        self._ref_frame_visible = not self._ref_frame_visible
        self.scene.scene.show_axes(self._ref_frame_visible)

        '''
        # If there is no reference frame in the scene, add it
        if not self.scene.scene.has_geometry("reference-frame-arrows"):
            coordinate_frame = o3d.geometry.TriangleMesh.create_coordinate_frame(size=3.0)
            mat = o3d.visualization.rendering.MaterialRecord()
            mat.base_color = [
                1.0,
                1.0,
                1.0,
                1.0
            ]
            self.scene.scene.add_geometry("reference-frame-arrows", coordinate_frame, mat)
        else:
            if self.scene.scene.has_geometry("reference-frame-arrows"):
                self.scene.scene.remove_geometry("reference-frame-arrows")
        '''

    def _toggle_reference_plane(self):
        '''
        Toggles the ground plane and the skybox.
        '''

        self._show_ground = not self._show_ground
        self.scene.scene.show_ground_plane(enable=self._show_ground, plane=o3d.visualization.rendering.Scene.GroundPlane.XY)
        self.scene.scene.show_skybox(enable=self._show_ground)

        '''
        # If plane does not exist, create it
        if not self.scene.scene.has_geometry("ref-plane"):

            # Specify the bounds of the bounding box
            ref_plane = o3d.geometry.TriangleMesh().create_box(1000, 1000, 0.1)
            translation = np.asarray([-500, -500, -1]).astype(np.float64)
            ref_plane.translate(translation)

            # Calculate the normals for the shader
            #ref_plane.compute_vertex_normals()

            # Create the material for the plane and set shader
            mat = o3d.visualization.rendering.MaterialRecord()
            #mat.shader = "defaultLitTransparency"
            mat.base_color = [
                0.2,
                0.2,
                0.2,
                0.1
            ]
            mat.has_alpha=True
            self.scene.scene.add_geometry("ref-plane", ref_plane, mat)
        else:
            # If it does exist, remove it
            self.scene.scene.remove_geometry("ref-plane")
        '''

    def _toggle_height_map(self):
        '''
        Colors all points on the scene according to their z (height) value.
        Does not preserve the original colouring :(.
        '''

        # If height map is not enabled, enable it
        if not self._height_map_on:
            # Enable the height map
            self._height_map_on = True

            # Find all z-values for all points
            all_z_values = []
            for pcd in self._scene_pcds.values():
                points = np.asarray(pcd.points)
                all_z_values.append(points[ : , 2])

            # Find min and max z-values
            all_z_values = np.concatenate(all_z_values)
            max_z = max(all_z_values)
            min_z = min(all_z_values)
            span_z = max_z - min_z

            # Get the color map and paint the points
            colormap = plt.get_cmap("viridis")
            for name, pcd in self._scene_pcds.items():
                # Calculate the normalized value of z each point
                points = np.asarray(pcd.points)
                normalized_z = (points[ : , 2] - min_z) / span_z

                # Map the colors and apply them to pcd points
                color_values = colormap(normalized_z)[:, :3]
                pcd.colors = o3d.utility.Vector3dVector(color_values)

                # Define the material for the pcd
                mat = o3d.visualization.rendering.MaterialRecord()
                mat.point_size = 5.0

                # Remove the geometry from the scene and add it back
                self.scene.scene.remove_geometry(name)
                self.scene.scene.add_geometry(name, pcd, mat)
        else:
            # If height map is already enabled, switch it off
            self._height_map_on = False

            # Recolor all points to random colors
            for name, pcd in self._scene_pcds.items():
                # Remove colors from the pcd
                pcd.colors = o3d.utility.Vector3dVector([])

                # Create the new material for the pcs
                mat = o3d.visualization.rendering.MaterialRecord()
                mat.point_size = 5.0
                mat.base_color = [
                    random.random(),
                    random.random(),
                    random.random(),
                    1.0
                ]

                # Remove and add the pcd with a new color
                self.scene.scene.remove_geometry(name)
                self.scene.scene.add_geometry(name, pcd, mat)

    # ------------ PCD ADDING ------------ #

    def _interpret_message(self, message : dict):
        '''
        Interprets message received from the server.
        '''
        print(f"o3d: Received a message: {message}")

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
                print(message["payload"].payload)
                odometry_pcd = self._odometry_to_pcd(message["payload"])

                # Add the new odometry data as a pcd to the scene and append to path.npy of the current session
                self._add_pcd(odometry_pcd)
                self._pcd_saver.update_position(np.asarray(odometry_pcd.points))

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

        # Clear the scene & reset the dictionary of stored pcds
        self.scene.scene.clear_geometry()
        self._scene_pcds.clear()
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
        Each added pcd has a unique name in the self._scene_pcds dictionary of type: 'pcd + self._id'.
        If the pcd_name parameter is specified, then the pcd is appended to already existing one
        :param pcd: Processed pcd, correctly oriented and translated by the self._lidar_to_pcd()
        :param pcd_name: If true, no new entry is
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

        # Append the name of the pcd to list of pcds in the scene
        self._scene_pcds["pcd" + str(self._id)] = pcd
        print(f"Pcds dict: {self._scene_pcds}")
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

    def _odometry_to_pcd(self, odometry_data : OdometryData) -> o3d.geometry.PointCloud:
        '''
        Converts the OdometryData object into a 3D pcd.
        :param odometry_data: OdometryData object. Data should be only in 2D format. (List of [x0, y0, x1, y0, ...])
        :return: Odometry readings converted t o3d format, represented as PointCloud object
        '''

        # List of floats [x0, y0, x1, y1]
        points = odometry_data.payload
        print(f"o3d: Converted odometry points: {points}")

        # Convert the flat list into ndarray of shape (N, 2)
        # and add z = 0 to each pair, making the shape (N, 3)
        np_points = np.asarray(points).reshape(-1, 2)
        zeros = np.zeros((np_points.shape[0], 1))
        np_points = np.hstack((np_points, zeros))

        # Create the pcd and return it
        odometry_pcd = o3d.geometry.PointCloud()
        odometry_pcd.points = o3d.utility.Vector3dVector(np_points)
        print(f"o3d: Odometry conversion to pcd ok")
        return odometry_pcd

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
