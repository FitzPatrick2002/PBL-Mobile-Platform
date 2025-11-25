# Here is a class for managing point cloud scans history
# and their integration into a single file

from pathlib import Path
import numpy as np
import open3d as o3d
import os
from matplotlib import pyplot as plt

# Folder structure
# * Main storage
#   * session-1
#       * numpy
#           * history
#               - scan-1.npy
#               - scan-2.npy
#               - scan-3.npy    // Single scan
#           - full-scan.npy         // All scans from the history glued together
#           - path.npy              // Stores positions of the platform during the single session
#
#       * pcd // Pcd data is generated on demand
#           * history
#               - scan-1.pcd
#               - scan-2.pcd
#               - scan-3.pcd    // Single scan
#           - full-scan.pcd         // All scans from the history glued together
#           - path.pcd              // Stores positions of the platform during the single session
#   * session-2
#   * session-3

# TO DO:
# 1. Option to save all the stuff as .pcd files
# 2. Test the saving as .pcd
# 3. Optimized saving stuff (appending to file ?)

class PCDSaver:
    def __init__(self, root : Path):
        '''
        Inits the PCDsaver class.
        Attributes:
            :param _root_dir: The root directory where data from driving sessions is saved.
            :param _working_dir: Current working directory within the _root_dir.
            :param _history_records_no: Number of records in the history folder of the current working directory.
                                        If no directory is selected its zero.
                                        Updated every time working directory is selected
        '''

        self._root_dir = root
        if not root.is_dir():
            print(f"Path {root} is not directory. Root dir for PcdSaver set to empty string.")
            self._root_dir = Path("")

        self._working_dir = Path("") # Current working directory (path to folder session-N)
        self._history_folder = Path("")
        self._combined_file = Path("")
        self._path_file = Path("")

        self._history_records_no = 0 # Number of

    # -------------------- Start / End -------------------- #

    def start_session(self, name : str) -> None:
        '''
        Creates a new folder where data from current session will be stored.
        Session folder structure looks as such:
            * session-N
                * history
                    - scan-0.npy
                    - scan-1.npy
                    ...
                - path.npy
                - combined.npy
        :param name: Name of the session directory. If such folder does not exist it will be created
                     with necessary subfolders.
        '''

        # Select working directory
        self._working_dir = self._root_dir / name

        # If the directory does not yet exist, create it
        if not self._working_dir.is_dir():
            self._working_dir.mkdir(parents=True, exist_ok=True)

            # Add history
            history = self._working_dir / "history"
            history.mkdir(parents=True, exist_ok=True)

            # Add the path file and the combined file
            combined = self._working_dir / "combined.npy"
            path = self._working_dir / "path.npy"

            # Create the combined.npy and path.npy files
            np.save(file=combined, arr=np.array([]))
            np.save(file=path, arr=np.array([]))
            print("New directory created")

        # Setup names of frequently used files
        self._history_folder = self._working_dir / "history"
        self._combined_file = self._working_dir / "combined.npy"
        self._path_file = self._working_dir / "path.npy"

        # Parse the history folder and count the number of files in it
        history_files = [f for f in os.listdir(self._history_folder) if f.startswith("scan-") and f.endswith(".npy")]
        self._history_records_no = len(history_files)


        print(f"History files count: {self._history_records_no}")

    def end_session(self):
        '''
        Deselects the working directory and resets all private fields of the class.
        '''
        self._working_dir = Path("")
        self._history_folder = Path("")
        self._combined_file = Path("")
        self._path_file = Path("")
        self._history_records_no = 0

    # -------------------- Modification -------------------- #

    def add_record(self, pts : np.ndarray) -> None:
        '''
        Adds a new history record in the history folder of the current working directory.
        Record will be names scan-K.npy, where K is the number of the scan.
        Appends the points to the combined.npy file.
        :param pts: Numpy array of shape (N, 3), which stores the points in cartesian reference frame.
                    N - number of points in the cloud.
        '''

        if self._working_dir == Path(""):
            print(f"Working dir is empty. Cannot add new point cloud.")
            return

        # Create new .npy file in history
        new_file = self._history_folder / (f"scan-{self._history_records_no}")
        np.save(file=new_file, arr=pts)

        # Increment the counter of history records
        self._history_records_no += 1

        # Append points to the combined.pcd
        combined_arr :np.ndarray = np.load(file=self._combined_file)

        if combined_arr.shape[0] == 0:
            combined_arr = pts
        else:
            combined_arr = np.concatenate((combined_arr, pts), axis=0)

        np.save(file=self._combined_file, arr=combined_arr)

    def update_position(self, pos : np.ndarray) -> None:
        '''
        Appends the points to the path.npy file.
        In case array of shape (N, 2) is provided, z-coordinate is assumed to be equal 0.
        :param pos: np.ndarray of shape (N, 2) or (N, 3).
        '''

        if not self._path_file.is_file():
            print(f"File: {self._path_file} (path file) cannot be opened.")
            return

        # If z position is not given, assume z = 0
        if pos.shape[1] == 2:
            pos = np.insert(pos, 2, 0, axis=1)
            print(pos)

        path_data = np.load(file=self._path_file)
        if path_data.shape[0] == 0:
            path_data = pos
        else:
            path_data = np.concatenate((path_data, pos), axis=0)

        # Save the updated path file
        np.save(file=self._path_file, arr=path_data)

    # -------------------- Getters -------------------- #

    def get_combined(self) -> np.ndarray:
        '''
        Returns data from the combined.npy
        :return: Combined data from all scans from selected session.
        '''

        if not self._working_dir.is_dir():
            print(f"Selected directory does not exist / is Null: {self._working_dir}.")
            return np.empty(shape=(0, 3), dtype=np.float32)

        # Read the combined.npy file and return it
        combined = np.load(file=self._combined_file)
        return combined

    def get_scan_data(self, idx : int) -> np.ndarray:
        '''
        Returns data from a particular scan.
        If such scan does not exist, empty np.ndarray is returned.
        :param idx: Number of the scan.
        :return: Scan data from history file 'scan-{idx}.npy'.
        '''

        scan_file = self._history_folder / f"scan-{idx}.npy"
        if not scan_file.is_file():
            print(f"Scan in file: {scan_file} could not have been opened")
            return np.empty(shape=(0, 3), dtype=np.float32)

        scan_data = np.load(file=scan_file)
        return scan_data

    def get_path(self):
        '''
        Returns data from the path.npy file as np.ndarray of shape (N, 3),
        where N is the number of saved position.
        :return: np.ndarray of shape(N, 3) with saved positions.
        '''

        if not self._path_file.is_file():
            print(f"Could not open: {self._path_file} which should contain the path data.")
            return np.empty(shape=(0, 3))

        path_data = np.load(file=self._path_file)
        return path_data

