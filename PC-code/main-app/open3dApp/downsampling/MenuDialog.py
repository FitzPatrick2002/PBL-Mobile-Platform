import open3d as o3d

class MenuDialog:
    def __init__(self, parent_window, callback):
        self.callback = callback
        self.dialog = o3d.visualization.gui.Dialog("Downsampling Settings")
        self.panel = o3d.visualization.gui.Vert(0.5)
        self.dialog.add_child(self.panel)

        # Slider
        self.percent_slider = o3d.visualization.gui.Slider(o3d.visualization.gui.Slider.INT)
        self.percent_slider.set_limits(0, 100)
        self.percent_slider.int_value = 50
        self.panel.add_child(o3d.visualization.gui.Label("Percent"))
        self.panel.add_child(self.percent_slider)

        # Combobox
        self.mode_combo = o3d.visualization.gui.Combobox()
        for i in range(3):
            self.mode_combo.add_item(f"Mode {i}")
        self.panel.add_child(o3d.visualization.gui.Label("Mode"))
        self.panel.add_child(self.mode_combo)

        # Button
        btn = o3d.visualization.gui.Button("Run")
        btn.set_on_clicked(self.on_submit)
        self.panel.add_child(btn)

        # Show dialog attached to the main window
        parent_window.show_dialog(self.dialog)

    def on_submit(self):
        percent = self.percent_slider.int_value
        mode = self.mode_combo.selected_index
        self.callback(percent, mode)