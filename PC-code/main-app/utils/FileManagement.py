from pathlib import Path
import numpy as np
import os
from typing import List

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

    # -------------------- Start / End Session -------------------- #

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
            np.save(file=combined, arr=np.array([]), allow_pickle=True)
            np.save(file=path, arr=np.array([]), allow_pickle=True)
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

    def add_record(self, pts : np.ndarray | List[float]) -> None:
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

        # If a list of floats was given, transform it to ndarray
        if isinstance(pts, list):
            pts = np.ndarray(pts).reshape(3, -1) # Or was it (-1, 3)?

        # Create new .npy file in history
        new_file = self._history_folder / (f"scan-{self._history_records_no}")
        np.save(file=new_file, arr=pts, allow_pickle=True)

        # Increment the counter of history records
        self._history_records_no += 1

        # Append points to the combined.pcd
        combined_arr :np.ndarray = np.load(file=self._combined_file, allow_pickle=True)

        if combined_arr.shape[0] == 0:
            combined_arr = pts
        else:
            combined_arr = np.concatenate((combined_arr, pts), axis=0)

        np.save(file=self._combined_file, arr=combined_arr, allow_pickle=True)

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

        path_data = np.load(file=self._path_file, allow_pickle=True)
        if path_data.shape[0] == 0:
            path_data = pos
        else:
            path_data = np.concatenate((path_data, pos), axis=0)

        # Save the updated path file
        np.save(file=self._path_file, arr=path_data, allow_pickle=True)

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
        combined = np.load(file=self._combined_file, allow_pickle=True)
        return combined.astype(np.float64)

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

        scan_data = np.load(file=scan_file, allow_pickle=True)
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

        path_data = np.load(file=self._path_file, allow_pickle=True)
        return path_data

    def get_sessions(self) -> List[str]:
        '''
        Returns list with sessions names from the root folder.
        :return: List[str]
        '''
        sessions = [d.name for d in Path(self._root_dir).iterdir() if d.is_dir()]
        return sessions

    def get_dirs(self) -> List[Path]:
        '''
        Returns a list of all directories in such order:
        root, working dir, working history dir, working combined file, working path file
        :return: list[str]
        '''
        dirs = [s for s in [self._root_dir, self._working_dir, self._history_folder, self._combined_file, self._path_file]]
        return dirs