def example():
    '''
    Exemplary use of the PCDSaver class.
    '''
    # 0. Create the saver object
    storage_dir = Path("storage-tests") # Directory where the sessions data will be stored
    saver = PCDSaver(storage_dir)

    # 1. Begin session

    # If given session already exist, it will be continued.
    # If such session does not exist it will be created.
    saver.start_session("session-1")

    # 2. Save scan data

    # 2.1 Generate dummy data -> Straight line in 3D
    x_data = np.linspace(0, 500, num=100).reshape(100, 1)
    y_data = x_data * 3.0 + 0.0
    z_data = x_data * 1.0 + 50.0
    pcd = np.hstack((x_data, y_data, z_data))

    print(pcd.shape)
    saver.add_record(pcd) # The shape of saved point cloud must be (N, 3). Scan will be saved to scan-0.npy file in the history folder

    # 2.2 Create and save second 3d line.
    y_data = x_data * (-1.0) - 70.0
    z_data = x_data * 1.0 + 50.0
    pcd = np.hstack((x_data, y_data, z_data))

    print(pcd.shape)
    saver.add_record(pcd)  # Scan will be saved to scan-1.npy file in the history folder

    # 3. Update path file (stores saved positions of the mobile platform)

    # 3.1 Generate some random path (straight line) and append to path
    y_data = x_data * (0.5) + 0.0
    pcd = np.hstack((x_data, y_data))
    saver.update_position(pcd)

    y_data = x_data * (-0.5) + 0.0
    pcd = np.hstack((x_data, y_data))
    saver.update_position(pcd)

    # 4. Retrieve and visualize data

    # 4.1 Path
    pcd = saver.get_path()
    ax = plt.axes(projection='3d')
    ax.plot3D(pcd[:, 0], pcd[:, 1], pcd[:, 2], 'red')
    plt.show() # Both lines should be visible, data from the second path has been appended. z = 0 <- should be like that

    # 4.2 Specific scan data
    pcd = saver.get_scan_data(idx=1) # Get second scan data
    ax = plt.axes(projection='3d')
    ax.plot3D(pcd[:, 0], pcd[:, 1], pcd[:, 2], 'red')
    plt.show()  # Only the second 3d line should be visible

    pcd = saver.get_scan_data(idx=100) # Message will be printed, empty array returned
    ax = plt.axes(projection='3d')
    ax.plot3D(pcd[:, 0], pcd[:, 1], pcd[:, 2], 'red')
    plt.show()  # Empty plot should be visible

    # 4.3 Combined scans data
    pcd = saver.get_combined()
    ax = plt.axes(projection='3d')
    ax.plot3D(pcd[:, 0], pcd[:, 1], pcd[:, 2], 'blue')
    plt.show() # Both 3D lines will be visible

