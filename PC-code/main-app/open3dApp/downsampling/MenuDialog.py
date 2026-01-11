"""
This module contains a dialog window from open3d with input fields for the downsampling algorithm.
"""
import open3d as o3d

class MenuDialog:
    """Dialog window with input fields for downsampling arguments."""
    def __init__(self, parent_window, callback):
        """Initialisation and setup of the dialog window.

        Keyword arguments:
            parent_window -- the parent window, needed for the thread sharing,
            callback -- callback method to be called on the 'Run' button press.
        """
        self.callback = callback
        self.window = parent_window
        self.dialog = o3d.visualization.gui.Dialog("Downsampling Settings")
        self.panel = o3d.visualization.gui.Vert(20,o3d.visualization.gui.Margins(15,15,15,15))

        """Combobox."""
        self.mode_row = o3d.visualization.gui.Horiz(8)
        self.mode_combo = o3d.visualization.gui.Combobox()
        for i in range(3):
            self.mode_combo.add_item(f"Mode {i}")
        self.mode_row.add_child(o3d.visualization.gui.Label("Mode"))
        self.mode_row.add_child(self.mode_combo)

        self.panel.add_child(self.mode_row)

        self.panel_percent = o3d.visualization.gui.Horiz(8)
        self.panel_lines = o3d.visualization.gui.Horiz(8)

        """Percent slider."""
        self.percent_slider = o3d.visualization.gui.Slider(o3d.visualization.gui.Slider.INT)
        self.percent_slider.set_limits(0, 100)
        self.percent_slider.int_value = 50
        self.percent_slider.tooltip = "For mode 0 - percent of filtered out points."
        self.panel_percent.add_child(o3d.visualization.gui.Label("Percent"))
        self.panel_percent.add_child(self.percent_slider)

        """Modes 1 and 2, Lines."""
        self.line_number = o3d.visualization.gui.NumberEdit(o3d.visualization.gui.NumberEdit.INT)
        self.line_number.set_limits(1, 1000)
        self.line_number.int_value = 2
        self.line_number.tooltip = "For modes 1 and 2 - number of lines to filter"
        self.line_number.set_preferred_width(80)
        self.panel_lines.add_child(o3d.visualization.gui.Label("Lines"))
        self.panel_lines.add_child(self.line_number)

        """Run button."""
        self.button_row = o3d.visualization.gui.Horiz()
        btn = o3d.visualization.gui.Button("Run")
        btn.set_on_clicked(self.on_submit)
        self.button_row.add_child(btn)


        self.panel.add_child(self.panel_lines)
        self.panel.add_child(self.panel_percent)
        self.panel.add_child(self.button_row)

        self.dialog.add_child(self.panel)

        """Show dialog attached to the main window."""
        parent_window.show_dialog(self.dialog)

    def on_submit(self):
        """Callback method for the 'Run' button."""
        percent = self.percent_slider.int_value
        mode = self.mode_combo.selected_index
        line = self.line_number.int_value
        self.callback(mode, percent,line)