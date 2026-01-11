"""
This module contains the PCDSaver utility class managing the scan sessions.
"""
from pathlib import Path
import numpy as np
import os
from typing import List, Tuple, Dict

class PCDSaver:
    def __init__(self, root : Path):
        """Initiates the PCDSaver class meant to store the scan sessions.

        Keyword arguments:
            Root -- The root directory where data from driving sessions is saved.
        """

        self._root_dir = root
        if not root.is_dir():
            print(f"Path {root} is not directory. Root dir for PcdSaver set to empty string.")
            self._root_dir = Path("")

        """_working_dir -- Current working directory within the _root_dir. (path to folder session-N)"""
        self._working_dir = Path("")
        """_history_records_no -- Number of records in the history folder of the current working directory.
                                        If no directory is selected its zero.
                                        Updated every time working directory is selected."""
        self._history_folder = Path("")
        self._temp_folder = Path("")
        self._combined_file = Path("")
        self._path_file = Path("")

        """Number of history records."""
        self._history_records_no = 0

    # -------------------- Start / End Session -------------------- #

    def start_session(self, name : str) -> None:
        """Creates a new folder where data from current session will be stored.
            Session folder structure looks as such:
                * session-N
                    * history
                        - scan-0.npy
                        - scan-1.npy
                        ...
                    * temp // <- Creation in progress TODO
                    - path.npy
                    - combined.npy

        history dir - Stores historical scans (each separately).
        temp - stores temporary files (like .csv files generated for the downsampling algorithm).
        path.npy - stores combined odometry data.
        combined.npy - stores combined point cloud scans.

        Keyword arguments:
            name -- Name of the session directory. If such folder does not exist it will be created
                     with necessary subfolders.
        """

        """Select working directory."""
        self._working_dir = self._root_dir / name

        """If the directory does not yet exist, create it."""
        if not self._working_dir.is_dir():
            self._working_dir.mkdir(parents=True, exist_ok=True)

            """Add history folder."""
            history = self._working_dir / "history"
            history.mkdir(parents=True, exist_ok=True)

            """Add temp folder."""
            temp_folder = self._working_dir / "temp"
            temp_folder.mkdir(parents=True, exist_ok=True)

            """Add the path file and the combined file."""
            combined = self._working_dir / "combined.npy"
            path = self._working_dir / "path.npy"

            """Create the combined.npy and path.npy files"""
            np.save(file=combined, arr=np.array([]), allow_pickle=True)
            np.save(file=path, arr=np.array([]), allow_pickle=True)
            print("New directory created")

        """Setup names of frequently used files."""
        self._history_folder = self._working_dir / "history"
        self._combined_file = self._working_dir / "combined.npy"
        self._path_file = self._working_dir / "path.npy"
        self._temp_folder = self._working_dir / "temp"

        """Parse the history folder and count the number of files in it."""
        history_files = [f for f in os.listdir(self._history_folder) if f.startswith("scan-") and f.endswith(".npy")]
        self._history_records_no = len(history_files)

        print(f"History files count: {self._history_records_no}")

    def end_session(self):
        """Deselects the working directory and resets all private fields of the class."""
        self._working_dir = Path("")
        self._history_folder = Path("")
        self._combined_file = Path("")
        self._path_file = Path("")
        self._history_records_no = 0

    # -------------------- Modification -------------------- #

    def add_record(self, pts : np.ndarray | List[float]) -> None:
        """Adds a new history record in the history folder of the current working directory.
            Record will be names scan-K.npy, where K is the number of the scan.
            Appends the points to the combined.npy file.

        Keyword arguments:
            pts -- Numpy array of shape (N, 3), which stores the points in cartesian reference frame.
                    N - number of points in the cloud.
        """

        if self._working_dir == Path(""):
            print(f"Working dir is empty. Cannot add new point cloud.")
            return

        """If a list of floats was given, transform it to ndarray."""
        if isinstance(pts, list):
            pts = np.ndarray(pts).reshape(3, -1) # Or was it (-1, 3)?

        print(f"PCDSaver: add_record(): Shape of the accepted array is: {pts.shape}")

        """Create new .npy file in history."""
        new_file = self._history_folder / (f"scan-{self._history_records_no}")
        np.save(file=new_file, arr=pts, allow_pickle=True)

        print(f"PCDSaver: add_record(): New history file created")

        """Increment the counter of history records."""
        self._history_records_no += 1

        """Append points to the combined.pcd."""
        combined_arr : np.ndarray = np.load(file=self._combined_file, allow_pickle=True)

        print(f"PCDSaver: add_record(): Combined file read")

        if combined_arr.shape[0] == 0:
            combined_arr = pts
        else:
            combined_arr = np.concatenate((combined_arr, pts), axis=0)

        np.save(file=self._combined_file, arr=combined_arr, allow_pickle=True)

    def update_position(self, pos : np.ndarray) -> None:
        """Appends the points to the path.npy file.
        In case array of shape (N, 2) is provided, z-coordinate is assumed to be equal 0.

        Keyword arguments:
            pos -- np.ndarray of shape (N, 2) or (N, 3).
        """

        if not self._path_file.is_file():
            print(f"File: {self._path_file} (path file) cannot be opened.")
            return

        """If z position is not given, assume z = 0."""
        if pos.shape[1] == 2:
            pos = np.insert(pos, 2, 0, axis=1)
            print(pos)

        path_data = np.load(file=self._path_file, allow_pickle=True)
        if path_data.shape[0] == 0:
            path_data = pos
        else:
            path_data = np.concatenate((path_data, pos), axis=0)

        """Save the updated path file."""
        np.save(file=self._path_file, arr=path_data, allow_pickle=True)

    # -------------------- Temporary Files  -------------------- #

    def prepare_for_downsampling(self):
        """Copies the combined.npy data and stores it in temp folder as [session-name]-downsample-source.csv.
        Creates an empty file [session-name]-downsample-target.csv where the downsampling content should be stored.
        """

        print(f"PCDSaver: convert_combined_to_csv(): Entered")

        """Load the numpy table from file."""
        combined_arr = np.load(self._combined_file, allow_pickle=True)

        print(f"PCDSaver: convert_combined_to_csv(): File read")

        """Store the copied data in the source csv in temp folder."""
        csv_src = self._temp_folder
        csv_src = csv_src / Path(self._working_dir.name + "-downsample-source.csv")

        print(f"PCDSaver: convert_combined_to_csv(): Csv paths created")

        np.savetxt(fname=csv_src, X=combined_arr, delimiter=";")

        print(f"PCDSaver: convert_combined_to_csv(): CSV saved")

        """Create the target csv where downsampling will dump the results."""
        csv_target = self._temp_folder / Path(self._working_dir.name + "-downsample-target.csv")
        with open(csv_target, "w") as f:
            pass

        print(f"PCDSaver: convert_combined_to_csv(): CSV created")

    def clear_temp_dir(self):
        """Clears the contents of the temporary directory (temp) which belong to the current session."""
        temp_contents = os.listdir(self._temp_folder)
        for f in temp_contents:
            elem_dir = self._temp_folder / f
            if elem_dir.is_file():
                try:
                    os.remove(elem_dir)
                    print(f"FileManager: Successfully deleted temporary file: {f}")
                except FileNotFoundError:
                    print(f"FIleManager: Could not find {f} in temp dir to delete it")

    def get_downsampling_files(self) -> Tuple[Path, Path]:
        """Returns the csv files which are needed for downsampling.
            Paths are returned in such order: (source, target)
            If they do not exist, they are created first.

        Returns:
            Tuple[Path, Path] -- (source, target).
        """

        """Prepare the directory names."""
        src_dir = self._temp_folder / Path(self._working_dir.name + "-downsample-source.csv")
        target_dir = self._temp_folder /  Path(self._working_dir.name + "-downsample-target.csv")

        print(f"PCDSaver: get_downsampling_files(): Names of dirs prepared")

        """Check if they exist & create them if they do not."""
        if (not src_dir.is_file()) or (not target_dir.is_file()):
            self.prepare_for_downsampling()
            print(f"PCDSaver: get_downsampling_files(): Source dir or target does not exist, creating...")

        """Returns the paths."""
        return (src_dir, target_dir)

    def apply_downsampling_changes(self):
        """Updates the current session after the downsampling procedure.
            Clears the contents of the combined file and writes the downsampled version into it.
            Stores the downsampled version of combined as a normal history file.
        """

        """Create pcd from the csv file which holds outcomes."""
        _, target_csv = self.get_downsampling_files()

        print(f"PCDSaver: apply_downsampling_changes(): target_csv: {target_csv}")
        try:
            downsampled_arr = np.genfromtxt(fname=str(target_csv.absolute()), delimiter=";", dtype=np.float64, invalid_raise=True)
            print(f"PCDSaver: apply_downsampling_changes(): target_csv has been read, adding donwsampled version to history and overwriting...")

            """Clear the combined.npy."""
            np.save(self._combined_file, arr=np.empty((0, 3)), allow_pickle=True)
            # open(self._combined_file, "w").close()
            print(f"PCDSaver: apply_downsampling_changes(): Clearing done")

            """Store the downsampled data in history and in the combined file."""
            self.add_record(downsampled_arr)
            print(f"PCDSaver: apply_downsampling_changes(): Record added - OK")
        except Exception as e:
            print(f"PCDSaver: apply_downsampling_changes(): ERROR: {e}")

    def get_session_temp_files(self) -> List[Path]:
        """Returns the list of temporary files associated with currently running session.

        Returns:
            list[Path]
        """

        """Get the names of files which contain the name of the currently used directory."""
        names = [self._temp_folder / f for f in os.listdir(self._temp_folder) if self._working_dir.name in f]

        """Filter out things that are not files."""
        names = [n for n in names if n.is_file()]

        return names

    # -------------------- Getters -------------------- #

    def get_combined(self) -> np.ndarray:
        """Returns data from the combined.npy

        Returns:
            Combined data from all scans from selected session.
        """

        if not self._working_dir.is_dir():
            print(f"Selected directory does not exist / is Null: {self._working_dir}.")
            return np.empty(shape=(0, 3), dtype=np.float32)

        # Read the combined.npy file and return it
        combined = np.load(file=self._combined_file, allow_pickle=True)
        return combined.astype(np.float64)

    def get_scan_data(self, idx : int) -> np.ndarray:
        """Returns data from a particular scan.
            If such scan does not exist, empty np.ndarray is returned.

        Keyword arguments:
            idx -- Number of the scan.

        Returns:
            Scan data from history file 'scan-{idx}.npy'.
        """
        scan_file = self._history_folder / f"scan-{idx}.npy"
        if not scan_file.is_file():
            print(f"Scan in file: {scan_file} could not have been opened")
            return np.empty(shape=(0, 3), dtype=np.float32)

        scan_data = np.load(file=scan_file, allow_pickle=True)
        return scan_data

    def get_path(self):
        """Returns data from the path.npy file as np.ndarray of shape (N, 3),
            where N is the number of saved position.

        Returns:
            np.ndarray -- of shape(N, 3) with saved positions.
        """

        if not self._path_file.is_file():
            print(f"Could not open: {self._path_file} which should contain the path data.")
            return np.empty(shape=(0, 3))

        path_data = np.load(file=self._path_file, allow_pickle=True)
        return path_data

    def get_sessions(self) -> List[str]:
        """Returns list with sessions names from the root folder.

        Returns:
            List[str]
        """
        sessions = [d.name for d in Path(self._root_dir).iterdir() if d.is_dir()]
        return sessions

    def get_dirs(self) -> Dict[str, Path]:
        """Returns a list of all directories in such order:
        root, working dir, working history dir, temp dir, working combined file, working path file.

        Returns:
            list[Path]
        """
        dirs = {
            "root"     : self._root_dir,
            "working"  : self._working_dir,
            "history"  : self._history_folder,
            "temp"     : self._temp_folder,
            "combined" : self._combined_file,
            "path"     : self._path_file
        }
        # dirs = [s for s in [self._root_dir, self._working_dir, self._history_folder, self._temp_folder, self._combined_file, self._path_file]]
        return dirs