def tests():
    main_storage = Path("storage-tests")
    saver = PCDSaver(main_storage)

    # start session
    saver.start_session(name="session-2")

    # Update the position file
    x_pos = np.linspace(0, 500, num=100)
    y_pos = x_pos * 1.0 + 0.0 # Slope = 3.0

    # Change to column vectors
    x_pos = x_pos.reshape((x_pos.shape[0], 1))
    y_pos = y_pos.reshape((y_pos.shape[0], 1))

    new_pos = np.hstack((x_pos, y_pos))
    print(new_pos)
    saver.update_position(new_pos)

    # Read file and visualize
    path_data = saver.get_path()
    fig = plt.figure()
    ax = plt.axes(projection='3d')
    print(path_data[:, 0])
    print(path_data[:, 1])
    print(path_data[:, 2])

    ax.plot3D(path_data[:, 0], path_data[:, 1], path_data[:, 2], 'green')
    plt.show()

    # Update the position file
    x_pos = np.linspace(500, 750, num=100)
    y_pos = x_pos * (-1.0) + 700.0  # Slope = -3.0

    # Change to column vectors
    x_pos = x_pos.reshape((x_pos.shape[0], 1))
    y_pos = y_pos.reshape((y_pos.shape[0], 1))

    new_pos = np.hstack((x_pos, y_pos))
    print(new_pos)
    saver.update_position(new_pos)

    # Read position file and visualize
    new_pos = saver.get_path()
    fig = plt.figure()
    ax = plt.axes(projection='3d')
    ax.plot3D(new_pos[:, 0], new_pos[:, 1], new_pos[:, 2], 'red')

    plt.show()

    # load dummy point cloud (received scan)
    temp = o3d.data.PLYPointCloud()
    pcd : o3d.geometry.PointCloud = o3d.io.read_point_cloud(temp.path)

    # Save dummy data
    input()
    saver.add_record(np.asarray(pcd.points))

    # Read the combined file
    temp = saver.get_combined()
    pcd_1 = o3d.geometry.PointCloud()
    pcd_1.points = o3d.utility.Vector3dVector(temp)
    o3d.visualization.draw_geometries([pcd_1])

    # Read the scan-1 file (bunny)
    scan_0 = saver.get_scan_data(1)
    pcd_3 = o3d.geometry.PointCloud()
    pcd_3.points = o3d.utility.Vector3dVector(scan_0)
    o3d.visualization.draw_geometries([pcd_3])

    # Read the scan-100 file (should fail)
    scan_100 = saver.get_scan_data(100)
    pcd_3 = o3d.geometry.PointCloud()
    pcd_3.points = o3d.utility.Vector3dVector(scan_100)
    o3d.visualization.draw_geometries([pcd_3])

    saver.end_session()
    saver.start_session(name="session-2")

    # Save modified dummy data
    temp = input()
    pcd = o3d.io.read_triangle_mesh(o3d.data.BunnyMesh().path).sample_points_poisson_disk(number_of_points=2000)
    saver.add_record(np.asarray(pcd.points))

    # Read the combined file
    temp = saver.get_combined()
    pcd_2 = o3d.geometry.PointCloud()
    pcd_2.points = o3d.utility.Vector3dVector(temp)
    o3d.visualization.draw_geometries([pcd_2])

    saver.end_session()

def testViewing():
    pass

def npMessing():
    temp = o3d.data.PLYPointCloud()
    pcd = o3d.io.read_point_cloud(temp.path)
    print(f"Point cloud shape: {np.asarray(pcd.points).shape}")
    o3d.visualization.draw_geometries([pcd])

example()
#tests()
# npMessing()
