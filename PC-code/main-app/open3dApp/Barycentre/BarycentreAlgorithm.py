import os
import csv
import open3d as o3d

class Barycentre:
    def __init__(self):
        pass

    def _calculate_barycentre(self, input_path: str):
        if (len(input_path) == 0):
            raise Exception(
                f"The input path cannot be empty"
            )

        if (os.path.exists(path=input_path) == False):
            raise Exception(
                f"The input path {input_path} does not exist"
            )

        if (os.path.getsize(input_path) == 0):
            raise Exception(
                f"The input path {input_path} cannot be empty"
            )

        self.__input_path = input_path
        self.__input_abs_path = os.path.abspath(input_path)

        self._result_x, self._result_y, self._result_z = 0, 0, 0
        index = 0
        with open(self.__input_abs_path, 'r') as file:
            csv_reader = csv.reader(file)
            # Iterate through each row in the CSV
            for row in csv_reader:
                index += 1

        with open(self.__input_abs_path, 'r') as file:
            csv_reader = csv.reader(file, delimiter=';')

            # Iterate through each row in the CSV
            for row in csv_reader:
                x,y,z = row
                self._result_x += float(x)
                self._result_y += float(y)
                self._result_z += float(z)

        self._result_x /= index
        self._result_y /= index
        self._result_z /= index
        self._result = "( " + str(self._result_x) + " , " + str(self._result_y) + " , " + str(self._result_z) + " )"

    def _result_window(self, parent_window, callback):
        self.callback = callback
        self.window = parent_window
        self.dialog = o3d.visualization.gui.Dialog("Barycentre result")
        self.panel = o3d.visualization.gui.Vert(20, o3d.visualization.gui.Margins(15, 15, 15, 15))

        # Title
        self.title_row = o3d.visualization.gui.Horiz(8)
        self.title_row.add_child(o3d.visualization.gui.Label("Barycentre:"))

        # Result
        self.result_row = o3d.visualization.gui.Horiz(8)
        self.result_row.add_child(o3d.visualization.gui.Label(self._result))

        # Button
        self.button_row = o3d.visualization.gui.Horiz()
        btn = o3d.visualization.gui.Button("Ok")
        btn.set_on_clicked(self.on_submit)
        self.button_row.add_child(btn)

        self.panel.add_child(self.title_row)
        self.panel.add_child(self.result_row)
        self.panel.add_child(self.button_row)

        self.dialog.add_child(self.panel)

        # Show dialog attached to the main window
        parent_window.show_dialog(self.dialog)

    def on_submit(self):
        self.callback()