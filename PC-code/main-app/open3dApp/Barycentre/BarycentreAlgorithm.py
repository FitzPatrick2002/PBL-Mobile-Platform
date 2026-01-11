"""
This module contains the algorithm for calculating the Barycentre.
It also contains the Open3D dialog for communicating the result.
"""
import csv

import numpy as np
import open3d as o3d

class Barycentre:
    """For calculating the Barycentre of the current session scan."""
    def __init__(self):
        """Empty constructor."""
        pass

    def _calculate_barycentre(self, input_path: str, parent_window, callback):
        """Calculates the barycentre for 3 dimensions, from the input path to the file.

        Args:
            input_path -- will be given by the PCDSaver.
            parent_window -- the parent window of this dialog.
            callback -- the callback function to be called on the 'OK' button press.
        """
        self.__input_abs_path = input_path

        self._result_x, self._result_y, self._result_z = 0, 0, 0
        index = 0
        with open(self.__input_abs_path, 'r') as file:
            csv_reader = csv.reader(file)
            """Iterate through each row in the CSV."""
            for row in csv_reader:
                index += 1

        with open(self.__input_abs_path, 'r') as file:
            csv_reader = csv.reader(file, delimiter=';')
            """Iterate through each row in the CSV."""
            for row in csv_reader:
                x,y,z = row
                self._result_x += float(x)
                self._result_y += float(y)
                self._result_z += float(z)

        self._result_x /= index
        self._result_y /= index
        self._result_z /= index
        self._result = "( " + str(self._result_x) + " , " + str(self._result_y) + " , " + str(self._result_z) + " )"
        self._result_window(parent_window, callback)

    def _result_window(self, parent_window, callback):
        """Creates a dialog window to show the calculation result."""
        self.callback = callback
        self.window = parent_window
        self.dialog = o3d.visualization.gui.Dialog("Barycentre result")
        self.panel = o3d.visualization.gui.Vert(20, o3d.visualization.gui.Margins(15, 15, 15, 15))

        """Title."""
        self.title_row = o3d.visualization.gui.Horiz(8)
        self.title_row.add_child(o3d.visualization.gui.Label("Barycentre:"))

        """Result."""
        self.result_row = o3d.visualization.gui.Horiz(8)
        self.result_row.add_child(o3d.visualization.gui.Label(self._result))

        """Ok button."""
        self.button_row = o3d.visualization.gui.Horiz()
        btn = o3d.visualization.gui.Button("Ok")
        btn.set_on_clicked(self.on_submit)
        self.button_row.add_child(btn)

        self.panel.add_child(self.title_row)
        self.panel.add_child(self.result_row)
        self.panel.add_child(self.button_row)

        self.dialog.add_child(self.panel)

        """Show dialog attached to the main window."""
        parent_window.show_dialog(self.dialog)

    def on_submit(self):
        """On pressing the 'Ok' button call the callback function."""
        self.callback()

    def get_barycenter_as_pcd(self) -> o3d.geometry.PointCloud:
        """Returns the barycenter as point cloud.

        Returns:
            o3d.geometry.PointCloud() -- containing single point, that is barycenter.
        """

        np_arr = np.asarray([[self._result_x, self._result_y, self._result_z]])
        pcd = o3d.geometry.PointCloud()
        pcd.points = o3d.utility.Vector3dVector(np_arr)

        return pcd


